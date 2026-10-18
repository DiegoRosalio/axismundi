#pragma once

#include <string>

namespace linalg::latex {

// Compiling an exported standalone figure to PDF with the system's LaTeX (no raylib).

// Full path of an executable found in PATH, or "" (no shell involved)
std::string findTool(const std::string& name);
// pdflatex, else lualatex, else "" (full path)
std::string latexCompiler();

// The first error lines of a LaTeX log: lines starting with "!" plus the "l.NN ..." line that
// follows each (where it happened), at most maxLines lines joined with newlines
std::string firstLatexErrors(const std::string& log, int maxLines = 4);

struct PdfResult {
    bool ok = false;
    std::string message;  // the error (LaTeX's first error lines, a timeout, a missing compiler...)
};

// Writes texSource to a temporary folder, runs the compiler there (argument list, no shell,
// nonstop mode), copies the PDF to outPath and removes the temporary folder. The compiler is
// killed after timeoutSeconds. Blocking: call it from a worker thread in the UI.
PdfResult compilePdf(const std::string& texSource, const std::string& outPath, int timeoutSeconds = 30,
                     const std::string& compiler = latexCompiler());

} // namespace linalg::latex
