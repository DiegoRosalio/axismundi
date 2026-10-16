#include "ui/Preferences.hpp"
#include <cstdlib>
#include <filesystem>

namespace linalg {

std::string Preferences::path() {
    std::filesystem::path base;
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
        base = xdg;
    } else if (const char* home = std::getenv("HOME"); home && *home) {
        base = std::filesystem::path(home) / ".config";
    } else {
        base = ".";
    }
    return (base / "axismundi" / "settings.txt").string();
}

bool Preferences::load(SceneData& data) {
    std::string error;
    return loadSceneFile(path(), data, error);
}

bool Preferences::save(const SceneData& data, std::string& error) {
    std::filesystem::path p = path();
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
    if (ec) {
        error = "cannot create " + p.parent_path().string() + ": " + ec.message();
        return false;
    }
    return saveSceneFile(p.string(), data, error);
}

} // namespace linalg
