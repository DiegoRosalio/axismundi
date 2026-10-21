#include "gtk/LessonCard.hpp"
#include "gtk/Inspector.hpp"
#include "gtk/MainWindow.hpp"
#include <adwaita.h>
#include <glibmm/main.h>
#include <glibmm/ustring.h>
#include <cstdio>
#include <string>

namespace linalg::gtk {

namespace {

constexpr const char* kCss = R"(
.lesson-card { padding: 14px 16px; }
.lesson-dot { min-height: 4px; border-radius: 2px; background: alpha(currentColor, 0.15); }
.lesson-dot.done { background: @success_color; }
.lesson-dot.current { background: @accent_bg_color; }
.lesson-task { padding: 10px 12px; border-radius: 10px; background: alpha(@accent_bg_color, 0.12);
               border: 1px solid alpha(@accent_bg_color, 0.45); }
.lesson-task image { color: @accent_color; }
.lesson-task.done { background: alpha(@success_color, 0.12); border-color: alpha(@success_color, 0.5); }
.lesson-task.done image { color: @success_color; }
.lesson-target { outline: 2px solid @accent_color; outline-offset: -2px; border-radius: 8px; }
)";

void loadCss() {
    static bool loaded = false;
    if (loaded) return;
    loaded = true;
    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css, kCss);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(css),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);
}

void wrapLabel(Gtk::Label& l, const char* cssClass = nullptr) {
    l.set_wrap(true);
    l.set_wrap_mode(Pango::WrapMode::WORD_CHAR);
    l.set_xalign(0.0f);
    l.set_max_width_chars(36);  // the card's width; without it the label asks for one long line
    if (cssClass) l.add_css_class(cssClass);
}

// Is `w` inside the visible part of every scrolled window around it?
bool inSight(GtkWidget* w) {
    for (GtkWidget* p = gtk_widget_get_parent(w); p; p = gtk_widget_get_parent(p)) {
        if (!GTK_IS_SCROLLED_WINDOW(p)) continue;
        graphene_rect_t b;
        if (!gtk_widget_compute_bounds(w, p, &b)) return false;
        float bottom = b.origin.y + b.size.height, right = b.origin.x + b.size.width;
        if (bottom < 8 || b.origin.y > gtk_widget_get_height(p) - 8 || right < 8 || b.origin.x > gtk_widget_get_width(p) - 8)
            return false;
    }
    return true;
}

} // namespace

LessonCard::LessonCard(MainWindow& window)
    : Gtk::Box(Gtk::Orientation::VERTICAL, 10), m_window(window), m_dots(Gtk::Orientation::HORIZONTAL, 4),
      m_taskBox(Gtk::Orientation::HORIZONTAL, 10), m_buttons(Gtk::Orientation::HORIZONTAL, 6) {
    loadCss();
    add_css_class("card");
    add_css_class("lesson-card");
    // No fixed width: the wrapped labels ask for about 36 characters, and the overlay gives less when the view is
    // narrow (a small window with the pane open)
    set_size_request(260, -1);
    set_halign(Gtk::Align::END);
    set_valign(Gtk::Align::START);
    set_margin(12);

    // The lesson, "Step 2 of 5", close
    auto* head = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    m_caption.add_css_class("caption-heading");
    m_caption.add_css_class("accent");
    m_caption.set_xalign(0.0f);
    m_caption.set_hexpand(true);
    m_caption.set_ellipsize(Pango::EllipsizeMode::END);
    m_count.add_css_class("caption");
    m_count.add_css_class("dim-label");
    m_close.set_icon_name("window-close-symbolic");
    m_close.add_css_class("flat");
    m_close.add_css_class("circular");
    m_close.set_valign(Gtk::Align::CENTER);
    m_close.set_tooltip_text("Close the lesson (the lesson list has it again)");
    m_close.signal_clicked().connect([this] { leave(); });
    head->append(m_caption);
    head->append(m_count);
    head->append(m_close);
    append(*head);

    m_dots.set_homogeneous(true);
    append(m_dots);

    wrapLabel(m_title, "title-3");
    wrapLabel(m_body);
    append(m_title);
    append(m_body);

    // The task, ticked when done, with the hint under it until then
    m_taskBox.add_css_class("lesson-task");
    m_taskIcon.set_valign(Gtk::Align::START);
    m_taskIcon.set_margin_top(2);
    auto* taskText = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
    taskText->set_hexpand(true);
    wrapLabel(m_task, "heading");
    m_task.set_max_width_chars(30);
    wrapLabel(m_hint, "caption");
    m_hint.set_max_width_chars(30);
    m_hint.add_css_class("dim-label");
    taskText->append(m_task);
    taskText->append(m_hint);
    m_taskBox.append(m_taskIcon);
    m_taskBox.append(*taskText);
    append(m_taskBox);

    m_back.set_label("Back");
    m_back.signal_clicked().connect([this] {
        m_run.back();
        tick();
    });
    m_skip.set_label("Skip");
    m_skip.add_css_class("flat");
    m_skip.set_tooltip_text("Count the task as done and go on");
    m_skip.signal_clicked().connect([this] {
        m_run.skip();
        tick();
    });
    m_next.add_css_class("suggested-action");
    m_next.set_hexpand(true);
    m_next.set_halign(Gtk::Align::END);
    m_next.signal_clicked().connect([this] {
        std::string title = m_run.lesson() ? m_run.lesson()->title : "";
        if (m_run.next()) {
            if (m_completed) m_completed(m_run.completed());
            leave();
            m_window.toast("Finished: " + title);
        } else {
            tick();
        }
    });
    m_buttons.append(m_back);
    m_buttons.append(m_skip);
    m_buttons.append(m_next);
    append(m_buttons);

    // The pointer at the step's control: no autohide, so it never takes the clicks or keys meant for the window
    m_popover = gtk_popover_new();
    gtk_popover_set_autohide(GTK_POPOVER(m_popover), FALSE);
    gtk_widget_set_focusable(m_popover, FALSE);
    m_popoverLabel = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(m_popoverLabel), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(m_popoverLabel), 30);
    gtk_label_set_xalign(GTK_LABEL(m_popoverLabel), 0.0f);
    gtk_widget_set_margin_start(m_popoverLabel, 4);
    gtk_widget_set_margin_end(m_popoverLabel, 4);
    gtk_popover_set_child(GTK_POPOVER(m_popover), m_popoverLabel);
    gtk_widget_set_parent(m_popover, GTK_WIDGET(gobj()));

    set_visible(false);
}

LessonCard::~LessonCard() {
    m_timer.disconnect();
    setHighlight(nullptr);
    if (m_popover) gtk_widget_unparent(m_popover);
}

void LessonCard::showList() {
    struct ListState {
        LessonCard* card;
        AdwDialog* dialog;
    };
    AdwDialog* dialog = adw_dialog_new();
    auto* st = new ListState{this, dialog};
    g_object_set_data_full(G_OBJECT(dialog), "axismundi-state", st, [](gpointer p) { delete static_cast<ListState*>(p); });
    adw_dialog_set_title(dialog, "Lessons");
    adw_dialog_set_content_width(dialog, 480);
    adw_dialog_set_content_height(dialog, 640);

    GtkWidget* page = adw_preferences_page_new();
    AdwPreferencesGroup* group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_description(group, "Short guided lessons. Each step sets up the scene, explains it and "
                                                 "gives you a small task that is checked as you work.");
    const auto& all = lessons();
    for (size_t i = 0; i < all.size(); i++) {
        GtkWidget* row = adw_action_row_new();
        adw_preferences_row_set_use_markup(ADW_PREFERENCES_ROW(row), FALSE);
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), all[i].title);
        char sub[160];
        std::snprintf(sub, sizeof(sub), "%s · %zu step%s", all[i].summary, all[i].steps.size(),
                      all[i].steps.size() == 1 ? "" : "s");
        adw_action_row_set_subtitle(ADW_ACTION_ROW(row), sub);
        GtkWidget* mark;
        if (m_run.lessonCompleted(static_cast<int>(i))) {
            mark = gtk_image_new_from_icon_name("object-select-symbolic");
            gtk_widget_add_css_class(mark, "success");
            gtk_widget_set_tooltip_text(mark, "Finished");
        } else {
            mark = gtk_label_new(std::to_string(i + 1).c_str());
            gtk_widget_add_css_class(mark, "dim-label");
            gtk_widget_add_css_class(mark, "numeric");
        }
        gtk_widget_set_size_request(mark, 20, -1);
        adw_action_row_add_prefix(ADW_ACTION_ROW(row), mark);
        adw_action_row_add_suffix(ADW_ACTION_ROW(row), gtk_image_new_from_icon_name("go-next-symbolic"));
        gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
        g_object_set_data(G_OBJECT(row), "lesson", GSIZE_TO_POINTER(i));
        g_signal_connect(row, "activated", G_CALLBACK(+[](GtkWidget* r, ListState* s) {
                             int lesson = static_cast<int>(GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(r), "lesson")));
                             LessonCard* card = s->card;
                             adw_dialog_close(s->dialog);  // may free s
                             card->start(lesson);
                         }),
                         st);
        adw_preferences_group_add(group, row);
    }
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), group);

    GtkWidget* toolbar = adw_toolbar_view_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), adw_header_bar_new());
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), page);
    adw_dialog_set_child(dialog, toolbar);
    adw_dialog_present(dialog, m_window.widget());
}

void LessonCard::start(int lesson, int step) {
    if (lesson < 0 || lesson >= static_cast<int>(lessons().size())) return;
    m_run.startLesson(lesson);
    if (step > 0 && step < static_cast<int>(lessons()[lesson].steps.size())) m_run.goToStep(step);
    set_visible(true);
    if (!m_timer.connected()) m_timer = Glib::signal_timeout().connect([this] {
        tick();
        return true;
    }, 100);
    tick();
}

void LessonCard::leave() {
    m_run.leave();
    m_timer.disconnect();
    hidePointer();
    setHighlight(nullptr);
    set_visible(false);
}

void LessonCard::tick() {
    if (!m_run.inLesson()) return;
    bool entering = m_run.setupPending();
    // The check reads values: evaluate what changed since the view last drew (the view may not be drawing)
    if (!entering) m_window.session().scene().evaluate();
    m_run.update(m_window.api());
    if (entering) {
        m_window.sceneChanged();
        showStep();
    }
    updateTask();
    updatePointer();
}

void LessonCard::showStep() {
    const Lesson& lesson = *m_run.lesson();
    const TutorialStep& step = *m_run.step();
    const LessonText* gtkText = gtkLessonText(step.title);
    int index = m_run.stepIndex(), count = static_cast<int>(lesson.steps.size());

    m_caption.set_text(Glib::ustring(lesson.title).uppercase());
    char progress[32];
    std::snprintf(progress, sizeof(progress), "Step %d of %d", index + 1, count);
    m_count.set_text(progress);
    for (GtkWidget* d : m_dotWidgets) gtk_box_remove(GTK_BOX(m_dots.gobj()), d);
    m_dotWidgets.clear();
    for (int i = 0; i < count; i++) {
        GtkWidget* d = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_add_css_class(d, "lesson-dot");
        if (i < index) gtk_widget_add_css_class(d, "done");
        if (i == index) gtk_widget_add_css_class(d, "current");
        gtk_box_append(GTK_BOX(m_dots.gobj()), d);
        m_dotWidgets.push_back(d);
    }

    m_title.set_text(step.title);
    m_body.set_text(gtkText && gtkText->body ? gtkText->body : step.body);
    const char* task = gtkText && gtkText->task ? gtkText->task : step.task;
    const char* hint = gtkText && gtkText->hint ? gtkText->hint : step.hint;
    m_taskBox.set_visible(task != nullptr);
    m_task.set_text(task ? task : "");
    m_hint.set_text(hint ? hint : "");
    gtk_label_set_text(GTK_LABEL(m_popoverLabel), task ? task : "");
    m_next.set_label(index == count - 1 ? "Finish" : "Next");
    m_scrolled = false;
    hidePointer();  // it comes back pointing at the new step's control
}

void LessonCard::updateTask() {
    const TutorialStep& step = *m_run.step();
    bool done = m_run.taskDone();
    if (done) m_taskBox.add_css_class("done");
    else m_taskBox.remove_css_class("done");
    m_taskIcon.set_from_icon_name(done ? "object-select-symbolic" : "radio-symbolic");
    m_hint.set_visible(!done && !m_hint.get_text().empty());
    m_back.set_sensitive(m_run.stepIndex() > 0);
    m_skip.set_visible(step.task && !m_run.stepDone());
    m_next.set_sensitive(m_run.stepDone());
}

void LessonCard::updatePointer() {
    const TutorialStep& step = *m_run.step();
    std::string id = m_run.anchor();
    GtkWidget* target = id.empty() ? nullptr : m_window.anchorWidget(id);
    bool taskOpen = step.task && !m_run.taskDone();
    bool dialogOpen = adw_application_window_get_visible_dialog(ADW_APPLICATION_WINDOW(m_window.widget())) != nullptr;
    // Read-only steps outline their control; steps with a task until it is done
    if (!target || !gtk_widget_is_drawable(target) || (step.task && !taskOpen) || dialogOpen) {
        setHighlight(nullptr);
        hidePointer();
        return;
    }
    GtkWidget* inspector = GTK_WIDGET(m_window.inspector().gobj());
    if (!m_scrolled && gtk_widget_is_ancestor(target, inspector)) m_scrolled = m_window.inspector().scrollTo(target);
    if (!inSight(target)) {
        setHighlight(nullptr);
        hidePointer();
        return;
    }
    setHighlight(target);
    if (!taskOpen) {
        hidePointer();
        return;
    }
    graphene_rect_t inCard, inWindow;
    if (!gtk_widget_compute_bounds(target, GTK_WIDGET(gobj()), &inCard) ||
        !gtk_widget_compute_bounds(target, m_window.widget(), &inWindow)) {
        hidePointer();
        return;
    }
    // Below a header-bar button; else to the side of the target that faces the middle of the window
    float centerX = inWindow.origin.x + inWindow.size.width / 2;
    GtkPositionType side = inWindow.origin.y < 60                                 ? GTK_POS_BOTTOM
                           : centerX < gtk_widget_get_width(m_window.widget()) / 2 ? GTK_POS_RIGHT
                                                                                    : GTK_POS_LEFT;
    GdkRectangle r = {static_cast<int>(inCard.origin.x), static_cast<int>(inCard.origin.y),
                      std::max(1, static_cast<int>(inCard.size.width)), std::max(1, static_cast<int>(inCard.size.height))};
    GdkRectangle old;
    bool had = gtk_popover_get_pointing_to(GTK_POPOVER(m_popover), &old);
    if (!had || old.x != r.x || old.y != r.y || old.width != r.width || old.height != r.height)
        gtk_popover_set_pointing_to(GTK_POPOVER(m_popover), &r);
    if (gtk_popover_get_position(GTK_POPOVER(m_popover)) != side) gtk_popover_set_position(GTK_POPOVER(m_popover), side);
    if (!gtk_widget_get_visible(m_popover)) gtk_popover_popup(GTK_POPOVER(m_popover));
}

void LessonCard::hidePointer() {
    if (m_popover && gtk_widget_get_visible(m_popover)) gtk_popover_popdown(GTK_POPOVER(m_popover));
}

void LessonCard::setHighlight(GtkWidget* widget) {
    if (widget == m_highlighted) return;
    if (m_highlighted) {
        gtk_widget_remove_css_class(m_highlighted, "lesson-target");
        g_object_remove_weak_pointer(G_OBJECT(m_highlighted), reinterpret_cast<gpointer*>(&m_highlighted));
    }
    m_highlighted = widget;
    if (m_highlighted) {
        gtk_widget_add_css_class(m_highlighted, "lesson-target");
        g_object_add_weak_pointer(G_OBJECT(m_highlighted), reinterpret_cast<gpointer*>(&m_highlighted));
    }
}

} // namespace linalg::gtk
