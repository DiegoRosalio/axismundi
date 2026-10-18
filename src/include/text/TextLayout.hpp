#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace linalg::text {

// Line wrapping and cursor arithmetic for single-paragraph text in a monospaced
// font (formulas): no raylib, so it is unit-tested. Positions are byte indices;
// UTF-8 continuation bytes are never split.

// Start index of every wrapped line (the first is 0). Lines hold at most
// maxChars characters and break after a space or an operator when one is near.
std::vector<size_t> wrapLines(const std::string& text, size_t maxChars);

// Characters (not bytes) between two byte positions
size_t charCount(const std::string& text, size_t from, size_t to);

// Line and column (in characters) of a cursor position
void cursorToLineCol(const std::string& text, const std::vector<size_t>& starts, size_t cursor, size_t& line,
                     size_t& column);
// Cursor position for a line and column, clamped to the line
size_t lineColToCursor(const std::string& text, const std::vector<size_t>& starts, size_t line, size_t column);

// Neighbouring character boundaries
size_t prevChar(const std::string& text, size_t pos);
size_t nextChar(const std::string& text, size_t pos);

} // namespace linalg::text
