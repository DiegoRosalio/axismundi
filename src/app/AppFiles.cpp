// App: Scene files, LaTeX files and preferences (the rest of App is in App.cpp)
#include "app/App.hpp"
#include "view/ViewStyle.hpp"
#include "app/Tour.hpp"
#include "edu/Examples.hpp"
#include "math/Decompositions.hpp"
#include "rendering/WorldMapping.hpp"
#include "ui/SceneIO.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"
#include <raylib.h>
#include <raygui.h>
#include <raymath.h>
#include <rlgl.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cmath>
#include <csignal>
#include <ctime>

namespace linalg {

using scene::SceneObject;
using scene::SpaceKind;

namespace {

const char* kSceneFile = "scene.txt";

} // namespace
void App::saveScene() {
    if (m_currentFile.empty()) {
        saveSceneAs();
        return;
    }
    writeScene(m_currentFile);
}

void App::saveSceneAs() {
    closePopovers();
    std::string name = m_currentFile.empty() ? std::string(kSceneFile)
                                             : std::filesystem::path(m_currentFile).filename().string();
    m_fileDialog.open(FileDialog::Mode::Save, currentFolder(), name, m_recentFiles);
}

void App::loadScene() {
    closePopovers();
    m_fileDialog.open(FileDialog::Mode::Open, currentFolder(), "", m_recentFiles);
}

std::string App::currentFolder() const {
    if (!m_currentFile.empty()) return std::filesystem::path(m_currentFile).parent_path().string();
    return m_lastFolder;
}

void App::rememberFile(const std::string& path) {
    m_currentFile = path;
    m_lastFolder = std::filesystem::path(path).parent_path().string();
    m_recentFiles.erase(std::remove(m_recentFiles.begin(), m_recentFiles.end(), path), m_recentFiles.end());
    m_recentFiles.insert(m_recentFiles.begin(), path);
    if (m_recentFiles.size() > 6) m_recentFiles.resize(6);
}

void App::newScene() {
    m_scenePanel.cancelEditing();
    m_scene.timeline() = scene::Timeline{};
    m_scene.clearAll();
    m_scene.commit();  // undoable
    m_currentFile.clear();
    resetCamera();
    toast("New scene (Ctrl+Z brings the previous one back)");
}

void App::exportView(double& azimuth, double& elevation) const {
    Vector3 p = m_camera.position(), t = m_camera.target();
    latex::viewFromDirection(worldToMath(Vector3{p.x - t.x, p.y - t.y, p.z - t.z}), azimuth, elevation);
}

std::string App::exportLatexCode(bool standalone) const {
    double az = 25, el = 30;
    exportView(az, el);
    scene::Scene& sc = const_cast<scene::Scene&>(m_scene);
    return m_exportPanel.generate(sc.space(spaceKind()), sc.selection(spaceKind()), az, el, standalone).code;
}

void App::writeLatex(const std::string& path) {
    std::ofstream out(path);
    out << exportLatexCode();
    if (out) toast("Saved " + std::filesystem::path(path).filename().string());
    else toast("Could not write " + path, true);
}

void App::writeScene(const std::string& path) {
    SceneData data;
    m_scene.save(data);
    putInt(data, "mode", static_cast<int>(m_mode));
    Vector3 p = m_camera.position(), t = m_camera.target();
    putFloat(data, "camera.px", p.x);
    putFloat(data, "camera.py", p.y);
    putFloat(data, "camera.pz", p.z);
    putFloat(data, "camera.tx", t.x);
    putFloat(data, "camera.ty", t.y);
    putFloat(data, "camera.tz", t.z);
    m_viewPanel.saveState(data);

    std::string error;
    if (saveSceneFile(path, data, error)) {
        rememberFile(path);
        toast("Saved " + std::filesystem::path(path).filename().string());
    } else {
        toast("Save failed: " + error, true);
    }
}

void App::convertOldKeyframes(const SceneData& data) {
    // Files from before the Time panel stored matrix keyframes for the old Animation
    // panel; they become K = keys(s, ...) driven by an animated parameter s
    int count = 0;
    if (!getInt(data, "anim.keyframes", count) || count < 2) return;
    std::vector<std::string> mats;
    for (int k = 0; k < count; k++) {
        auto it = data.find("anim.key" + std::to_string(k));
        if (it == data.end()) continue;
        std::istringstream in(it->second);
        float m[4][4];
        bool ok = true;
        for (int i = 0; i < 4 && ok; i++)
            for (int j = 0; j < 4 && ok; j++) ok = static_cast<bool>(in >> m[i][j]);
        if (!ok) continue;
        std::ostringstream text;
        text << "[";
        for (int i = 0; i < 4; i++) {
            text << (i ? ", [" : "[");
            for (int j = 0; j < 4; j++) text << (j ? ", " : "") << m[i][j];
            text << "]";
        }
        text << "]";
        mats.push_back(text.str());
    }
    if (mats.size() < 2) return;
    scene::Space& sp = m_scene.space(SpaceKind::Euclidean3D);
    SceneObject* s = m_scene.add(SpaceKind::Euclidean3D, "parameter", "s");
    auto* p = static_cast<scene::ParameterObject*>(s);
    p->setMode(scene::ParameterObject::Mode::Animate);
    p->setProp("min", 0.0);
    p->setProp("max", 1.0);
    Playback pb = p->playback();
    pb.repeat = Repeat::Loop;
    pb.duration = 4.0f;
    p->setPlayback(pb);
    std::string formula = "keys(" + s->name();
    for (const auto& m : mats) formula += ", " + m;
    formula += ")";
    SceneObject* k = m_scene.add(SpaceKind::Euclidean3D, "matrix", "K");
    static_cast<scene::MatrixObject*>(k)->setSize(4);
    static_cast<scene::MatrixObject*>(k)->setByFormula(true);
    k->setField("value", formula);
    (void)sp;
    m_scene.evaluate();
    m_scene.commit();
    toast("Old keyframes became the matrix " + k->name() + " = keys(" + s->name() + ", ...)");
}

void App::readScene(const std::string& path) {
    SceneData data;
    std::string error;
    if (!loadSceneFile(path, data, error) || !m_scene.load(data, error)) {
        toast("Could not open " + std::filesystem::path(path).filename().string() + ": " + error, true);
        return;
    }
    m_scenePanel.cancelEditing();
    int m = static_cast<int>(m_mode);
    if (getInt(data, "mode", m) && m >= 0 && m < kAppModeCount) {
        m_mode = static_cast<AppMode>(m);
        m_lastMode = m_mode;  // keep the saved camera instead of the mode default
    }
    Vector3 p = m_camera.position(), t = m_camera.target();
    getFloat(data, "camera.px", p.x);
    getFloat(data, "camera.py", p.y);
    getFloat(data, "camera.pz", p.z);
    getFloat(data, "camera.tx", t.x);
    getFloat(data, "camera.ty", t.y);
    getFloat(data, "camera.tz", t.z);
    m_camera.setView(p, t);
    convertOldKeyframes(data);
    m_viewPanel.loadState(data);
    rememberFile(path);
    toast("Opened " + std::filesystem::path(path).filename().string() + " (Ctrl+Z goes back)");
}

void App::loadPreferences(SceneData& prefs) {
    Preferences::load(prefs);
}

void App::applyPreferences(const SceneData& prefs) {
    std::string theme;
    if (getString(prefs, "ui.theme", theme)) {
        ui::setTheme(theme == "light" ? ui::ThemeKind::Light : ui::ThemeKind::Dark);
    }
    float zoom = 1.0f;
    if (getFloat(prefs, "ui.zoom", zoom)) ui::setZoom(zoom);
    getBool(prefs, "ui.tooltips", m_uiSettings.tooltips);
    getBool(prefs, "ui.dim_unselected", m_uiSettings.dimUnselected);
    getBool(prefs, "ui.hud", m_uiSettings.showHud);
    std::string surfaces, style;
    bool oldSmooth = true;
    if (getString(prefs, "ui.surfaces", surfaces)) {
        m_uiSettings.surfaces = surfaces == "smooth"    ? UiSettings::Surfaces::Smooth
                                : surfaces == "faceted" ? UiSettings::Surfaces::Faceted
                                                        : UiSettings::Surfaces::StyleDefault;
    } else if (getBool(prefs, "ui.smooth_surfaces", oldSmooth) && !oldSmooth) {
        m_uiSettings.surfaces = UiSettings::Surfaces::Faceted;  // older preference files
    }
    // The last style picked: used at startup and for new scenes
    if (getString(prefs, "view.style", style)) m_viewPanel.settings().style = view::styleFromKey(style);
    m_tutorialPanel.loadState(prefs);
    m_exportPanel.loadState(prefs);
    bool sideCollapsed = false;
    if (getBool(prefs, "ui.sidebar_collapsed", sideCollapsed)) SideBar::setCollapsed(sideCollapsed);
    for (auto* p : m_panels.panels()) p->loadState(prefs);
    getString(prefs, "files.folder", m_lastFolder);
    m_recentFiles.clear();
    for (int i = 0; i < 6; i++) {
        std::string f;
        if (getString(prefs, "files.recent." + std::to_string(i), f) && !f.empty()) m_recentFiles.push_back(f);
    }
}

void App::savePreferences() {
    SceneData prefs;
    putFloat(prefs, "ui.zoom", ui::zoom());
    prefs["ui.theme"] = ui::themeKind() == ui::ThemeKind::Light ? "light" : "dark";
    putBool(prefs, "ui.tooltips", m_uiSettings.tooltips);
    putBool(prefs, "ui.dim_unselected", m_uiSettings.dimUnselected);
    putBool(prefs, "ui.hud", m_uiSettings.showHud);
    static const char* kSurfaces[3] = {"style", "smooth", "faceted"};
    prefs["ui.surfaces"] = kSurfaces[static_cast<int>(m_uiSettings.surfaces)];
    prefs["view.style"] = view::styleKey(m_viewPanel.settings().style);
    if (!IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) && !IsWindowFullscreen()) {
        putInt(prefs, "window.width", GetScreenWidth());
        putInt(prefs, "window.height", GetScreenHeight());
    }
    m_tutorialPanel.saveState(prefs);
    m_exportPanel.saveState(prefs);
    putBool(prefs, "ui.sidebar_collapsed", SideBar::collapsed());
    for (auto* p : m_panels.panels()) p->saveState(prefs);
    if (!m_lastFolder.empty()) prefs["files.folder"] = m_lastFolder;
    for (size_t i = 0; i < m_recentFiles.size(); i++) prefs["files.recent." + std::to_string(i)] = m_recentFiles[i];
    std::string error;
    Preferences::save(prefs, error);  // best effort: nothing useful to show while closing
}

} // namespace linalg
