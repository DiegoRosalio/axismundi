#pragma once

#include <string>
#include <vector>

namespace linalg::expr {

// The in-app formula reference: sections of entries, each with a syntax line,
// an explanation and an example that the reference panel evaluates live.
// Tests check that every example compiles and that every built-in function
// is documented, so this stays in step with the engine.
struct ReferenceEntry {
    std::string syntax;       // e.g. "cross(a, b)"
    std::string text;         // what it does
    std::string example;      // a formula to try ("" = none)
    bool usesObjects = false; // the example needs objects from the scene (a, b, M, ...)
};

struct ReferenceSection {
    std::string title;
    std::string intro;
    std::vector<ReferenceEntry> entries;
};

const std::vector<ReferenceSection>& reference();

// Names of all built-in functions (for tests and completeness checks)
std::vector<std::string> functionNames();

} // namespace linalg::expr
