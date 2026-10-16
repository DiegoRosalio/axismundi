#pragma once

namespace linalg {

// Full-screen list of keyboard shortcuts, toggled with H.
class HelpOverlay {
public:
    void toggle() { m_visible = !m_visible; }
    void hide() { m_visible = false; }
    bool visible() const { return m_visible; }
    void draw() const;

private:
    bool m_visible = false;
};

} // namespace linalg
