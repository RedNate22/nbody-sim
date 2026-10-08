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
 * @param total_body_count Total number of bodies to generate, including
 *                          the black hole. Clamped to [2, MAX_BODIES];
 *                          values <= 0 default to MAX_BODIES.
 *
 * Fills bodies[0] with the black hole and every remaining slot up to
 * total_body_count with a randomly placed star on a circular orbit
 * around it. Sets body_count to total_body_count.
 */
static void init_stars_orbiting_blackhole(float centerX, float centerY, int total_body_count) {
    if (total_body_count <= 0) total_body_count = DEFAULT_STAR_SCENARIO_BODY_COUNT;
    if (total_body_count > MAX_BODIES) total_body_count = MAX_BODIES;
    if (total_body_count < 2) total_body_count = 2;

    const float central_mass = 3000.0f;
    bodies[0] = (Body){ 0.0f, 0.0f, 0, 0, central_mass, 22.0f, (Color){25, 15, 35, 255} };

    const int star_count = total_body_count - 1;

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
 * @param requested_body_count Total body count to generate, for scenarios
 *                              whose size isn't fixed (currently only
 *                              MODE_STARS_ORBITING_BLACKHOLE). Values
 *                              <= 0 use that scenario's default. Ignored
 *                              by scenarios with a fixed body count.
 */
void init_bodies(SimMode mode, float centerX, float centerY, int requested_body_count) {
    switch (mode) {
        case MODE_STARS_ORBITING_BLACKHOLE:
            init_stars_orbiting_blackhole(centerX, centerY, requested_body_count);
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

/* Barnes-Hut quadtree. Space is split into a square that holds every body,
   then each square is split into four smaller squares (quadrants) until
   every square holds at most one body. Each square stores the total mass
   inside it and the centre of that mass. When summing forces on a body,
   a square that is far away compared to its width is treated as one lump
   at its centre of mass instead of visiting every body inside it. */

#define THETA 0.5f
#define MAX_TREE_NODES (8 * MAX_BODIES)
#define MAX_TREE_DEPTH 32

typedef struct {
    float cx, cy;   // centre of the square
    float half;     // half the width of the square
    float mass;     // total mass of every body inside square
    float mx, my;   // centre of mass, a mass weighted sum until tree is built
    int count;      // no. bodies inside square
    int body;       // body index for a single body leaf, otherwise -1
    int child[4];   // node indices, -1 if absent
} TreeNode;

/* Nodes live in one fixed array that is reused every step, children are
   referenced by array index, so building the tree needs no malloc. */
static TreeNode tree[MAX_TREE_NODES];
static int tree_count;

// creates an empty node for a square and returns its index in tree[]
static int new_node(float cx, float cy, float half) {
    if (tree_count >= MAX_TREE_NODES) {
        fprintf(stderr, "quadtree node array is full (%d nodes)\n", MAX_TREE_NODES);
        exit(1);
    }

    TreeNode *t = &tree[tree_count];
    t->cx = cx;
    t->cy = cy;
    t->half = half;
    t->mass = 0.0f;
    t->mx = 0.0f;
    t->my = 0.0f;
    t->count = 0;
    t->body = -1;
    for (int q = 0; q < 4; q++) {
        t->child[q] = -1;
    }
    return tree_count++;
}

// bit 0 is right of the centre, bit 1 is above it
static int quadrant(const TreeNode *t, const Body *b) {
    return (b->x >= t->cx ? 1 : 0) + (b->y >= t->cy ? 2 : 0);
}

// returns the index of quadrant q of node n, creating it on first use
static int get_child(int n, int q) {
    if (tree[n].child[q] < 0) {
        float h = tree[n].half * 0.5f;
        float cx = tree[n].cx + ((q & 1) ? h : -h);
        float cy = tree[n].cy + ((q & 2) ? h : -h);
        tree[n].child[q] = new_node(cx, cy, h);
    }
    return tree[n].child[q];
}

// places body b into the tree, splitting squares until it has its own
static void tree_insert(int b) {
    float m = bodies[b].mass;
    int n = 0;
    int depth = 0;

    /* Walk down from the root square, adding the body's mass to every
       square passed through, until the body reaches an empty square. */
    for (;;) {
        TreeNode *t = &tree[n];
        t->count++;
        t->mass += m;
        t->mx += bodies[b].x * m;
        t->my += bodies[b].y * m;

        // square was empty, the body is stored here as a leaf
        if (t->count == 1) {
            t->body = b;
            return;
        }

        // depth guard, bodies at (almost) the same position stay lumped in one node
        if (depth >= MAX_TREE_DEPTH) {
            t->body = -1;
            return;
        }

        /* Square already held exactly one body, so that body is moved
           down into the matching quadrant to make room for the new one. */
        if (t->body >= 0) {
            int old = t->body;
            float old_mass = bodies[old].mass;
            TreeNode *c = &tree[get_child(n, quadrant(t, &bodies[old]))];
            c->count = 1;
            c->mass = old_mass;
            c->mx = bodies[old].x * old_mass;
            c->my = bodies[old].y * old_mass;
            c->body = old;
            t->body = -1;
        }

        n = get_child(n, quadrant(t, &bodies[b]));
        depth++;
    }
}

// rebuilds the whole tree from the current body positions
static void build_tree(void) {
    tree_count = 0;
    if (body_count <= 0) return;

    // root square is the smallest square around every body
    float min_x = bodies[0].x, max_x = bodies[0].x;
    float min_y = bodies[0].y, max_y = bodies[0].y;
    for (int i = 1; i < body_count; i++) {
        if (bodies[i].x < min_x) min_x = bodies[i].x;
        if (bodies[i].x > max_x) max_x = bodies[i].x;
        if (bodies[i].y < min_y) min_y = bodies[i].y;
        if (bodies[i].y > max_y) max_y = bodies[i].y;
    }
    float width = max_x - min_x;
    if (max_y - min_y > width) width = max_y - min_y;

    new_node((min_x + max_x) * 0.5f, (min_y + max_y) * 0.5f, width * 0.5f);
    for (int i = 0; i < body_count; i++) {
        tree_insert(i);
    }

    // convert the weighted sums to centres of mass
    for (int n = 0; n < tree_count; n++) {
        TreeNode *t = &tree[n];
        if (t->body >= 0) {
            t->mx = bodies[t->body].x;
            t->my = bodies[t->body].y;
        } else if (t->mass > 0.0f) {
            t->mx /= t->mass;
            t->my /= t->mass;
        }
    }
}

// sums the acceleration on body i by walking the tree from the root
static void tree_accel(int i, float *ax_out, float *ay_out) {
    float ax = 0.0f, ay = 0.0f;
    
    /* Squares still to visit, starting with the root. Each visited square
       either adds its force or is replaced by its children. Every opened
       square adds at most four children, so the stack never grows past
       four per tree level. */
    int stack[4 * (MAX_TREE_DEPTH + 1)];
    int sp = 0;
    stack[sp++] = 0;

    while (sp > 0) {
        const TreeNode *t = &tree[stack[--sp]];
        float dx = t->mx - bodies[i].x;
        float dy = t->my - bodies[i].y;
        float dist2 = dx*dx + dy*dy;
        bool is_leaf = t->child[0] < 0 && t->child[1] < 0 &&
                       t->child[2] < 0 && t->child[3] < 0;

        if (is_leaf) {
            // a body exerts no force on itself
            if (t->body == i) continue;
        } else {
            /* Square is too close for its contents to be treated as one
               lump (width / distance is not below THETA), so its children
               are visited instead. */
            float width = 2.0f * t->half;
            if (width * width >= THETA * THETA * dist2) {
                for (int q = 0; q < 4; q++) {
                    if (t->child[q] >= 0) stack[sp++] = t->child[q];
                }
                continue;
            }
        }

        dist2 += SOFTENING*SOFTENING;
        float dist = sqrtf(dist2);
        float accel_scale = GRAV_CONST * t->mass / (dist2 * dist);
        ax += dx * accel_scale;
        ay += dy * accel_scale;
    }

    *ax_out = ax;
    *ay_out = ay;
}

/**
 * Advances the simulation by one timestep.
 *
 * @param dt Size of the timestep to integrate over.
 *
 * Computes the combined gravitational acceleration on every body using a
 * Barnes-Hut quadtree, where distant groups of bodies are summed as a
 * single lump at their centre of mass (see THETA) and nearby bodies are
 * summed individually, then integrates velocity and position forward
 * using semi-implicit (symplectic) Euler integration.
 */
void update_bodies(float dt) {
    float ax[MAX_BODIES], ay[MAX_BODIES];

    build_tree();
    for (int i = 0; i < body_count; i++) {
        tree_accel(i, &ax[i], &ay[i]);
    }

    /* Semi-implicit (symplectic) Euler:
       v(t+dt) = v(t) + a(t)*dt
       x(t+dt) = x(t) + v(t+dt)*dt
       Position advanced via just-updated velocity v(t+dt) 
       rather than v(t) */
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