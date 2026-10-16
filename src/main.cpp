#include <raylib.h>
#include <raymath.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include "math/Vector3D.hpp"
#include "math/Matrix3D.hpp"
#include "math/Matrix4D.hpp"
#include "rendering/Grid.hpp"
#include "rendering/VectorRenderer.hpp"
#include "rendering/TransformVisualizer.hpp"
#include "ui/InputPanel.hpp"

#include <iostream>

using namespace linalg;

int main() {
    const int screenWidth = 1600;
    const int screenHeight = 900;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Linear Algebra 3D Visualizer");
    SetWindowMinSize(800, 600);
    SetTargetFPS(60);

    // Setup 3D camera - First Person
    Camera3D camera = {};
    camera.position = Vector3{5.0f, 3.0f, 5.0f};
    camera.target = Vector3{0.0f, 0.0f, 0.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Camera settings
    float cameraMoveSpeed = 5.0f;
    float cameraMouseSensitivity = 0.003f;

    // Disable cursor for FPS-style camera
    DisableCursor();

    // Create visualization objects
    Grid grid(10, 1.0f);
    VectorRenderer vectorRenderer;
    TransformVisualizer transformViz;
    InputPanel inputPanel;

    while (!WindowShouldClose()) {
        // Update
        //----------------
        inputPanel.update();

        // Toggle cursor with TAB key
        if (IsKeyPressed(KEY_TAB)) {
            if (IsCursorHidden()) {
                EnableCursor();
            } else {
                DisableCursor();
            }
        }

        // Mouse look (always active when cursor is hidden)
        if (IsCursorHidden()) {
            Vector2 mouseDelta = GetMouseDelta();

            // Calculate camera direction
            Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
            Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));

            // Yaw (horizontal rotation)
            float yaw = -mouseDelta.x * cameraMouseSensitivity;
            forward = Vector3RotateByAxisAngle(forward, camera.up, yaw);
            right = Vector3RotateByAxisAngle(right, camera.up, yaw);

            // Pitch (vertical rotation)
            float pitch = -mouseDelta.y * cameraMouseSensitivity;
            Vector3 newForward = Vector3RotateByAxisAngle(forward, right, pitch);

            // Limit pitch to avoid flipping
            float angle = Vector3Angle(newForward, camera.up);
            if (angle > 0.1f && angle < 3.04f) { // ~5 to 175 degrees
                forward = newForward;
            }

            camera.target = Vector3Add(camera.position, forward);
        }

        // WASD movement (always active)
        {
            float deltaTime = GetFrameTime();
            float speed = cameraMoveSpeed * deltaTime;

            // Speed adjustment with Shift
            if (IsKeyDown(KEY_LEFT_SHIFT)) {
                speed *= 2.0f;
            }

            Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
            Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));

            if (IsKeyDown(KEY_W)) camera.position = Vector3Add(camera.position, Vector3Scale(forward, speed));
            if (IsKeyDown(KEY_S)) camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, speed));
            if (IsKeyDown(KEY_A)) camera.position = Vector3Subtract(camera.position, Vector3Scale(right, speed));
            if (IsKeyDown(KEY_D)) camera.position = Vector3Add(camera.position, Vector3Scale(right, speed));

            // Up/Down with Space/Ctrl
            if (IsKeyDown(KEY_SPACE)) camera.position.y += speed;
            if (IsKeyDown(KEY_LEFT_CONTROL)) camera.position.y -= speed;

            // Update target to follow position
            camera.target = Vector3Add(camera.position, forward);
        }

        // Update transformations when matrix changes
        if (inputPanel.matrixChanged()) {
            if (inputPanel.isUsing4x4()) {
                transformViz.setTransform4D(inputPanel.getMatrix4D());
            } else {
                transformViz.setTransform3D(inputPanel.getMatrix3D());
            }
        }

        // Update vectors when they change
        if (inputPanel.vectorsChanged()) {
            vectorRenderer.clearVectors();

            Vector3D vecA = inputPanel.getVectorA();
            Vector3D vecB = inputPanel.getVectorB();

            // Add vectors to renderer
            vectorRenderer.addVector(VectorObject(Vector3D::zero(), vecA, YELLOW, "A"));
            vectorRenderer.addVector(VectorObject(Vector3D::zero(), vecB, MAGENTA, "B"));

            // Show vector sum
            Vector3D sum = vecA + vecB;
            vectorRenderer.addVector(VectorObject(Vector3D::zero(), sum, ORANGE, "A+B"));

            // Show cross product
            Vector3D cross = vecA.cross(vecB);
            vectorRenderer.addVector(VectorObject(Vector3D::zero(), cross, SKYBLUE, "A×B"));
        }

        inputPanel.resetChangedFlags();

        // Draw
        //----------------
        BeginDrawing();
            ClearBackground(RAYWHITE);

            // 3D rendering
            BeginMode3D(camera);
                grid.draw();
                grid.drawAxes(5.0f);
                transformViz.draw();
                vectorRenderer.draw();
            EndMode3D();

            // 2D UI overlay
            inputPanel.draw();

            // Help text (responsive to window size)
            int currentWidth = GetScreenWidth();
            int currentHeight = GetScreenHeight();

            if (IsCursorHidden()) {
                DrawText("WASD: Move | Mouse: Look", currentWidth - 500, 10, 28, DARKBLUE);
                DrawText("Space/Ctrl: Up/Down | Shift: Sprint", currentWidth - 500, 44, 24, DARKGRAY);
                DrawText("TAB: Show Cursor for UI", currentWidth - 500, 74, 24, RED);
            } else {
                DrawText("TAB: Hide Cursor for Camera", currentWidth - 500, 10, 28, RED);
                DrawText("Click UI to edit values", currentWidth - 500, 44, 24, DARKGRAY);
            }

            DrawFPS(currentWidth - 100, currentHeight - 30);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
