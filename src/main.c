#include "raylib.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define FPS 144
#define N 3

#define GRAV_CONST 6.674e-3f  // scaled up from the real constant so motion is visible
#define SOFTENING 5.0f  // prevents divide-by-near-zero at close range

typedef struct {
    float x, y;  // position
    float vx, vy;  // velocity
    float mass;
    float radius;  // used only for drawing
} Body;

// declare at file-scope to prevent stack overflow later
// (sizeof(Body) * N)
static Body bodies[N];


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