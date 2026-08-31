#ifndef BODY_H
#define BODY_H

#include "raylib.h"

#define MAX_BODIES 1200

#define GRAV_CONST 6.674e-3f  // scaled up from the real G so motion is visible
#define SOFTENING 5.0f        // prevents divide-by-near-zero at close range

typedef struct {
    float x, y;  // position
    float vx, vy;  // velocity
    float mass;
    float radius;  // rendering only
    Color color;
} Body;

typedef enum {
  MODE_STARS_ORBITING_BLACKHOLE,
  MODE_PLANETS_ORBITING_STAR,
  MODE_SOLAR_SYSTEM,
  MODE_COUNT // total mode count for cycling
} SimMode;

extern const char *MODE_NAMES[MODE_COUNT];

extern Body bodies[MAX_BODIES];
extern int body_count; // how many entries in bodies[] are active this mode

void init_bodies(SimMode mode, float centerX, float centerY);
void update_bodies(float dt);

#endif