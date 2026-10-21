#pragma once

#include <cstddef>

namespace linalg {

// A number for a formula field: up to 4 decimals, trailing zeros trimmed (keeping one: "2.0"), never "-0.0"
void formatNumber(char* buf, size_t size, float value);

} // namespace linalg
