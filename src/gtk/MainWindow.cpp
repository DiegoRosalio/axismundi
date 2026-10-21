#include "gtk/MainWindow.hpp"
#include "app/Version.hpp"
#include "gtk/AddButton.hpp"
#include "gtk/Inspector.hpp"
#include "gtk/InfoPane.hpp"
#include "gtk/LessonCard.hpp"
#include "gtk/SceneRenderer.hpp"
#include "gtk/GalleryDialog.hpp"
#include "gtk/ReferenceDialog.hpp"
#include "gtk/WindowApi.hpp"
#include "gtk/TimeBar.hpp"
#include "gtk/ViewMenu.hpp"
#include "gtk/ObjectsList.hpp"
#include "gtk/SceneArea.hpp"
#include "ui/SceneIO.hpp"
#include "ui/core/Theme.hpp"
#include <adwaita.h>
#include <giomm/menu.h>
#include <glibmm/main.h>
#include <gtkmm/button.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/overlay.h>
#include <gtkmm/paned.h>
#include <gtkmm/shortcutcontroller.h>
#include <gtkmm/togglebutton.h>
#include <filesystem>
#include <unistd.h>

namespace linalg::gtk {

namespace {

std::string fileName(const std::string& path) { return std::filesystem::path(path).filename().string(); }

GtkFileDialog* sceneDialog(const char* title) {
    GtkFileDialog* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, title);
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Scenes (*.txt)");
    gtk_file_filter_add_pattern(filter, "*.txt");
    GListStore* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    g_object_unref(filters);
    g_object_unref(filter);
    // Start in the working folder (where the app was started)
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd))) {
        GFile* folder = g_file_new_for_path(cwd);
        gtk_file_dialog_set_initial_folder(dialog, folder);
        g_object_unref(folder);
    }
    return dialog;
}

} // namespace

MainWindow::MainWindow(const Glib::RefPtr<Gtk::Application>& app, int width, int height) {
    m_window = adw_application_window_new(app->gobj());
    gtk_window_set_title(GTK_WINDOW(m_window), "axismundi");
    gtk_window_set_default_size(GTK_WINDOW(m_window), width, height);

    m_area = Gtk::make_managed<SceneArea>(m_session);
    m_area->onSceneChanged([this] { sceneChanged(); });

    // The pane on the right of the view: Analysis, Steps, Breakdown
    m_info = Gtk::make_managed<InfoPane>(m_session);
    m_paneSplit = ADW_OVERLAY_SPLIT_VIEW(adw_overlay_split_view_new());
    adw_overlay_split_view_set_sidebar_position(m_paneSplit, GTK_PACK_END);
    adw_overlay_split_view_set_sidebar(m_paneSplit, GTK_WIDGET(m_info->gobj()));
    adw_overlay_split_view_set_show_sidebar(m_paneSplit, FALSE);
    adw_overlay_split_view_set_min_sidebar_width(m_paneSplit, 300);
    adw_overlay_split_view_set_max_sidebar_width(m_paneSplit, 400);
    g_signal_connect_swapped(m_paneSplit, "notify::show-sidebar", G_CALLBACK(+[](MainWindow* self) {
                                 bool open = adw_overlay_split_view_get_show_sidebar(self->m_paneSplit);
                                 self->m_info->setOpen(open);
                                 self->m_info->refresh();
                                 self->m_area->changed();
                             }),
                             this);
    // While the breakdown plays, the view shows the matrix part-way (and doesn't dim the others)
    m_area->renderer().setFrameHooks(
        [this](double dt) {
            // Surfaces: smooth or faceted, as the preference (or the view style) says
            using S = UiSettings::Surfaces;
            m_session.sceneView().setSmoothSurfaces(m_settings.surfaces == S::StyleDefault ? view::activeStyle().smoothDefault
                                                                                           : m_settings.surfaces == S::Smooth);
            if (!m_info->breakdownActive()) return false;
            m_info->breakdown().update(static_cast<float>(dt));
            return m_info->breakdown().playing();
        },
        [this](view::DrawOptions& o) {
            o.dimUnselected = m_settings.dimUnselected;
            o.printScope = m_export.options.scope;  // the Paper preview shows what the export would
            o.printGrayShading = m_export.options.grayShading;
            if (!m_info->breakdownActive()) return;
            o.dimUnselected = false;
            m_session.applyBreakdown(o, m_info->breakdown());
        });

    m_split = ADW_OVERLAY_SPLIT_VIEW(adw_overlay_split_view_new());
    adw_overlay_split_view_set_sidebar(m_split, buildSidebar());
    adw_overlay_split_view_set_content(m_paneSplit, buildContent());
    adw_overlay_split_view_set_content(m_split, GTK_WIDGET(m_paneSplit));
    adw_overlay_split_view_set_min_sidebar_width(m_split, 280);
    adw_overlay_split_view_set_max_sidebar_width(m_split, 380);

    m_toasts = ADW_TOAST_OVERLAY(adw_toast_overlay_new());
    adw_toast_overlay_set_child(m_toasts, GTK_WIDGET(m_split));
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(m_window), GTK_WIDGET(m_toasts));

    // Below 700 px the sidebar floats over the view instead of taking space from it
    AdwBreakpoint* narrow = adw_breakpoint_new(adw_breakpoint_condition_parse("max-width: 700sp"));
    GValue collapsed = G_VALUE_INIT;
    g_value_init(&collapsed, G_TYPE_BOOLEAN);
    g_value_set_boolean(&collapsed, TRUE);
    adw_breakpoint_add_setter(narrow, G_OBJECT(m_split), "collapsed", &collapsed);
    adw_breakpoint_add_setter(narrow, G_OBJECT(m_paneSplit), "collapsed", &collapsed);
    g_value_unset(&collapsed);
    adw_application_window_add_breakpoint(ADW_APPLICATION_WINDOW(m_window), narrow);

    buildActions(app);

    g_signal_connect(adw_style_manager_get_default(), "notify::dark", G_CALLBACK(onDarkChanged), this);
    applyDark();

    // The list and the inspector follow the scene (values change with the clock, edits, undo); cheap when nothing
    // changed
    Glib::signal_timeout().connect([this] {
        m_list->refresh();
        m_inspector->refresh();
        m_info->refresh();
        m_time->refresh();
        return true;
    }, 150);
}

GtkWidget* MainWindow::buildSidebar() {
    GtkWidget* header = adw_header_bar_new();  // window buttons: the split view shows those at the window's edges
    GtkWidget* title = adw_window_title_new("Objects", "");
    adw_header_bar_set_title_widget(ADW_HEADER_BAR(header), title);
    m_add = Gtk::make_managed<AddButton>(m_session);
    m_add->onChoice([this](const AddChoice& choice) { addObject(choice); });
    adw_header_bar_pack_start(ADW_HEADER_BAR(header), GTK_WIDGET(m_add->gobj()));

    auto* gallery = Gtk::make_managed<Gtk::Button>();
    gallery->set_icon_name("view-app-grid-symbolic");
    gallery->set_tooltip_text("Examples: ready-made scenes to explore");
    gallery->set_action_name("win.gallery");
    adw_header_bar_pack_end(ADW_HEADER_BAR(header), GTK_WIDGET(gallery->gobj()));
    m_galleryButton = GTK_WIDGET(gallery->gobj());
    auto* lessons = Gtk::make_managed<Gtk::Button>();
    lessons->set_icon_name("help-browser-symbolic");
    lessons->set_tooltip_text("Lessons: guided steps with small tasks");
    lessons->set_action_name("win.lessons");
    adw_header_bar_pack_end(ADW_HEADER_BAR(header), GTK_WIDGET(lessons->gobj()));
    m_lessonsButton = GTK_WIDGET(lessons->gobj());

    m_list = Gtk::make_managed<ObjectsList>(m_session);
    m_inspector = Gtk::make_managed<Inspector>(m_session);
    m_list->onChanged([this] {
        m_area->changed();
        m_inspector->refresh();
    });
    m_inspector->onChanged([this] { sceneChanged(); });
    m_inspector->onToast([this](const std::string& message) { toast(message); });

    // The list above, the inspector below; the divider can be dragged
    auto* paned = Gtk::make_managed<Gtk::Paned>(Gtk::Orientation::VERTICAL);
    paned->set_start_child(*m_list);
    paned->set_end_child(*m_inspector);
    paned->set_resize_start_child(false);
    paned->set_shrink_start_child(false);
    paned->set_shrink_end_child(false);
    paned->set_wide_handle(true);
    paned->set_position(220);

    GtkWidget* toolbar = adw_toolbar_view_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), header);
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), GTK_WIDGET(paned->gobj()));
    return toolbar;
}

GtkWidget* MainWindow::buildContent() {
    GtkWidget* header = adw_header_bar_new();

    auto* sidebar = Gtk::make_managed<Gtk::ToggleButton>();
    sidebar->set_icon_name("sidebar-show-symbolic");
    sidebar->set_tooltip_text("Objects (Ctrl+B)");
    sidebar->set_active(true);
    g_object_bind_property(m_split, "show-sidebar", sidebar->gobj(), "active",
                           static_cast<GBindingFlags>(G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE));
    adw_header_bar_pack_start(ADW_HEADER_BAR(header), GTK_WIDGET(sidebar->gobj()));

    auto* undoButton = Gtk::make_managed<Gtk::Button>();
    undoButton->set_icon_name("edit-undo-symbolic");
    undoButton->set_tooltip_text("Undo (Ctrl+Z)");
    undoButton->set_action_name("win.undo");
    auto* redoButton = Gtk::make_managed<Gtk::Button>();
    redoButton->set_icon_name("edit-redo-symbolic");
    redoButton->set_tooltip_text("Redo (Ctrl+Shift+Z)");
    redoButton->set_action_name("win.redo");
    adw_header_bar_pack_start(ADW_HEADER_BAR(header), GTK_WIDGET(undoButton->gobj()));
    adw_header_bar_pack_start(ADW_HEADER_BAR(header), GTK_WIDGET(redoButton->gobj()));

    // The space switch as the title
    m_spaces = ADW_TOGGLE_GROUP(adw_toggle_group_new());
    const char* spaces[][2] = {{"space3d", "3D space"}, {"complex", "Complex plane"}};
    for (auto& s : spaces) {
        AdwToggle* toggle = adw_toggle_new();
        adw_toggle_set_name(toggle, s[0]);
        adw_toggle_set_label(toggle, s[1]);
        adw_toggle_group_add(m_spaces, toggle);
    }
    adw_toggle_group_set_active_name(m_spaces, "space3d");
    g_signal_connect(m_spaces, "notify::active-name", G_CALLBACK(onSpaceChanged), this);
    adw_header_bar_set_title_widget(ADW_HEADER_BAR(header), GTK_WIDGET(m_spaces));

    // The main menu
    auto menu = Gio::Menu::create();
    auto files = Gio::Menu::create();
    files->append("_New Scene", "win.new");
    files->append("_Open…", "win.open");
    files->append("_Save", "win.save");
    files->append("Save _As…", "win.save-as");
    files->append("_Examples…", "win.gallery");
    files->append("_Lessons…", "win.lessons");
    files->append("Export to _LaTeX…", "win.export");
    menu->append_section(files);
    auto help = Gio::Menu::create();
    help->append("_Keyboard Shortcuts", "win.shortcuts");
    help->append("_Formula Reference", "win.reference");
    help->append("_Preferences", "win.preferences");
    help->append("_About axismundi", "win.about");
    menu->append_section(help);
    auto* pane = Gtk::make_managed<Gtk::ToggleButton>();
    m_paneButton = GTK_WIDGET(pane->gobj());
    pane->set_icon_name("sidebar-show-right-symbolic");
    pane->set_tooltip_text("Analysis, steps and breakdown (F9)");
    g_object_bind_property(m_paneSplit, "show-sidebar", pane->gobj(), "active",
                           static_cast<GBindingFlags>(G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE));

    auto* menuButton = Gtk::make_managed<Gtk::MenuButton>();
    menuButton->set_icon_name("open-menu-symbolic");
    menuButton->set_tooltip_text("Main menu");
    menuButton->set_menu_model(menu);
    menuButton->set_primary(true);  // F10 opens it
    adw_header_bar_pack_end(ADW_HEADER_BAR(header), GTK_WIDGET(menuButton->gobj()));
    adw_header_bar_pack_end(ADW_HEADER_BAR(header), GTK_WIDGET(pane->gobj()));

    auto* viewMenu = m_viewMenu = Gtk::make_managed<ViewMenu>(m_session);
    viewMenu->onChanged([this] {
        m_area->changed();
        m_list->refresh();  // the dots follow the style's palette
    });
    viewMenu->onCamera([this](int view) {
        CameraController& camera = m_session.camera();
        switch (view) {
            case 1: camera.frontView(); break;
            case 2: camera.rightView(); break;
            case 3: camera.topView(); break;
            case 4: {
                int h = m_area->get_height() * m_area->get_scale_factor();
                if (!m_session.focusSelection(m_area->renderer().frameOptions(h))) toast("Select an object to focus on");
                break;
            }
            default: m_session.resetCamera(); break;
        }
        m_area->changed();
    });
    adw_header_bar_pack_end(ADW_HEADER_BAR(header), GTK_WIDGET(viewMenu->gobj()));

    // The scene clock under the view
    m_time = Gtk::make_managed<TimeBar>(m_session);
    m_time->onChanged([this] {
        m_area->changed();
        m_inspector->refresh();
    });

    // The current lesson step floats over the view
    m_lessons = Gtk::make_managed<LessonCard>(*this);
    m_lessons->onCompleted([this](unsigned int finished) {
        m_settings.lessonsFinished = finished;
        if (m_settingsLoaded) m_settings.save();
    });
    auto* overlay = Gtk::make_managed<Gtk::Overlay>();
    overlay->set_child(*m_area);
    overlay->add_overlay(*m_lessons);

    GtkWidget* toolbar = adw_toolbar_view_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), header);
    adw_toolbar_view_add_bottom_bar(ADW_TOOLBAR_VIEW(toolbar), GTK_WIDGET(m_time->gobj()));
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), GTK_WIDGET(overlay->gobj()));
    return toolbar;
}

void MainWindow::buildActions(const Glib::RefPtr<Gtk::Application>& app) {
    m_actions = Gio::SimpleActionGroup::create();
    auto add = [&](const char* name, std::function<void()> run, std::vector<Glib::ustring> accels) {
        m_actions->add_action(name, run);
        if (!accels.empty()) app->set_accels_for_action(std::string("win.") + name, accels);
    };
    add("undo", [this] { undo(); }, {"<Control>z"});
    add("redo", [this] { redo(); }, {"<Control><Shift>z", "<Control>y"});
    // Delete has no window-wide key: in a text field it must edit the text. The list and the view have it.
    add("delete", [this] {
        if (m_session.deleteSelection()) sceneChanged();
    }, {});
    // Play / pause: Space in the view (in a text field it must type a space)
    add("play", [this] {
        m_session.scene().togglePlay();
        m_time->refresh();
        m_area->changed();
    }, {});
    {
        auto keys = Gtk::ShortcutController::create();
        keys->set_scope(Gtk::ShortcutScope::LOCAL);
        keys->add_shortcut(Gtk::Shortcut::create(Gtk::KeyvalTrigger::create(GDK_KEY_space), Gtk::NamedAction::create("win.play")));
        m_area->add_controller(keys);
    }
    for (Gtk::Widget* w : std::initializer_list<Gtk::Widget*>{m_list, m_area}) {
        auto keys = Gtk::ShortcutController::create();
        keys->set_scope(Gtk::ShortcutScope::LOCAL);
        keys->add_shortcut(Gtk::Shortcut::create(Gtk::KeyvalTrigger::create(GDK_KEY_Delete),
                                                 Gtk::NamedAction::create("win.delete")));
        w->add_controller(keys);
    }
    add("duplicate", [this] {
        if (m_session.duplicateSelection()) sceneChanged();
    }, {"<Control>d"});
    add("new", [this] {
        m_session.scene().clearAll();
        m_session.scene().commit();
        m_currentFile.clear();
        sceneChanged();
        toast("New scene (Ctrl+Z goes back)");
    }, {"<Control>n"});
    add("open", [this] { chooseFileToOpen(); }, {"<Control>o"});
    add("save", [this] {
        if (m_currentFile.empty()) chooseFileToSave();
        else saveFile(m_currentFile);
    }, {"<Control>s"});
    add("save-as", [this] { chooseFileToSave(); }, {"<Control><Shift>s"});
    add("toggle-sidebar", [this] { showSidebar(!adw_overlay_split_view_get_show_sidebar(m_split)); }, {"<Control>b"});
    add("toggle-pane", [this] {
        adw_overlay_split_view_set_show_sidebar(m_paneSplit, !adw_overlay_split_view_get_show_sidebar(m_paneSplit));
    }, {"F9"});
    add("space3d", [this] { setSpace("space3d"); }, {"<Alt>1"});
    add("complex", [this] { setSpace("complex"); }, {"<Alt>2"});
    add("add", [this] { openAddMenu(); }, {"<Control><Shift>a"});
    add("about", [this] { about(); }, {});
    add("preferences", [this] { showPreferences(); }, {"<Control>comma"});
    add("reference", [this] { showReference(); }, {"F1"});
    add("gallery", [this] { showGallery(); }, {});
    add("lessons", [this] { m_lessons->showList(); }, {});
    add("export", [this] { showExport(); }, {"<Control>e"});
    add("shortcuts", [this] { showShortcuts(); }, {"<Control>question"});
    gtk_widget_insert_action_group(m_window, "win", G_ACTION_GROUP(m_actions->gobj()));
}

void MainWindow::present() { gtk_window_present(GTK_WINDOW(m_window)); }

void MainWindow::openAddMenu() {
    showSidebar(true);
    m_add->popup();
}

void MainWindow::closeAddMenu() { m_add->popdown(); }

void MainWindow::toast(const std::string& message) {
    AdwToast* t = adw_toast_new(message.c_str());
    adw_toast_set_timeout(t, 3);
    adw_toast_overlay_add_toast(m_toasts, t);
}

void MainWindow::showSidebar(bool show) { adw_overlay_split_view_set_show_sidebar(m_split, show); }

MainWindow::~MainWindow() = default;

AppApi& MainWindow::api() {
    if (!m_api) m_api = std::make_unique<WindowApi>(*this);
    return *m_api;
}

void MainWindow::closeDialogs() {
    for (int i = 0; i < 8; i++) {  // dialogs can stack; a few at most
        AdwDialog* d = adw_application_window_get_visible_dialog(ADW_APPLICATION_WINDOW(m_window));
        if (!d) break;
        adw_dialog_force_close(d);
    }
}

void MainWindow::showShortcuts() {
    struct Item {
        const char* title;
        const char* keys;
    };
    struct Section {
        const char* title;
        std::vector<Item> items;
    };
    const Section sections[] = {
        {"Scene", {{"New scene", "<Control>n"}, {"Open a scene", "<Control>o"}, {"Save", "<Control>s"},
                   {"Save as", "<Control><Shift>s"}, {"Export to LaTeX", "<Control>e"}, {"Undo", "<Control>z"},
                   {"Redo", "<Control><Shift>z"}}},
        {"Objects", {{"Add an object", "<Control><Shift>a"}, {"Duplicate the selection", "<Control>d"},
                     {"Delete the selection (in the list or the view)", "Delete"}}},
        {"View", {{"3D space", "<Alt>1"}, {"Complex plane", "<Alt>2"}, {"Show or hide the objects", "<Control>b"},
                  {"Analysis, steps and breakdown", "F9"}, {"Play or pause the clock (in the view)", "space"}}},
        {"Help", {{"Formula reference", "F1"}, {"Preferences", "<Control>comma"}, {"Keyboard shortcuts", "<Control>question"},
                  {"Main menu", "F10"}}},
    };
    AdwDialog* dialog = adw_shortcuts_dialog_new();
    for (const Section& section : sections) {
        AdwShortcutsSection* sec = adw_shortcuts_section_new(section.title);
        for (const Item& item : section.items) adw_shortcuts_section_add(sec, adw_shortcuts_item_new(item.title, item.keys));
        adw_shortcuts_dialog_add(ADW_SHORTCUTS_DIALOG(dialog), sec);
    }
    // In the view: the mouse
    AdwShortcutsSection* mouse = adw_shortcuts_section_new("Mouse in the View");
    adw_shortcuts_section_add(mouse, adw_shortcuts_item_new("Turn the camera: drag", ""));
    adw_shortcuts_section_add(mouse, adw_shortcuts_item_new("Move the camera: right-drag or Shift+drag", ""));
    adw_shortcuts_section_add(mouse, adw_shortcuts_item_new("Zoom: the wheel", ""));
    adw_shortcuts_section_add(mouse, adw_shortcuts_item_new("Select: click an object (Ctrl+click adds)", ""));
    adw_shortcuts_dialog_add(ADW_SHORTCUTS_DIALOG(dialog), mouse);
    adw_dialog_present(dialog, m_window);
}

void MainWindow::showExport() {
    showExportDialog(m_window, m_session, m_export,
                     {[this](const std::string& message) { toast(message); }, [this] {
                          m_area->changed();
                          m_list->refresh();
                      }});
}

void MainWindow::showGallery() {
    if (m_gallery) return;
    m_gallery = showGalleryDialog(m_window, api(), [this](const char* title) {
        sceneChanged();
        toast(std::string("Example: ") + title);
    });
    g_object_add_weak_pointer(G_OBJECT(m_gallery), reinterpret_cast<gpointer*>(&m_gallery));
}

GtkWidget* MainWindow::anchorWidget(const std::string& id) {
    bool paneOpen = adw_overlay_split_view_get_show_sidebar(m_paneSplit);
    if (id == "objects.list") return GTK_WIDGET(m_list->gobj());
    if (id == "objects.add") return GTK_WIDGET(m_add->gobj());
    if (id == "topbar.gallery") return m_galleryButton;
    if (id == "topbar.tutorial") return m_lessonsButton;
    // The pane: its button while it is closed, else its page buttons (or the whole Analysis page)
    if (id == "topbar.breakdown" || id == "topbar.analysis") return paneOpen ? m_info->switcher() : m_paneButton;
    if (id == "panel.analysis") return paneOpen ? GTK_WIDGET(m_info->gobj()) : m_paneButton;
    return m_inspector->anchor(id);
}

void MainWindow::showReference() { showReferenceDialog(m_window, m_session); }

void MainWindow::showPreferences() { showSettingsDialog(m_window, m_settings, [this] { applySettings(); }); }

void MainWindow::openViewMenu(bool open) {
    if (open) m_viewMenu->popup();
    else m_viewMenu->popdown();
}

void MainWindow::loadSettings() {
    m_settings.load();
    m_settingsLoaded = true;
    m_lessons->runner().setCompleted(m_settings.lessonsFinished);
    applySettings();
}

void MainWindow::applySettings() {
    setTheme(m_settings.theme);
    m_area->changed();
}

void MainWindow::showPane(const std::string& page) {
    if (!page.empty()) m_info->showPage(page);
    adw_overlay_split_view_set_show_sidebar(m_paneSplit, !page.empty());
}

void MainWindow::sceneChanged() {
    m_area->changed();
    m_list->refresh();
    m_inspector->refresh();
    m_info->refresh();
}

void MainWindow::addObject(const AddChoice& choice) {
    if (scene::SceneObject* o = applyAddChoice(m_session.scene(), m_session.spaceKind(), choice)) {
        toast("Added " + o->name());
        sceneChanged();
    }
}

void MainWindow::undo() {
    m_inspector->flush();  // typing in progress becomes its own step first
    if (!m_session.scene().canUndo()) return;
    m_session.scene().undo();
    sceneChanged();
}

void MainWindow::redo() {
    if (!m_session.scene().canRedo()) return;
    m_session.scene().redo();
    sceneChanged();
}

void MainWindow::openFile(const std::string& path) {
    SceneData data;
    std::string error, note;
    if (!loadSceneFile(path, data, error) || !m_session.loadFrom(data, error, note)) {
        toast("Could not open " + fileName(path) + ": " + error);
        return;
    }
    m_currentFile = path;
    syncSpaceSwitch();
    sceneChanged();
    toast(note.empty() ? "Opened " + fileName(path) + " (Ctrl+Z goes back)" : note);
}

void MainWindow::saveFile(const std::string& path) {
    SceneData data;
    m_session.saveTo(data);
    std::string error;
    if (saveSceneFile(path, data, error)) {
        m_currentFile = path;
        toast("Saved " + fileName(path));
    } else {
        toast("Save failed: " + error);
    }
}

void MainWindow::chooseFileToOpen() {
    GtkFileDialog* dialog = sceneDialog("Open a Scene");
    gtk_file_dialog_open(dialog, GTK_WINDOW(m_window), nullptr, [](GObject* source, GAsyncResult* result, gpointer self) {
        GFile* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), result, nullptr);
        if (!file) return;  // cancelled
        if (char* path = g_file_get_path(file)) {
            static_cast<MainWindow*>(self)->openFile(path);
            g_free(path);
        }
        g_object_unref(file);
    }, this);
    g_object_unref(dialog);
}

void MainWindow::chooseFileToSave() {
    GtkFileDialog* dialog = sceneDialog("Save the Scene");
    gtk_file_dialog_set_initial_name(dialog, m_currentFile.empty() ? "scene.txt" : fileName(m_currentFile).c_str());
    gtk_file_dialog_save(dialog, GTK_WINDOW(m_window), nullptr, [](GObject* source, GAsyncResult* result, gpointer self) {
        GFile* file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), result, nullptr);
        if (!file) return;
        if (char* path = g_file_get_path(file)) {
            std::string p = path;
            if (std::filesystem::path(p).extension().empty()) p += ".txt";
            static_cast<MainWindow*>(self)->saveFile(p);
            g_free(path);
        }
        g_object_unref(file);
    }, this);
    g_object_unref(dialog);
}

void MainWindow::about() {
    AdwDialog* dialog = adw_about_dialog_new();
    AdwAboutDialog* a = ADW_ABOUT_DIALOG(dialog);
    adw_about_dialog_set_application_name(a, "axismundi");
    adw_about_dialog_set_application_icon(a, "com.bisonanon.axismundi");
    adw_about_dialog_set_version(a, kVersion);
    adw_about_dialog_set_developer_name(a, "Diego Rosalio");
    adw_about_dialog_set_comments(a, "Linear algebra, calculus and complex numbers you can see and move.");
    adw_about_dialog_set_license_type(a, GTK_LICENSE_GPL_3_0);
    adw_about_dialog_set_copyright(a, "© 2026 Diego Rosalio");
    adw_dialog_present(dialog, m_window);
}

void MainWindow::setSpace(const std::string& name) {
    adw_toggle_group_set_active_name(m_spaces, name.c_str());  // onSpaceChanged does the rest
}

void MainWindow::syncSpaceSwitch() {
    m_syncingSpace = true;
    adw_toggle_group_set_active_name(m_spaces, m_session.mode() == AppMode::Complex ? "complex" : "space3d");
    m_syncingSpace = false;
}

void MainWindow::onSpaceChanged(AdwToggleGroup* group, void*, MainWindow* self) {
    if (self->m_syncingSpace) return;
    const char* name = adw_toggle_group_get_active_name(group);
    bool complex = name && std::string(name) == "complex";
    self->m_session.setMode(complex ? AppMode::Complex : AppMode::Space3D);
    self->sceneChanged();
}

void MainWindow::setTheme(const std::string& theme) {
    m_theme = theme;
    AdwColorScheme scheme = theme == "dark"    ? ADW_COLOR_SCHEME_FORCE_DARK
                            : theme == "light" ? ADW_COLOR_SCHEME_FORCE_LIGHT
                                               : ADW_COLOR_SCHEME_DEFAULT;
    adw_style_manager_set_color_scheme(adw_style_manager_get_default(), scheme);
}

void MainWindow::onDarkChanged(void*, void*, MainWindow* self) { self->applyDark(); }

// The 3D view follows the window's dark/light look (the system setting, or --theme)
void MainWindow::applyDark() {
    bool dark = adw_style_manager_get_dark(adw_style_manager_get_default());
    ui::setTheme(dark ? ui::ThemeKind::Dark : ui::ThemeKind::Light);
    m_area->changed();
    if (m_list) m_list->refresh();  // the dots follow the theme's palette
}

} // namespace linalg::gtk
