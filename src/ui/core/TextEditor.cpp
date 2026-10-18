#include "ui/core/TextEditor.hpp"
#include "text/TextLayout.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"
#include <algorithm>
#include <cmath>

namespace linalg::ui {

namespace {

constexpr float kPadX = 10.0f, kPadY = 7.0f;

float charWidth() {
    return std::max(1.0f, textWidth("M", FontRole::Mono));
}
float lineHeight() {
    return fontSize(FontRole::Mono) * 1.3f;
}
size_t charsPerLine(float width) {
    float inner = width - 2 * px(kPadX);
    return static_cast<size_t>(std::max(1.0f, std::floor(inner / charWidth())));
}

bool ctrlDown() {
    return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
}
bool shiftDown() {
    return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}
bool pressedOrRepeat(int key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

} // namespace

EditorState& inlineEditorState() {
    static EditorState state;
    return state;
}

int editorLines(const std::string& text, float width) {
    return static_cast<int>(text::wrapLines(text, charsPerLine(width)).size());
}

float editorHeight(const std::string& text, float width, int maxLines) {
    int lines = std::clamp(editorLines(text, width), 1, std::max(1, maxLines));
    return 2 * px(kPadY) + lines * lineHeight() + px(4);
}

EditorEvent textEditor(Rectangle r, int id, std::string& s, bool active, bool invalid, EditorState& st,
                       bool selectAllOnFocus) {
    const Theme& t = theme();
    EditorEvent event = EditorEvent::None;
    bool hover = hovered(r);
    float radius = px(kRadius);

    if (active && st.id != id) {  // focused by Tab (or programmatically)
        st = EditorState{};
        st.id = id;
        st.cursor = st.anchor = s.size();
        if (selectAllOnFocus) st.anchor = 0;
    }
    // The state is shared by all inline editors: only the active one may touch it
    if (active) {
        st.cursor = std::min(st.cursor, s.size());
        st.anchor = std::min(st.anchor, s.size());
    }

    const size_t perLine = charsPerLine(r.width);
    std::vector<size_t> starts = text::wrapLines(s, perLine);
    const float lh = lineHeight(), cw = charWidth();
    // A field always shows at least one line: when it is barely taller than a line (small windows,
    // where the mono font stops shrinking), that line is centered instead of padded from the top
    const float left = r.x + px(kPadX);
    const bool tight = r.height - 2 * px(kPadY) < lh;
    const float top = tight ? r.y + (r.height - lh) / 2 : r.y + px(kPadY);

    auto positionAt = [&](Vector2 m) {
        float sc = active ? st.scroll : 0.0f;  // an inactive field is not scrolled
        size_t line = static_cast<size_t>(std::max(0.0f, std::floor((m.y - top + sc) / lh)));
        size_t col = static_cast<size_t>(std::max(0.0f, std::round((m.x - left) / cw)));
        return text::lineColToCursor(s, starts, line, col);
    };

    // Mouse: activate, place the cursor, select by dragging, commit on a click outside
    Vector2 mouse = GetMousePosition();
    if (!active) {
        if (clicked(r)) {
            event = EditorEvent::Activated;
            st = EditorState{};
            st.id = id;
            if (selectAllOnFocus) {
                st.anchor = 0;
                st.cursor = s.size();
            } else {
                st.cursor = st.anchor = positionAt(mouse);
                st.dragging = true;
            }
        }
    } else {
        if (ui::mousePressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mouse, r) && canInteract()) {
                st.cursor = positionAt(mouse);
                if (!shiftDown()) st.anchor = st.cursor;
                st.dragging = true;
            } else {
                event = EditorEvent::Commit;
            }
        }
        if (st.dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) st.cursor = positionAt(mouse);
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) st.dragging = false;
    }

    // Keyboard
    if (active && event == EditorEvent::None) {
        size_t a = std::min(st.anchor, st.cursor), b = std::max(st.anchor, st.cursor);
        bool hasSelection = a != b;
        auto eraseSelection = [&]() {
            s.erase(a, b - a);
            st.cursor = st.anchor = a;
            hasSelection = false;
        };
        auto moved = [&](size_t pos) {
            st.cursor = pos;
            if (!shiftDown()) st.anchor = pos;
        };
        bool changed = false;
        if (ctrlDown()) {
            if (IsKeyPressed(KEY_A)) {
                st.anchor = 0;
                st.cursor = s.size();
            }
            if (IsKeyPressed(KEY_C) && hasSelection) SetClipboardText(s.substr(a, b - a).c_str());
            if (IsKeyPressed(KEY_X) && hasSelection) {
                SetClipboardText(s.substr(a, b - a).c_str());
                eraseSelection();
                changed = true;
            }
            if (IsKeyPressed(KEY_V)) {
                const char* clip = GetClipboardText();
                if (clip) {
                    std::string paste;
                    for (const char* c = clip; *c; c++)
                        if (*c != '\n' && *c != '\r' && *c != '\t') paste += *c;
                    if (hasSelection) eraseSelection();
                    s.insert(st.cursor, paste);
                    st.cursor = st.anchor = st.cursor + paste.size();
                    changed = true;
                }
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) event = EditorEvent::Expand;
            if (pressedOrRepeat(KEY_HOME)) moved(0);
            if (pressedOrRepeat(KEY_END)) moved(s.size());
        } else {
            int c;
            while ((c = GetCharPressed()) > 0) {
                if (c < 32) continue;
                int bytes = 0;
                const char* utf8 = CodepointToUTF8(c, &bytes);
                if (hasSelection) eraseSelection();
                s.insert(st.cursor, std::string(utf8, bytes));
                st.cursor = st.anchor = st.cursor + bytes;
                changed = true;
            }
            if (pressedOrRepeat(KEY_BACKSPACE)) {
                if (hasSelection) eraseSelection();
                else if (st.cursor > 0) {
                    size_t p = text::prevChar(s, st.cursor);
                    s.erase(p, st.cursor - p);
                    st.cursor = st.anchor = p;
                }
                changed = true;
            }
            if (pressedOrRepeat(KEY_DELETE)) {
                if (hasSelection) eraseSelection();
                else if (st.cursor < s.size()) s.erase(st.cursor, text::nextChar(s, st.cursor) - st.cursor);
                changed = true;
            }
            if (pressedOrRepeat(KEY_LEFT)) moved(hasSelection && !shiftDown() ? a : text::prevChar(s, st.cursor));
            if (pressedOrRepeat(KEY_RIGHT)) moved(hasSelection && !shiftDown() ? b : text::nextChar(s, st.cursor));
            if (pressedOrRepeat(KEY_UP) || pressedOrRepeat(KEY_DOWN) || pressedOrRepeat(KEY_HOME) ||
                pressedOrRepeat(KEY_END)) {
                size_t line = 0, col = 0;
                text::cursorToLineCol(s, starts, st.cursor, line, col);
                if (pressedOrRepeat(KEY_UP) && line > 0) moved(text::lineColToCursor(s, starts, line - 1, col));
                if (pressedOrRepeat(KEY_DOWN) && line + 1 < starts.size())
                    moved(text::lineColToCursor(s, starts, line + 1, col));
                if (pressedOrRepeat(KEY_HOME)) moved(starts[line]);
                if (pressedOrRepeat(KEY_END))
                    moved(line + 1 < starts.size() ? text::prevChar(s, starts[line + 1]) : s.size());
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) event = EditorEvent::Commit;
        }
        if (changed) {
            st.lastInput = ui::now();
            starts = text::wrapLines(s, perLine);
        }
    }

    // Frame (same look as ui::textField)
    fillRounded(r, radius, active ? t.panel : t.panelAlt);
    Color border = invalid ? t.danger : active ? t.accent : hover ? t.borderStrong : t.border;
    strokeRounded(r, radius, active || invalid ? pxi(1.5f) : pxi(1.0f), border);
    if (active) {
        Rectangle ring = {r.x - px(2), r.y - px(2), r.width + px(4), r.height + px(4)};
        strokeRounded(ring, radius + px(2), pxi(2.0f), invalid ? Fade(t.danger, 0.3f) : t.accentSoft);
    }

    // Keep the cursor line visible
    size_t curLine = 0, curCol = 0;
    text::cursorToLineCol(s, starts, st.cursor, curLine, curCol);
    float visibleH = tight ? lh : r.height - 2 * px(kPadY);
    if (active) {
        float cy = curLine * lh;
        if (cy < st.scroll) st.scroll = cy;
        if (cy + lh > st.scroll + visibleH) st.scroll = cy + lh - visibleH;
        // Never scroll further than the text needs (the field grows a frame after the text)
        st.scroll = std::clamp(st.scroll, 0.0f, std::max(0.0f, starts.size() * lh - visibleH));
    }
    float scroll = active ? st.scroll : 0.0f;

    // No scissor here (the side bar's scroll area already uses it and raylib cannot nest it):
    // only lines that fit inside the field are drawn
    auto lineVisible = [&](float y) {
        return tight ? y >= top - 0.5f && y <= top + 0.5f : y >= r.y + px(1) && y + lh <= r.y + r.height - px(1);
    };
    size_t selA = std::min(st.anchor, st.cursor), selB = std::max(st.anchor, st.cursor);
    for (size_t i = 0; i < starts.size(); i++) {
        size_t end = i + 1 < starts.size() ? starts[i + 1] : s.size();
        float y = top + i * lh - scroll;
        if (!lineVisible(y)) continue;
        if (active && selA != selB && selA < end && selB > starts[i]) {
            size_t from = std::max(selA, starts[i]), to = std::min(selB, end);
            float x0 = left + text::charCount(s, starts[i], from) * cw;
            float x1 = left + text::charCount(s, starts[i], to) * cw;
            DrawRectangleRec({x0, y, x1 - x0, lh}, t.accentSoft);
        }
        std::string part = s.substr(starts[i], end - starts[i]);
        text(part.c_str(), left, y + (lh - fontSize(FontRole::Mono)) / 2, FontRole::Mono, t.text);
    }
    if (active) {
        // Blinking cursor (solid right after typing)
        bool on = std::fmod(ui::now() - st.lastInput, 1.0) < 0.6;
        float x = left + curCol * cw;
        float y = top + curLine * lh - scroll;
        if (on && lineVisible(y)) DrawRectangleRec({x, y + px(2), std::max(1.0f, px(1.5f)), lh - px(4)}, t.accent);
    }
    // More text below (when not editing, or scrolled): a hint at the bottom edge
    if (!active && starts.size() * lh > visibleH + 0.5f)
        text("…", r.x + r.width - px(16), r.y + r.height - lh - px(2), FontRole::Mono, t.textMuted);
    return event;
}

} // namespace linalg::ui
