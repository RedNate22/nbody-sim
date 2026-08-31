#define _POSIX_C_SOURCE 200809L
#include "raylib.h"
#include "raymath.h"
#include "body.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define FPS 60

typedef struct {
    bool headless;
    bool compare;
    int mode;
    const char *scenario_path;
    const char *out_path;
    const char *compare_a;
    const char *compare_b;
    float dt;
    unsigned long steps;
    float tol;
} CliOptions;

static bool arg_value(const char *arg, const char *flag, const char **out_value) {
    size_t flag_len = strlen(flag);
    if (strncmp(arg, flag, flag_len) != 0) return false;
    if (arg[flag_len] != '=') return false;
    *out_value = arg + flag_len + 1;
    return true;
}

static void parse_cli_options(int argc, char *argv[], CliOptions *opt) {
    opt->headless = false;
    opt->compare = false;
    opt->mode = 0;
    opt->scenario_path = NULL;
    opt->out_path = NULL;
    opt->compare_a = NULL;
    opt->compare_b = NULL;
    opt->dt = 1.0f / 60.0f;
    opt->steps = 3600;
    opt->tol = 1e-3f;

    const char *value;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--headless") == 0) {
            opt->headless = true;
        } else if (arg_value(argv[i], "--mode", &value)) {
            opt->mode = atoi(value);
        } else if (arg_value(argv[i], "--scenario", &value)) {
            opt->scenario_path = value;
        } else if (arg_value(argv[i], "--out", &value)) {
            opt->out_path = value;
        } else if (arg_value(argv[i], "--dt", &value)) {
            opt->dt = (float)atof(value);
        } else if (arg_value(argv[i], "--steps", &value)) {
            opt->steps = strtoul(value, NULL, 10);
        } else if (arg_value(argv[i], "--tol", &value)) {
            opt->tol = (float)atof(value);
        } else if (arg_value(argv[i], "--compare-a", &value)) {
            opt->compare_a = value;
            opt->compare = true;
        } else if (arg_value(argv[i], "--compare-b", &value)) {
            opt->compare_b = value;
            opt->compare = true;
        } else {
            fprintf(stderr, "unrecognized argument: %s\n", argv[i]);
        }
    }
}

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static int run_headless_benchmark(const CliOptions *opt) {
    if (opt->scenario_path) {
        int count;
        SnapshotHeader header;
        if (!load_bodies(opt->scenario_path, bodies, &count, &header)) {
            fprintf(stderr, "failed to load scenario: %s\n", opt->scenario_path);
            return 1;
        }
        body_count = count;
    } else {
        init_bodies((SimMode)opt->mode, SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
    }

    unsigned long progress_interval = opt->steps / 10;
    if (progress_interval == 0) progress_interval = 1;

    double start_time = now_seconds();

    for (unsigned long i = 0; i < opt->steps; i++) {
        update_bodies(opt->dt);

        if ((i + 1) % progress_interval == 0 || i + 1 == opt->steps) {
            double elapsed = now_seconds() - start_time;
            printf("step %lu / %lu (%.0f%%), %.2fs elapsed\n", i + 1, opt->steps,
                100.0 * (double)(i + 1) / (double)opt->steps, elapsed);
        }
    }

    double total_time = now_seconds() - start_time;

    const char *out_path = opt->out_path ? opt->out_path : "output.nbs";
    if (!save_bodies(out_path, bodies, body_count, opt->dt, opt->steps)) {
        fprintf(stderr, "failed to write output: %s\n", out_path);
        return 1;
    }

    double steps_per_sec = total_time > 0.0 ? (double)opt->steps / total_time : 0.0;
    printf("ran %lu steps at dt=%g in %.3fs (%.1f steps/sec), wrote %d bodies to %s\n",
        opt->steps, opt->dt, total_time, steps_per_sec, body_count, out_path);

    return 0;
}

static Body diff_bodies_a[MAX_BODIES];
static Body diff_bodies_b[MAX_BODIES];

static int run_diff(const CliOptions *opt) {
    int count_a, count_b;
    SnapshotHeader header_a, header_b;

    if (!load_bodies(opt->compare_a, diff_bodies_a, &count_a, &header_a)) {
        fprintf(stderr, "failed to load %s\n", opt->compare_a);
        return 1;
    }
    if (!load_bodies(opt->compare_b, diff_bodies_b, &count_b, &header_b)) {
        fprintf(stderr, "failed to load %s\n", opt->compare_b);
        return 1;
    }
    if (count_a != count_b) {
        fprintf(stderr, "body count mismatch: %d vs %d\n", count_a, count_b);
        return 1;
    }
    if (header_a.steps_run != header_b.steps_run || header_a.dt != header_b.dt) {
        fprintf(stderr, "warning: comparing runs with different dt/steps (%g/%llu vs %g/%llu)\n",
            header_a.dt, (unsigned long long)header_a.steps_run,
            header_b.dt, (unsigned long long)header_b.steps_run);
    }

    float max_pos_delta = 0.0f;
    float max_vel_delta = 0.0f;
    bool within_tol = true;

    for (int i = 0; i < count_a; i++) {
        const Body *a = &diff_bodies_a[i];
        const Body *b = &diff_bodies_b[i];

        float dpx = a->x - b->x;
        float dpy = a->y - b->y;
        float pos_delta = sqrtf(dpx * dpx + dpy * dpy);

        float dvx = a->vx - b->vx;
        float dvy = a->vy - b->vy;
        float vel_delta = sqrtf(dvx * dvx + dvy * dvy);

        if (pos_delta > max_pos_delta) max_pos_delta = pos_delta;
        if (vel_delta > max_vel_delta) max_vel_delta = vel_delta;

        if (pos_delta > opt->tol) {
            within_tol = false;
            printf("body id %d: position delta %g exceeds tolerance %g\n", a->id, pos_delta, opt->tol);
        }
    }

    printf("max position delta: %g\n", max_pos_delta);
    printf("max velocity delta: %g\n", max_vel_delta);
    printf("%s\n", within_tol ? "PASS: within tolerance" : "FAIL: exceeded tolerance");

    return within_tol ? 0 : 1;
}

int main(int argc, char *argv[]) {
    SetTraceLogLevel(LOG_WARNING);  // suppress the countless "INFO:" dumps on run

    CliOptions opt;
    parse_cli_options(argc, argv, &opt);

    if (opt.compare) {
        return run_diff(&opt);
    }

    if (opt.headless) {
        return run_headless_benchmark(&opt);
    }

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "n-body-sim");

    int monitor = 0;
    Vector2 monitorPos = GetMonitorPosition(monitor);
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);

    int posX = (int)monitorPos.x + (monitorWidth - SCREEN_WIDTH) / 2;
    int posY = (int)monitorPos.y + (monitorHeight - SCREEN_HEIGHT) / 2;

    SetWindowPosition(posX, posY);
    SetTargetFPS(FPS);

    SimMode mode = MODE_STARS_ORBITING_BLACKHOLE;
    init_bodies(mode, SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);

    bool paused = false;
    char info_text[128];

    Camera2D camera = { 0 };
    camera.target = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    camera.offset = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    const char *scenario_path = opt.scenario_path ? opt.scenario_path : SCENARIO_DEFAULT_PATH;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_R)) {
            init_bodies(mode, SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
        }

        if (IsKeyPressed(KEY_ONE)) {
            mode = MODE_STARS_ORBITING_BLACKHOLE;
            init_bodies(mode, SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
        }
        if (IsKeyPressed(KEY_TWO)) {
            mode = MODE_PLANETS_ORBITING_STAR;
            init_bodies(mode, SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
        }
        if (IsKeyPressed(KEY_THREE)) {
            mode = MODE_SOLAR_SYSTEM;
            init_bodies(mode, SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
        }
        if (IsKeyPressed(KEY_FOUR)) {
            int count;
            SnapshotHeader header;
            if (load_bodies(scenario_path, bodies, &count, &header)) {
                mode = MODE_CUSTOM;
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

        /* keep animation consistent regardless of frame rate */
        float dt = GetFrameTime();  

        /* Physics step number for measuring computational cost (not motion speed) */ 
        double step_start = GetTime();
        if (!paused) update_bodies(dt);
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
                "id %d\nmass %.1f\npos (%.1f, %.1f)\nvel (%.2f, %.2f)",
                b->id, b->mass, b->x, b->y, b->vx, b->vy);
            Vector2 mouseScreen = GetMousePosition();
            DrawText(tooltip, (int)mouseScreen.x + 12, (int)mouseScreen.y + 12, 16, RAYWHITE);
        }

        DrawFPS(10, 10);
        snprintf(info_text, sizeof(info_text),
            "%s   Bodies: %d   Physics step: %.2f ms   %s",
            MODE_NAMES[mode], body_count, step_ms, paused ? "(PAUSED)" : "");
        DrawText(info_text, 10, 35, 20, RAYWHITE);
        DrawText("SPACE: pause   R: reset   1/2/3: switch scenario   4: custom scenario   S: save scenario",
            10, SCREEN_HEIGHT - 30, 18, GRAY);

        EndDrawing();
    }
    CloseWindow();
    return 0;
}