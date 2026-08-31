#include "raylib.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define FPS 60
#define N 1200

#define GRAV_CONST 6.674e-3f  // scaled up from the real G so motion is visible
#define SOFTENING 5.0f  // prevents divide-by-near-zero at close range

typedef struct {
    float x, y;  // position
    float vx, vy;  // velocity
    float mass;
    float radius;  // rendering only
    Color color;
} Body;

/* Rough star colours across the temperature spectrum: cool red/orage
through white to hot blue-white */
static const Color STAR_COLORS[] = {
    (Color){255, 180, 120, 255},  // cool orange
    (Color){255, 214, 170, 255},  // warm white
    (Color){255, 244, 214, 255},  // yellow-white, sun-like
    (Color){255, 255, 255, 255},  // white
    (Color){202, 216, 255, 255},  // blue-white, hot
};
#define STAR_COLOR_COUNT (sizeof(STAR_COLORS) / sizeof(STAR_COLORS[0]))

// declare at file-scope to prevent stack overflow later
static Body bodies[N];

/**
 * @brief Initializes the global bodies[] array with a central mass and a
 *        disk of smaller bodies orbiting it.
 *
 * bodies[0] is placed at (centerX, centerY) as a fixed, heavy central mass.
 * Every other body is placed at a random angle and radius around it, with
 * a velocity computed for a circular orbit:
 *
 *     v = sqrt(G * M / r)
 *
 * derived by setting gravitational force equal to centripetal force
 * (G*M*m/r^2 = m*v^2/r, with the orbiting body's mass m cancelling out).
 *
 * @param centerX X coordinate of the central mass.
 * @param centerY Y coordinate of the central mass.
 */
void init_bodies(float centerX, float centerY) {
    bodies[0] = (Body) {
        centerX, 
        centerY, 
        0, 
        0, 
        50000.0f, 
        12.0f,
        (Color){255, 244, 214, 255}
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

        Color color = STAR_COLORS[rand() % STAR_COLOR_COUNT];
        bodies[i] = (Body) { 
            x, 
            y, 
            vx, 
            vy, 
            1.0f, 
            1.5f,
            color
        };
    }
}

/**
 * @brief Advances the simulation by one time step using sequential
 *        O(n^2) brute-force gravity.
 *
 * Implements the n-body equation of motion:
 *
 *     m_i * d^2(q_i)/dt^2 = sum_(j!=i) [ G * m_j * (q_j - q_i) / ||q_j - q_i||^3 ]
 *
 * (m_i cancels out of both sides, so it never appears below). A softening
 * term (epsilon^2) is added to the squared distance to avoid dividing by
 * a near-zero distance when two bodies pass close together.
 *
 * Runs in two passes: accelerations are fully computed from the positions
 * at the start of the step (Pass 1) before any body moves, then velocity
 * and position are integrated using semi-implicit Euler (Pass 2). This
 * ordering ensures every body's force calculation uses one consistent
 * snapshot of the system, not a mix of old and already-updated positions.
 *
 * @param dt Time step in seconds (from GetFrameTime()).
 */
void update_bodies(float dt) {
    float ax[N], ay[N];

    for (int i = 0; i < N; i++) {
        ax[i] = 0.0f;
        ay[i] = 0.0f;

        for (int j = 0; j < N; j++) {
            if (i == j) continue;
            
            float dx = bodies[j].x - bodies[i].x;
            float dy = bodies[j].y - bodies[i].y;
            float dist2 = dx*dx + dy*dy + SOFTENING*SOFTENING;
            float dist = sqrtf(dist2);

            float accel_scale = GRAV_CONST * bodies[j].mass / (dist2 * dist);
            ax[i] += dx * accel_scale;
            ay[i] += dy * accel_scale;
        }
    }

    for (int i = 0; i < N; i++) {
        bodies[i].vx += ax[i] * dt;
        bodies[i].vy += ay[i] * dt;
        bodies[i].x += bodies[i].vx * dt;
        bodies[i].y += bodies[i].vy * dt;
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
        double step_ms = (GetTime() - step_start * 1000.0);

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