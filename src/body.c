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

static const Color STAR_COLORS[] = {
    (Color){255, 180, 120, 255},  // cool orange
    (Color){255, 214, 170, 255},  // warm white
    (Color){255, 244, 214, 255},  // yellow-white, sun-like
    (Color){255, 255, 255, 255},  // white
    (Color){202, 216, 255, 255},  // blue-white, hot
};
#define STAR_COLOR_COUNT (sizeof(STAR_COLORS) / sizeof(STAR_COLORS[0]))

static const Color PLANET_COLORS[] = {
    (Color){160, 160, 160, 255},  // grey rock
    (Color){200, 170, 120, 255},  // dusty tan
    (Color){120, 150, 200, 255},  // icy blue
    (Color){180,  90,  60, 255},  // rusty red
    (Color){210, 180, 140, 255},  // sandy gas giant
};
#define PLANET_COLOR_COUNT (sizeof(PLANET_COLORS) / sizeof(PLANET_COLORS[0]))

#define ORBIT_MASS_MIN 1.0e-7f   // Mercury-equivalent
#define ORBIT_MASS_MAX 1.0e-3f   // Jupiter-equivalent

#define DISC_MASS_FRACTION 0.10f   // total disc mass, as a fraction of the central mass
#define STAR_MASS_RAND_MIN 0.5f    // random spread around the mean star mass
#define STAR_MASS_RAND_MAX 1.5f

#define STAR_DRAW_MIN 1.0f
#define STAR_DRAW_MAX 3.5f

#define PLANET_DRAW_MIN 3.0f
#define PLANET_DRAW_MAX 8.0f

#define PLANETS_ORBITING_STAR_COUNT 15

typedef struct {
    float distance;
    float mass;
    float draw_radius;
    Color color;
} PlanetSpec;

static const PlanetSpec SOLAR_SYSTEM_PLANETS[] = {
    /* distance: world units, 40 units = 1 AU
    mass: real solar-mass fractions (Sol = 1.0) */
    {  15.6f, 1.660e-7f,  4.0f, (Color){169, 169, 169, 255} },  // Mercury  (0.39 AU, 0.055 Earth masses)
    {  28.8f, 2.447e-6f,  6.0f, (Color){230, 200, 150, 255} },  // Venus    (0.72 AU, 0.815 Earth masses)
    {  40.0f, 3.003e-6f,  6.5f, (Color){ 70, 130, 180, 255} },  // Earth    (1.00 AU, 1.000 Earth masses)
    {  60.8f, 3.213e-7f,  5.0f, (Color){193,  68,  14, 255} },  // Mars     (1.52 AU, 0.107 Earth masses)
    { 208.0f, 9.543e-4f, 16.0f, (Color){210, 180, 140, 255} },  // Jupiter  (5.20 AU, 317.8 Earth masses)
    { 383.2f, 2.857e-4f, 14.0f, (Color){230, 220, 170, 255} },  // Saturn   (9.58 AU, 95.2 Earth masses)
    { 768.0f, 4.365e-5f, 10.0f, (Color){175, 238, 238, 255} },  // Uranus   (19.2 AU, 14.5 Earth masses)
    {1204.0f, 5.150e-5f, 10.0f, (Color){ 60,  90, 200, 255} },  // Neptune  (30.1 AU, 17.1 Earth masses)
    {1580.0f, 6.552e-9f,  2.5f, (Color){222, 202, 176, 255} },  // Pluto    (39.5 AU, 0.0022 Earth masses)
};
#define SOLAR_SYSTEM_PLANET_COUNT (sizeof(SOLAR_SYSTEM_PLANETS) / sizeof(SOLAR_SYSTEM_PLANETS[0]))

static float orbital_speed(float centralMass, float radius) {
    return sqrtf(GRAV_CONST * centralMass / radius);
}

static int compare_dist2(const void *a, const void *b) {
    const Body *ba = (const Body *)a;
    const Body *bb = (const Body *)b;
    float da = ba->x * ba->x + ba->y * ba->y;
    float db = bb->x * bb->x + bb->y * bb->y;
    return (da > db) - (da < db);
}

/**
 * Generates the stars orbiting a black hole scenario.
 *
 * @param centerX X coordinate of the central black hole.
 * @param centerY Y coordinate of the central black hole.
 *
 * Fills bodies[0] with the black hole and every remaining slot up to
 * MAX_BODIES with a randomly placed star on a circular orbit around it.
 * Sets body_count to MAX_BODIES.
 */
static void init_stars_orbiting_blackhole(float centerX, float centerY) {
    const float central_mass = 3000.0f;
    bodies[0] = (Body){ 0.0f, 0.0f, 0, 0, central_mass, 22.0f, (Color){25, 15, 35, 255} };

    const int star_count = MAX_BODIES - 1;

    /* Mean star mass is derived from body count, not fixed, so total disc
       mass stays a constant fraction of the central mass regardless of N.
       Without this, more bodies means a proportionally heavier, less
       stable disc instead of a smoother, more stable one. */
    const float mean_star_mass = (DISC_MASS_FRACTION * central_mass) / (float)star_count;

    const float inner_radius = 30.0f;
    const float outer_radius = sqrtf((float)star_count) * 30.0f;
    const float t2 = (inner_radius * inner_radius) / (outer_radius * outer_radius);

    for (int i = 1; i <= star_count; i++) {
        float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;

        float r_frac = ((float)rand() / RAND_MAX) * (1.0f - t2) + t2;
        float radius = outer_radius * sqrtf(r_frac);

        float x = cosf(angle) * radius;
        float y = sinf(angle) * radius;

        float mass = mean_star_mass * (STAR_MASS_RAND_MIN +
            ((float)rand() / RAND_MAX) * (STAR_MASS_RAND_MAX - STAR_MASS_RAND_MIN));
        float draw_radius = STAR_DRAW_MIN +
            (STAR_DRAW_MAX - STAR_DRAW_MIN) * (mass / mean_star_mass - STAR_MASS_RAND_MIN) /
            (STAR_MASS_RAND_MAX - STAR_MASS_RAND_MIN);
        Color color = STAR_COLORS[rand() % STAR_COLOR_COUNT];

        bodies[i] = (Body){ x, y, 0.0f, 0.0f, mass, draw_radius, color };
    }

    qsort(&bodies[1], star_count, sizeof(Body), compare_dist2);

    float enclosed_mass = central_mass;
    for (int i = 1; i <= star_count; i++) {
        float r = sqrtf(bodies[i].x * bodies[i].x + bodies[i].y * bodies[i].y);
        float speed = orbital_speed(enclosed_mass, r);
        bodies[i].vx = -(bodies[i].y / r) * speed;
        bodies[i].vy =  (bodies[i].x / r) * speed;
        enclosed_mass += bodies[i].mass;
    }

    for (int i = 0; i <= star_count; i++) {
        bodies[i].x += centerX;
        bodies[i].y += centerY;
    }

    body_count = 1 + star_count;
}

/**
 * Generates the planets orbiting a star scenario.
 *
 * @param centerX X coordinate of the central star.
 * @param centerY Y coordinate of the central star.
 *
 * Fills bodies[0] with the star and every remaining slot up to
 * MAX_BODIES with a randomly placed planet on a circular orbit around
 * it. Sets body_count to MAX_BODIES.
 */
static void init_planets_orbiting_star(float centerX, float centerY) {
    bodies[0] = (Body){ centerX, centerY, 0, 0, 1.0f, 18.0f, (Color){255, 244, 214, 255} };

    for (int i = 1; i <= PLANETS_ORBITING_STAR_COUNT; i++) {
        float angle  = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float radius = 50.0f + ((float)rand() / RAND_MAX) * 850.0f;

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

    body_count = 1 + PLANETS_ORBITING_STAR_COUNT;
}

/**
 * Generates the solar system scenario.
 *
 * @param centerX X coordinate of the central star.
 * @param centerY Y coordinate of the central star.
 *
 * Fills bodies[0] with the star and one slot per entry in
 * SOLAR_SYSTEM_PLANETS, each placed at a random angle at its specified
 * (compressed) distance with a circular orbit speed. Sets body_count to
 * one plus SOLAR_SYSTEM_PLANET_COUNT.
 */
static void init_solar_system(float centerX, float centerY) {
    bodies[0] = (Body){ centerX, centerY, 0, 0, 1.0f, 8.0f, (Color){255, 244, 214, 255} };

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

/**
 * Assigns a stable id to every active body.
 *
 * Sets bodies[i].id to i for every i in [0, body_count). Called after
 * generating a new scenario so every body can be identified later, for
 * example by hover inspection or when matching bodies between two saved
 * files.
 */
void assign_ids(void) {
    for (int i = 0; i < body_count; i++) {
        bodies[i].id = i;
    }
}

/**
 * Populates bodies[] and body_count for the given scenario.
 *
 * @param mode Which built in scenario to generate. MODE_CUSTOM is not
 *             handled here, loading a scenario from disk is done
 *             separately with load_bodies.
 * @param centerX X coordinate to center the generated scenario on.
 * @param centerY Y coordinate to center the generated scenario on.
 */
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

/**
 * Advances the simulation by one timestep.
 *
 * @param dt Size of the timestep to integrate over.
 *
 * Computes the combined gravitational acceleration on every body from
 * every other body by direct pairwise summation, then integrates
 * velocity and position forward using explicit Euler integration.
 */
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

/**
 * Writes an array of bodies to a binary scenario file.
 *
 * @param path File path to write to.
 * @param src Array of bodies to write.
 * @param count Number of bodies in src.
 * @param source_mode Which built-in scenario these bodies were generated
 *                     from, recorded in the file header so a later load
 *                     can restore mode-specific behavior (e.g. time scale).
 * @param dt Timestep the bodies were produced with, recorded in the file
 *           header for reference only.
 * @param steps_run Number of steps already applied to these bodies,
 *                  recorded in the file header for reference only.
 * @return true if the file was written successfully, false if it could
 *         not be opened or the write did not complete.
 */
bool save_bodies(const char *path, const Body *src, int count, SimMode source_mode, float dt, unsigned long steps_run) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    SnapshotHeader header;
    header.magic = SNAPSHOT_MAGIC;
    header.version = SNAPSHOT_VERSION;
    header.body_count = count;
    header.source_mode = (int32_t)source_mode;
    header.dt = dt;
    header.steps_run = (uint64_t)steps_run;

    bool ok = fwrite(&header, sizeof(header), 1, f) == 1;
    if (ok && count > 0) {
        ok = fwrite(src, sizeof(Body), (size_t)count, f) == (size_t)count;
    }

    fclose(f);
    return ok;
}

/**
 * Reads an array of bodies from a binary scenario file.
 *
 * @param path File path to read from.
 * @param dest Array to read bodies into. Must be large enough to hold
 *             the file's body count, up to MAX_BODIES.
 * @param count_out Set to the number of bodies read on success.
 * @param header_out If not NULL, receives the full snapshot header on
 *                    success, including the recorded source mode.
 * @return true on success. false if the file is missing, is not a valid
 *         scenario file, has an unsupported version, has a body count
 *         out of range, or has an invalid source mode, in which case
 *         dest is left untouched.
 */
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
    if (header.source_mode < 0 || header.source_mode >= MODE_COUNT) {
        fprintf(stderr, "%s: invalid source mode %d\n", path, header.source_mode);
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