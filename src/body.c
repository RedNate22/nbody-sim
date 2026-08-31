#include "body.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

Body bodies[MAX_BODIES];
int body_count = 0;

const char *MODE_NAMES[MODE_COUNT] = {
    "Stars Orbiting Black Hole",
    "Planets Orbiting Star",
    "Solar System",
    "Custom Scenario"
};

/* Star colours: cool red/orange through white to hot blue-white.
   Used for the central star and for the "stars orbiting a black hole" mode. */
static const Color STAR_COLORS[] = {
    (Color){255, 180, 120, 255},  // cool orange
    (Color){255, 214, 170, 255},  // warm white
    (Color){255, 244, 214, 255},  // yellow-white, sun-like
    (Color){255, 255, 255, 255},  // white
    (Color){202, 216, 255, 255},  // blue-white, hot
};
#define STAR_COLOR_COUNT (sizeof(STAR_COLORS) / sizeof(STAR_COLORS[0]))

/* Muted rock/gas colours for planets, distinct from the star palette. */
static const Color PLANET_COLORS[] = {
    (Color){160, 160, 160, 255},  // grey rock
    (Color){200, 170, 120, 255},  // dusty tan
    (Color){120, 150, 200, 255},  // icy blue
    (Color){180,  90,  60, 255},  // rusty red
    (Color){210, 180, 140, 255},  // sandy gas giant
};
#define PLANET_COLOR_COUNT (sizeof(PLANET_COLORS) / sizeof(PLANET_COLORS[0]))

#define ORBIT_MASS_MIN 1.0f
#define ORBIT_MASS_MAX 400.0f
#define STAR_ORBIT_MASS_MIN 50.0f
#define STAR_ORBIT_MASS_MAX 600.0f

#define PLANET_DRAW_MIN 3.0f
#define PLANET_DRAW_MAX 8.0f
#define STAR_DRAW_MIN 1.0f
#define STAR_DRAW_MAX 3.5f

/* Fixed, order-preserving (not to-scale) layout for the solar system mode.
   Real distances span ~75x from Mercury to Neptune, too wide a range to
   render meaningfully at this window size without either crowding the
   inner planets into a single blob or pushing the outer ones off-screen,
   so distances here are compressed while keeping the correct ORDER.
   Masses likewise preserve relative scale (gas giants >> terrestrials)
   without matching real mass ratios exactly. */
typedef struct {
    float distance;
    float mass;
    float draw_radius;
    Color color;
} PlanetSpec;

static const PlanetSpec SOLAR_SYSTEM_PLANETS[] = {
    {  80.0f,   5.0f,  4.0f, (Color){169, 169, 169, 255} },  // Mercury
    { 120.0f,  12.0f,  6.0f, (Color){230, 200, 150, 255} },  // Venus
    { 160.0f,  15.0f,  6.5f, (Color){ 70, 130, 180, 255} },  // Earth
    { 210.0f,   8.0f,  5.0f, (Color){193,  68,  14, 255} },  // Mars
    { 320.0f, 800.0f, 16.0f, (Color){210, 180, 140, 255} },  // Jupiter
    { 420.0f, 500.0f, 14.0f, (Color){230, 220, 170, 255} },  // Saturn
    { 520.0f, 200.0f, 10.0f, (Color){175, 238, 238, 255} },  // Uranus
    { 620.0f, 220.0f, 10.0f, (Color){ 60,  90, 200, 255} },  // Neptune
    { 700.0f, 3.0f,   2.5f,  (Color){222, 202, 176, 255} },  // Pluto
};
#define SOLAR_SYSTEM_PLANET_COUNT (sizeof(SOLAR_SYSTEM_PLANETS) / sizeof(SOLAR_SYSTEM_PLANETS[0]))

/* Shared helper: circular orbit speed v = sqrt(G*M/r), derived from
   G*M*m/r^2 = m*v^2/r with the orbiting body's own mass cancelling out. */
static float orbital_speed(float centralMass, float radius) {
    return sqrtf(GRAV_CONST * centralMass / radius);
}

static void init_stars_orbiting_blackhole(float centerX, float centerY) {
    bodies[0] = (Body){ centerX, centerY, 0, 0, 150000.0f, 22.0f, (Color){25, 15, 35, 255} };

    for (int i = 1; i < MAX_BODIES; i++) {
        float angle  = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float radius = 50.0f + ((float)rand() / RAND_MAX) * 400.0f;

        float x = centerX + cosf(angle) * radius;
        float y = centerY + sinf(angle) * radius;

        float speed = orbital_speed(bodies[0].mass, radius);
        float vx = -sinf(angle) * speed;
        float vy =  cosf(angle) * speed;

        float mass = STAR_ORBIT_MASS_MIN + ((float)rand() / RAND_MAX) * (STAR_ORBIT_MASS_MAX - STAR_ORBIT_MASS_MIN);
        float draw_radius = STAR_DRAW_MIN +
            (STAR_DRAW_MAX - STAR_DRAW_MIN) * (mass - STAR_ORBIT_MASS_MIN) / (STAR_ORBIT_MASS_MAX - STAR_ORBIT_MASS_MIN);
        Color color = STAR_COLORS[rand() % STAR_COLOR_COUNT];

        bodies[i] = (Body){ x, y, vx, vy, mass, draw_radius, color };
    }

    body_count = MAX_BODIES;
}

static void init_planets_orbiting_star(float centerX, float centerY) {
    bodies[0] = (Body){ centerX, centerY, 0, 0, 50000.0f, 18.0f, (Color){255, 244, 214, 255} };

    for (int i = 1; i < MAX_BODIES; i++) {
        float angle  = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float radius = 50.0f + ((float)rand() / RAND_MAX) * 400.0f;

        float x = centerX + cosf(angle) * radius;
        float y = centerY + sinf(angle) * radius;

        float speed = orbital_speed(bodies[0].mass, radius);
        float vx = -sinf(angle) * speed;
        float vy =  cosf(angle) * speed;

        float mass = ORBIT_MASS_MIN + ((float)rand() / RAND_MAX) * (ORBIT_MASS_MAX - ORBIT_MASS_MIN);
        float draw_radius = PLANET_DRAW_MIN +
            (PLANET_DRAW_MAX - PLANET_DRAW_MIN) * (mass - ORBIT_MASS_MIN) / (ORBIT_MASS_MAX - ORBIT_MASS_MIN);
        Color color = PLANET_COLORS[rand() % PLANET_COLOR_COUNT];

        bodies[i] = (Body){ x, y, vx, vy, mass, draw_radius, color };
    }

    body_count = MAX_BODIES;
}

static void init_solar_system(float centerX, float centerY) {
    bodies[0] = (Body){ centerX, centerY, 0, 0, 50000.0f, 22.0f, (Color){255, 244, 214, 255} };

    for (int i = 0; i < (int)SOLAR_SYSTEM_PLANET_COUNT; i++) {
        const PlanetSpec *p = &SOLAR_SYSTEM_PLANETS[i];

        float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float x = centerX + cosf(angle) * p->distance;
        float y = centerY + sinf(angle) * p->distance;

        float speed = orbital_speed(bodies[0].mass, p->distance);
        float vx = -sinf(angle) * speed;
        float vy =  cosf(angle) * speed;

        bodies[i + 1] = (Body){ x, y, vx, vy, p->mass, p->draw_radius, p->color };
    }

    body_count = 1 + (int)SOLAR_SYSTEM_PLANET_COUNT;
}

void assign_ids(void) {
    for (int i = 0; i < body_count; i++) {
        bodies[i].id = i;
    }
}

void init_bodies(SimMode mode, float centerX, float centerY) {
    switch (mode) {
        case MODE_STARS_ORBITING_BLACKHOLE:
            init_stars_orbiting_blackhole(centerX, centerY); 
            break;
        case MODE_PLANETS_ORBITING_STAR:
            init_planets_orbiting_star(centerX, centerY);
            break;
        case MODE_SOLAR_SYSTEM:
            init_solar_system(centerX, centerY);
            break;
        default: 
            break;
    }

    assign_ids();
}

void update_bodies(float dt) {
    float ax[MAX_BODIES], ay[MAX_BODIES];

    for (int i = 0; i < body_count; i++) {
        ax[i] = 0.0f;
        ay[i] = 0.0f;

        for (int j = 0; j < body_count; j++) {
            if (i == j) continue;

            float dx = bodies[j].x - bodies[i].x;
            float dy = bodies[j].y - bodies[i].y;
            float dist2 = dx*dx + dy*dy + SOFTENING*SOFTENING;
            float dist  = sqrtf(dist2);

            float accel_scale = GRAV_CONST * bodies[j].mass / (dist2 * dist);
            ax[i] += dx * accel_scale;
            ay[i] += dy * accel_scale;
        }
    }

    for (int i = 0; i < body_count; i++) {
        bodies[i].vx += ax[i] * dt;
        bodies[i].vy += ay[i] * dt;
        bodies[i].x  += bodies[i].vx * dt;
        bodies[i].y  += bodies[i].vy * dt;
    }
}

bool save_bodies(const char *path, const Body *src, int count, float dt, unsigned long steps_run) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    SnapshotHeader header;
    header.magic = SNAPSHOT_MAGIC;
    header.version = SNAPSHOT_VERSION;
    header.body_count = count;
    header.dt = dt;
    header.steps_run = (uint64_t)steps_run;

    bool ok = fwrite(&header, sizeof(header), 1, f) == 1;
    if (ok && count > 0) {
        ok = fwrite(src, sizeof(Body), (size_t)count, f) == (size_t)count;
    }

    fclose(f);
    return ok;
}

bool load_bodies(const char *path, Body *dest, int *count_out, SnapshotHeader *header_out) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    SnapshotHeader header;
    if (fread(&header, sizeof(header), 1, f) != 1) {
        fclose(f);
        return false;
    }
    if (header.magic != SNAPSHOT_MAGIC) {
        fprintf(stderr, "%s: not a valid scenario file\n", path);
        fclose(f);
        return false;
    }
    if (header.version != SNAPSHOT_VERSION) {
        fprintf(stderr, "%s: unsupported scenario version %u\n", path, header.version);
        fclose(f);
        return false;
    }
    if (header.body_count < 0 || header.body_count > MAX_BODIES) {
        fprintf(stderr, "%s: body count %d out of range\n", path, header.body_count);
        fclose(f);
        return false;
    }

    bool ok = true;
    if (header.body_count > 0) {
        ok = fread(dest, sizeof(Body), (size_t)header.body_count, f) == (size_t)header.body_count;
    }
    fclose(f);

    if (ok) {
        *count_out = header.body_count;
        if (header_out) *header_out = header;
    }
    return ok;
}