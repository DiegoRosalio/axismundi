#include "gtk/FormulaDialog.hpp"
#include "app/Session.hpp"
#include "math/expr/Expr.hpp"
#include "scene/Summaries.hpp"
#include <adwaita.h>

namespace linalg::gtk {

namespace {

struct FormulaState {
    Session* session;
    scene::SpaceKind kind;
    scene::ObjectId object;
    std::string key;
    std::function<void(const std::string&)> apply;
    AdwDialog* dialog = nullptr;
    GtkTextBuffer* buffer = nullptr;
    GtkWidget* result = nullptr;
    GtkWidget* applyButton = nullptr;

    std::string text() const {
        GtkTextIter a, b;
        gtk_text_buffer_get_bounds(buffer, &a, &b);
        char* raw = gtk_text_buffer_get_text(buffer, &a, &b, FALSE);
        std::string t = raw;
        g_free(raw);
        for (char& c : t)
            if (c == '\n') c = ' ';  // one formula, written over several lines
        return t;
    }
    void update() {
        std::string t = text();
        const scene::Space& space = session->scene().space(kind);
        const scene::SceneObject* o = space.find(object);
        const scene::Field* f = o ? o->field(key) : nullptr;
        bool ok = false;
        std::string shown = f ? scene::fieldPreview(space, *o, *f, t, ok) : "The object is gone.";
        gtk_label_set_text(GTK_LABEL(result), shown.c_str());
        if (ok) gtk_widget_remove_css_class(result, "error");
        else gtk_widget_add_css_class(result, "error");
        expr::Program probe;  // unknown names are allowed (they may come later), syntax errors are not
        gtk_widget_set_sensitive(applyButton, f && probe.compile(t, f->variables()));
    }
    void commit() {
        if (!gtk_widget_get_sensitive(applyButton)) return;
        apply(text());
        adw_dialog_close(dialog);
    }
};

} // namespace

void showFormulaDialog(GtkWidget* parent, Session& session, scene::SpaceKind kind, scene::ObjectId object,
                       const std::string& key, const std::string& text, std::function<void(const std::string&)> apply) {
    auto* st = new FormulaState{&session, kind, object, key, std::move(apply)};
    const scene::SceneObject* o = session.scene().space(kind).find(object);
    const scene::Field* f = o ? o->field(key) : nullptr;
    std::string title = o && f ? o->name() + " · " + f->label : "Formula";

    AdwDialog* dialog = adw_dialog_new();
    st->dialog = dialog;
    g_object_set_data_full(G_OBJECT(dialog), "axismundi-state", st, [](gpointer p) { delete static_cast<FormulaState*>(p); });
    adw_dialog_set_title(dialog, title.c_str());
    adw_dialog_set_content_width(dialog, 560);

    GtkWidget* view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD_CHAR);
    for (auto margin : {gtk_text_view_set_left_margin, gtk_text_view_set_right_margin, gtk_text_view_set_top_margin,
                        gtk_text_view_set_bottom_margin})
        margin(GTK_TEXT_VIEW(view), 10);
    st->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    gtk_text_buffer_set_text(st->buffer, text.c_str(), -1);
    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), view);
    gtk_widget_set_size_request(scroll, -1, 160);
    gtk_widget_add_css_class(scroll, "card");

    st->result = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(st->result), TRUE);
    gtk_label_set_xalign(GTK_LABEL(st->result), 0.0f);
    gtk_label_set_selectable(GTK_LABEL(st->result), TRUE);
    gtk_widget_add_css_class(st->result, "monospace");
    GtkWidget* hint = gtk_label_new("Ctrl+Enter applies, Esc cancels. Names refer to objects in this space; t is the "
                                    "scene time. The formula reference is in the main menu (F1).");
    gtk_label_set_wrap(GTK_LABEL(hint), TRUE);
    gtk_label_set_xalign(GTK_LABEL(hint), 0.0f);
    gtk_widget_add_css_class(hint, "dim-label");
    gtk_widget_add_css_class(hint, "caption");

    GtkWidget* buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(buttons, GTK_ALIGN_END);
    GtkWidget* cancel = gtk_button_new_with_label("Cancel");
    st->applyButton = gtk_button_new_with_label("Apply");
    gtk_widget_add_css_class(st->applyButton, "suggested-action");
    gtk_box_append(GTK_BOX(buttons), cancel);
    gtk_box_append(GTK_BOX(buttons), st->applyButton);
    g_signal_connect_swapped(cancel, "clicked", G_CALLBACK(adw_dialog_close), dialog);
    g_signal_connect_swapped(st->applyButton, "clicked", G_CALLBACK(+[](FormulaState* s) { s->commit(); }), st);
    g_signal_connect_swapped(st->buffer, "changed", G_CALLBACK(+[](FormulaState* s) { s->update(); }), st);

    // Ctrl+Enter applies (plain Enter starts a new line)
    GtkEventController* keys = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(keys, GTK_PHASE_CAPTURE);
    g_signal_connect(keys, "key-pressed",
                     G_CALLBACK(+[](GtkEventControllerKey*, guint key, guint, GdkModifierType mods, FormulaState* s) -> gboolean {
                         bool enter = key == GDK_KEY_Return || key == GDK_KEY_KP_Enter;
                         if (!enter || !(mods & GDK_CONTROL_MASK)) return FALSE;
                         s->commit();
                         return TRUE;
                     }),
                     st);
    gtk_widget_add_controller(view, keys);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 18);
    gtk_widget_set_margin_end(box, 18);
    gtk_widget_set_margin_bottom(box, 18);
    for (GtkWidget* w : {scroll, st->result, hint, buttons}) gtk_box_append(GTK_BOX(box), w);
    GtkWidget* toolbar = adw_toolbar_view_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), adw_header_bar_new());
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), box);
    adw_dialog_set_child(dialog, toolbar);
    adw_dialog_set_focus(dialog, view);
    st->update();
    adw_dialog_present(dialog, parent);
}

} // namespace linalg::gtk
