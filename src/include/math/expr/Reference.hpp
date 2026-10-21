#pragma once

#include "math/expr/Expr.hpp"
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

// A short one-line result: 512, 1 + 2i, (1, 0, 0), [1 2 0; 0 1 0; 0 0 1] (the error text for an error)
std::string compactValue(const Value& v);
// Evaluate a formula as the reference shows it: x = 1, y = 2, u = 0.5, z = 3, t = time, names from the
// resolver (nullptr: none); `error` is set when it fails. `variables`: exampleVariables of its section.
Value evaluateExample(const std::string& text, const Resolver* resolver, double time, unsigned variables,
                      std::string& error);
unsigned exampleVariables(const ReferenceSection& section);

// Names of all built-in functions (for tests and completeness checks)
std::vector<std::string> functionNames();

} // namespace linalg::expr
