#include "gtk/InfoPane.hpp"
#include "app/Session.hpp"
#include "scene/Analysis.hpp"
#include <adwaita.h>
#include <gtkmm/button.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/scrolledwindow.h>
#include <cmath>

namespace linalg::gtk {

namespace {

// A page: a scrolled column with margins
Gtk::Box* scrolledColumn(Gtk::ScrolledWindow*& scroll) {
    scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    scroll->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    scroll->set_vexpand(true);
    auto* column = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 12);
    column->set_margin(12);
    scroll->set_child(*column);
    return column;
}

Gtk::Label* wrappedLabel(const std::string& text) {
    auto* l = Gtk::make_managed<Gtk::Label>(text);
    l->set_wrap(true);
    l->set_wrap_mode(Pango::WrapMode::WORD_CHAR);
    l->set_xalign(0.0f);
    l->set_selectable(true);
    return l;
}

void clear(Gtk::Box& box) {
    while (Gtk::Widget* child = box.get_first_child()) box.remove(*child);
}

} // namespace

InfoPane::InfoPane(Session& session) : Gtk::Box(Gtk::Orientation::VERTICAL), m_session(session) {
    m_stack = ADW_VIEW_STACK(adw_view_stack_new());
    adw_view_stack_add_titled_with_icon(m_stack, buildAnalysis(), "analysis", "Analysis", "accessories-calculator-symbolic");
    adw_view_stack_add_titled_with_icon(m_stack, buildSteps(), "steps", "Steps", "view-list-ordered-symbolic");
    adw_view_stack_add_titled_with_icon(m_stack, buildBreakdown(), "breakdown", "Breakdown", "object-rotate-right-symbolic");
    g_signal_connect_swapped(m_stack, "notify::visible-child-name", G_CALLBACK(+[](InfoPane* self) { self->refresh(); }),
                             this);

    GtkWidget* switcher = m_switcher = adw_view_switcher_new();
    adw_view_switcher_set_stack(ADW_VIEW_SWITCHER(switcher), m_stack);
    adw_view_switcher_set_policy(ADW_VIEW_SWITCHER(switcher), ADW_VIEW_SWITCHER_POLICY_NARROW);
    GtkWidget* header = adw_header_bar_new();
    adw_header_bar_set_show_start_title_buttons(ADW_HEADER_BAR(header), FALSE);
    adw_header_bar_set_show_end_title_buttons(ADW_HEADER_BAR(header), FALSE);
    adw_header_bar_set_title_widget(ADW_HEADER_BAR(header), switcher);

    GtkWidget* toolbar = adw_toolbar_view_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), header);
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), GTK_WIDGET(m_stack));
    gtk_widget_set_vexpand(toolbar, TRUE);
    gtk_box_append(gobj(), toolbar);
}

void InfoPane::showPage(const std::string& name) { adw_view_stack_set_visible_child_name(m_stack, name.c_str()); }

std::string InfoPane::page() const {
    const char* name = adw_view_stack_get_visible_child_name(m_stack);
    return name ? name : "";
}

bool InfoPane::breakdownActive() const {
    return m_open && page() == "breakdown" && m_session.mode() == AppMode::Space3D && m_session.targetMatrix();
}

void InfoPane::refresh() {
    // The breakdown starts over each time it comes into view, as in the raylib app
    bool active = breakdownActive();
    if (active && !m_wasActive) m_player.restart();
    m_wasActive = active;
    if (!m_open) return;
    std::string p = page();
    if (p == "analysis") refreshAnalysis();
    else if (p == "steps") refreshSteps();
    else refreshBreakdown();
}

// --- Analysis ------------------------------------------------------------------------------------------------

GtkWidget* InfoPane::buildAnalysis() {
    Gtk::ScrolledWindow* scroll;
    m_analysis = scrolledColumn(scroll);
    return GTK_WIDGET(scroll->gobj());
}

void InfoPane::refreshAnalysis() {
    scene::SpaceKind kind = m_session.spaceKind();
    scene::Scene& sc = m_session.scene();
    scene::Analysis a = scene::analyze(sc.space(kind), sc.selection(kind));
    std::string key;
    for (const auto& s : a.sections) key += s.key + "|" + s.title + "|" + std::to_string(s.lines.size()) + ";";
    if (key != m_analysisKey) {
        m_analysisKey = key;
        clear(*m_analysis);
        m_analysisLines.clear();
        GtkWidget* group = adw_preferences_group_new();
        for (const auto& s : a.sections) {
            GtkWidget* expander = adw_expander_row_new();
            adw_preferences_row_set_use_markup(ADW_PREFERENCES_ROW(expander), FALSE);
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(expander), s.title.c_str());
            if (s.tip) gtk_widget_set_tooltip_text(expander, s.tip);
            auto open = m_expanded.find(s.key);
            adw_expander_row_set_expanded(ADW_EXPANDER_ROW(expander), open != m_expanded.end() ? open->second : s.expanded);
            for (const auto& line : s.lines) {
                Gtk::Label* label = wrappedLabel(line.text);
                label->set_margin_start(12);
                label->set_margin_end(12);
                label->set_margin_top(line.style == scene::AnalysisLine::Style::Mono ? 0 : 6);
                label->set_margin_bottom(line.style == scene::AnalysisLine::Style::Mono ? 0 : 6);
                if (line.style == scene::AnalysisLine::Style::Mono) label->add_css_class("monospace");
                if (line.style == scene::AnalysisLine::Style::Small) label->add_css_class("caption");
                if (line.tone == scene::AnalysisLine::Tone::Muted) label->add_css_class("dim-label");
                if (line.tone == scene::AnalysisLine::Tone::Eigen) label->add_css_class("accent");
                if (line.tone == scene::AnalysisLine::Tone::Danger) label->add_css_class("error");
                if (line.tip) label->set_tooltip_text(line.tip);
                GtkWidget* row = gtk_list_box_row_new();
                gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), FALSE);
                gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), GTK_WIDGET(label->gobj()));
                adw_expander_row_add_row(ADW_EXPANDER_ROW(expander), row);
                m_analysisLines.push_back(label);
            }
            // Remember the user's open / closed choice for this section
            using Remembered = std::pair<InfoPane*, std::string>;
            auto onExpanded = +[](GtkWidget* row, GParamSpec*, gpointer data) {
                auto* p = static_cast<Remembered*>(data);
                p->first->m_expanded[p->second] = adw_expander_row_get_expanded(ADW_EXPANDER_ROW(row));
            };
            auto forget = +[](gpointer data, GClosure*) { delete static_cast<Remembered*>(data); };
            g_signal_connect_data(expander, "notify::expanded", G_CALLBACK(onExpanded), new Remembered(this, s.key), forget,
                                  GConnectFlags(0));
            adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), expander);
        }
        gtk_box_append(m_analysis->gobj(), group);
        return;
    }
    // Same sections: only the numbers changed (the clock, an edit)
    size_t i = 0;
    for (const auto& s : a.sections)
        for (const auto& line : s.lines) {
            if (i < m_analysisLines.size() && m_analysisLines[i]->get_text() != line.text) m_analysisLines[i]->set_text(line.text);
            i++;
        }
}

// --- Steps ---------------------------------------------------------------------------------------------------

GtkWidget* InfoPane::buildSteps() {
    Gtk::ScrolledWindow* scroll;
    Gtk::Box* column = scrolledColumn(scroll);
    auto* hint = wrappedLabel("A worked calculation with the selected objects (else a, b, M, z1, z2), updated live.");
    hint->add_css_class("dim-label");
    hint->add_css_class("caption");
    column->append(*hint);
    m_topics = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    column->append(*m_topics);
    m_steps = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
    column->append(*m_steps);
    return GTK_WIDGET(scroll->gobj());
}

void InfoPane::refreshSteps() {
    StepsInput in = m_session.stepsInput();
    int& topic = in.complexPlane ? m_complexTopic : m_matrixTopic;
    if (m_topicButtons.empty() || in.complexPlane != m_topicsComplex) {
        m_topicsComplex = in.complexPlane;
        clear(*m_topics);
        m_topicButtons.clear();
        auto* flow = Gtk::make_managed<Gtk::FlowBox>();
        flow->set_selection_mode(Gtk::SelectionMode::NONE);
        flow->set_max_children_per_line(6);
        flow->set_column_spacing(6);
        flow->set_row_spacing(6);
        const auto& topics = stepTopics(in.complexPlane);
        for (size_t i = 0; i < topics.size(); i++) {
            auto* b = Gtk::make_managed<Gtk::ToggleButton>(topics[i]);
            b->add_css_class("pill");
            b->add_css_class("small");
            if (!m_topicButtons.empty()) b->set_group(*m_topicButtons.front());
            b->set_active(static_cast<int>(i) == topic);
            int index = static_cast<int>(i);
            b->signal_toggled().connect([this, b, index] {
                if (!b->get_active()) return;
                (m_topicsComplex ? m_complexTopic : m_matrixTopic) = index;
                m_stepsText.clear();
                refreshSteps();
            });
            flow->append(*b);
            m_topicButtons.push_back(b);
        }
        m_topics->append(*flow);
    }
    Explanation e = explainTopic(in, topic);
    std::string text;
    for (const auto& line : e) text += std::to_string(static_cast<int>(line.kind)) + line.text + "\n";
    if (text == m_stepsText) return;
    m_stepsText = text;
    clear(*m_steps);
    for (const auto& line : e) {
        Gtk::Label* l = wrappedLabel(line.text);
        switch (line.kind) {
            case ExplainLine::Kind::Heading:
                l->add_css_class("heading");
                l->set_margin_top(10);
                break;
            case ExplainLine::Kind::Formula:
                l->add_css_class("monospace");
                l->add_css_class("accent");
                break;
            case ExplainLine::Kind::Step: l->add_css_class("monospace"); break;
            case ExplainLine::Kind::Result:
                l->add_css_class("monospace");
                l->add_css_class("success");
                l->set_margin(8);
                break;
            case ExplainLine::Kind::Note:
                l->add_css_class("caption");
                l->add_css_class("dim-label");
                break;
        }
        if (line.kind == ExplainLine::Kind::Result) {
            // The result stands out in a card
            auto* card = Gtk::make_managed<Gtk::Box>();
            card->add_css_class("card");
            card->set_margin_top(4);
            card->set_margin_bottom(4);
            card->append(*l);
            m_steps->append(*card);
        } else {
            m_steps->append(*l);
        }
    }
}

// --- Breakdown -----------------------------------------------------------------------------------------------

GtkWidget* InfoPane::buildBreakdown() {
    Gtk::ScrolledWindow* scroll;
    Gtk::Box* column = scrolledColumn(scroll);
    auto* hint = wrappedLabel("Every matrix is a rotation, a stretch along the axes and another rotation (plus a move "
                              "for 4×4). Play it, or click a step.");
    hint->add_css_class("dim-label");
    hint->add_css_class("caption");
    column->append(*hint);
    m_breakdownNote = wrappedLabel("");
    column->append(*m_breakdownNote);
    m_breakdownSteps = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
    column->append(*m_breakdownSteps);

    auto* controls = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
    auto button = [&](const char* icon, const char* tip, std::function<void()> run) {
        auto* b = Gtk::make_managed<Gtk::Button>();
        b->set_icon_name(icon);
        b->set_tooltip_text(tip);
        b->add_css_class("flat");
        b->signal_clicked().connect([this, run] {
            run();
            refreshBreakdown();
        });
        controls->append(*b);
        return b;
    };
    button("view-refresh-symbolic", "Start again from the identity and play", [this] { m_player.restart(); });
    button("go-previous-symbolic", "Back one step", [this] { m_player.previous(); });
    m_playButton = button("media-playback-start-symbolic", "Play or pause", [this] { m_player.togglePlay(); });
    button("go-next-symbolic", "Jump to the end of the next step", [this] { m_player.next(); });
    m_breakdownScale = Gtk::make_managed<Gtk::Scale>(Gtk::Orientation::HORIZONTAL);
    m_breakdownScale->set_range(0.0, 1.0);
    m_breakdownScale->set_draw_value(false);
    m_breakdownScale->set_hexpand(true);
    m_breakdownScale->signal_value_changed().connect([this] {
        if (m_seeking) return;
        m_player.seek(static_cast<float>(m_breakdownScale->get_value()));
    });
    controls->append(*m_breakdownScale);
    column->append(*controls);
    return GTK_WIDGET(scroll->gobj());
}

void InfoPane::refreshBreakdown() {
    bool space3D = m_session.mode() == AppMode::Space3D;
    bool hasMatrix = m_session.targetMatrix() != nullptr;
    if (space3D && hasMatrix) m_player.setMatrix(m_session.matrix());
    const auto& steps = m_player.breakdown().steps;
    int n = space3D && hasMatrix ? static_cast<int>(steps.size()) : 0;
    std::string key = std::to_string(n) + (space3D ? "" : "c") + (hasMatrix ? "" : "-");
    for (int i = 0; i < n; i++) key += steps[i].title + "|" + steps[i].description + ";";
    if (key != m_breakdownKey) {
        m_breakdownKey = key;
        clear(*m_breakdownSteps);
        m_stepRows.clear();
        m_breakdownNote->set_text(!space3D     ? "The breakdown works on a matrix in the 3D space."
                                  : !hasMatrix ? "Add a matrix (or select one) to break it down."
                                  : n == 0     ? "M is the identity: nothing moves."
                                               : "");
        m_breakdownNote->set_visible(!m_breakdownNote->get_text().empty());
        for (int i = 0; i < n; i++) {
            auto* button = Gtk::make_managed<Gtk::Button>();
            button->add_css_class("flat");
            auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);
            auto* badge = Gtk::make_managed<Gtk::Label>(std::to_string(i + 1));
            badge->add_css_class("heading");
            badge->set_width_chars(2);
            badge->set_valign(Gtk::Align::START);
            auto* texts = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 2);
            texts->set_hexpand(true);
            auto* title = Gtk::make_managed<Gtk::Label>(steps[i].title);
            title->set_xalign(0.0f);
            title->add_css_class("heading");
            auto* description = Gtk::make_managed<Gtk::Label>(steps[i].description);
            description->set_xalign(0.0f);
            description->set_wrap(true);
            description->add_css_class("caption");
            description->add_css_class("dim-label");
            auto* bar = Gtk::make_managed<Gtk::ProgressBar>();
            texts->append(*title);
            texts->append(*description);
            texts->append(*bar);
            box->append(*badge);
            box->append(*texts);
            button->set_child(*box);
            button->signal_clicked().connect([this, i] {
                m_player.seek(static_cast<float>(i + 1));
                refreshBreakdown();
            });
            m_breakdownSteps->append(*button);
            m_stepRows.push_back({badge, button, bar});
        }
        m_seeking = true;
        m_breakdownScale->set_range(0.0, std::max(1, n));
        m_seeking = false;
        m_breakdownScale->get_parent()->set_visible(n > 0);
    }
    float t = m_player.progress();
    for (int i = 0; i < static_cast<int>(m_stepRows.size()); i++) {
        bool done = t >= i + 1 - 1e-4f;
        bool current = !done && t > i - 1e-4f && t < i + 1;
        StepRow& r = m_stepRows[i];
        std::string badge = done ? "✓" : std::to_string(i + 1);
        if (r.badge->get_text() != badge) r.badge->set_text(badge);
        if (current) r.row->add_css_class("accent");
        else r.row->remove_css_class("accent");
        auto* bar = static_cast<Gtk::ProgressBar*>(r.bar);
        bar->set_visible(current);
        if (current) bar->set_fraction(t - i);
    }
    m_seeking = true;
    if (std::fabs(m_breakdownScale->get_value() - t) > 1e-4) m_breakdownScale->set_value(t);
    m_seeking = false;
    m_playButton->set_icon_name(m_player.playing() ? "media-playback-pause-symbolic" : "media-playback-start-symbolic");
}

} // namespace linalg::gtk
