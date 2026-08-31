#include "raylib.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define FPS 144
#define N 50  // no. of simulated bodies

#define GRAV_CONST 6.674e-3f  // scaled up from the real G so motion is visible
#define SOFTENING 5.0f  // prevents divide-by-near-zero at close range

typedef struct {
    float x, y;  // position
    float vx, vy;  // velocity
    float mass;
    float radius;  // rendering only
} Body;

// declare at file-scope to prevent stack overflow later
static Body bodies[N];

/*
 * Places bodies in an orbiting disk around a heavy central mass at
 * (centerX, centerY). Body 0 is the fixed central mass; every other body
 * gets a random position and a velocity that makes it orbit in a circle.
 */
void init_bodies(float centerX, float centerY) {
    bodies[0] = (Body) {
        centerX, 
        centerY, 
        0, 
        0, 
        50000.0f, 
        12.0f 
    };

    for (int i = 1; i < N; i ++) {
        /* Random point on a disk: pick an angle (0 to 2*PI, a full circle)
        and a distance from the center */
        float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float radius = 50.0f + ((float)rand() / RAND_MAX) * 400.0f;  // between 50-450 pixels

        /* Convert polar to Cartesian coords */
        float x = centerX + cosf(angle) * radius;
        float y = centerY + sinf(angle) * radius;

        /* Circular orbit speed: v = sqrt(G*M/r) */ 
        float speed = sqrtf(GRAV_CONST * bodies[0].mass / radius);

        /* perpendicular to radius, so it orbits instead of falling in */
        float vx = -sinf(angle) * speed;
        float vy = cosf(angle) * speed;

        bodies[i] = (Body) { 
            x, 
            y, 
            vx, 
            vy, 
            1.0f, 
            1.5f 
        };
    }
}


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