#include "ui/Preferences.hpp"
#include <cstdlib>
#include <filesystem>

namespace linalg {

std::string Preferences::path(const std::string& file) {
    std::filesystem::path base;
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
        base = xdg;
    } else if (const char* home = std::getenv("HOME"); home && *home) {
        base = std::filesystem::path(home) / ".config";
    } else {
        base = ".";
    }
    return (base / "axismundi" / file).string();
}

bool Preferences::load(SceneData& data, const std::string& file) {
    std::string error;
    return loadSceneFile(path(file), data, error);
}

bool Preferences::save(const SceneData& data, std::string& error, const std::string& file) {
    std::filesystem::path p = path(file);
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
    if (ec) {
        error = "cannot create " + p.parent_path().string() + ": " + ec.message();
        return false;
    }
    return saveSceneFile(p.string(), data, error);
}

} // namespace linalg
