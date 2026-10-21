#pragma once

// What the 3D view writes on top of itself, as a list in screen pixels: label pills, plain texts (axis numbers)
// and leader lines. SceneView and the axis numbers fill it; each front end paints it with its own text
// rendering (ui::paintOverlay in the raylib app). Laying labels out needs text sizes, so the code that fills
// the list gets a TextMeasure from the painter's side.

#include <raylib.h>
#include <string>
#include <vector>

namespace linalg::gfx {

enum class TextStyle {
    Label,       // object and axis labels
    LabelLarge,  // the same, bigger (view styles with large labels)
    Small,       // axis numbers
};

struct OverlayItem {
    enum class Kind { Pill, Text, Line };
    Kind kind = Kind::Text;
    Rectangle rect{0, 0, 0, 0};  // Pill: the pill; Text: the box the text is placed in
    std::string text;
    TextStyle style = TextStyle::Label;
    Color color{0, 0, 0, 255};       // text or line color
    Color background{0, 0, 0, 0};    // Pill: fill (alpha 0: the theme's label pill color)
    Color ring{0, 0, 0, 0};          // Pill: outline when alpha > 0 (selected objects)
    Vector2 from{0, 0}, to{0, 0};    // Line ends
    float width = 1.0f;              // Line thickness in pixels
};

using Overlay = std::vector<OverlayItem>;

class TextMeasure {
public:
    virtual ~TextMeasure() = default;
    virtual float width(const char* text, TextStyle style) const = 0;
    virtual float lineHeight(TextStyle style) const = 0;  // the font size in pixels
};

} // namespace linalg::gfx
