#ifndef ERAS_H
#define ERAS_H

#include "scene.h"
#include "props.h"

/* ------------------------------------------------------------------ */
/* Time ripple tuning                                                  */
/* ------------------------------------------------------------------ */
#define RIPPLE_SPEED       120.0f  /* metres per second the wavefront travels  */
#define RIPPLE_RAMP         40.0f  /* metres behind the front until fully new  */
#define RIPPLE_BLEND_TIME    1.5f  /* seconds for sky, fog, light and skyline  */
#define RIPPLE_VISIBLE     240.0f  /* fog end (180) plus the farthest camera (60) */
#define RIPPLE_RING_HEIGHT   2.0f

/* ------------------------------------------------------------------ */
/* Era themes                                                          */
/* ------------------------------------------------------------------ */
enum RoadStyle {
    ROAD_DIRT, ROAD_GRAVEL, ROAD_CONCRETE,
    ROAD_ASPHALT_OLD, ROAD_ASPHALT, ROAD_ASPHALT_DARK
};

enum MarkingStyle {
    MARK_NONE, MARK_RUTS, MARK_SLAB_JOINTS, MARK_WHITE_DASH,
    MARK_WHITE_DASH_EDGES, MARK_YELLOW_DASH, MARK_CATS_EYES,
    MARK_MODERN, MARK_MODERN_CYCLE_LANE
};

enum { ROW_NEAR, ROW_FAR };             /* indices into rowProps  */
enum { COL_NEAR, COL_FAR, COL_BACK };   /* indices into slotProps */

typedef struct EraTheme {
    /* Global look, blended over the whole screen during a ripple. */
    float sky[3];          /* clear colour and fog colour            */
    float ground[3];
    float sunColor[3];
    float ambient[3];
    float fogStart, fogEnd;
    float portal[3];       /* tunnel glow and ripple ring on arrival */

    /* Road, changed strip by strip. */
    int   roadStyle;       /* surface colour (and gravel speckles)   */
    int   markingStyle;

    /* Roadside, changed object by object. rowProps repeat in every
     * slot and join up (fences, wires, walls, barriers); slotProps
     * place one object per slot and column. */
    int   rowProps[2];                         /* near, far          */
    int   slotProps[SLOTS_PER_PERIOD][3];      /* near, far, back    */

    int   skyline;
} EraTheme;

/* The ripple is pure background state: where and when the model swap
 * happened. It knows nothing about which car effect caused it. */
typedef struct RippleState {
    int   active;
    int   fromEra, toEra;
    float cx, cz;          /* world-space centre                     */
    float time;            /* seconds since the swap                 */
} RippleState;

int             eraThemeCount(void);
const EraTheme *getEraTheme(int index);

const float *roadColor(int roadStyle);
void  lerpColor(float out[3], const float a[3], const float b[3], float t);
void  skylineColor(float out[3], const EraTheme *t);

/* Copies b, then blends the global values (sky, ground, sun, ambient,
 * fog, portal) from a towards b by t in 0..1. */
void  blendThemeGlobals(EraTheme *out, const EraTheme *a, const EraTheme *b, float t);

float smoothstep01(float t);
float rippleRadius(const RippleState *r);
float rippleProgress(const RippleState *r, float distance);  /* 0..1, linear */
float rippleGlobalFactor(const RippleState *r);              /* 0..1, eased  */

#endif
