#ifndef BODY_H
#define BODY_H

#include "raylib.h"
#include <stdbool.h>
#include <stdint.h>

#define MAX_BODIES 1200

#define GRAV_CONST 6.674e-3f  // scaled up from the real G so motion is visible
#define SOFTENING 5.0f        // prevents divide-by-near-zero at close range

#define SNAPSHOT_MAGIC 0x4E424F44u // 'NBOD'
#define SNAPSHOT_VERSION 1u
#define SCENARIO_DEFAULT_PATH "scenario.nbs"

typedef struct {
    float x, y;  // position
    float vx, vy;  // velocity
    float mass;
    float radius;  // rendering only
    Color color;
    int id;
} Body;

typedef struct {
  uint32_t magic;
  uint32_t version;
  int32_t body_count;

  // both 0 for a fresh scenario
  float dt;
  uint64_t steps_run;
} SnapshotHeader;

typedef enum {
  MODE_STARS_ORBITING_BLACKHOLE,
  MODE_PLANETS_ORBITING_STAR,
  MODE_SOLAR_SYSTEM,
  MODE_CUSTOM,
  MODE_COUNT // total mode count for cycling
} SimMode;

extern const char *MODE_NAMES[MODE_COUNT];

extern Body bodies[MAX_BODIES];
extern int body_count; // how many entries in bodies[] are active this mode

void init_bodies(SimMode mode, float centerX, float centerY);
void update_bodies(float dt);
void assign_ids(void);

bool save_bodies(const char *path, const Body *src, int count, float dt,
                 unsigned long steps_run);
bool load_bodies(const char *path, Body *dest, int *count_out,
                 SnapshotHeader *header_out);

#endif