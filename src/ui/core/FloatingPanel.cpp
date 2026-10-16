#include "ui/core/FloatingPanel.hpp"
#include "ui/SideBar.hpp"
#include "ui/TopBar.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <raygui.h>
#include <algorithm>

namespace linalg::ui {

namespace {
FloatingPanel* g_owner = nullptr;
}

FloatingPanel::FloatingPanel(std::string id, std::string title, float widthDesign, Vector2 defaultAnchor,
                             bool visibleByDefault)
    : m_id(std::move(id)), m_title(std::move(title)), m_widthDesign(widthDesign), m_defaultAnchor(defaultAnchor),
      m_defaultVisible(visibleByDefault), m_anchor(defaultAnchor), m_visible(visibleByDefault) {}

Rectangle FloatingPanel::area() {
    float margin = px(12.0f);
    float left = SideBar::width() + margin;
    float top = TopBar::height() + margin;
    return {left, top, GetScreenWidth() - left - margin, GetScreenHeight() - top - margin};
}

float FloatingPanel::headerHeight() { return px(42.0f); }

void FloatingPanel::resetLayout() {
    m_anchor = m_defaultAnchor;
    m_visible = m_defaultVisible;
    m_collapsed = false;
    m_scroll = 0.0f;
}

bool FloatingPanel::begin(float contentHeight, Rectangle& content) {
    if (!m_visible) {
        m_bounds = {0, 0, 0, 0};
        return false;
    }
    const Theme& t = theme();
    Rectangle a = area();
    float header = headerHeight();
    float width = std::min(px(m_widthDesign), a.width);
    float bodyH = m_collapsed ? 0.0f : std::min(contentHeight, std::max(0.0f, a.height - header));
    float height = header + bodyH;
    m_contentHeight = contentHeight;

    // Input ownership: lock widgets unless this panel is on top under the mouse
    m_wasLocked = GuiIsLocked();
    Vector2 mouse = GetMousePosition();
    m_lockedForInput = CheckCollisionPointRec(mouse, m_bounds) && g_owner != this;
    if (m_lockedForInput) GuiLock();

    // Dragging by the title bar
    float freeW = std::max(1.0f, a.width - width);
    float freeH = std::max(1.0f, a.height - height);
    Rectangle titleBar = {a.x + m_anchor.x * freeW, a.y + m_anchor.y * freeH, width - px(80.0f), header};
    if (canInteract() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, titleBar)) {
        m_dragging = true;
        m_dragOffset = {mouse.x - titleBar.x, mouse.y - titleBar.y};
    }
    if (m_dragging) {
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            m_dragging = false;
        } else {
            m_anchor.x = std::clamp((mouse.x - m_dragOffset.x - a.x) / freeW, 0.0f, 1.0f);
            m_anchor.y = std::clamp((mouse.y - m_dragOffset.y - a.y) / freeH, 0.0f, 1.0f);
        }
    }

    m_bounds = snap({a.x + m_anchor.x * freeW, a.y + m_anchor.y * freeH, width, height});
    anchor(("panel." + m_id).c_str(), m_bounds);
    card(m_bounds);

    // Header: title, collapse and close
    Rectangle head = {m_bounds.x + px(14.0f), m_bounds.y, m_bounds.width - px(20.0f), header};
    Rectangle close = takeRight(head, 28.0f, 2.0f);
    Rectangle fold = takeRight(head, 28.0f, 4.0f);
    close.y = fold.y = m_bounds.y + (header - px(28.0f)) / 2;
    close.height = fold.height = px(28.0f);
    if (m_dragging) fillRounded({m_bounds.x, m_bounds.y, m_bounds.width, header}, px(kCardRadius), t.hover);
    textIn(head, ellipsize(m_title.c_str(), FontRole::Title, head.width).c_str(), FontRole::Title, t.text);
    if (iconButton(fold, m_collapsed ? Icon::ChevronDown : Icon::Minus)) m_collapsed = !m_collapsed;
    if (iconButton(close, Icon::Close)) m_visible = false;
    tooltip(fold, m_collapsed ? "Expand" : "Collapse to the title bar");
    tooltip(close, "Close (reopen it from the top bar or its shortcut)");
    tooltip({m_bounds.x, m_bounds.y, fold.x - m_bounds.x, header}, "Drag to move");

    if (m_collapsed || !m_visible) {
        if (m_lockedForInput && !m_wasLocked) GuiUnlock();
        return false;
    }

    DrawRectangleRec({m_bounds.x, m_bounds.y + header - pxi(1.0f), m_bounds.width, pxi(1.0f)}, t.border);
    m_body = {m_bounds.x, m_bounds.y + header, m_bounds.width, bodyH};

    // Scroll when the content is taller than the space available
    float maxScroll = std::max(0.0f, contentHeight - bodyH);
    if (CheckCollisionPointRec(mouse, m_body) && canInteract()) m_scroll -= GetMouseWheelMove() * px(48.0f);
    m_scroll = std::clamp(m_scroll, 0.0f, maxScroll);

    BeginScissorMode(static_cast<int>(m_body.x), static_cast<int>(m_body.y), static_cast<int>(m_body.width),
                     static_cast<int>(m_body.height));
    setClip(m_body);
    content = {m_body.x, m_body.y - m_scroll, m_body.width, contentHeight};
    return true;
}

void FloatingPanel::end() {
    EndScissorMode();
    clearClip();

    float maxScroll = std::max(0.0f, m_contentHeight - m_body.height);
    if (maxScroll > 0.0f) {
        float barH = std::max(px(24.0f), m_body.height * m_body.height / m_contentHeight);
        float barY = m_body.y + (m_scroll / maxScroll) * (m_body.height - barH);
        float w = px(4.0f);
        fillRounded({m_body.x + m_body.width - w - px(4.0f), barY, w, barH}, w / 2, theme().borderStrong);
    }
    if (m_lockedForInput && !m_wasLocked) GuiUnlock();
}

void FloatingPanel::saveState(SceneData& data) const {
    std::string p = "panel." + m_id + ".";
    putFloat(data, p + "x", m_anchor.x);
    putFloat(data, p + "y", m_anchor.y);
    putBool(data, p + "visible", m_visible);
    putBool(data, p + "collapsed", m_collapsed);
}

void FloatingPanel::loadState(const SceneData& data) {
    std::string p = "panel." + m_id + ".";
    if (getFloat(data, p + "x", m_anchor.x)) m_anchor.x = std::clamp(m_anchor.x, 0.0f, 1.0f);
    if (getFloat(data, p + "y", m_anchor.y)) m_anchor.y = std::clamp(m_anchor.y, 0.0f, 1.0f);
    getBool(data, p + "visible", m_visible);
    getBool(data, p + "collapsed", m_collapsed);
}

// ---------------------------------------------------------------------------

void PanelManager::add(FloatingPanel* panel) {
    m_panels.push_back(panel);
}

FloatingPanel* PanelManager::owner() {
    return g_owner;
}

void PanelManager::beginFrame() {
    // Topmost visible panel under the mouse (bounds from last frame)
    Vector2 mouse = GetMousePosition();
    g_owner = nullptr;
    for (auto it = m_panels.rbegin(); it != m_panels.rend(); ++it) {
        if ((*it)->contains(mouse)) {
            g_owner = *it;
            break;
        }
    }
    // A click raises the panel it lands on
    if (g_owner && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !GuiIsLocked()) bringToFront(g_owner);
    // A panel being dragged keeps the mouse even when it lags behind the cursor
    for (auto* p : m_panels) {
        if (p->m_dragging) g_owner = p;
    }
}

void PanelManager::bringToFront(FloatingPanel* panel) {
    auto it = std::find(m_panels.begin(), m_panels.end(), panel);
    if (it == m_panels.end()) return;
    m_panels.erase(it);
    m_panels.push_back(panel);
}

bool PanelManager::anyContains(Vector2 p) const {
    for (auto* panel : m_panels)
        if (panel->contains(p)) return true;
    return false;
}

void PanelManager::resetLayout() {
    for (auto* p : m_panels) p->resetLayout();
}

} // namespace linalg::ui
