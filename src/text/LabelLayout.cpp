#include "text/LabelLayout.hpp"
#include <algorithm>
#include <numeric>

namespace linalg::text {

bool overlaps(const Box& a, const Box& b, float gap) {
    return a.x < b.x + b.w + gap && b.x < a.x + a.w + gap && a.y < b.y + b.h + gap && b.y < a.y + a.h + gap;
}

std::vector<Box> layoutLabels(const std::vector<LabelRequest>& labels, float gap) {
    std::vector<size_t> order(labels.size());
    std::iota(order.begin(), order.end(), size_t{0});
    std::stable_sort(order.begin(), order.end(),
                     [&](size_t a, size_t b) { return labels[a].priority > labels[b].priority; });

    std::vector<Box> result(labels.size());
    std::vector<Box> placed;
    placed.reserve(labels.size());
    auto isFree = [&](const Box& b) {
        for (const Box& p : placed)
            if (overlaps(b, p, gap)) return false;
        return true;
    };

    for (size_t index : order) {
        const LabelRequest& r = labels[index];
        // Mirror of the preferred spot on the other side of the anchor
        float mirrorDx = -r.dx - r.w;
        float step = r.h + gap;
        const float candidates[][2] = {
            {r.dx, r.dy},         {r.dx, r.dy - step},     {r.dx, r.dy + step},
            {mirrorDx, r.dy},     {mirrorDx, r.dy - step}, {mirrorDx, r.dy + step},
        };
        Box chosen{};
        bool found = false;
        for (const auto& c : candidates) {
            Box b{r.anchorX + c[0], r.anchorY + c[1], r.w, r.h};
            if (isFree(b)) {
                chosen = b;
                found = true;
                break;
            }
        }
        if (!found) {
            // Stack below the preferred spot
            Box b{r.anchorX + r.dx, r.anchorY + r.dy, r.w, r.h};
            for (int k = 0; k < 64 && !isFree(b); k++) b.y += step;
            chosen = b;
        }
        placed.push_back(chosen);
        result[index] = chosen;
    }
    return result;
}

} // namespace linalg::text
