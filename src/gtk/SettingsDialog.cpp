#include "gtk/SettingsDialog.hpp"
#include <adwaita.h>
#include <memory>

namespace linalg::gtk {

namespace {

constexpr const char* kFile = "gtk.txt";

const char* surfacesKey(UiSettings::Surfaces s) {
    return s == UiSettings::Surfaces::Smooth ? "smooth" : s == UiSettings::Surfaces::Faceted ? "faceted" : "style";
}

} // namespace

void GtkSettings::load() {
    SceneData data;
    if (!Preferences::load(data, kFile)) return;
    if (auto it = data.find("gtk.theme"); it != data.end()) theme = it->second == "light" || it->second == "dark" ? it->second : "";
    getBool(data, "ui.dim_unselected", dimUnselected);
    if (auto it = data.find("ui.surfaces"); it != data.end())
        surfaces = it->second == "smooth"    ? UiSettings::Surfaces::Smooth
                   : it->second == "faceted" ? UiSettings::Surfaces::Faceted
                                             : UiSettings::Surfaces::StyleDefault;
    int finished = 0;
    if (getInt(data, "tutorial.completed", finished)) lessonsFinished = static_cast<unsigned int>(finished);
}

void GtkSettings::save() const {
    SceneData data;
    data["gtk.theme"] = theme.empty() ? "system" : theme;
    putBool(data, "ui.dim_unselected", dimUnselected);
    data["ui.surfaces"] = surfacesKey(surfaces);
    putInt(data, "tutorial.completed", static_cast<int>(lessonsFinished));
    std::string error;
    Preferences::save(data, error, kFile);  // a failure only loses the preference
}

void showSettingsDialog(GtkWidget* parent, GtkSettings& settings, std::function<void()> changed) {
    // What the rows' callbacks share; deleted with the dialog
    struct State {
        GtkSettings* settings;
        std::function<void()> changed;
        void apply() {
            settings->save();
            if (changed) changed();
        }
    };
    auto* state = new State{&settings, std::move(changed)};

    AdwDialog* dialog = adw_preferences_dialog_new();
    g_object_set_data_full(G_OBJECT(dialog), "axismundi-state", state, [](gpointer p) { delete static_cast<State*>(p); });
    AdwPreferencesPage* page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "General");
    adw_preferences_page_set_icon_name(page, "preferences-system-symbolic");

    // Appearance: follow the system, or always light / dark
    AdwPreferencesGroup* look = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(look, "Appearance");
    GtkWidget* theme = adw_combo_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(theme), "Style");
    const char* themes[] = {"Follow the System", "Light", "Dark", nullptr};
    GtkStringList* themeModel = gtk_string_list_new(themes);
    adw_combo_row_set_model(ADW_COMBO_ROW(theme), G_LIST_MODEL(themeModel));
    g_object_unref(themeModel);
    adw_combo_row_set_selected(ADW_COMBO_ROW(theme), settings.theme == "light" ? 1 : settings.theme == "dark" ? 2 : 0);
    g_signal_connect(theme, "notify::selected", G_CALLBACK(+[](GtkWidget* row, GParamSpec*, State* s) {
                         guint i = adw_combo_row_get_selected(ADW_COMBO_ROW(row));
                         s->settings->theme = i == 1 ? "light" : i == 2 ? "dark" : "";
                         s->apply();
                     }),
                     state);
    adw_preferences_group_add(look, theme);
    adw_preferences_page_add(page, look);

    // The 3D view
    AdwPreferencesGroup* view = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(view, "3D View");
    GtkWidget* dim = adw_switch_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(dim), "Dim the Other Objects");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(dim), "While something is selected, the rest fades a little");
    adw_switch_row_set_active(ADW_SWITCH_ROW(dim), settings.dimUnselected);
    g_signal_connect(dim, "notify::active", G_CALLBACK(+[](GtkWidget* row, GParamSpec*, State* s) {
                         s->settings->dimUnselected = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
                         s->apply();
                     }),
                     state);
    adw_preferences_group_add(view, dim);
    GtkWidget* surfaces = adw_combo_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(surfaces), "Surfaces");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(surfaces), "Smooth shading, or flat cells that show the grid of samples");
    const char* kinds[] = {"As the View Style Says", "Smooth", "Faceted", nullptr};
    GtkStringList* surfaceModel = gtk_string_list_new(kinds);
    adw_combo_row_set_model(ADW_COMBO_ROW(surfaces), G_LIST_MODEL(surfaceModel));
    g_object_unref(surfaceModel);
    adw_combo_row_set_selected(ADW_COMBO_ROW(surfaces), static_cast<guint>(settings.surfaces));
    g_signal_connect(surfaces, "notify::selected", G_CALLBACK(+[](GtkWidget* row, GParamSpec*, State* s) {
                         s->settings->surfaces = static_cast<UiSettings::Surfaces>(adw_combo_row_get_selected(ADW_COMBO_ROW(row)));
                         s->apply();
                     }),
                     state);
    adw_preferences_group_add(view, surfaces);
    adw_preferences_page_add(page, view);

    adw_preferences_dialog_add(ADW_PREFERENCES_DIALOG(dialog), page);
    adw_dialog_present(dialog, parent);
}

} // namespace linalg::gtk
