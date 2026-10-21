#include "gtk/GalleryDialog.hpp"
#include "app/AppApi.hpp"
#include "edu/Examples.hpp"
#include <adwaita.h>
#include <string>

namespace linalg::gtk {

namespace {

struct GalleryState {
    AppApi* api;
    std::function<void(const char*)> opened;
    AdwDialog* dialog;
};

} // namespace

void showGalleryDialog(GtkWidget* parent, AppApi& api, std::function<void(const char*)> opened) {
    AdwDialog* dialog = adw_dialog_new();
    auto* st = new GalleryState{&api, std::move(opened), dialog};
    g_object_set_data_full(G_OBJECT(dialog), "axismundi-state", st, [](gpointer p) { delete static_cast<GalleryState*>(p); });
    adw_dialog_set_title(dialog, "Examples");
    adw_dialog_set_content_width(dialog, 520);
    adw_dialog_set_content_height(dialog, 620);

    GtkWidget* page = adw_preferences_page_new();
    AdwPreferencesGroup* intro = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_description(intro, "Each example replaces what is in its space.");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), intro);
    const auto& list = examples();
    for (ExampleCategory category : {ExampleCategory::Matrices, ExampleCategory::Plots, ExampleCategory::Complex}) {
        AdwPreferencesGroup* group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
        adw_preferences_group_set_title(group, categoryName(category));
        for (size_t i = 0; i < list.size(); i++) {
            if (list[i].category != category) continue;
            GtkWidget* row = adw_action_row_new();
            adw_preferences_row_set_use_markup(ADW_PREFERENCES_ROW(row), FALSE);
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), list[i].title);
            adw_action_row_set_subtitle(ADW_ACTION_ROW(row), list[i].description);
            gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
            adw_action_row_add_suffix(ADW_ACTION_ROW(row), gtk_image_new_from_icon_name("go-next-symbolic"));
            g_object_set_data(G_OBJECT(row), "example", GSIZE_TO_POINTER(i));
            g_signal_connect(row, "activated", G_CALLBACK(+[](GtkWidget* r, GalleryState* s) {
                                 const Example& e = examples()[GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(r), "example"))];
                                 e.apply(*s->api);
                                 if (s->opened) s->opened(e.title);
                                 adw_dialog_close(s->dialog);
                             }),
                             st);
            adw_preferences_group_add(group, row);
        }
        adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), group);
    }

    GtkWidget* toolbar = adw_toolbar_view_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), adw_header_bar_new());
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), page);
    adw_dialog_set_child(dialog, toolbar);
    adw_dialog_present(dialog, parent);
}

} // namespace linalg::gtk
