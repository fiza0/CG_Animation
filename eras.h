#ifndef ERAS_H
#define ERAS_H

#include "scene.h"
#include "props.h"

#define RIPPLE_SPEED       120.0f
#define RIPPLE_RAMP         40.0f
#define RIPPLE_BLEND_TIME    1.5f
#define RIPPLE_VISIBLE     240.0f
#define RIPPLE_RING_HEIGHT   2.0f

enum RoadStyle {
    ROAD_DIRT, ROAD_GRAVEL, ROAD_CONCRETE,
    ROAD_ASPHALT_OLD, ROAD_ASPHALT, ROAD_ASPHALT_DARK
};

enum MarkingStyle {
    MARK_NONE, MARK_RUTS, MARK_SLAB_JOINTS, MARK_WHITE_DASH,
    MARK_WHITE_DASH_EDGES, MARK_YELLOW_DASH, MARK_CATS_EYES,
    MARK_MODERN, MARK_MODERN_CYCLE_LANE
};

enum { ROW_NEAR, ROW_FAR };
enum { COL_NEAR, COL_FAR, COL_BACK };

typedef struct EraTheme {
    float sky[3];
    float ground[3];
    float sunColor[3];
    float ambient[3];
    float fogStart, fogEnd;
    float portal[3];

    int   roadStyle;
    int   markingStyle;

    int   rowProps[2];
    int   slotProps[SLOTS_PER_PERIOD][3];

    int   skyline;
} EraTheme;

typedef struct RippleState {
    int   active;
    int   fromEra, toEra;
    float cx, cz;
    float time;
} RippleState;

int             eraThemeCount(void);
const EraTheme *getEraTheme(int index);

const float *roadColor(int roadStyle);
void  lerpColor(float out[3], const float a[3], const float b[3], float t);
void  skylineColor(float out[3], const EraTheme *t);

void  blendThemeGlobals(EraTheme *out, const EraTheme *a, const EraTheme *b, float t);

float smoothstep01(float t);
float rippleRadius(const RippleState *r);
float rippleProgress(const RippleState *r, float distance);
float rippleGlobalFactor(const RippleState *r);

#endif
