// Tests: Text and label layout (wrapping formulas, placing labels without overlaps)
#include "TestSupport.hpp"

void testLabelLayout() {
    using text::Box;
    using text::LabelRequest;
    CHECK(text::overlaps(Box{0, 0, 10, 10}, Box{5, 5, 10, 10}));
    CHECK(!text::overlaps(Box{0, 0, 10, 10}, Box{12, 0, 10, 10}));
    CHECK(text::overlaps(Box{0, 0, 10, 10}, Box{11, 0, 10, 10}, 2.0f));  // within the gap

    // A lone label keeps its preferred spot
    auto one = text::layoutLabels({LabelRequest{100, 100, 40, 16, 8, -16, 0}});
    CHECK(one.size() == 1);
    CHECK_NEAR(one[0].x, 108, 1e-6);
    CHECK_NEAR(one[0].y, 84, 1e-6);

    // Two labels at the same anchor: the second moves, nothing overlaps, order is kept
    std::vector<LabelRequest> same = {{100, 100, 40, 16, 8, -16, 0}, {100, 100, 50, 16, 8, -16, 0}};
    auto two = text::layoutLabels(same);
    CHECK_NEAR(two[0].y, 84, 1e-6);  // first one in place
    CHECK(!text::overlaps(two[0], two[1], 2.0f));
    CHECK_NEAR(two[1].w, 50, 1e-6);

    // Priority wins the preferred spot regardless of order
    same[1].priority = 1;
    auto prio = text::layoutLabels(same);
    CHECK_NEAR(prio[1].x, 108, 1e-6);
    CHECK_NEAR(prio[1].y, 84, 1e-6);
    CHECK(!text::overlaps(prio[0], prio[1], 2.0f));

    // Many labels at one point: all placed without overlaps (stacking after the six spots)
    std::vector<LabelRequest> many(10, LabelRequest{200, 200, 30, 14, 8, -14, 0});
    auto boxes = text::layoutLabels(many);
    bool clean = true;
    for (size_t i = 0; i < boxes.size(); i++)
        for (size_t j = i + 1; j < boxes.size(); j++)
            if (text::overlaps(boxes[i], boxes[j], 2.0f)) clean = false;
    CHECK(clean);
}

void testTextLayout() {
    using namespace text;
    std::string s = "sin(sqrt(x^2 + y^2))/(1 + 0.05(x^2 + y^2))";
    auto starts = wrapLines(s, 16);
    CHECK(starts.size() >= 3);
    CHECK(starts[0] == 0);
    for (size_t i = 0; i < starts.size(); i++) {
        size_t end = i + 1 < starts.size() ? starts[i + 1] : s.size();
        CHECK(charCount(s, starts[i], end) <= 16);
    }
    // Breaks prefer operators or spaces: no line except the last ends in the middle of "sqrt"
    CHECK(s.substr(starts[1] - 1, 1).find_first_of(" +-*/,^)") != std::string::npos);
    CHECK(wrapLines("short", 40).size() == 1);
    CHECK(wrapLines("", 10).size() == 1);
    // A long word without break characters is split hard
    auto hard = wrapLines("abcdefghijklmnop", 5);
    CHECK(hard.size() == 4 && hard[1] == 5);
    // Cursor <-> line/column round trip
    size_t line = 0, col = 0;
    cursorToLineCol(s, starts, starts[1] + 2, line, col);
    CHECK(line == 1 && col == 2);
    CHECK(lineColToCursor(s, starts, 1, 2) == starts[1] + 2);
    CHECK(lineColToCursor(s, starts, 0, 999) < starts[1]);       // clamped to the line
    CHECK(lineColToCursor(s, starts, 99, 0) == starts.back());    // clamped to the last line
    cursorToLineCol(s, starts, s.size(), line, col);
    CHECK(line == starts.size() - 1);
    // UTF-8: never split a character
    std::string u = "a\xCF\x80" "b";  // a pi b
    CHECK(nextChar(u, 1) == 3 && prevChar(u, 3) == 1);
    CHECK(charCount(u, 0, u.size()) == 3);
}
