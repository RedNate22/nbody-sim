#ifndef BODY_H
#define BODY_H

#include "raylib.h"

#define N 1200

#define GRAV_CONST 6.674e-3f  // scaled up from the real G so motion is visible
#define SOFTENING 5.0f  // prevents divide-by-near-zero at close range
#define PLANET_MASS_MIN 1.0f
#define PLANET_MASS_MAX 400.0f

typedef struct {
    float x, y;  // position
    float vx, vy;  // velocity
    float mass;
    float radius;  // rendering only
    Color color;
} Body;

extern Body bodies[N];

void init_bodies(float centerX, float centerY);
void update_bodies(float dt);

#endif