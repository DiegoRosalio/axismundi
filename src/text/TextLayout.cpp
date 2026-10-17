#include "text/TextLayout.hpp"
#include <algorithm>
#include <cstring>

namespace linalg::text {

namespace {
bool isContinuation(unsigned char c) {
    return (c & 0xC0) == 0x80;
}
bool isBreakAfter(char c) {
    return std::strchr(" +-*/,^)", c) != nullptr && c != '\0';
}
} // namespace

size_t prevChar(const std::string& s, size_t pos) {
    if (pos == 0) return 0;
    pos--;
    while (pos > 0 && isContinuation(static_cast<unsigned char>(s[pos]))) pos--;
    return pos;
}

size_t nextChar(const std::string& s, size_t pos) {
    if (pos >= s.size()) return s.size();
    pos++;
    while (pos < s.size() && isContinuation(static_cast<unsigned char>(s[pos]))) pos++;
    return pos;
}

size_t charCount(const std::string& s, size_t from, size_t to) {
    size_t n = 0;
    for (size_t i = from; i < to && i < s.size(); i++)
        if (!isContinuation(static_cast<unsigned char>(s[i]))) n++;
    return n;
}

std::vector<size_t> wrapLines(const std::string& s, size_t maxChars) {
    std::vector<size_t> starts = {0};
    if (maxChars < 1) maxChars = 1;
    size_t start = 0;
    while (charCount(s, start, s.size()) > maxChars) {
        // The byte position after maxChars characters
        size_t limit = start;
        for (size_t k = 0; k < maxChars; k++) limit = nextChar(s, limit);
        // Prefer breaking after a space or operator in the last ~third of the line
        size_t brk = 0;
        size_t minPos = start + std::max<size_t>(1, (limit - start) * 2 / 3);
        for (size_t p = limit; p > minPos; p--) {
            if (isBreakAfter(s[p - 1])) {
                brk = p;
                break;
            }
        }
        start = brk ? brk : limit;
        starts.push_back(start);
    }
    return starts;
}

void cursorToLineCol(const std::string& s, const std::vector<size_t>& starts, size_t cursor, size_t& line,
                     size_t& column) {
    line = 0;
    for (size_t i = 1; i < starts.size(); i++)
        if (cursor >= starts[i]) line = i;
    column = charCount(s, starts[line], cursor);
}

size_t lineColToCursor(const std::string& s, const std::vector<size_t>& starts, size_t line, size_t column) {
    if (starts.empty()) return 0;
    line = std::min(line, starts.size() - 1);
    size_t end = line + 1 < starts.size() ? starts[line + 1] : s.size();
    size_t pos = starts[line];
    for (size_t k = 0; k < column && pos < end; k++) pos = nextChar(s, pos);
    // On a wrapped line the end belongs to the next line; stay before it
    if (line + 1 < starts.size() && pos == end && end > starts[line]) pos = prevChar(s, end);
    return pos;
}

} // namespace linalg::text
