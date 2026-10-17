#pragma once

#include <raylib.h>
#include <string>

namespace linalg::ui {

// Multi-line editor for formulas: the text wraps by width (monospaced font),
// so long formulas stay readable while shown and while typing. One editor is
// active at a time; its cursor and selection live in EditorState.
struct EditorState {
    int id = -1;          // which field the state belongs to
    size_t cursor = 0;
    size_t anchor = 0;    // selection = [min(anchor, cursor), max(...))
    float scroll = 0.0f;  // pixels, when the text has more lines than fit
    bool dragging = false;
    double lastInput = 0.0;
};

enum class EditorEvent {
    None,
    Activated,  // clicked while inactive (the caller makes it active)
    Commit,     // Enter, or a click outside
    Expand,     // Ctrl+Enter: open the big editor
};

// Pixel height for `text` in a field `width` wide, between 1 and maxLines lines
float editorHeight(const std::string& text, float width, int maxLines);
// Number of wrapped lines `text` needs in a field `width` wide
int editorLines(const std::string& text, float width);

// Draw (and, when active, edit) the text. `id` ties the state to the field. With
// selectAllOnFocus, a click or Tab into the field selects its text so typing replaces it
// (short fields: numbers, names); otherwise a click places the cursor (long formulas).
EditorEvent textEditor(Rectangle r, int id, std::string& text, bool active, bool invalid, EditorState& state,
                       bool selectAllOnFocus = false);

// The shared state of the active inline editor (one at a time)
EditorState& inlineEditorState();

} // namespace linalg::ui
