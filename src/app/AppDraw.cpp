// App: Drawing: the 3D scene and labels, the UI frame, floating panels, HUD, toasts (the rest of App is in App.cpp)
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
view::DrawOptions App::drawOptions() const {
    view::DrawOptions o;
    o.view = &const_cast<ViewPanel&>(m_viewPanel).settings();
    o.selection = &const_cast<scene::Scene&>(m_scene).selection(spaceKind());
    o.print = view::activeStyle().print;
    o.printScope = m_exportPanel.options().scope;
    o.printGrayShading = m_exportPanel.options().grayShading;
    // Not while a lesson or the breakdown highlights things itself
    o.dimUnselected = m_uiSettings.dimUnselected && !m_tutorialWindow.visible() && !breakdownActive();
    o.solo = m_viewPanel.settings().soloSelection;
    const SceneObject* target = targetMatrix();
    if (target && m_mode == AppMode::Space3D) {
        if (breakdownActive()) {
            o.overrideMatrix = target->id();
            o.overrideValue = m_breakdownPanel.current();
            int steps = static_cast<int>(m_breakdownPanel.breakdown().steps.size());
            int done = std::min(m_breakdownPanel.completedSteps(), steps);
            for (int k = 1; k <= done && k < steps; k++) o.ghosts.push_back(m_breakdownPanel.breakdown().after(k));
        }
    }
    return o;
}

void App::drawScene() {
    BeginMode3D(m_camera.camera());
    // The print preview shows the grid only when the export has one, in light gray
    bool print = view::activeStyle().print;
    if (!print || m_exportPanel.options().grid) {
        if (m_mode == AppMode::Space3D) m_grid.draw();
        else m_grid.drawPlaneXY();
    }
    m_sceneView.draw3D(m_scene.space(spaceKind()), drawOptions());
    EndMode3D();
}

void App::drawSceneLabels() {
    m_sceneView.drawLabels(m_scene.space(spaceKind()), drawOptions(), m_camera.camera());
}

float App::viewportLeft() const {
    return SideBar::width();
}

float App::viewportRight() const {
    return static_cast<float>(GetScreenWidth());
}

void App::drawUi(float deltaTime) {
    // An open popover owns the mouse over its area
    Vector2 mouse = GetMousePosition();
    bool popoverOwnsMouse = m_fileDialog.isOpen() || m_formulaEditor.isOpen() || m_settingsPopover.contains(mouse) ||
                            m_colorPopover.contains(mouse) || m_addMenu.contains(mouse);
    bool wasLocked = GuiIsLocked();
    if (popoverOwnsMouse) GuiLock();

    if (m_uiSettings.showHud) drawHud();
    drawSoloChip();

    // Side bar: objects and inspector of the current space, then the view settings
    if (SideBar::collapsed()) {
        SideBar::StripActions strip = m_sideBar.drawCollapsed();
        if (strip.expand) setSideBarCollapsed(false);
        if (strip.add) {
            closePopovers();
            m_addMenu.open(strip.addButton);
        }
    } else {
        Rectangle area = m_sideBar.begin();
        m_scenePanel.setSolo(m_viewPanel.settings().soloSelection);
        float y = m_scenePanel.draw(area.x, area.y, area.width, m_scene, spaceKind(), m_sceneView);
        m_viewPanel.setCurrentFile(m_currentFile);
        y = m_viewPanel.draw(area.x, y, area.width, m_mode);
        m_sideBar.end(y);
    }

    ScenePanelRequests requests = m_scenePanel.takeRequests();
    if (requests.openAdd) {
        closePopovers();
        m_addMenu.open(requests.addAnchor);
    }
    if (requests.openColor) {
        closePopovers();
        openColorPicker();
    }
    if (requests.toggleSolo) toggleSolo();
    if (requests.soloOff) m_viewPanel.settings().soloSelection = false;
    if (requests.loadDemo) {
        resetScene();
        toast("Demo scene loaded (Ctrl+Z goes back)");
    }
    if (requests.openFormula) {
        closePopovers();
        m_formulaEditor.open(spaceKind(), requests.formulaObject, requests.formulaKey, requests.formulaText,
                             requests.formulaCommit);
    }
    if (requests.openReference) {
        m_referenceWindow.setVisible(true);
        m_panels.bringToFront(&m_referenceWindow);
    }
    if (requests.openTime) {
        m_animationWindow.setVisible(true);
        m_panels.bringToFront(&m_animationWindow);
    }
    if (!requests.message.empty()) toast(requests.message, requests.messageIsError);
    if (m_viewPanel.takeResetCameraRequest()) resetCamera();
    switch (m_viewPanel.takeCameraViewRequest()) {
        case 1: m_camera.frontView(); break;
        case 2: m_camera.rightView(); break;
        case 3: m_camera.topView(); break;
        case 4: focusSelection(); break;
        default: break;
    }
    if (m_viewPanel.takeSaveRequest()) saveScene();
    if (m_viewPanel.takeSaveAsRequest()) saveSceneAs();
    if (m_viewPanel.takeLoadRequest()) loadScene();
    if (m_viewPanel.takeNewRequest()) newScene();

    drawFloatingPanels();

    TopBarState state;
    state.helpVisible = m_help.visible();
    state.settingsOpen = m_settingsPopover.isOpen();
    state.fullscreen = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
    state.stepsVisible = m_stepsWindow.visible();
    state.breakdownVisible = m_breakdownWindow.visible();
    state.galleryVisible = m_galleryWindow.visible();
    state.tutorialVisible = m_tutorialWindow.visible();
    state.space3D = (m_mode == AppMode::Space3D);
    state.analysisVisible = m_analysisWindow.visible();
    state.timeVisible = m_animationWindow.visible();
    state.playing = m_scene.timeline().playing;
    state.referenceVisible = m_referenceWindow.visible();
    state.exportVisible = m_exportWindow.visible();
    state.sidebarCollapsed = SideBar::collapsed();
    TopBarActions actions = m_topBar.draw(m_mode, state);
    if (actions.toggleTheme) toggleTheme();
    if (actions.toggleHelp) m_help.toggle();
    if (actions.toggleFullscreen) toggleFullscreen();
    if (actions.screenshot) requestScreenshot(false);
    auto toggleWindow = [this](ui::FloatingPanel& w) {
        w.toggle();
        if (w.visible()) m_panels.bringToFront(&w);
    };
    if (actions.toggleTutorial) toggleWindow(m_tutorialWindow);
    if (actions.toggleGallery) toggleWindow(m_galleryWindow);
    if (actions.toggleBreakdown) toggleWindow(m_breakdownWindow);
    if (actions.toggleSteps) toggleWindow(m_stepsWindow);
    if (actions.toggleAnalysis) toggleWindow(m_analysisWindow);
    if (actions.toggleTime) toggleWindow(m_animationWindow);
    if (actions.toggleReference) toggleWindow(m_referenceWindow);
    if (actions.toggleExport) toggleWindow(m_exportWindow);
    if (actions.toggleSidebar) setSideBarCollapsed(!SideBar::collapsed());
    if (actions.togglePlay) TimePanel::togglePlay(m_scene);
    if (actions.toggleSettings) {
        bool open = !m_settingsPopover.isOpen();
        closePopovers();
        showSettings(open);
    }

    if (popoverOwnsMouse && !wasLocked) GuiUnlock();

    SettingsActions settingsActions = m_settingsPopover.draw(m_uiSettings);
    if (settingsActions.resetLayout) {
        m_panels.resetLayout();
        toast("Panel layout reset");
    }

    ColorPopover::Result color = m_colorPopover.draw();
    if (color.changed || color.reset) {
        SceneObject* o = m_scene.space(spaceKind()).find(static_cast<scene::ObjectId>(m_colorPopover.slot()));
        if (o) {
            o->customColor = color.changed && !color.reset;
            if (o->customColor) o->color = {color.color.r, color.color.g, color.color.b, 255};
            o->touch();
            m_scene.commit();
        }
    }

    AddChoice choice;
    if (m_addMenu.draw(m_scene, spaceKind(), choice)) {
        if (SceneObject* o = applyAddChoice(m_scene, spaceKind(), choice)) toast("Added " + o->name());
    }

    drawTutorialHighlight();
    if (m_formulaEditor.draw(m_scene)) {
        m_referenceWindow.setVisible(true);
        m_panels.bringToFront(&m_referenceWindow);
    }
    if (m_fileDialog.draw()) {
        if (m_fileDialog.kind() == FileDialog::Kind::Latex) writeLatex(m_fileDialog.path());
        else if (m_fileDialog.kind() == FileDialog::Kind::Pdf) {
            m_exportPanel.startPdf(exportLatexCode(true), m_fileDialog.path());
            toast("Compiling the PDF…");
        }
        else if (m_fileDialog.mode() == FileDialog::Mode::Save) writeScene(m_fileDialog.path());
        else readScene(m_fileDialog.path());
    }
    drawToast(deltaTime);
    m_help.draw();
    ui::drawTooltip(m_uiSettings.tooltips && !m_help.visible());
}

StepsInput App::stepsInput() const {
    StepsInput in;
    in.mode = m_mode;
    in.matrix = matrix();
    in.is4x4 = using4x4();
    // Vectors: the selected ones, else a and b
    std::vector<Vector3D> vectors;
    const scene::Space& sp = m_scene.space(SpaceKind::Euclidean3D);
    for (auto id : const_cast<scene::Scene&>(m_scene).selection(SpaceKind::Euclidean3D).ids) {
        const SceneObject* o = sp.find(id);
        if (o && o->value().type == expr::Type::Vector) vectors.push_back(o->value().vector);
    }
    in.a = vectors.size() > 0 ? vectors[0] : vectorA();
    in.b = vectors.size() > 1 ? vectors[1] : vectorB();
    // Complex numbers: the selected ones, else z1 and z2
    std::vector<Complex> numbers;
    const scene::Space& cp = m_scene.space(SpaceKind::Complex);
    for (auto id : const_cast<scene::Scene&>(m_scene).selection(SpaceKind::Complex).ids) {
        const SceneObject* o = cp.find(id);
        if (o && o->value().isNumber() && o->typeId() != "parameter")
            numbers.push_back(Complex(static_cast<float>(o->value().re), static_cast<float>(o->value().im)));
    }
    in.z1 = numbers.size() > 0 ? numbers[0] : complexValue("z1");
    in.z2 = numbers.size() > 1 ? numbers[1] : complexValue("z2");
    in.theta = static_cast<float>(parameterValue("theta", AppMode::Complex));
    return in;
}

void App::drawFloatingPanels() {
    bool space3d = (m_mode == AppMode::Space3D);
    for (ui::FloatingPanel* p : m_panels.panels()) {
        Rectangle content;
        if (p == &m_analysisWindow) {
            if (p->begin(m_analysisHeight, content)) {
                m_analysisHeight = m_infoPanel.drawContent(content);
                p->end();
            }
        } else if (p == &m_exportWindow) {
            if (p->begin(m_exportHeight, content)) {
                double az = 25, el = 30;
                exportView(az, el);
                ExportPanel::Actions act;
                m_exportHeight = m_exportPanel.drawContent(content, m_scene.space(spaceKind()),
                                                           m_scene.selection(spaceKind()), az, el, act);
                p->end();
                if (act.copy) {
                    SetClipboardText(exportLatexCode().c_str());
                    toast("LaTeX copied: paste it into your document");
                }
                if (act.save) {
                    closePopovers();
                    m_fileDialog.open(FileDialog::Mode::Save, currentFolder(), "figure", {}, FileDialog::Kind::Latex);
                }
                if (act.savePdf) {
                    closePopovers();
                    m_fileDialog.open(FileDialog::Mode::Save, currentFolder(), "figure", {}, FileDialog::Kind::Pdf);
                }
            }
        } else if (p == &m_referenceWindow) {
            if (p->begin(m_referenceHeight, content)) {
                m_referenceHeight = m_referencePanel.drawContent(content, &m_scene.space(spaceKind()),
                                                                 m_scene.timeline().time);
                p->end();
            }
        } else if (p == &m_animationWindow) {
            if (p->begin(m_animationHeight, content)) {
                m_animationHeight = m_timePanel.drawContent(content, m_scene, spaceKind());
                p->end();
            }
        } else if (p == &m_breakdownWindow) {
            if (!space3d) continue;
            if (p->begin(m_breakdownHeight, content)) {
                m_breakdownHeight = m_breakdownPanel.drawContent(content);
                p->end();
            }
        } else if (p == &m_tutorialWindow) {
            if (p->begin(m_tutorialHeight, content)) {
                m_tutorialHeight = m_tutorialPanel.drawContent(content, *this);
                p->end();
            }
        } else if (p == &m_galleryWindow) {
            if (p->begin(m_galleryHeight, content)) {
                int chosen = -1;
                m_galleryHeight = m_galleryPanel.drawContent(content, chosen);
                p->end();
                if (chosen >= 0) {
                    const Example& e = examples()[chosen];
                    m_scenePanel.cancelEditing();
                    e.apply(*this);
                    m_galleryWindow.setVisible(false);
                    toast(std::string("Example: ") + e.title);
                }
            }
        } else if (p == &m_stepsWindow) {
            if (p->begin(m_stepsHeight, content)) {
                m_stepsHeight = m_stepsPanel.drawContent(content, stepsInput());
                p->end();
            }
        }
    }
}

void App::drawTutorialHighlight() {
    if (!m_tutorialWindow.visible() || m_tutorialWindow.collapsed()) return;
    std::string id = m_tutorialPanel.currentAnchor();
    Rectangle r;
    if (id.empty() || !ui::findAnchor(id, r)) return;
    // Pulsing outline around the control the current step is about
    float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 4.0f);
    float grow = ui::px(4.0f + 2.0f * pulse);
    Rectangle ring = {r.x - grow, r.y - grow, r.width + 2 * grow, r.height + 2 * grow};
    Color c = ui::theme().accent;
    Color soft = c;
    soft.a = static_cast<unsigned char>(40 + 50 * pulse);
    ui::strokeRounded(ring, ui::px(ui::kRadius + 4), ui::px(5.0f), soft);
    ui::strokeRounded(ring, ui::px(ui::kRadius + 4), ui::pxi(2.0f), c);
}

void App::drawHud() {
    const ui::Theme& t = ui::theme();
    bool ui = uiMode();
    float x = viewportLeft() + ui::px(16.0f);
    float bottom = static_cast<float>(GetScreenHeight()) - ui::px(14.0f);

    bool orbit = m_camera.mode() == CameraMode::Orbit;
    const char* orbitText = m_mode == AppMode::Complex
                                ? "Click to select  ·  drag: pan  ·  wheel: zoom  ·  H: help"
                                : "Click to select  ·  drag: orbit  ·  right-drag: pan  ·  wheel: zoom  ·  . key: focus";
    const char* modeText = orbit ? orbitText
                           : ui  ? "UI mode  ·  click objects to select  ·  Tab: camera  ·  H: help"
                                 : "Camera mode  ·  WASD move, mouse look  ·  Esc or Tab: free the mouse";
    Vector3 p = m_camera.position();
    const char* camText = TextFormat("Camera (%.2f, %.2f, %.2f)  ·  %d FPS", p.x, p.y, p.z, GetFPS());

    float lineH = ui::px(20.0f);
    float w = std::max(ui::textWidth(modeText, ui::FontRole::Small), ui::textWidth(camText, ui::FontRole::Small));
    Rectangle box = {x, bottom - 2 * lineH - ui::px(10.0f), w + ui::px(20.0f), 2 * lineH + ui::px(10.0f)};
    ui::fillRounded(box, ui::px(8.0f), t.labelBg);
    float tx = box.x + ui::px(10.0f);
    // A small dot shows the input mode at a glance
    DrawCircleV({tx + ui::px(3), box.y + ui::px(5) + lineH / 2}, ui::px(3.5f), ui || orbit ? t.accent : t.success);
    ui::textIn({tx + ui::px(12), box.y + ui::px(5), w, lineH}, modeText, ui::FontRole::Small, t.text);
    ui::textIn({tx, box.y + ui::px(5) + lineH, w, lineH}, camText, ui::FontRole::Small, t.textMuted);
}

// "Only the selection" chip at the top of the view while solo is on
void App::drawSoloChip() {
    const ViewSettings& view = m_viewPanel.settings();
    if (!view.soloSelection || m_scene.selection(spaceKind()).empty()) return;
    const ui::Theme& t = ui::theme();
    const char* text = "Only the selection  ·  O shows everything";
    float w = ui::textWidth(text, ui::FontRole::Small) + ui::px(34.0f);
    Rectangle box = {viewportLeft() + ui::px(16.0f), TopBar::height() + ui::px(12.0f), w, ui::px(28.0f)};
    ui::fillRounded(box, box.height / 2, t.labelBg);
    ui::strokeRounded(box, box.height / 2, ui::px(1.0f), t.accent);
    ui::drawIcon(ui::Icon::Solo, {box.x + ui::px(6.0f), box.y + ui::px(5.0f), ui::px(18.0f), ui::px(18.0f)}, t.accent);
    ui::textIn({box.x + ui::px(26.0f), box.y, w, box.height}, text, ui::FontRole::Small, t.text);
}

void App::drawToast(float deltaTime) {
    if (m_toast.timeLeft <= 0.0f) return;
    m_toast.timeLeft -= deltaTime;
    const ui::Theme& t = ui::theme();

    float alpha = m_toast.timeLeft < 0.4f ? m_toast.timeLeft / 0.4f : 1.0f;
    float w = ui::textWidth(m_toast.text.c_str(), ui::FontRole::Body) + ui::px(56.0f);
    float h = ui::px(42.0f);
    float left = viewportLeft(), right = viewportRight();
    Rectangle box = {left + (right - left - w) / 2, TopBar::height() + ui::px(16.0f), w, h};

    Color accent = m_toast.error ? t.danger : t.success;
    ui::shadow(box, ui::px(10), ui::px(10));
    ui::fillRounded(box, ui::px(10), Fade(t.panel, alpha));
    ui::strokeRounded(box, ui::px(10), ui::pxi(1), Fade(t.border, alpha));
    float iconSize = ui::px(18.0f);
    Rectangle icon = {box.x + ui::px(14), box.y + (h - iconSize) / 2, iconSize, iconSize};
    ui::drawIcon(m_toast.error ? ui::Icon::Close : ui::Icon::Check, icon, Fade(accent, alpha));
    ui::textIn({icon.x + iconSize + ui::px(10), box.y, w, h}, m_toast.text.c_str(), ui::FontRole::Body,
               Fade(t.text, alpha));
}

} // namespace linalg
