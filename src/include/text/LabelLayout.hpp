#pragma once

#include <vector>

namespace linalg::text {

// Placing screen labels so they don't overlap (no raylib: unit-tested).
struct Box {
    float x = 0, y = 0, w = 0, h = 0;
};

struct LabelRequest {
    float anchorX = 0, anchorY = 0;  // screen point the label belongs to
    float w = 0, h = 0;              // label size
    float dx = 0, dy = 0;            // preferred top-left, relative to the anchor
    int priority = 0;                // higher is placed first and keeps its preferred spot
};

bool overlaps(const Box& a, const Box& b, float gap = 0.0f);

// One box per request, in the same order. Greedy: by priority (then input order), each label
// takes the first free spot among its preferred one, above/below it, and the mirrored spots on the
// left of the anchor; if all are taken it is stacked below its preferred spot until it fits.
std::vector<Box> layoutLabels(const std::vector<LabelRequest>& labels, float gap = 2.0f);

} // namespace linalg::text
