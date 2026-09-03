#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "body.h"
#include "cli.h"
#include "benchmark.h"

/**
 * Reads the current monotonic clock time.
 *
 * @return Elapsed time in seconds since an unspecified fixed point,
 *         suitable only for measuring differences between two calls.
 */
static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/**
 * Runs the simulation with no window, for scripted or automated testing.
 *
 * @param opt Parsed options. If opt->scenario_path is set, bodies are
 *            loaded from that file, otherwise a scenario is generated
 *            using opt->mode. The simulation is then advanced opt->steps
 *            times at fixed dt opt->dt, with progress printed every 10%
 *            of the run, before the resulting bodies are written to
 *            opt->out_path. The output file records opt->mode as its
 *            source mode regardless of whether a scenario was generated
 *            or loaded from opt->scenario_path.
 * @return 0 on success, 1 if the scenario could not be loaded or the
 *         result could not be written.
 */
int run_headless_benchmark(const CliOptions *opt, int screen_width, int screen_height) {
    if (opt->scenario_path) {
        int count;
        SnapshotHeader header;
        if (!load_bodies(opt->scenario_path, bodies, &count, &header)) {
            fprintf(stderr, "failed to load scenario: %s\n", opt->scenario_path);
            return 1;
        }
        body_count = count;
    } else {
        init_bodies((SimMode)opt->mode, screen_width / 2.0f, screen_height / 2.0f, opt->bodies);
    }

    unsigned long progress_interval = opt->steps / 10;
    if (progress_interval == 0) progress_interval = 1;

    double start_time = now_seconds();

    for (unsigned long i = 0; i < opt->steps; i++) {
        update_bodies(opt->dt);

        if ((i + 1) % progress_interval == 0 || i + 1 == opt->steps) {
            double elapsed = now_seconds() - start_time;
            printf("step %lu / %lu (%.0f%%), %d bodies, %.2fs elapsed\n", i + 1, opt->steps,
                100.0 * (double)(i + 1) / (double)opt->steps, body_count, elapsed);
        }
    }

    double total_time = now_seconds() - start_time;

    const char *out_path = opt->out_path ? opt->out_path : "scenario.nbs";
    if (!save_bodies(out_path, bodies, body_count, (SimMode)opt->mode, opt->dt, opt->steps)) {
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

/**
 * Compares two saved result files body by body, matched by id.
 *
 * @param opt Parsed options. opt->compare_a and opt->compare_b name the
 *            two files to load, opt->tol is the largest position
 *            difference allowed before a body is reported as failing.
 * @return 0 if both files loaded, had matching body counts, every id in
 *         A was found in B, and every matched body's position difference
 *         was within opt->tol. 1 if either file failed to load, the body
 *         counts differ, a body id was out of range or unmatched, or the
 *         tolerance was exceeded.
 */
int run_diff(const CliOptions *opt) {
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

    int id_to_index_b[MAX_BODIES];
    for (int i = 0; i < MAX_BODIES; i++) id_to_index_b[i] = -1;
    for (int j = 0; j < count_b; j++) {
        int id = diff_bodies_b[j].id;
        if (id < 0 || id >= MAX_BODIES) {
            fprintf(stderr, "%s: body %d has out-of-range id %d\n", opt->compare_b, j, id);
            return 1;
        }
        id_to_index_b[id] = j;
    }

    float max_pos_delta = 0.0f;
    float max_vel_delta = 0.0f;
    bool within_tol = true;

    for (int i = 0; i < count_a; i++) {
        const Body *a = &diff_bodies_a[i];

        int id = a->id;
        if (id < 0 || id >= MAX_BODIES || id_to_index_b[id] < 0) {
            fprintf(stderr, "%s: body id %d not found in %s\n", opt->compare_a, id, opt->compare_b);
            within_tol = false;
            continue;
        }
        const Body *b = &diff_bodies_b[id_to_index_b[id]];

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
