#include "export/PdfCompile.hpp"
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

namespace linalg::latex {

namespace fs = std::filesystem;

std::string findTool(const std::string& name) {
    if (name.empty()) return {};
    if (name.find('/') != std::string::npos) return access(name.c_str(), X_OK) == 0 ? name : std::string();
    const char* path = std::getenv("PATH");
    std::stringstream dirs(path ? path : "/usr/local/bin:/usr/bin:/bin");
    std::string dir;
    while (std::getline(dirs, dir, ':')) {
        if (dir.empty()) continue;
        std::string candidate = dir + "/" + name;
        std::error_code ec;
        if (access(candidate.c_str(), X_OK) == 0 && fs::is_regular_file(candidate, ec)) return candidate;
    }
    return {};
}

std::string latexCompiler() {
    std::string tool = findTool("pdflatex");
    return tool.empty() ? findTool("lualatex") : tool;
}

std::string firstLatexErrors(const std::string& log, int maxLines) {
    std::vector<std::string> lines;
    std::stringstream in(log);
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    std::string out;
    int count = 0;
    for (size_t i = 0; i < lines.size() && count < maxLines; i++) {
        if (lines[i].empty() || lines[i][0] != '!') continue;
        out += (out.empty() ? "" : "\n") + lines[i];
        count++;
        // Where it happened: the next "l.NN" line within a few lines
        for (size_t j = i + 1; j < lines.size() && j <= i + 8 && count < maxLines; j++) {
            if (lines[j].rfind("l.", 0) == 0) {
                out += "\n" + lines[j];
                count++;
                break;
            }
        }
    }
    return out;
}

PdfResult compilePdf(const std::string& texSource, const std::string& outPath, int timeoutSeconds,
                     const std::string& compiler) {
    PdfResult result;
    if (compiler.empty()) {
        result.message = "No LaTeX compiler found (pdflatex or lualatex). Install TeX Live.";
        return result;
    }
    std::error_code ec;
    std::string pattern = (fs::temp_directory_path(ec) / "axismundi-pdf-XXXXXX").string();
    std::vector<char> dirName(pattern.begin(), pattern.end());
    dirName.push_back('\0');
    if (!mkdtemp(dirName.data())) {
        result.message = "Could not create a temporary folder.";
        return result;
    }
    fs::path dir = dirName.data();
    auto cleanup = [&] { fs::remove_all(dir, ec); };
    {
        std::ofstream tex(dir / "figure.tex");
        tex << texSource;
        if (!tex) {
            cleanup();
            result.message = "Could not write the temporary .tex file.";
            return result;
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        cleanup();
        result.message = "Could not start " + compiler + ".";
        return result;
    }
    if (pid == 0) {
        // Child: run in the temporary folder, output to /dev/null, no input
        if (chdir(dir.c_str()) != 0) _exit(127);
        int null = open("/dev/null", O_RDWR);
        if (null >= 0) {
            dup2(null, 0);
            dup2(null, 1);
            dup2(null, 2);
        }
        execl(compiler.c_str(), compiler.c_str(), "-interaction=nonstopmode", "-halt-on-error", "figure.tex",
              static_cast<char*>(nullptr));
        _exit(127);
    }

    // Parent: wait with a timeout
    int status = 0;
    bool finished = false;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeoutSeconds);
    while (std::chrono::steady_clock::now() < deadline) {
        pid_t done = waitpid(pid, &status, WNOHANG);
        if (done == pid) {
            finished = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    if (!finished) {
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        cleanup();
        result.message = "LaTeX took longer than " + std::to_string(timeoutSeconds) + " s and was stopped.";
        return result;
    }

    fs::path pdf = dir / "figure.pdf";
    bool exited = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    if (!exited || !fs::exists(pdf, ec)) {
        std::ifstream logFile(dir / "figure.log");
        std::stringstream log;
        log << logFile.rdbuf();
        std::string errors = firstLatexErrors(log.str());
        if (WIFEXITED(status) && WEXITSTATUS(status) == 127 && errors.empty())
            result.message = "Could not run " + compiler + ".";
        else
            result.message = errors.empty() ? std::string("LaTeX failed (no error message in the log).") : errors;
        cleanup();
        return result;
    }
    fs::copy_file(pdf, outPath, fs::copy_options::overwrite_existing, ec);
    cleanup();
    if (ec) {
        result.message = "Could not write " + outPath + ": " + ec.message();
        return result;
    }
    result.ok = true;
    return result;
}

} // namespace linalg::latex
