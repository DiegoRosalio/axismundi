#include <raylib.h>
#include <raygui.h>

#include "math/Matrix3D.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include "rendering/ArgandRenderer.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/Grid.hpp"
#include "rendering/Labels.hpp"
#include "rendering/TransformVisualizer.hpp"
#include "rendering/VectorRenderer.hpp"
#include "ui/AppMode.hpp"
#include "ui/ComplexPanel.hpp"
#include "ui/HelpOverlay.hpp"
#include "ui/InfoPanel.hpp"
#include "ui/InputPanel.hpp"
#include "ui/SceneIO.hpp"
#include "ui/SideBar.hpp"
#include "ui/ViewPanel.hpp"
#include "ui/Widgets.hpp"

#include <string>

using namespace linalg;

namespace {

const char* kSceneFile = "scene.txt";

// Short status message shown at the bottom of the screen
struct Toast {
    std::string text;
    Color color = DARKGREEN;
    float timeLeft = 0.0f;

    void show(const std::string& message, Color c) {
        text = message;
        color = c;
        timeLeft = 3.0f;
    }

    void draw(float deltaTime) {
        if (timeLeft <= 0.0f) return;
        timeLeft -= deltaTime;
        int fontSize = 24;
        int width = MeasureText(text.c_str(), fontSize);
        int x = (GetScreenWidth() - width) / 2;
        int y = GetScreenHeight() - 180;
        float alpha = timeLeft < 0.5f ? timeLeft / 0.5f : 1.0f;
        DrawRectangle(x - 14, y - 10, width + 28, fontSize + 20, Fade(RAYWHITE, 0.95f * alpha));
        DrawRectangleLines(x - 14, y - 10, width + 28, fontSize + 20, Fade(color, alpha));
        DrawText(text.c_str(), x, y, fontSize, Fade(color, alpha));
    }
};

bool isIdentity(const Matrix4D& m) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (m.m[i][j] != (i == j ? 1.0f : 0.0f)) return false;
    return true;
}

void drawAxisLabels(const Camera3D& camera, float length) {
    drawWorldLabel(camera, {length, 0, 0}, "X", 22, RED);
    drawWorldLabel(camera, {0, length, 0}, "Y", 22, DARKGREEN);
    drawWorldLabel(camera, {0, 0, length}, "Z", 22, BLUE);
}

} // namespace

int main() {
    const int screenWidth = 1600;
    const int screenHeight = 900;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Linear Algebra 3D Visualizer");
    SetWindowMinSize(800, 600);
    SetTargetFPS(60);

    CameraController camera;
    Grid grid(10, 1.0f);
    VectorRenderer vectorRenderer;
    TransformVisualizer transformViz;
    InputPanel inputPanel;
    ComplexPanel complexPanel;
    ArgandRenderer argand;
    InfoPanel infoPanel;
    ViewPanel viewPanel;
    SideBar sideBar;
    HelpOverlay help;
    Toast toast;

    AppMode mode = AppMode::LinearAlgebra;
    AppMode lastMode = mode;

    // Committed inputs, refreshed when the panel reports a change
    Matrix4D matrix = Matrix4D::identity();
    Vector3D vecA, vecB;

    // Start in camera mode (cursor captured)
    DisableCursor();

    auto saveScene = [&]() {
        SceneData data;
        putInt(data, "mode", static_cast<int>(mode));
        Vector3 p = camera.position(), t = camera.target();
        putFloat(data, "camera.px", p.x);
        putFloat(data, "camera.py", p.y);
        putFloat(data, "camera.pz", p.z);
        putFloat(data, "camera.tx", t.x);
        putFloat(data, "camera.ty", t.y);
        putFloat(data, "camera.tz", t.z);
        inputPanel.saveState(data);
        complexPanel.saveState(data);
        viewPanel.saveState(data);

        std::string error;
        if (saveSceneFile(kSceneFile, data, error))
            toast.show(std::string("Scene saved to ") + kSceneFile, DARKGREEN);
        else
            toast.show("Save failed: " + error, MAROON);
    };

    auto loadScene = [&]() {
        SceneData data;
        std::string error;
        if (!loadSceneFile(kSceneFile, data, error)) {
            toast.show("Load failed: " + error, MAROON);
            return;
        }
        int m = static_cast<int>(mode);
        if (getInt(data, "mode", m) && m >= 0 && m < kAppModeCount) {
            mode = static_cast<AppMode>(m);
            lastMode = mode;  // keep the saved camera instead of the mode default
        }
        Vector3 p = camera.position(), t = camera.target();
        getFloat(data, "camera.px", p.x);
        getFloat(data, "camera.py", p.y);
        getFloat(data, "camera.pz", p.z);
        getFloat(data, "camera.tx", t.x);
        getFloat(data, "camera.ty", t.y);
        getFloat(data, "camera.tz", t.z);
        camera.setView(p, t);
        inputPanel.loadState(data);
        complexPanel.loadState(data);
        viewPanel.loadState(data);
        toast.show(std::string("Scene loaded from ") + kSceneFile, DARKGREEN);
    };

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        ViewSettings& view = viewPanel.settings();

        // Input
        //----------------
        bool editing = inputPanel.isEditing() || complexPanel.isEditing();

        // TAB switches between camera mode (cursor captured) and UI mode.
        // While a field is being edited, TAB moves to the next field instead.
        if (!editing && IsKeyPressed(KEY_TAB)) {
            if (IsCursorHidden()) EnableCursor();
            else DisableCursor();
        }
        bool uiMode = !IsCursorHidden();
        if (uiMode) GuiUnlock();
        else GuiLock();

        inputPanel.update();
        complexPanel.update(deltaTime);

        // Keyboard shortcuts (ignored while typing in a field)
        if (!editing) {
            bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

            if (IsKeyPressed(KEY_H)) help.toggle();
            if (IsKeyPressed(KEY_G)) view.showGrid = !view.showGrid;
            if (IsKeyPressed(KEY_L)) view.showLabels = !view.showLabels;
            if (IsKeyPressed(KEY_M)) infoPanel.toggleVisible();
            if (IsKeyPressed(KEY_C)) camera.reset(mode);
            if (IsKeyPressed(KEY_ONE)) mode = AppMode::LinearAlgebra;
            if (IsKeyPressed(KEY_TWO)) mode = AppMode::Complex;
            if (IsKeyPressed(KEY_THREE)) mode = AppMode::Plotting;
            if (IsKeyPressed(KEY_F5)) saveScene();
            if (IsKeyPressed(KEY_F9)) loadScene();

            if (mode == AppMode::LinearAlgebra) {
                if (ctrl) {
                    // Ctrl is "move down" in camera mode, so undo/redo is UI-mode only
                    if (uiMode && IsKeyPressed(KEY_Z)) inputPanel.undo();
                    if (uiMode && IsKeyPressed(KEY_Y)) inputPanel.redo();
                } else {
                    if (IsKeyPressed(KEY_I)) inputPanel.applyPreset(MatrixPreset::Identity);
                    if (IsKeyPressed(KEY_R)) inputPanel.applyPreset(MatrixPreset::Rotate45Z);
                    if (IsKeyPressed(KEY_X)) inputPanel.applyPreset(MatrixPreset::ShearX);
                    if (IsKeyPressed(KEY_F)) inputPanel.applyPreset(MatrixPreset::ReflectX);
                    // S moves the camera backwards in camera mode
                    if (uiMode && IsKeyPressed(KEY_S)) inputPanel.applyPreset(MatrixPreset::Scale2);
                }
            }
        }

        if (mode != lastMode) {
            // A field in the panel being hidden would otherwise stay "active"
            inputPanel.finishEditing();
            complexPanel.finishEditing();
            camera.reset(mode);
            lastMode = mode;
        }

        camera.update(deltaTime);

        // Update
        //----------------
        if (inputPanel.matrixChanged() || inputPanel.vectorsChanged()) {
            matrix = inputPanel.getActiveMatrix4D();
            vecA = inputPanel.getVectorA();
            vecB = inputPanel.getVectorB();

            if (inputPanel.isUsing4x4()) transformViz.setTransform4D(matrix);
            else transformViz.setTransform3D(inputPanel.getMatrix3D());

            infoPanel.update(matrix, inputPanel.isUsing4x4(), vecA, vecB);
        }
        inputPanel.resetChangedFlags();

        vectorRenderer.clearVectors();
        vectorRenderer.addVector(VectorObject(Vector3D::zero(), vecA, YELLOW, "A"));
        vectorRenderer.addVector(VectorObject(Vector3D::zero(), vecB, MAGENTA, "B"));
        vectorRenderer.addVector(VectorObject(Vector3D::zero(), vecA + vecB, ORANGE, "A+B"));
        vectorRenderer.addVector(VectorObject(Vector3D::zero(), vecA.cross(vecB), SKYBLUE, "AxB"));
        if (view.showTransformedVectors && !isIdentity(matrix)) {
            Vector3D origin = matrix.transform(Vector3D::zero());
            vectorRenderer.addVector(VectorObject(origin, matrix.transform(vecA) - origin, GOLD, "MA"));
            vectorRenderer.addVector(VectorObject(origin, matrix.transform(vecB) - origin, PINK, "MB"));
        }

        grid.setShowGrid(view.showGrid);
        grid.setShowAxes(view.showAxes);

        // Draw
        //----------------
        BeginDrawing();
            ClearBackground(RAYWHITE);
            const Camera3D& cam = camera.camera();

            BeginMode3D(cam);
                if (mode == AppMode::Complex) {
                    grid.drawPlaneXY();
                    argand.draw(complexPanel.scene(), view.axisLength, view.showAxes);
                } else {
                    grid.draw();
                    grid.drawAxes(view.axisLength);
                }
                if (mode == AppMode::LinearAlgebra) {
                    transformViz.draw();
                    vectorRenderer.draw();
                    if (view.showEigenvectors) {
                        TransformVisualizer::drawEigenvectors(infoPanel.eigen(), view.axisLength);
                    }
                }
            EndMode3D();

            // Labels for 3D objects
            if (view.showLabels) {
                if (mode == AppMode::Complex) {
                    argand.drawLabels(cam, complexPanel.scene(), view.axisLength, view.showAxes);
                } else if (view.showAxes) {
                    drawAxisLabels(cam, view.axisLength);
                }
                if (mode == AppMode::LinearAlgebra) {
                    vectorRenderer.drawLabels(cam);
                    if (view.showEigenvectors) TransformVisualizer::drawEigenLabels(cam, infoPanel.eigen());
                }
            }

            // Side bar
            float y = sideBar.begin(mode);
            if (mode == AppMode::LinearAlgebra) {
                y = inputPanel.draw(SideBar::kPanelX, y, SideBar::kPanelWidth) + SideBar::kGap;
            } else if (mode == AppMode::Complex) {
                y = complexPanel.draw(SideBar::kPanelX, y, SideBar::kPanelWidth) + SideBar::kGap;
            }
            y = viewPanel.draw(SideBar::kPanelX, y, SideBar::kPanelWidth, mode);
            sideBar.end(y);

            if (viewPanel.takeResetCameraRequest()) camera.reset(mode);
            if (viewPanel.takeSaveRequest()) saveScene();
            if (viewPanel.takeLoadRequest()) loadScene();

            if (mode == AppMode::LinearAlgebra) infoPanel.draw();

            // HUD
            int hudX = static_cast<int>(SideBar::kWidth) + 12;
            if (uiMode) {
                DrawText("UI mode: click a field to edit  |  TAB: camera mode  |  H: help",
                         hudX, 12, kUiTextSmall, DARKGRAY);
            } else {
                DrawText("Camera mode: WASD move, mouse look, Space/Ctrl up/down  |  TAB: UI  |  H: help",
                         hudX, 12, kUiTextSmall, DARKBLUE);
            }
            Vector3 p = camera.position();
            DrawText(TextFormat("Camera: (%.2f, %.2f, %.2f)", p.x, p.y, p.z), hudX, 38, kUiTextSmall, GRAY);
            DrawFPS(hudX, 64);

            toast.draw(deltaTime);
            help.draw();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
