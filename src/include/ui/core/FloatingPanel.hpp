#pragma once

#include "ui/SceneIO.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace linalg::ui {

// A card inside the 3D viewport with a title bar: drag it by the title,
// collapse it with the chevron, close it with ×. Its position is stored as a
// fraction of the free space, so it keeps its place at any window size.
//
//   Rectangle content;
//   if (panel.begin(contentHeight, content)) {
//       ...draw into content...
//       panel.end();
//   }
class FloatingPanel {
public:
    FloatingPanel(std::string id, std::string title, float widthDesign, Vector2 defaultAnchor,
                  bool visibleByDefault = true);

    // Draws the frame. `contentHeight` is the pixel height the content wants
    // (usually measured last frame); it is clamped to the viewport. Returns
    // false when hidden or collapsed (then do not call end()).
    bool begin(float contentHeight, Rectangle& content);
    void end();

    const std::string& id() const { return m_id; }
    bool visible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
    void toggle() { m_visible = !m_visible; }
    bool collapsed() const { return m_collapsed; }
    Rectangle bounds() const { return m_bounds; }
    bool contains(Vector2 p) const { return m_visible && CheckCollisionPointRec(p, m_bounds); }
    void resetLayout();

    // Scroll offset for content taller than the panel (pixels)
    float scroll() const { return m_scroll; }

    void saveState(SceneData& data) const;
    void loadState(const SceneData& data);

    // Free area panels live in (the viewport right of the side bar, below the top bar)
    static Rectangle area();
    static float headerHeight();

private:
    friend class PanelManager;

    std::string m_id;
    std::string m_title;
    float m_widthDesign;
    Vector2 m_defaultAnchor;
    bool m_defaultVisible;

    Vector2 m_anchor;          // 0..1 within the free area
    bool m_visible;
    bool m_collapsed = false;
    bool m_dragging = false;
    Vector2 m_dragOffset = {0, 0};
    float m_scroll = 0.0f;
    float m_contentHeight = 0.0f;
    Rectangle m_bounds = {0, 0, 0, 0};
    Rectangle m_body = {0, 0, 0, 0};
    bool m_lockedForInput = false;
    bool m_wasLocked = false;
};

// Keeps floating panels in z-order and decides which one owns the mouse, so a
// click never reaches widgets in a panel hidden underneath another.
class PanelManager {
public:
    void add(FloatingPanel* panel);

    // Call once per frame before drawing any panel
    void beginFrame();

    // Draw order (back to front)
    const std::vector<FloatingPanel*>& panels() const { return m_panels; }
    void bringToFront(FloatingPanel* panel);
    bool anyContains(Vector2 p) const;
    void resetLayout();

    // Owner of the mouse this frame (nullptr: no panel under the mouse)
    static FloatingPanel* owner();

private:
    std::vector<FloatingPanel*> m_panels;
};

} // namespace linalg::ui
