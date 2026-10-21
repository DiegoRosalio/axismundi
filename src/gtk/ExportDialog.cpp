#include "gtk/ExportDialog.hpp"
#include "app/Session.hpp"
#include "export/PdfCompile.hpp"
#include "rendering/WorldMapping.hpp"
#include <adwaita.h>
#include <glibmm/main.h>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>

namespace linalg::gtk {

namespace {

const std::string& compiler() {
    static const std::string found = latex::latexCompiler();  // looked up once
    return found;
}

std::string fileName(const std::string& path) { return std::filesystem::path(path).filename().string(); }

struct ExportState {
    Session* session;
    ExportSettings* settings;
    ExportCallbacks callbacks;
    GtkTextBuffer* buffer = nullptr;
    GtkWidget* status = nullptr;
    std::string shown;  // the code in the preview
    guint timer = 0;

    latex::Options options(bool standalone) const {
        latex::Options o = settings->options;
        Vector3 p = session->camera().position(), t = session->camera().target();
        latex::viewFromDirection(worldToMath(Vector3{p.x - t.x, p.y - t.y, p.z - t.z}), o.azimuth, o.elevation);
        o.azimuth = std::round(o.azimuth);
        o.elevation = std::round(o.elevation);
        if (standalone) o.standalone = true;
        return o;
    }
    latex::Result generate(bool standalone = false) const {
        scene::Scene& sc = session->scene();
        scene::SpaceKind kind = session->spaceKind();
        return latex::exportLatex(sc.space(kind), sc.selection(kind), options(standalone));
    }
    void refresh() {
        latex::Result r = generate();
        if (r.code != shown) {
            shown = r.code;
            gtk_text_buffer_set_text(buffer, shown.c_str(), -1);
        }
        std::string text = std::to_string(r.exported) + (r.exported == 1 ? " object" : " objects") +
                           (r.flat ? ", 2D axis" : ", 3D axis");
        if (!r.skipped.empty()) {
            text += ". Not exported: ";
            for (size_t i = 0; i < r.skipped.size(); i++) text += (i ? ", " : "") + r.skipped[i];
        }
        gtk_label_set_text(GTK_LABEL(status), text.c_str());
    }
    void setPreview(bool on) {
        ViewSettings& v = session->view();
        if (on == settings->preview) return;
        if (on) {
            settings->styleBefore = v.style == view::StyleId::Paper ? view::StyleId::Colorful : v.style;
            v.style = view::StyleId::Paper;
        } else {
            v.style = settings->styleBefore;
        }
        settings->preview = on;
        if (callbacks.viewChanged) callbacks.viewChanged();
    }
};

// What a file dialog's answer needs (the export dialog may be closed by then)
struct SaveJob {
    std::string code;
    std::function<void(const std::string&)> toast;
    bool pdf;
};

GtkFileDialog* fileDialog(const char* title, const char* name, const char* pattern, const char* filterName) {
    GtkFileDialog* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, title);
    gtk_file_dialog_set_initial_name(dialog, name);
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, filterName);
    gtk_file_filter_add_pattern(filter, pattern);
    GListStore* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    g_object_unref(filters);
    g_object_unref(filter);
    return dialog;
}

void saved(GObject* source, GAsyncResult* result, gpointer data) {
    std::unique_ptr<SaveJob> job(static_cast<SaveJob*>(data));
    GFile* file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), result, nullptr);
    if (!file) return;  // cancelled
    char* raw = g_file_get_path(file);
    g_object_unref(file);
    if (!raw) return;
    std::string path = raw;
    g_free(raw);
    if (!job->pdf) {
        std::ofstream out(path);
        out << job->code;
        job->toast(out ? "Saved " + fileName(path) : "Could not write " + path);
        return;
    }
    // The PDF compiles in the background; a timer reports when it's done
    struct Pdf {
        std::atomic<bool> done{false};
        latex::PdfResult result;
    };
    auto pdf = std::make_shared<Pdf>();
    std::string code = job->code;
    std::thread([pdf, code, path] {
        pdf->result = latex::compilePdf(code, path, 30, compiler());
        pdf->done = true;
    }).detach();
    job->toast("Compiling the PDF…");
    auto toast = job->toast;
    Glib::signal_timeout().connect([pdf, toast, path] {
        if (!pdf->done) return true;
        toast(pdf->result.ok ? "Saved " + fileName(path) : "PDF failed: " + pdf->result.message);
        return false;
    }, 250);
}

GtkWidget* switchRow(const char* title, const char* subtitle, bool active) {
    GtkWidget* row = adw_switch_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
    if (subtitle) adw_action_row_set_subtitle(ADW_ACTION_ROW(row), subtitle);
    adw_switch_row_set_active(ADW_SWITCH_ROW(row), active);
    return row;
}

GtkWidget* comboRow(const char* title, std::initializer_list<const char*> items, int selected) {
    GtkWidget* row = adw_combo_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
    GtkStringList* model = gtk_string_list_new(nullptr);
    for (const char* item : items) gtk_string_list_append(model, item);
    adw_combo_row_set_model(ADW_COMBO_ROW(row), G_LIST_MODEL(model));
    g_object_unref(model);
    adw_combo_row_set_selected(ADW_COMBO_ROW(row), static_cast<guint>(selected));
    return row;
}

} // namespace

void showExportDialog(GtkWidget* parent, Session& session, ExportSettings& settings, ExportCallbacks callbacks) {
    auto* st = new ExportState;
    st->session = &session;
    st->settings = &settings;
    st->callbacks = std::move(callbacks);
    AdwDialog* dialog = adw_dialog_new();
    adw_dialog_set_title(dialog, "Export to LaTeX");
    adw_dialog_set_content_width(dialog, 620);
    adw_dialog_set_content_height(dialog, 720);
    g_object_set_data_full(G_OBJECT(dialog), "axismundi-state", st, [](gpointer p) { delete static_cast<ExportState*>(p); });
    latex::Options& o = settings.options;

    GtkWidget* page = adw_preferences_page_new();
    AdwPreferencesGroup* what = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(what, "Figure");
    adw_preferences_group_set_description(what, "A black-and-white PGFPlots figure of this space, seen as the camera sees it.");
    GtkWidget* scope = comboRow("Objects", {"Visible Objects", "Selection"}, static_cast<int>(o.scope));
    GtkWidget* axis = comboRow("Axis", {"Automatic", "2D", "3D"}, static_cast<int>(o.view));
    gtk_widget_set_tooltip_text(axis, "Automatic picks 2D for the complex plane or when everything lies in z = 0");
    GtkWidget* width = adw_spin_row_new_with_range(0.3, 1.0, 0.05);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(width), "Width");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(width), "Fraction of the text width");
    adw_spin_row_set_digits(ADW_SPIN_ROW(width), 2);
    adw_spin_row_set_value(ADW_SPIN_ROW(width), o.width);
    for (GtkWidget* r : {scope, axis, width}) adw_preferences_group_add(what, r);

    AdwPreferencesGroup* look = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(look, "Drawing");
    GtkWidget* axes = switchRow("Axes", nullptr, o.axes);
    GtkWidget* legend = switchRow("Legend", "Names of the plots (vectors are labelled at their tips)", o.legend);
    GtkWidget* grid = switchRow("Grid", nullptr, o.grid);
    GtkWidget* gray = switchRow("Gray Shading", "Surfaces shaded by height, else white with a black mesh", o.grayShading);
    GtkWidget* standalone = switchRow("Complete Document", "A document that compiles on its own", o.standalone);
    GtkWidget* preview = switchRow("Black-and-White Preview", "The 3D view in the Paper style while this is open",
                                   settings.preview);
    for (GtkWidget* r : {axes, legend, grid, gray, standalone, preview}) adw_preferences_group_add(look, r);

    // Every change regenerates the code
    g_signal_connect(scope, "notify::selected", G_CALLBACK(+[](GtkWidget* r, GParamSpec*, ExportState* s) {
                         s->settings->options.scope = static_cast<latex::Scope>(adw_combo_row_get_selected(ADW_COMBO_ROW(r)));
                         s->refresh();
                     }), st);
    g_signal_connect(axis, "notify::selected", G_CALLBACK(+[](GtkWidget* r, GParamSpec*, ExportState* s) {
                         s->settings->options.view = static_cast<latex::View>(adw_combo_row_get_selected(ADW_COMBO_ROW(r)));
                         s->refresh();
                     }), st);
    g_signal_connect(width, "notify::value", G_CALLBACK(+[](GtkWidget* r, GParamSpec*, ExportState* s) {
                         s->settings->options.width = adw_spin_row_get_value(ADW_SPIN_ROW(r));
                         s->refresh();
                     }), st);
    // The switches, by their place in this table
    static bool latex::Options::* const kSwitches[] = {&latex::Options::axes, &latex::Options::legend, &latex::Options::grid,
                                                       &latex::Options::grayShading, &latex::Options::standalone};
    auto boolRow = [st](GtkWidget* row, int index) {
        g_object_set_data(G_OBJECT(row), "switch", GINT_TO_POINTER(index));
        g_signal_connect(row, "notify::active", G_CALLBACK(+[](GtkWidget* r, GParamSpec*, ExportState* s) {
                             int i = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(r), "switch"));
                             s->settings->options.*kSwitches[i] = adw_switch_row_get_active(ADW_SWITCH_ROW(r));
                             s->refresh();
                         }), st);
    };
    boolRow(axes, 0);
    boolRow(legend, 1);
    boolRow(grid, 2);
    boolRow(gray, 3);
    boolRow(standalone, 4);
    g_signal_connect(preview, "notify::active", G_CALLBACK(+[](GtkWidget* r, GParamSpec*, ExportState* s) {
                         s->setPreview(adw_switch_row_get_active(ADW_SWITCH_ROW(r)));
                     }), st);

    // The code
    AdwPreferencesGroup* code = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(code, "Code");
    st->status = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(st->status), TRUE);
    gtk_label_set_xalign(GTK_LABEL(st->status), 0.0f);
    gtk_widget_add_css_class(st->status, "dim-label");
    gtk_widget_add_css_class(st->status, "caption");
    GtkWidget* view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 8);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(view), 8);
    st->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), view);
    gtk_widget_set_size_request(scroll, -1, 240);
    gtk_widget_add_css_class(scroll, "card");
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_append(GTK_BOX(box), st->status);
    gtk_box_append(GTK_BOX(box), scroll);

    GtkWidget* buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_set_homogeneous(GTK_BOX(buttons), TRUE);
    GtkWidget* copy = gtk_button_new_with_label("Copy");
    GtkWidget* saveTex = gtk_button_new_with_label("Save .tex…");
    GtkWidget* savePdf = gtk_button_new_with_label("Save PDF…");
    gtk_widget_add_css_class(saveTex, "suggested-action");
    if (compiler().empty()) {
        gtk_widget_set_sensitive(savePdf, FALSE);
        gtk_widget_set_tooltip_text(savePdf, "Needs pdflatex or lualatex (TeX Live)");
    }
    g_signal_connect(copy, "clicked", G_CALLBACK(+[](GtkWidget* b, ExportState* s) {
                         gdk_clipboard_set_text(gtk_widget_get_clipboard(b), s->generate().code.c_str());
                         s->callbacks.toast("LaTeX code copied");
                     }), st);
    auto onSaveTex = +[](GtkWidget* b, ExportState* s) {
        GtkFileDialog* d = fileDialog("Save the LaTeX Figure", "figure.tex", "*.tex", "LaTeX (*.tex)");
        auto* job = new SaveJob{s->generate().code, s->callbacks.toast, false};
        gtk_file_dialog_save(d, GTK_WINDOW(gtk_widget_get_root(b)), nullptr, saved, job);
        g_object_unref(d);
    };
    auto onSavePdf = +[](GtkWidget* b, ExportState* s) {
        GtkFileDialog* d = fileDialog("Save the Figure as PDF", "figure.pdf", "*.pdf", "PDF (*.pdf)");
        auto* job = new SaveJob{s->generate(true).code, s->callbacks.toast, true};
        gtk_file_dialog_save(d, GTK_WINDOW(gtk_widget_get_root(b)), nullptr, saved, job);
        g_object_unref(d);
    };
    g_signal_connect(saveTex, "clicked", G_CALLBACK(onSaveTex), st);
    g_signal_connect(savePdf, "clicked", G_CALLBACK(onSavePdf), st);
    for (GtkWidget* b : {copy, saveTex, savePdf}) gtk_box_append(GTK_BOX(buttons), b);
    gtk_box_append(GTK_BOX(box), buttons);
    adw_preferences_group_add(code, box);

    for (AdwPreferencesGroup* g : {what, look, code}) adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), g);
    GtkWidget* toolbar = adw_toolbar_view_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), adw_header_bar_new());
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), page);
    adw_dialog_set_child(dialog, toolbar);

    // The scene can still change while the dialog is open (the clock plays): refresh twice a second
    st->refresh();
    st->timer = g_timeout_add(500, +[](gpointer s) -> gboolean {
        static_cast<ExportState*>(s)->refresh();
        return G_SOURCE_CONTINUE;
    }, st);
    g_signal_connect(dialog, "closed", G_CALLBACK(+[](AdwDialog*, ExportState* s) {
                         g_source_remove(s->timer);
                         s->setPreview(false);  // the preview ends with the dialog
                     }), st);
    adw_dialog_present(dialog, parent);
}

} // namespace linalg::gtk
