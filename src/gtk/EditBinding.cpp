#include "gtk/EditBinding.hpp"
#include "math/expr/Expr.hpp"
#include <adwaita.h>

namespace linalg::gtk {

EditBinding* EditBinding::attach(GtkWidget* widget, Commit commit, bool formula) {
    auto* binding = new EditBinding(widget, std::move(commit), formula);
    g_object_set_data_full(G_OBJECT(widget), "axismundi-edit", binding,
                           [](gpointer p) { delete static_cast<EditBinding*>(p); });
    return binding;
}

EditBinding::EditBinding(GtkWidget* widget, Commit commit, bool formula)
    : m_widget(widget), m_commit(std::move(commit)), m_formula(formula) {
    g_signal_connect_swapped(widget, "changed", G_CALLBACK(+[](EditBinding* self) { self->typed(); }), this);
    // Enter: an entry's "activate", an AdwEntryRow's "entry-activated"
    const char* enter = ADW_IS_ENTRY_ROW(widget) ? "entry-activated" : "activate";
    g_signal_connect_swapped(widget, enter, G_CALLBACK(+[](EditBinding* self) { self->commit(); }), this);

    GtkEventController* focus = gtk_event_controller_focus_new();
    g_signal_connect_swapped(focus, "leave", G_CALLBACK(+[](EditBinding* self) { self->commit(); }), this);
    gtk_widget_add_controller(widget, focus);

    GtkEventController* keys = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(keys, GTK_PHASE_CAPTURE);
    g_signal_connect(keys, "key-pressed",
                     G_CALLBACK(+[](GtkEventControllerKey*, guint key, guint, GdkModifierType, EditBinding* self) -> gboolean {
                         if (key != GDK_KEY_Escape || !self->m_dirty) return FALSE;
                         self->revert();
                         return TRUE;
                     }),
                     this);
    gtk_widget_add_controller(widget, keys);
}

void EditBinding::show(const std::string& text, const std::string& error) {
    m_error = error;
    if (m_dirty) return;
    m_saved = text;
    if (gtk_editable_get_text(GTK_EDITABLE(m_widget)) != text) {
        m_setting = true;
        gtk_editable_set_text(GTK_EDITABLE(m_widget), text.c_str());
        m_setting = false;
    }
    setError(error);
}

void EditBinding::typed() {
    if (m_setting) return;
    std::string text = gtk_editable_get_text(GTK_EDITABLE(m_widget));
    m_dirty = text != m_saved;
    if (!m_dirty) {
        setError(m_error);
        return;
    }
    if (m_formula) {
        expr::Program probe;  // live syntax check (names are checked once committed)
        setError(probe.compile(text) ? "" : "This formula doesn't parse yet");
    }
}

void EditBinding::commit() {
    if (!m_dirty) return;
    m_dirty = false;
    std::string text = gtk_editable_get_text(GTK_EDITABLE(m_widget));
    if (m_commit(text)) m_saved = text;
    else revert();
}

void EditBinding::flush() { commit(); }

void EditBinding::revert() {
    m_dirty = false;
    m_setting = true;
    gtk_editable_set_text(GTK_EDITABLE(m_widget), m_saved.c_str());
    m_setting = false;
    setError(m_error);
}

void EditBinding::setTip(const std::string& tip) {
    m_tip = tip;
    setError(m_dirty ? std::string() : m_error);
}

void EditBinding::setError(const std::string& error) {
    if (error.empty()) gtk_widget_remove_css_class(m_widget, "error");
    else gtk_widget_add_css_class(m_widget, "error");
    const std::string& tip = error.empty() ? m_tip : error;
    gtk_widget_set_tooltip_text(m_widget, tip.empty() ? nullptr : tip.c_str());
}

} // namespace linalg::gtk
