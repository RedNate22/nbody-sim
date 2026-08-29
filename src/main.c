#include "raylib.h"

int main() {
    InitWindow(1280, 720, "N-Body");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        // update_bodies(bodies, n, dt);

        BeginDrawing();
        ClearBackground(BLACK);
        // for (int i = 0; i < n; i++) {
        //     DrawCircleV((Vector2){bodies[i].x, bodies[i].y}, bodies[i].radius, WHITE);
        // }
        EndDrawing();
    }
    CloseWindow();

    return 0;
}