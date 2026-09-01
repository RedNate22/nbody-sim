#include <stdio.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"
#include "body.h"
#include "cli.h"
#include "benchmark.h"

#define DEFAULT_SCREEN_WIDTH 1920
#define DEFAULT_SCREEN_HEIGHT 1080
#define TIME_SCALE 5.0f          // simulated days advanced per real second
#define PHYSICS_SUBSTEP_DAYS 1.0f // max simulated days per integration step

int main(int argc, char *argv[]) {
    SetTraceLogLevel(LOG_WARNING);  // suppress the countless "INFO:" dumps on run

    CliOptions opt;
    parse_cli_options(argc, argv, &opt);

    if (opt.compare) {
        return run_diff(&opt);
    }

    if (opt.headless) {
        return run_headless_benchmark(&opt, DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT);
    }

    InitWindow(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT, "n-body-sim");  // placeholder, resized below
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    int monitor = 0;
    Vector2 monitorPos = GetMonitorPosition(monitor);
    int screen_width = GetMonitorWidth(monitor);
    int screen_height = GetMonitorHeight(monitor);

    SetWindowPosition((int)monitorPos.x, (int)monitorPos.y);

    int refresh_rate = GetMonitorRefreshRate(monitor);
    if (refresh_rate <= 0) refresh_rate = 60;
    SetTargetFPS(refresh_rate);

    SimMode mode = MODE_STARS_ORBITING_BLACKHOLE;
    init_bodies(mode, screen_width / 2.0f, screen_height / 2.0f);

    bool paused = false;
    int startup_frame = 0;
    double total_simulated_days = 0.0;
    char info_text[128];

    Camera2D camera = { 0 };
    camera.target = (Vector2){ screen_width / 2.0f, screen_height / 2.0f };
    camera.offset = (Vector2){ screen_width / 2.0f, screen_height / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    const char *scenario_path = opt.scenario_path ? opt.scenario_path : SCENARIO_DEFAULT_PATH;

    while (!WindowShouldClose()) {
        // prevents annoying bug where it maximizes to the wrong monitor
        if (startup_frame < 10) {
            if (startup_frame == 9) MaximizeWindow();
            startup_frame++;
        }

        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_R)) {
            init_bodies(mode, screen_width / 2.0f, screen_height / 2.0f);
            total_simulated_days = 0.0;
        }

        if (IsKeyPressed(KEY_ONE)) {
            mode = MODE_STARS_ORBITING_BLACKHOLE;
            init_bodies(mode, screen_width / 2.0f, screen_height / 2.0f);
            total_simulated_days = 0.0;
        }
        if (IsKeyPressed(KEY_TWO)) {
            mode = MODE_PLANETS_ORBITING_STAR;
            init_bodies(mode, screen_width / 2.0f, screen_height / 2.0f);
            total_simulated_days = 0.0;
        }
        if (IsKeyPressed(KEY_THREE)) {
            mode = MODE_SOLAR_SYSTEM;
            init_bodies(mode, screen_width / 2.0f, screen_height / 2.0f);
            total_simulated_days = 0.0;
        }
        if (IsKeyPressed(KEY_FOUR)) {
            int count;
            SnapshotHeader header;
            if (load_bodies(scenario_path, bodies, &count, &header)) {
                mode = MODE_CUSTOM;
                total_simulated_days = 0.0;
                body_count = count;
            } else {
                fprintf(stderr, "no scenario file at %s yet, press S on another mode first to create one\n", scenario_path);
            }
        }
        if (IsKeyPressed(KEY_S)) {
            if (save_bodies(scenario_path, bodies, body_count, 0.0f, 0)) {
                printf("saved current scenario to %s\n", scenario_path);
            }
        }
        
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f / camera.zoom);
            camera.target = Vector2Add(camera.target, delta);
        }

        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            Vector2 mouseWorldBefore = GetScreenToWorld2D(GetMousePosition(), camera);

            camera.zoom += wheel * 0.1f * camera.zoom;
            if (camera.zoom < 0.1f) camera.zoom = 0.1f;
            if (camera.zoom > 10.0f) camera.zoom = 10.0f;

            Vector2 mouseWorldAfter = GetScreenToWorld2D(GetMousePosition(), camera);
            camera.target = Vector2Add(camera.target,
                Vector2Subtract(mouseWorldBefore, mouseWorldAfter));
        }

        float real_dt = GetFrameTime();  // wall-clock time
        float dt_days = real_dt * TIME_SCALE;

        /* Substepping bounds the simulated time covered by a single
        integration step to PHYSICS_SUBSTEP_DAYS, required for
        numerical stability on short-period orbits (e.g. Mercury,
        period 88 days) when TIME_SCALE or frame time is large. */
        double step_start = GetTime();
        if (!paused) {
            int substeps = (int)ceilf(dt_days / PHYSICS_SUBSTEP_DAYS);
            if (substeps < 1) substeps = 1;
            float substep_dt = dt_days / (float)substeps;
            for (int s = 0; s < substeps; s++) {
                update_bodies(substep_dt);
            }
            total_simulated_days += dt_days;
        }
        double step_ms = (GetTime() - step_start) * 1000.0;

        BeginDrawing();
        BeginMode2D(camera);
        ClearBackground(BLACK);
        for (int i = 0; i < body_count; i++) {
            DrawCircleV((Vector2) {
                bodies[i].x, 
                bodies[i].y}, 
                bodies[i].radius, 
                bodies[i].color);
        }
        EndMode2D();

        Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
        int hovered_index = -1;
        const float hover_pick_radius = 4.0f;

        for (int i = 0; i < body_count; i++) {
            float dx = bodies[i].x - mouseWorld.x;
            float dy = bodies[i].y - mouseWorld.y;
            float dist = sqrtf(dx * dx + dy * dy);
            float pick_radius = bodies[i].radius > hover_pick_radius ? bodies[i].radius : hover_pick_radius;
            if (dist <= pick_radius) {
                hovered_index = i;
                break;
            }
        }

        if (hovered_index >= 0) {
            Body *b = &bodies[hovered_index];
            char tooltip[160];
            snprintf(tooltip, sizeof(tooltip),
                "id %d\nmass %.2e Msun (%.2f Earth masses)\npos (%.1f, %.1f)\nvel (%.3f, %.3f)",
                b->id, b->mass, b->mass / 3.003e-6f, b->x, b->y, b->vx, b->vy);
            Vector2 mouseScreen = GetMousePosition();
            DrawText(tooltip, (int)mouseScreen.x + 12, (int)mouseScreen.y + 12, 16, RAYWHITE);
        }

        DrawFPS(10, 10);
        snprintf(info_text, sizeof(info_text),
            "%s   Bodies: %d   Physics step: %.2f ms   Day %.1f   %s",
            MODE_NAMES[mode], body_count, step_ms, total_simulated_days, paused ? "(PAUSED)" : "");
        DrawText(info_text, 10, 35, 20, RAYWHITE);
        DrawText("SPACE: pause   R: reset   1/2/3: switch scenario   4: custom scenario   S: save scenario",
            10, 60, 18, GRAY);

        EndDrawing();
    }
    CloseWindow();
    return 0;
}