#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

struct Building {
    Vector3 pos;
    Vector3 size;
    Color color;
};

static float Clamp01(float x) { return std::max(0.0f, std::min(1.0f, x)); }

int main() {
    const int screenW = 1280;
    const int screenH = 720;

    InitWindow(screenW, screenH, "Mini Vice City - Mobile");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    Camera3D camera{};
    camera.position = {0.0f, 9.0f, -13.0f};
    camera.target = {0.0f, 1.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Vector3 player = {0.0f, 0.9f, 0.0f};
    Vector3 car = {5.0f, 0.65f, 4.0f};
    float playerYaw = 0.0f;
    float carYaw = 0.0f;
    float carSpeed = 0.0f;
    float walkSpeed = 5.0f;
    bool inCar = false;
    int wanted = 0;
    float wantedTimer = 0.0f;
    int cash = 250;

    // A compact procedural city: roads run along the X and Z axes.
    std::vector<Building> buildings;
    for (int x = -3; x <= 3; ++x) {
        for (int z = -3; z <= 3; ++z) {
            if ((x == 0) || (z == 0)) continue; // leave broad streets
            float h = 3.0f + (float)((x*x + z*z + 3) % 5) * 1.25f;
            Vector3 p = {(float)x * 13.0f, h * 0.5f, (float)z * 13.0f};
            Vector3 s = {8.0f, h, 8.0f};
            Color c = (Color){(unsigned char)(90 + (x*17+z*9+120)%70),
                              (unsigned char)(105 + (x*11-z*7+100)%65),
                              (unsigned char)(135 + (x*5+z*13+100)%75), 255};
            buildings.push_back({p, s, c});
        }
    }

    // Touch controls: left half = virtual stick, right half = action buttons.
    Vector2 stickOrigin = {115.0f, (float)GetScreenHeight() - 125.0f};
    Vector2 stickNow = stickOrigin;
    bool stickActive = false;
    bool accelerate = false, brake = false, actionPressed = false;

    auto drawButton = [](Rectangle r, const char* label, bool active) {
        DrawCircle((int)(r.x + r.width/2), (int)(r.y + r.height/2),
                   r.width/2, active ? Fade(ORANGE, 0.85f) : Fade(DARKBLUE, 0.72f));
        DrawCircleLines((int)(r.x + r.width/2), (int)(r.y + r.height/2),
                        r.width/2, RAYWHITE);
        int tw = MeasureText(label, 18);
        DrawText(label, (int)(r.x + (r.width-tw)/2),
                 (int)(r.y + r.height/2-9), 18, RAYWHITE);
    };

    Rectangle gasBtn = {(float)GetScreenWidth()-145.0f, (float)GetScreenHeight()-175.0f, 88, 88};
    Rectangle brakeBtn = {(float)GetScreenWidth()-245.0f, (float)GetScreenHeight()-105.0f, 68, 68};
    Rectangle actionBtn = {(float)GetScreenWidth()-135.0f, (float)GetScreenHeight()-285.0f, 78, 78};

    while (!WindowShouldClose()) {
        float dt = std::min(GetFrameTime(), 0.033f);
        float moveX = 0.0f, moveZ = 0.0f;
        accelerate = false;
        brake = false;
        actionPressed = false;

        // Keyboard controls are useful for desktop testing; touch is the main mobile input.
        if (IsKeyDown(KEY_A)) moveX -= 1.0f;
        if (IsKeyDown(KEY_D)) moveX += 1.0f;
        if (IsKeyDown(KEY_W)) moveZ += 1.0f;
        if (IsKeyDown(KEY_S)) moveZ -= 1.0f;
        if (IsKeyDown(KEY_SPACE)) actionPressed = true;
        if (IsKeyDown(KEY_UP)) accelerate = true;
        if (IsKeyDown(KEY_DOWN)) brake = true;

        int touchCount = GetTouchPointCount();
        stickActive = false;
        Vector2 currentStick = stickOrigin;
        for (int i = 0; i < touchCount; ++i) {
            Vector2 t = GetTouchPosition(i);
            if (t.x < (float)GetScreenWidth() * 0.52f) {
                if (!stickActive) {
                    stickActive = true;
                    currentStick = t;
                }
            } else {
                if (CheckCollisionPointRec(t, gasBtn)) accelerate = true;
                if (CheckCollisionPointRec(t, brakeBtn)) brake = true;
                if (CheckCollisionPointRec(t, actionBtn)) actionPressed = true;
            }
        }

        if (stickActive) {
            Vector2 delta = Vector2Subtract(currentStick, stickOrigin);
            float len = Vector2Length(delta);
            if (len > 58.0f) delta = Vector2Scale(Vector2Normalize(delta), 58.0f);
            moveX += Clamp01((delta.x + 58.0f) / 116.0f) * 2.0f - 1.0f;
            moveZ += -(Clamp01((delta.y + 58.0f) / 116.0f) * 2.0f - 1.0f);
            stickNow = Vector2Add(stickOrigin, delta);
        } else {
            stickNow = stickOrigin;
        }

        if (IsKeyPressed(KEY_E)) actionPressed = true;

        if (actionPressed) {
            Vector3 focus = inCar ? car : player;
            if (!inCar && Vector3Distance(player, car) < 3.2f) {
                inCar = true;
                wanted = std::max(wanted, 1);
                wantedTimer = 12.0f;
            } else if (inCar) {
                inCar = false;
                player = {car.x + 2.0f, 0.9f, car.z};
                carSpeed = 0.0f;
            } else {
                // A simple pickup reward gives the prototype a small game loop.
                cash += 10;
            }
        }

        if (inCar) {
            if (accelerate) carSpeed += 15.0f * dt;
            if (brake) carSpeed -= 19.0f * dt;
            carSpeed *= std::pow(0.985f, dt * 60.0f);
            carSpeed = std::max(-8.0f, std::min(22.0f, carSpeed));
            float steering = moveX;
            carYaw += steering * dt * (0.8f + std::fabs(carSpeed) * 0.035f);
            car.x += std::sinf(carYaw) * carSpeed * dt;
            car.z += std::cosf(carYaw) * carSpeed * dt;
            car.x = std::max(-48.0f, std::min(48.0f, car.x));
            car.z = std::max(-48.0f, std::min(48.0f, car.z));
        } else {
            Vector3 direction = {moveX, 0.0f, moveZ};
            if (Vector3Length(direction) > 1.0f) direction = Vector3Normalize(direction);
            if (Vector3Length(direction) > 0.05f) {
                playerYaw = std::atan2f(direction.x, direction.z);
                player.x += direction.x * walkSpeed * dt;
                player.z += direction.z * walkSpeed * dt;
            }
            player.x = std::max(-48.0f, std::min(48.0f, player.x));
            player.z = std::max(-48.0f, std::min(48.0f, player.z));
        }

        if (wanted > 0) {
            wantedTimer -= dt;
            if (wantedTimer <= 0.0f) wanted = 0;
        }

        Vector3 focus = inCar ? car : player;
        Vector3 behind = inCar
            ? Vector3{-std::sinf(carYaw) * 10.0f, 6.0f, -std::cosf(carYaw) * 10.0f}
            : Vector3{-std::sinf(playerYaw) * 8.0f, 5.0f, -std::cosf(playerYaw) * 8.0f};
        Vector3 desiredCam = Vector3Add(focus, behind);
        camera.position = Vector3Lerp(camera.position, desiredCam, 5.0f * dt);
        camera.target = Vector3Lerp(camera.target, Vector3Add(focus, {0, 1.0f, 0}), 8.0f * dt);

        BeginDrawing();
        ClearBackground((Color){115, 185, 220, 255});

        BeginMode3D(camera);
        DrawPlane({0, -0.08f, 0}, {120, 120}, (Color){65, 125, 83, 255});

        // Main roads and side streets.
        DrawCube({0, 0.0f, 0}, 120, 0.12f, 13, (Color){48, 49, 55, 255});
        DrawCube({0, 0.01f, 0}, 13, 0.12f, 120, (Color){48, 49, 55, 255});
        for (int i = -5; i <= 5; ++i) {
            DrawCube({(float)i*10.0f, 0.02f, 0}, 3.0f, 0.025f, 0.18f, YELLOW);
            DrawCube({0, 0.02f, (float)i*10.0f}, 0.18f, 0.025f, 3.0f, YELLOW);
        }

        for (const Building& b : buildings) {
            DrawCube(b.pos, b.size.x, b.size.y, b.size.z, b.color);
            DrawCubeWires(b.pos, b.size.x, b.size.y, b.size.z, Fade(BLACK, 0.25f));
            // Bright window bands make the blocky city easier to read.
            for (float y = 1.0f; y < b.size.y - 0.5f; y += 1.5f) {
                DrawCube({b.pos.x, y, b.pos.z + b.size.z/2 + 0.015f},
                         b.size.x*0.72f, 0.28f, 0.035f, (Color){245, 205, 115, 255});
            }
        }

        // Parked player car.
        DrawCube({car.x, car.y, car.z}, 2.3f, 0.9f, 4.2f, RED);
        DrawCube({car.x, car.y+0.58f, car.z-0.15f}, 1.8f, 0.55f, 2.0f, SKYBLUE);
        DrawCubeWires({car.x, car.y, car.z}, 2.3f, 0.9f, 4.2f, MAROON);
        for (int side : {-1, 1}) {
            for (int front : {-1, 1}) {
                DrawCylinder({car.x + side*1.12f, car.y-0.2f, car.z + front*1.25f},
                             0.28f, 0.28f, 0.35f, 12, BLACK);
            }
        }

        if (!inCar) {
            DrawCube({player.x, player.y, player.z}, 0.8f, 1.8f, 0.65f, BLUE);
            DrawSphere({player.x, player.y+1.12f, player.z}, 0.38f, BEIGE);
            DrawCubeWires({player.x, player.y, player.z}, 0.8f, 1.8f, 0.65f, DARKBLUE);
        }

        // Simple palm-like trees around the city edges.
        for (int i = -5; i <= 5; ++i) {
            for (int side : {-1, 1}) {
                Vector3 p = {(float)i*9.0f, 0.0f, (float)side*54.0f};
                DrawCylinder({p.x, 2.0f, p.z}, 0.22f, 0.18f, 4.0f, 8, BROWN);
                DrawSphere({p.x, 4.2f, p.z}, 1.25f, GREEN);
            }
        }
        EndMode3D();

        DrawRectangle(12, 12, 230, 76, Fade(BLACK, 0.55f));
        DrawText("MINI VICE CITY", 24, 20, 22, RAYWHITE);
        DrawText(TextFormat("$ %d", cash), 24, 51, 20, GOLD);
        DrawText(inCar ? "IN VEHICLE" : "ON FOOT", 150, 54, 16, RAYWHITE);

        if (wanted > 0) {
            DrawText("WANTED: *", GetScreenWidth()-190, 22, 24, RED);
        }

        // Touch UI.
        DrawCircle((int)stickOrigin.x, (int)stickOrigin.y, 62, Fade(DARKBLUE, 0.35f));
        DrawCircleLines((int)stickOrigin.x, (int)stickOrigin.y, 62, Fade(RAYWHITE, 0.8f));
        DrawCircle((int)stickNow.x, (int)stickNow.y, 27, Fade(RAYWHITE, 0.7f));

        drawButton(gasBtn, inCar ? "GAS" : "RUN", accelerate);
        drawButton(brakeBtn, "BRAKE", brake);
        drawButton(actionBtn, inCar ? "EXIT" : (Vector3Distance(player, car) < 3.2f ? "ENTER" : "ACT"), actionPressed);

        DrawText("Touch: left stick | right buttons", 14, GetScreenHeight()-24, 14, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}