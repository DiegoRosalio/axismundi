#pragma once

namespace linalg {

// The tabs of the app: each one is a space with its own objects
enum class AppMode {
    Space3D = 0,   // vectors, matrices, plots... (z up)
    Complex = 1,   // the complex plane
};

constexpr int kAppModeCount = 2;

} // namespace linalg
