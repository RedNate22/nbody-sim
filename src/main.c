#include "raylib.h"
#include "body.h"
#include <stdio.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define FPS 60

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "n-body-sim");

    int monitor = 0;

    Vector2 monitorPos = GetMonitorPosition(monitor);
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);

    int posX = (int)monitorPos.x + (monitorWidth - SCREEN_WIDTH) / 2;
    int posY = (int)monitorPos.y + (monitorHeight - SCREEN_HEIGHT) / 2;

    SetWindowPosition(posX, posY);
    SetTargetFPS(FPS);

    init_bodies(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);

    bool paused = false;
    char info_text[128];

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_R)) {
            init_bodies(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
        }

        /* keep animation consistent regardless of frame rate */
        float dt = GetFrameTime();  

        /* Physics step number for measuring computational cost (not motion speed) */ 
        double step_start = GetTime();
        if (!paused) update_bodies(dt);
        double step_ms = (GetTime() - step_start) * 1000.0;

        BeginDrawing();
        ClearBackground(BLACK);
        for (int i = 0; i < N; i++) {
            DrawCircleV((Vector2) {
                bodies[i].x, 
                bodies[i].y}, 
                bodies[i].radius, 
                bodies[i].color);
        }

        DrawFPS(10, 10);
        snprintf(
            info_text, 
            sizeof(info_text), 
            "Bodies: %d   Physics step: %.2f ms   %s",
            N, step_ms, 
            paused ? "(PAUSED)" : "");
        DrawText(info_text, 10, 35, 20, RAYWHITE);
        DrawText("SPACE: pause   R: reset", 
            10, SCREEN_HEIGHT - 30, 
            18, GRAY);

        EndDrawing();
    }
    CloseWindow();
    return 0;
}