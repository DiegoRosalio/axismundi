#pragma once

namespace linalg {

enum class AppMode {
    LinearAlgebra = 0,
    Complex = 1,
    Plotting = 2,
};

constexpr int kAppModeCount = 3;

} // namespace linalg
