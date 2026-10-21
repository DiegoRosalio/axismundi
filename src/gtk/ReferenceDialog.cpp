#include "gtk/ReferenceDialog.hpp"
#include "app/Session.hpp"
#include "math/expr/Reference.hpp"
#include <adwaita.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace linalg::gtk {

namespace {

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// What the dialog's callbacks share; deleted with the dialog
struct ReferenceState {
    Session* session;
    GtkWidget* tryResult;
    struct Row {
        GtkWidget* row;
        GtkWidget* group;
        std::string haystack;  // syntax + text + example, lower case
    };
    std::vector<Row> rows;
    std::vector<GtkWidget*> groups;

    const expr::Resolver* resolver() { return &session->scene().space(session->spaceKind()); }
    double time() { return session->scene().timeline().time; }
};

std::string resultText(ReferenceState& st, const std::string& formula, unsigned variables, bool usesObjects) {
    std::string error;
    expr::Value v = expr::evaluateExample(formula, st.resolver(), st.time(), variables, error);
    if (v.ok()) return "= " + expr::compactValue(v);
    return usesObjects ? "needs your objects" : error;
}

} // namespace

void showReferenceDialog(GtkWidget* parent, Session& session) {
    auto* st = new ReferenceState{&session, nullptr, {}, {}};
    AdwDialog* dialog = adw_dialog_new();
    adw_dialog_set_title(dialog, "Formula Reference");
    adw_dialog_set_content_width(dialog, 560);
    adw_dialog_set_content_height(dialog, 640);
    g_object_set_data_full(G_OBJECT(dialog), "axismundi-state", st, [](gpointer p) { delete static_cast<ReferenceState*>(p); });

    GtkWidget* page = adw_preferences_page_new();

    // Try a formula against the current space's objects
    AdwPreferencesGroup* tryGroup = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_description(tryGroup, "Type any formula; it is evaluated with your objects, x = 1, y = 2 "
                                                    "and t = the scene time.");
    GtkWidget* tryRow = adw_entry_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(tryRow), "Try a Formula");
    adw_preferences_group_add(tryGroup, tryRow);
    GtkWidget* result = adw_action_row_new();
    gtk_widget_add_css_class(result, "property");
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(result), "Result");
    adw_action_row_set_subtitle_selectable(ADW_ACTION_ROW(result), TRUE);
    gtk_widget_set_visible(result, FALSE);
    adw_preferences_group_add(tryGroup, result);
    st->tryResult = result;
    g_signal_connect(tryRow, "changed", G_CALLBACK(+[](GtkEditable* e, ReferenceState* s) {
                         std::string text = gtk_editable_get_text(e);
                         gtk_widget_set_visible(s->tryResult, !text.empty());
                         if (text.empty()) return;
                         std::string error;
                         expr::Value v = expr::evaluateExample(text, s->resolver(), s->time(), 0, error);
                         std::string shown = v.ok() ? "= " + expr::compactValue(v) + "   (" + v.typeName() + ")" : error;
                         adw_action_row_set_subtitle(ADW_ACTION_ROW(s->tryResult), shown.c_str());
                     }),
                     st);
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), tryGroup);

    // The sections
    for (const auto& section : expr::reference()) {
        AdwPreferencesGroup* group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
        adw_preferences_group_set_title(group, section.title.c_str());
        if (!section.intro.empty()) {
            gchar* intro = g_markup_escape_text(section.intro.c_str(), -1);
            adw_preferences_group_set_description(group, intro);
            g_free(intro);
        }
        unsigned variables = expr::exampleVariables(section);
        for (const auto& entry : section.entries) {
            GtkWidget* row = adw_action_row_new();
            adw_preferences_row_set_use_markup(ADW_PREFERENCES_ROW(row), FALSE);
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), entry.syntax.c_str());
            std::string subtitle = entry.text;
            if (!entry.example.empty())
                subtitle += "\ne.g. " + entry.example + "   " + resultText(*st, entry.example, variables, entry.usesObjects);
            adw_action_row_set_subtitle(ADW_ACTION_ROW(row), subtitle.c_str());
            adw_action_row_set_subtitle_selectable(ADW_ACTION_ROW(row), TRUE);
            if (!entry.example.empty()) {
                // Put the example into the try field
                GtkWidget* use = gtk_button_new_from_icon_name("edit-paste-symbolic");
                gtk_widget_set_valign(use, GTK_ALIGN_CENTER);
                gtk_widget_add_css_class(use, "flat");
                gtk_widget_set_tooltip_text(use, "Try this example above");
                g_object_set_data_full(G_OBJECT(use), "example", g_strdup(entry.example.c_str()), g_free);
                g_object_set_data(G_OBJECT(use), "try", tryRow);
                g_signal_connect(use, "clicked", G_CALLBACK(+[](GtkWidget* b, gpointer) {
                                     auto* tr = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(b), "try"));
                                     gtk_editable_set_text(GTK_EDITABLE(tr),
                                                           static_cast<const char*>(g_object_get_data(G_OBJECT(b), "example")));
                                     gtk_widget_grab_focus(tr);
                                 }),
                                 nullptr);
                adw_action_row_add_suffix(ADW_ACTION_ROW(row), use);
            }
            adw_preferences_group_add(group, row);
            st->rows.push_back({row, GTK_WIDGET(group), lower(entry.syntax + " " + entry.text + " " + entry.example)});
        }
        st->groups.push_back(GTK_WIDGET(group));
        adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), group);
    }

    // Search: hides the rows (and sections) that don't match
    GtkWidget* search = gtk_search_entry_new();
    gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(search), "Search, e.g. cross, inverse, angle");
    gtk_widget_set_hexpand(search, TRUE);
    g_signal_connect(search, "search-changed", G_CALLBACK(+[](GtkSearchEntry* e, ReferenceState* s) {
                         std::string q = lower(gtk_editable_get_text(GTK_EDITABLE(e)));
                         for (GtkWidget* g : s->groups) {
                             bool any = false;
                             for (auto& r : s->rows) {
                                 if (r.group != g) continue;
                                 bool show = q.empty() || r.haystack.find(q) != std::string::npos;
                                 gtk_widget_set_visible(r.row, show);
                                 any = any || show;
                             }
                             gtk_widget_set_visible(g, any);
                         }
                     }),
                     st);
    GtkWidget* header = adw_header_bar_new();
    GtkWidget* clamp = adw_clamp_new();
    adw_clamp_set_maximum_size(ADW_CLAMP(clamp), 400);
    adw_clamp_set_child(ADW_CLAMP(clamp), search);
    adw_header_bar_set_title_widget(ADW_HEADER_BAR(header), clamp);

    GtkWidget* toolbar = adw_toolbar_view_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), header);
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), page);
    adw_dialog_set_child(dialog, toolbar);
    adw_dialog_present(dialog, parent);
}

} // namespace linalg::gtk
