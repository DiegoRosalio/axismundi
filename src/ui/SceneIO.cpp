#include "ui/SceneIO.hpp"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

namespace linalg {

namespace {

std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

} // namespace

bool saveSceneFile(const std::string& path, const SceneData& data, std::string& error) {
    std::ofstream out(path);
    if (!out) {
        error = "cannot write " + path + ": " + std::strerror(errno);
        return false;
    }
    out << "# axismundi scene\n";
    for (const auto& [key, value] : data) {
        out << key << " = " << value << "\n";
    }
    if (!out) {
        error = "write failed for " + path;
        return false;
    }
    return true;
}

bool loadSceneFile(const std::string& path, SceneData& data, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "cannot open " + path;
        return false;
    }
    data.clear();
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        lineNo++;
        std::string t = trim(line);
        if (t.empty() || t[0] == '#') continue;
        size_t eq = t.find('=');
        if (eq == std::string::npos) {
            error = path + ":" + std::to_string(lineNo) + ": expected 'key = value'";
            return false;
        }
        data[trim(t.substr(0, eq))] = trim(t.substr(eq + 1));
    }
    return true;
}

void putFloat(SceneData& data, const std::string& key, float value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6g", value);
    data[key] = buf;
}

void putBool(SceneData& data, const std::string& key, bool value) {
    data[key] = value ? "1" : "0";
}

void putInt(SceneData& data, const std::string& key, int value) {
    data[key] = std::to_string(value);
}

bool getFloat(const SceneData& data, const std::string& key, float& out) {
    auto it = data.find(key);
    if (it == data.end()) return false;
    char* end = nullptr;
    float v = std::strtof(it->second.c_str(), &end);
    if (end == it->second.c_str() || *end != '\0' || !std::isfinite(v)) return false;
    out = v;
    return true;
}

bool getBool(const SceneData& data, const std::string& key, bool& out) {
    auto it = data.find(key);
    if (it == data.end()) return false;
    if (it->second == "1" || it->second == "true") { out = true; return true; }
    if (it->second == "0" || it->second == "false") { out = false; return true; }
    return false;
}

bool getInt(const SceneData& data, const std::string& key, int& out) {
    auto it = data.find(key);
    if (it == data.end()) return false;
    char* end = nullptr;
    long v = std::strtol(it->second.c_str(), &end, 10);
    if (end == it->second.c_str() || *end != '\0') return false;
    out = static_cast<int>(v);
    return true;
}

bool getString(const SceneData& data, const std::string& key, std::string& out) {
    auto it = data.find(key);
    if (it == data.end()) return false;
    out = it->second;
    return true;
}

bool getText(const SceneData& data, const std::string& key, char* buf, size_t size) {
    auto it = data.find(key);
    if (it == data.end() || size == 0) return false;
    snprintf(buf, size, "%s", it->second.c_str());
    return true;
}

} // namespace linalg
