#ifndef BODY_H
#define BODY_H

#include "raylib.h"
#include <stdbool.h>
#include <stdint.h>

#define MAX_BODIES 20000
#define DEFAULT_STAR_SCENARIO_BODY_COUNT 1200

/* Units: distance 1 AU = 40 world units, time is in days.
  GRAV_CONST is the real gravitational constant in AU^3/(Msun*day^2)
  (the square of the Gaussian gravitational constant k=0.01720209895),
  rescaled for world units as G_world = G_au * AU_TO_UNITS^3 */
#define AU_TO_UNITS 40.0f
#define GRAV_CONST 18.9384f
#define SOFTENING 2.0f // ~0.05 AU

#define SNAPSHOT_MAGIC 0x4E424F44u // 'NBOD'
#define SNAPSHOT_VERSION 1u
#define SCENARIO_DEFAULT_PATH "scenario.nbs"

typedef struct {
  float x, y;   // position
  float vx, vy; // velocity
  float mass;
  float radius; // rendering only
  Color color;
  int id;
} Body;

typedef struct {
  uint32_t magic;
  uint32_t version;
  int32_t body_count;
  int32_t source_mode;

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

void init_bodies(SimMode mode, float centerX, float centerY,
                 int requested_body_count);
void update_bodies(float dt);
void assign_ids(void);

bool save_bodies(const char *path, const Body *src, int count,
                 SimMode source_mode, float dt, unsigned long steps_run);
bool load_bodies(const char *path, Body *dest, int *count_out,
                 SnapshotHeader *header_out);

#endif