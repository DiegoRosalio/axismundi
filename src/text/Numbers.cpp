#include "text/Numbers.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace linalg {

void formatNumber(char* buf, size_t size, float value) {
    if (std::fabs(value) < 5e-5f) value = 0.0f;  // avoid "-0.0"
    snprintf(buf, size, "%.4f", value);

    // Trim trailing zeros, keeping one digit after the decimal point
    char* dot = std::strchr(buf, '.');
    if (!dot) return;
    char* end = buf + std::strlen(buf) - 1;
    while (end > dot + 1 && *end == '0') *end-- = '\0';
}

} // namespace linalg
