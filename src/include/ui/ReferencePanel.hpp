#pragma once

#include "math/expr/Expr.hpp"
#include <raylib.h>
#include <string>

namespace linalg {

// The "Formula reference" panel: a searchable cheat sheet of the expression
// language (math/expr/Reference) with live results, and a box to try any
// formula against the objects of the current space.
class ReferencePanel {
public:
    // Returns the content height (pixels). `resolver` supplies the current
    // space's objects; `time` is the scene time used for t.
    float drawContent(Rectangle area, const expr::Resolver* resolver, double time);

    bool isEditing() const { return m_activeId >= 0; }
    void cancelEditing() { m_activeId = -1; }
    void setTryText(const std::string& text);  // also used by the tour
    void setSearch(const std::string& text);

private:
    char m_search[64] = "";
    char m_try[256] = "";
    int m_activeId = -1;
    int m_section = -1;  // -1 = all sections
};

} // namespace linalg
