#pragma once

#include <raylib.h>

namespace linalg {

class App;

// Content of the "Example gallery" panel: category tabs and a responsive
// grid of example cards. Clicking a card loads it.
class GalleryPanel {
public:
    // Returns the content height in pixels; sets `chosen` to the clicked example index
    float drawContent(Rectangle area, int& chosen);

private:
    int m_category = 0;  // 0 = all
};

} // namespace linalg
