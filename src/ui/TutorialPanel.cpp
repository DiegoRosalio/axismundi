#include "ui/TutorialPanel.hpp"
#include "app/App.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <cstdio>

namespace linalg {

void TutorialPanel::startLesson(int lesson) {
    m_lesson = lesson;
    m_step = 0;
    m_needsSetup = true;
}

void TutorialPanel::enterStep(App& app) {
    const TutorialStep& s = lessons()[m_lesson].steps[m_step];
    m_context = TutorialContext{};
    m_done = false;
    if (s.setup) s.setup(app);
    m_needsSetup = false;
}

bool TutorialPanel::stepDone() const {
    if (m_lesson < 0) return false;
    const TutorialStep& s = lessons()[m_lesson].steps[m_step];
    return !s.task || m_done;
}

void TutorialPanel::update(App& app) {
    if (m_lesson < 0) return;
    if (m_needsSetup) enterStep(app);
    const TutorialStep& s = lessons()[m_lesson].steps[m_step];
    if (!app.uiMode()) m_context.cameraModeUsed = true;
    // Once done, a task stays done even if the user keeps experimenting
    if (s.check && !m_done && s.check(app, m_context)) m_done = true;
}

std::string TutorialPanel::currentAnchor() const {
    if (m_lesson < 0) return "";
    const TutorialStep& s = lessons()[m_lesson].steps[m_step];
    return s.anchor ? s.anchor : "";
}

float TutorialPanel::drawContent(Rectangle area, App& app) {
    const ui::Theme& t = ui::theme();
    float pad = ui::px(ui::kPad);
    ui::Column col(area.x + pad, area.y + ui::px(12.0f), area.width - 2 * pad);
    const auto& all = lessons();

    // --- Lesson list -------------------------------------------------------
    if (m_lesson < 0) {
        if (!m_finishedMessage.empty()) {
            Rectangle note = col.row(34);
            Color soft = t.success;
            soft.a = 36;
            ui::fillRounded(note, ui::px(ui::kRadius), soft);
            ui::drawIcon(ui::Icon::Check, {note.x + ui::px(10), note.y + ui::px(9), ui::px(16), ui::px(16)}, t.success);
            ui::textIn({note.x + ui::px(34), note.y, note.width, note.height}, m_finishedMessage.c_str(),
                       ui::FontRole::Body, t.text);
        }
        float h = ui::textWrapped({col.x(), col.y(), col.width(), 0},
                                  "Short guided lessons. Each step sets up the scene, explains it and gives you "
                                  "a small task that is checked as you work.",
                                  ui::FontRole::Small, t.textMuted);
        col.space(h / ui::scale() + 4.0f);

        for (int i = 0; i < static_cast<int>(all.size()); i++) {
            Rectangle row = col.row(50.0f);
            bool hover = ui::hovered(row);
            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                startLesson(i);
                m_finishedMessage.clear();
            }
            ui::fillRounded(row, ui::px(ui::kRadius), hover ? t.hover : t.panelAlt);
            bool done = (m_completed >> i) & 1u;
            float r = ui::px(13.0f);
            Vector2 c = {row.x + ui::px(10) + r, row.y + row.height / 2};
            DrawCircleV(c, r, done ? t.success : t.panel);
            if (done) {
                ui::drawIcon(ui::Icon::Check, {c.x - r * 0.6f, c.y - r * 0.6f, r * 1.2f, r * 1.2f}, t.onAccent);
            } else {
                DrawRing(c, r - ui::pxi(1), r, 0, 360, 32, t.borderStrong);
                char num[8];
                snprintf(num, sizeof(num), "%d", i + 1);
                ui::textIn({c.x - r, c.y - r, 2 * r, 2 * r}, num, ui::FontRole::Small, t.textMuted, ui::Align::Center);
            }
            float tx = c.x + r + ui::px(12);
            Rectangle chevron = {row.x + row.width - ui::px(26), row.y + (row.height - ui::px(14)) / 2, ui::px(14),
                                 ui::px(14)};
            ui::drawIcon(ui::Icon::ChevronRight, chevron, hover ? t.text : t.textFaint);
            ui::text(all[i].title, tx, row.y + ui::px(7), ui::FontRole::BodyStrong, t.text);
            char sub[96];
            snprintf(sub, sizeof(sub), "%s · %zu step%s", all[i].summary, all[i].steps.size(),
                     all[i].steps.size() == 1 ? "" : "s");
            std::string s = ui::ellipsize(sub, ui::FontRole::Small, chevron.x - tx - ui::px(6));
            ui::text(s.c_str(), tx, row.y + ui::px(28), ui::FontRole::Small, t.textMuted);
        }
        return col.y() - area.y + ui::px(4.0f);
    }

    // --- Step view ---------------------------------------------------------
    const Lesson& lesson = all[m_lesson];
    const TutorialStep& step = lesson.steps[m_step];
    int count = static_cast<int>(lesson.steps.size());

    Rectangle head = col.row(28);
    Rectangle back = ui::takeLeft(head, 110.0f, 8.0f);
    if (ui::button(back, "Lessons", ui::ButtonKind::Ghost, true, ui::Icon::ChevronLeft)) {
        m_lesson = -1;
        return col.y() - area.y;
    }
    char progress[32];
    snprintf(progress, sizeof(progress), "Step %d of %d", m_step + 1, count);
    ui::textIn(head, progress, ui::FontRole::Small, t.textMuted, ui::Align::Right);

    // Progress dots
    Rectangle dots = col.row(10);
    float dotW = std::min(ui::px(28.0f), (dots.width - ui::px(4) * (count - 1)) / count);
    for (int i = 0; i < count; i++) {
        Rectangle d = {dots.x + i * (dotW + ui::px(4)), dots.y + ui::px(3), dotW, ui::px(4)};
        ui::fillRounded(d, ui::px(2), i < m_step ? t.success : i == m_step ? t.accent : t.border);
    }

    ui::textIn(col.row(18), ui::upper(lesson.title).c_str(), ui::FontRole::Caption, t.accent);
    float h = ui::textWrapped({col.x(), col.y(), col.width(), 0}, step.title, ui::FontRole::Title, t.text);
    col.space(h / ui::scale() + 2.0f);
    h = ui::textWrapped({col.x(), col.y(), col.width(), 0}, step.body, ui::FontRole::Body, t.text, true, 1.45f);
    col.space(h / ui::scale() + 6.0f);

    // Task box
    if (step.task) {
        bool done = m_done;
        float innerW = col.width() - ui::px(46);
        float taskH = ui::textWrapped({0, 0, innerW, 0}, step.task, ui::FontRole::BodyStrong, t.text, false);
        float hintH = (!done && step.hint)
                          ? ui::textWrapped({0, 0, innerW, 0}, step.hint, ui::FontRole::Small, t.text, false) + ui::px(4)
                          : 0.0f;
        Rectangle box = col.rowPx(taskH + hintH + ui::px(24));
        Color fill = done ? t.success : t.accent;
        fill.a = done ? 36 : 26;
        ui::fillRounded(box, ui::px(ui::kRadius), fill);
        ui::strokeRounded(box, ui::px(ui::kRadius), ui::pxi(1), done ? t.success : t.accent);
        float r = ui::px(10);
        Vector2 c = {box.x + ui::px(12) + r, box.y + ui::px(12) + r};
        if (done) {
            DrawCircleV(c, r, t.success);
            ui::drawIcon(ui::Icon::Check, {c.x - r * 0.65f, c.y - r * 0.65f, r * 1.3f, r * 1.3f}, t.onAccent);
        } else {
            DrawRing(c, r - ui::pxi(1.5f), r, 0, 360, 32, t.accent);
        }
        float tx = box.x + ui::px(40);
        ui::textWrapped({tx, box.y + ui::px(12), innerW, 0}, step.task, ui::FontRole::BodyStrong, t.text);
        if (hintH > 0) {
            ui::textWrapped({tx, box.y + ui::px(14) + taskH, innerW, 0}, step.hint, ui::FontRole::Small, t.textMuted);
        }
        col.space(4.0f);
    }

    // Navigation
    auto buttons = ui::splitRow(col.row(ui::kRowHeight), {1, 1, 1.3f}, 8.0f);
    if (ui::button(buttons[0], "Back", ui::ButtonKind::Secondary, m_step > 0, ui::Icon::ChevronLeft)) {
        m_step--;
        m_needsSetup = true;
    }
    bool last = (m_step == count - 1);
    bool canAdvance = stepDone();
    if (step.task && !canAdvance) {
        if (ui::button(buttons[1], "Skip", ui::ButtonKind::Ghost)) canAdvance = true, m_done = true;
    }
    if (ui::button(buttons[2], last ? "Finish" : "Next", ui::ButtonKind::Primary, stepDone(),
                   last ? ui::Icon::Check : ui::Icon::ChevronRight)) {
        if (last) {
            m_completed |= 1u << m_lesson;
            m_finishedMessage = std::string("Finished: ") + lesson.title;
            app.toast(m_finishedMessage);
            m_lesson = -1;
        } else {
            m_step++;
            m_needsSetup = true;
        }
    }
    return col.y() - area.y + ui::px(4.0f);
}

void TutorialPanel::saveState(SceneData& prefs) const {
    putInt(prefs, "tutorial.completed", static_cast<int>(m_completed));
}

void TutorialPanel::loadState(const SceneData& prefs) {
    int v = 0;
    if (getInt(prefs, "tutorial.completed", v)) m_completed = static_cast<unsigned int>(v);
}

} // namespace linalg
