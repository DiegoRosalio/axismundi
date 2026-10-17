#include "math/Contour.hpp"
#include <cmath>
#include <cstdint>
#include <map>

namespace linalg {

std::vector<ContourSegment> marchingSquares(const std::vector<double>& v, int nx, int ny, double xMin, double xMax,
                                            double yMin, double yMax) {
    std::vector<ContourSegment> out;
    if (nx < 1 || ny < 1 || static_cast<int>(v.size()) < (nx + 1) * (ny + 1)) return out;
    auto idx = [nx](int i, int j) { return j * (nx + 1) + i; };
    auto xAt = [&](int i) { return xMin + (xMax - xMin) * i / nx; };
    auto yAt = [&](int j) { return yMin + (yMax - yMin) * j / ny; };

    // Corners: 0 = (i,j), 1 = (i+1,j), 2 = (i+1,j+1), 3 = (i,j+1).
    // Edges: 0 = bottom (0-1), 1 = right (1-2), 2 = top (2-3), 3 = left (3-0).
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            double c[4] = {v[idx(i, j)], v[idx(i + 1, j)], v[idx(i + 1, j + 1)], v[idx(i, j + 1)]};
            if (!std::isfinite(c[0]) || !std::isfinite(c[1]) || !std::isfinite(c[2]) || !std::isfinite(c[3]))
                continue;
            int mask = (c[0] > 0) | ((c[1] > 0) << 1) | ((c[2] > 0) << 2) | ((c[3] > 0) << 3);
            if (mask == 0 || mask == 15) continue;
            double px[4] = {xAt(i), xAt(i + 1), xAt(i + 1), xAt(i)};
            double py[4] = {yAt(j), yAt(j), yAt(j + 1), yAt(j + 1)};
            auto edgePoint = [&](int e, double& x, double& y) {
                int a = e, b = (e + 1) % 4;
                double t = c[a] / (c[a] - c[b]);
                x = px[a] + (px[b] - px[a]) * t;
                y = py[a] + (py[b] - py[a]) * t;
            };
            auto emit = [&](int e0, int e1) {
                ContourSegment s;
                edgePoint(e0, s.x0, s.y0);
                edgePoint(e1, s.x1, s.y1);
                out.push_back(s);
            };
            double center = (c[0] + c[1] + c[2] + c[3]) / 4.0;
            switch (mask) {
                case 1: case 14: emit(3, 0); break;
                case 2: case 13: emit(0, 1); break;
                case 3: case 12: emit(3, 1); break;
                case 4: case 11: emit(1, 2); break;
                case 6: case 9:  emit(0, 2); break;
                case 7: case 8:  emit(3, 2); break;
                case 5:  // corners 0 and 2 positive
                    if (center > 0) { emit(0, 1); emit(2, 3); }
                    else            { emit(3, 0); emit(1, 2); }
                    break;
                case 10: // corners 1 and 3 positive
                    if (center > 0) { emit(3, 0); emit(1, 2); }
                    else            { emit(0, 1); emit(2, 3); }
                    break;
                default: break;
            }
        }
    }
    return out;
}

std::vector<Polyline> chainSegments(const std::vector<ContourSegment>& segs, double tol) {
    // Index segment end points by a quantized key
    using Key = std::pair<int64_t, int64_t>;
    auto key = [tol](double x, double y) {
        return Key{static_cast<int64_t>(std::llround(x / tol)), static_cast<int64_t>(std::llround(y / tol))};
    };
    std::multimap<Key, size_t> ends;  // end point -> segment index
    for (size_t k = 0; k < segs.size(); k++) {
        ends.insert({key(segs[k].x0, segs[k].y0), k});
        ends.insert({key(segs[k].x1, segs[k].y1), k});
    }
    std::vector<bool> used(segs.size(), false);
    auto takeNext = [&](double x, double y, size_t& found, bool& reversed) {
        auto range = ends.equal_range(key(x, y));
        for (auto it = range.first; it != range.second; ++it) {
            if (used[it->second]) continue;
            const ContourSegment& s = segs[it->second];
            found = it->second;
            reversed = !(key(s.x0, s.y0) == key(x, y));
            return true;
        }
        return false;
    };

    std::vector<Polyline> lines;
    for (size_t k = 0; k < segs.size(); k++) {
        if (used[k]) continue;
        used[k] = true;
        Polyline line = {{segs[k].x0, segs[k].y0}, {segs[k].x1, segs[k].y1}};
        // Grow forwards from the end, then backwards from the start
        for (int dir = 0; dir < 2; dir++) {
            while (true) {
                auto [x, y] = dir == 0 ? line.back() : line.front();
                size_t next;
                bool reversed;
                if (!takeNext(x, y, next, reversed)) break;
                used[next] = true;
                const ContourSegment& s = segs[next];
                std::pair<double, double> far = reversed ? std::make_pair(s.x0, s.y0) : std::make_pair(s.x1, s.y1);
                if (dir == 0) line.push_back(far);
                else line.insert(line.begin(), far);
            }
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

} // namespace linalg
