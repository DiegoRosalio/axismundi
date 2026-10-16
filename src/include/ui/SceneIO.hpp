#pragma once

#include <cstddef>
#include <map>
#include <string>

namespace linalg {

// Scene state as flat "key = value" pairs. Each component writes and reads
// its own keys (prefixed, e.g. "matrix.", "view.", "camera.").
using SceneData = std::map<std::string, std::string>;

bool saveSceneFile(const std::string& path, const SceneData& data, std::string& error);
bool loadSceneFile(const std::string& path, SceneData& data, std::string& error);

// Typed helpers. The getters leave `out` unchanged when the key is missing
// or malformed, so loading an older or partial file keeps current values.
void putFloat(SceneData& data, const std::string& key, float value);
void putBool(SceneData& data, const std::string& key, bool value);
void putInt(SceneData& data, const std::string& key, int value);
bool getFloat(const SceneData& data, const std::string& key, float& out);
bool getBool(const SceneData& data, const std::string& key, bool& out);
bool getInt(const SceneData& data, const std::string& key, int& out);
bool getString(const SceneData& data, const std::string& key, std::string& out);

// Copy a stored string into a fixed char buffer (truncating safely).
bool getText(const SceneData& data, const std::string& key, char* buf, size_t size);

} // namespace linalg
