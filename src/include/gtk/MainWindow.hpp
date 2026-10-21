#pragma once

// The GTK app's main window (an AdwApplicationWindow). A split view: the objects list (with the Add button) and
// the inspector of the selected object in the sidebar, the 3D view as the content, a header bar with undo/redo, the 3D space / complex plane switch and
// the main menu (scene files, About). Toasts report what happened. libadwaita has no C++ wrapper, so its
// widgets are made through its C API and our own widgets (gtkmm) are put inside them.

#include "app/AppApi.hpp"
#include "app/Session.hpp"
#include "gtk/ExportDialog.hpp"
#include "gtk/SettingsDialog.hpp"
#include "scene/Templates.hpp"
#include <giomm/simpleactiongroup.h>
#include <gtkmm/application.h>
#include <memory>
#include <string>

typedef struct _GtkWidget GtkWidget;
typedef struct _AdwToggleGroup AdwToggleGroup;
typedef struct _AdwToastOverlay AdwToastOverlay;
typedef struct _AdwOverlaySplitView AdwOverlaySplitView;

namespace linalg::gtk {

class SceneArea;
class ObjectsList;
class AddButton;
class Inspector;
class InfoPane;
class TimeBar;
class ViewMenu;
class WindowApi;

class MainWindow {
public:
    MainWindow(const Glib::RefPtr<Gtk::Application>& app, int width, int height);
    ~MainWindow();

    void present();
    GtkWidget* widget() const { return m_window; }
    SceneArea& sceneArea() { return *m_area; }
    Inspector& inspector() { return *m_inspector; }
    InfoPane& infoPane() { return *m_info; }
    // The pane on the right (Analysis, Steps, Breakdown): open it on a page, or close it ("")
    void showPane(const std::string& page);
    Session& session() { return m_session; }

    // Space switch: "space3d" or "complex"
    void setSpace(const std::string& name);
    // Dark or light: follow the system (empty), or force "dark" / "light"
    void setTheme(const std::string& theme);
    const std::string& theme() const { return m_theme; }
    void showSidebar(bool show);
    // The preferences in ~/.config/axismundi/gtk.txt (not read in the tour, so its screenshots repeat)
    void loadSettings();
    void applySettings();
    void showPreferences();
    void showReference();
    void showGallery();
    void showExport();
    void showShortcuts();
    void closeDialogs();  // every dialog open over the window
    // What gallery examples and lessons use to drive the window
    AppApi& api();
    void openViewMenu(bool open);
    void toast(const std::string& message);

    // The scene changed (an edit, undo, a file): redraw the view and update the list
    void sceneChanged();
    void addObject(const AddChoice& choice);
    void undo();
    void redo();
    void openFile(const std::string& path);
    void openAddMenu();
    void closeAddMenu();
    void saveFile(const std::string& path);

private:
    void buildActions(const Glib::RefPtr<Gtk::Application>& app);
    GtkWidget* buildSidebar();
    GtkWidget* buildContent();
    void chooseFileToOpen();
    void chooseFileToSave();
    void about();
    void syncSpaceSwitch();
    static void onSpaceChanged(AdwToggleGroup* group, void* pspec, MainWindow* self);
    static void onDarkChanged(void* manager, void* pspec, MainWindow* self);
    void applyDark();

    Session m_session;
    GtkWidget* m_window = nullptr;      // owned by GTK (the application's window list)
    AdwToggleGroup* m_spaces = nullptr;
    AdwToastOverlay* m_toasts = nullptr;
    AdwOverlaySplitView* m_split = nullptr;      // the objects on the left
    AdwOverlaySplitView* m_paneSplit = nullptr;  // the pane on the right
    SceneArea* m_area = nullptr;        // managed gtkmm widgets inside the window
    ObjectsList* m_list = nullptr;
    Inspector* m_inspector = nullptr;
    InfoPane* m_info = nullptr;
    TimeBar* m_time = nullptr;
    ViewMenu* m_viewMenu = nullptr;
    std::unique_ptr<WindowApi> m_api;
    AddButton* m_add = nullptr;
    Glib::RefPtr<Gio::SimpleActionGroup> m_actions;
    std::string m_theme;                // the last setTheme ("" = the system's)
    GtkSettings m_settings;
    ExportSettings m_export;
    std::string m_currentFile;          // the last file opened or saved ("" = none yet)
    bool m_syncingSpace = false;
};

} // namespace linalg::gtk
