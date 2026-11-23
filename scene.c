#include <stddef.h>   /* defines NULL */
#include <math.h>     /* floorf, fabsf, hypotf */
#include <string.h>
#include "scene.h"
#include "eras.h"
#include "props.h"

/* The scene is drawn as 12 m strips from snap - 300 to snap + 300. */
#define STRIPS_EACH_SIDE ((int)(ROAD_HALF_LENGTH / SLOT_LENGTH))
#define RING_SLICES      96

/* A quadric object is GLU's helper for drawing cylinders and discs. */
static GLUquadric *quad = NULL;

/* In model inspection mode the near-side props are hidden, so they
 * never stand between the camera and the car. */
static int hideNearSide = 0;

void sceneHideNearSide(int hide) { hideNearSide = hide; }

/* ------------------------------------------------------------------ */
/* Initialisation: OpenGL state that never changes                    */
/* ------------------------------------------------------------------ */
void initScene(void)
{
    glEnable(GL_DEPTH_TEST);   /* nearer surfaces hide farther ones          */
    glEnable(GL_NORMALIZE);    /* keep normals unit-length after glScalef    */
    glShadeModel(GL_SMOOTH);

    /* Lighting on, with glColor driving the material colour. */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    /* Specular highlights: compute them from the real eye position (a
     * local viewer), so they slide over curved panels as the camera
     * moves, and add them on top of the lit, coloured surface as a
     * separate step (separate specular), so highlights stay white even
     * on dark or red paint. */
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
#ifdef GL_LIGHT_MODEL_COLOR_CONTROL
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
#endif

    /* Linear fog: objects blend into the sky with distance. The colour
     * and distances come from the era theme every frame. */
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);

    quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);

    initProps();               /* compile every prop into a display list */
}

/* ------------------------------------------------------------------ */
/* Per-frame global look                                               */
/* ------------------------------------------------------------------ */

/* The sky colour is shared by the clear colour and the fog, so distant
 * objects fade into the horizon instead of ending abruptly. */
void applyThemeGlobals(const EraTheme *t)
{
    GLfloat fogColor[4] = { t->sky[0], t->sky[1], t->sky[2], 1.0f };
    glClearColor(t->sky[0], t->sky[1], t->sky[2], 1.0f);
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_START, t->fogStart);
    glFogf(GL_FOG_END,   t->fogEnd);
}

/* A sun (directional light) plus soft ambient fill. Their colours give
 * each era its mood: warm and dim for old photographs, cool and white
 * for today. */
void setupLighting(const EraTheme *t)
{
    /* w = 0 makes this a directional light: parallel rays, like the sun.
     * It must be set AFTER the camera transform so the sun stays fixed
     * in the world instead of following the camera. */
    GLfloat sunDir[]   = { 0.4f, 1.0f, 0.3f, 0.0f };
    GLfloat sunColor[] = { t->sunColor[0], t->sunColor[1], t->sunColor[2], 1.0f };
    GLfloat ambient[]  = { t->ambient[0],  t->ambient[1],  t->ambient[2],  1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, sunDir);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  sunColor);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
    /* The sun's highlight has the sun's colour (warm in the early eras,
     * cool white today). Only the cars have specular materials. */
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunColor);
}

/* ------------------------------------------------------------------ */
/* Helper: flat horizontal rectangle at height y, facing up            */
/* ------------------------------------------------------------------ */
static void flatRect(float x0, float z0, float x1, float z1, float y)
{
    glNormal3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex3f(x0, y, z0);
        glVertex3f(x0, y, z1);
        glVertex3f(x1, y, z1);
        glVertex3f(x1, y, z0);
    glEnd();
}

static void scaledColor(const float c[3], float k)
{
    float r = c[0] * k, g = c[1] * k, b = c[2] * k;
    glColor3f(r > 1.0f ? 1.0f : r, g > 1.0f ? 1.0f : g, b > 1.0f ? 1.0f : b);
}

/* ------------------------------------------------------------------ */
/* Road markings for one 12 m strip starting at x0                     */
/*                                                                     */
/* Each layer is raised slightly above the one below it. Two surfaces  */
/* at exactly the same height "fight" in the depth buffer and flicker  */
/* (z-fighting), so: ground y=0, road y=0.01, cycle lane and speckles  */
/* y=0.015, paint y=0.02, cat's eyes y=0.03.                           */
/* ------------------------------------------------------------------ */

/* 3 m dash, 3 m gap: two dashes per strip. 6 m divides both 12 and 96,
 * so the dashes line up across strips and across scene snaps. */
static void centreDashes(float x0)
{
    flatRect(x0,        -0.08f, x0 + 3.0f, 0.08f, 0.02f);
    flatRect(x0 + 6.0f, -0.08f, x0 + 9.0f, 0.08f, 0.02f);
}

static void edgeLines(float x0)
{
    float x1 = x0 + SLOT_LENGTH;
    flatRect(x0, -ROAD_HALF_WIDTH + 0.2f,  x1, -ROAD_HALF_WIDTH + 0.35f, 0.02f);
    flatRect(x0,  ROAD_HALF_WIDTH - 0.35f, x1,  ROAD_HALF_WIDTH - 0.2f,  0.02f);
}

/* Gravel: small lighter and darker flecks. The pattern depends only on
 * the slot number, so it repeats exactly every scene period. */
static void gravelSpeckles(float x0, int slot, const float road[3])
{
    unsigned s = 2654435761u * (unsigned)(slot + 1);
    for (int i = 0; i < 40; i++) {
        s = s * 1664525u + 1013904223u;  float rx = (float)(s >> 8) / 16777216.0f;
        s = s * 1664525u + 1013904223u;  float rz = (float)(s >> 8) / 16777216.0f;
        float x = x0 + rx * (SLOT_LENGTH - 0.2f);
        float z = -ROAD_HALF_WIDTH + 0.2f + rz * (2.0f * ROAD_HALF_WIDTH - 0.4f);
        float size = 0.08f + 0.10f * rx * rz;
        scaledColor(road, (i & 1) ? 0.75f : 1.2f);
        flatRect(x, z, x + size, z + size, 0.015f);
    }
}

/* Cat's eyes: small reflectors drawn unlit so they seem to glow. */
static void catsEyes(float x0)
{
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.95f, 0.75f);
    for (int i = 0; i < 2; i++) {
        float x = x0 + 4.5f + 6.0f * i;      /* in the gaps between dashes */
        flatRect(x - 0.08f, -0.08f, x + 0.08f, 0.08f, 0.03f);
    }
    float xe = x0 + 6.0f;                    /* one pair along the edges */
    flatRect(xe - 0.08f, -ROAD_HALF_WIDTH + 0.45f, xe + 0.08f, -ROAD_HALF_WIDTH + 0.6f, 0.03f);
    flatRect(xe - 0.08f,  ROAD_HALF_WIDTH - 0.6f,  xe + 0.08f,  ROAD_HALF_WIDTH - 0.45f, 0.03f);
    glEnable(GL_LIGHTING);
}

static void drawMarkings(float x0, int slot, int roadStyle, int marking,
                         const float road[3])
{
    float x1 = x0 + SLOT_LENGTH;

    if (roadStyle == ROAD_GRAVEL) gravelSpeckles(x0, slot, road);

    switch (marking) {
        case MARK_RUTS:            /* two darker wheel tracks per lane */
            scaledColor(road, 0.72f);
            for (int lane = 0; lane < 2; lane++) {
                float zc = lane ? 2.0f : -2.0f;
                flatRect(x0, zc - 0.95f, x1, zc - 0.62f, 0.02f);
                flatRect(x0, zc + 0.62f, x1, zc + 0.95f, 0.02f);
            }
            break;

        case MARK_SLAB_JOINTS:     /* cross joints every 4 m, centre joint */
            scaledColor(road, 0.60f);
            for (int j = 0; j < 3; j++) {
                float x = x0 + 4.0f * j;
                flatRect(x, -ROAD_HALF_WIDTH, x + 0.06f, ROAD_HALF_WIDTH, 0.02f);
            }
            flatRect(x0, -0.03f, x1, 0.03f, 0.02f);
            break;

        case MARK_WHITE_DASH:
            glColor3f(0.92f, 0.92f, 0.90f);
            centreDashes(x0);
            break;

        case MARK_WHITE_DASH_EDGES:
            glColor3f(0.92f, 0.92f, 0.90f);
            centreDashes(x0);
            edgeLines(x0);
            break;

        case MARK_YELLOW_DASH:     /* the original road */
            glColor3f(0.95f, 0.95f, 0.95f);
            edgeLines(x0);
            glColor3f(0.95f, 0.85f, 0.20f);
            centreDashes(x0);
            break;

        case MARK_CATS_EYES:
            glColor3f(0.95f, 0.95f, 0.95f);
            edgeLines(x0);
            centreDashes(x0);
            catsEyes(x0);
            break;

        case MARK_MODERN_CYCLE_LANE:   /* green strip inside the near edge */
            glColor3f(0.20f, 0.55f, 0.30f);
            flatRect(x0, 2.35f, x1, ROAD_HALF_WIDTH - 0.4f, 0.015f);
            glColor3f(1.0f, 1.0f, 1.0f);
            flatRect(x0, 2.2f, x1, 2.3f, 0.02f);
            /* fall through: plus the normal modern markings */
        case MARK_MODERN:
            glColor3f(1.0f, 1.0f, 1.0f);
            edgeLines(x0);
            centreDashes(x0);
            break;

        default:                   /* MARK_NONE */
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Roadside objects for one strip                                      */
/* ------------------------------------------------------------------ */

/* Scale of one object during a ripple. The new era's object grows from
 * its base as the wave passes, the old one sinks; smoothstep makes the
 * motion ease in and out. Outside a ripple (rip == NULL) it is 1. */
static void placeProp(int type, float x, float z, float centreX,
                      const RippleState *rip, int isOld)
{
    if (type == PROP_NONE) return;

    float s = 1.0f;
    if (rip) {
        float pz = (type == PROP_GANTRY) ? 0.0f : z;
        float d  = hypotf(centreX - rip->cx, pz - rip->cz);   /* 2D distance */
        float p  = smoothstep01(rippleProgress(rip, d));
        s = isOld ? 1.0f - p : p;
    }
    drawProp(type, x, z, s);
}

static void drawStripProps(const EraTheme *t, float x0, int slot,
                           const RippleState *rip, int isOld)
{
    float xm = x0 + 0.5f * SLOT_LENGTH;   /* middle of the slot */

    /* Rows start at the slot start and run 12 m; measure the ripple
     * distance from their middle. */
    if (!hideNearSide)
        placeProp(t->rowProps[ROW_NEAR], x0, ROW_NEAR_Z, xm, rip, isOld);
    placeProp(t->rowProps[ROW_FAR],  x0, ROW_FAR_Z,  xm, rip, isOld);

    const int *sp = t->slotProps[slot];
    if (!hideNearSide)
        placeProp(sp[COL_NEAR], xm, SLOT_NEAR_Z, xm, rip, isOld);
    placeProp(sp[COL_FAR],  xm, SLOT_FAR_Z,  xm, rip, isOld);

    float backZ = (sp[COL_BACK] == PROP_TURBINE) ? TURBINE_Z
                : (slot & 1) ? BACK_Z_ODD : BACK_Z_EVEN;
    placeProp(sp[COL_BACK], xm, backZ, xm, rip, isOld);
}

/* ------------------------------------------------------------------ */
/* The road is only 600 m long, but the car drives forever. Instead of */
/* building an infinite road, we move the whole scene along with the   */
/* car, in jumps of exactly SCENE_PERIOD. Because the scene looks      */
/* identical after each jump, the viewer never notices it: like a      */
/* treadmill whose belt pattern repeats.                               */
/*                                                                     */
/* Everything is drawn as 12 m strips, so during a ripple each strip   */
/* (and each object on it) can be at its own stage of the change.      */
/* ------------------------------------------------------------------ */
void drawScene(float followX, const EraTheme *cur, const EraTheme *prev,
               const RippleState *rip)
{
    float snap = floorf(followX / SCENE_PERIOD) * SCENE_PERIOD;
    int rippling = (prev != NULL && rip != NULL && rip->active);

    for (int k = -STRIPS_EACH_SIDE; k < STRIPS_EACH_SIDE; k++) {
        float x0   = snap + k * SLOT_LENGTH;
        int   slot = ((k % SLOTS_PER_PERIOD) + SLOTS_PER_PERIOD) % SLOTS_PER_PERIOD;

        float ground[3], road[3];
        const EraTheme *markTheme = cur;

        if (rippling) {
            /* Road and ground: distance along x only. */
            float p = rippleProgress(rip, fabsf(x0 + 0.5f * SLOT_LENGTH - rip->cx));
            lerpColor(ground, prev->ground, cur->ground, p);
            lerpColor(road, roadColor(prev->roadStyle), roadColor(cur->roadStyle), p);
            if (p <= 0.5f) markTheme = prev;   /* markings swap halfway */
        } else {
            memcpy(ground, cur->ground, sizeof ground);
            memcpy(road, roadColor(cur->roadStyle), sizeof road);
        }

        glColor3fv(ground);                  /* grass strip */
        flatRect(x0, GROUND_FAR_Z, x0 + SLOT_LENGTH, GROUND_NEAR_Z, 0.0f);

        glColor3fv(road);                    /* asphalt (or earth) strip */
        flatRect(x0, -ROAD_HALF_WIDTH, x0 + SLOT_LENGTH, ROAD_HALF_WIDTH, 0.01f);
        drawMarkings(x0, slot, markTheme->roadStyle, markTheme->markingStyle, road);

        if (rippling) {
            drawStripProps(prev, x0, slot, rip, 1);   /* sinking  */
            drawStripProps(cur,  x0, slot, rip, 0);   /* growing  */
        } else {
            drawStripProps(cur, x0, slot, NULL, 0);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Skyline backdrop                                                    */
/*                                                                     */
/* Drawn first, unlit and without fog, so the silhouettes stay faintly */
/* visible beyond the fog. Each layer is drawn with the depth test on, */
/* so where its own shapes overlap only the first one is kept and the  */
/* alpha is not applied twice. The depth buffer is cleared between and */
/* after the layers, so the backdrop never hides the real scene.       */
/* ------------------------------------------------------------------ */
void drawBackdrop(float followX, const EraTheme *cur, const EraTheme *prev,
                  const RippleState *rip)
{
    float col[3];

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (prev != NULL && rip != NULL && rip->active) {
        float s = rippleGlobalFactor(rip);     /* crossfade: old first */
        skylineColor(col, prev);
        drawSkyline(prev->skyline, followX, col, 1.0f - s);
        glClear(GL_DEPTH_BUFFER_BIT);
        skylineColor(col, cur);
        drawSkyline(cur->skyline, followX, col, s);
    } else {
        skylineColor(col, cur);
        drawSkyline(cur->skyline, followX, col, 1.0f);
    }

    glPopAttrib();
    glClear(GL_DEPTH_BUFFER_BIT);
}

/* ------------------------------------------------------------------ */
/* Ripple wavefront                                                    */
/*                                                                     */
/* An open cylinder at the wave's radius. Additive blending makes it   */
/* brighten whatever is behind it, like light; it does not write depth */
/* so it never hides anything, and it fades out as it grows.           */
/* ------------------------------------------------------------------ */
void drawRippleRing(const RippleState *rip, const float color[3])
{
    if (rip == NULL || !rip->active) return;

    float radius = rippleRadius(rip);
    float alpha  = 0.6f * (1.0f - radius / (RIPPLE_VISIBLE + RIPPLE_RAMP));
    if (alpha <= 0.0f || radius <= 0.0f) return;

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    glColor4f(color[0], color[1], color[2], alpha);
    glPushMatrix();
        glTranslatef(rip->cx, 0.0f, rip->cz);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);    /* cylinder axis up */
        gluCylinder(quad, radius, radius, RIPPLE_RING_HEIGHT, RING_SLICES, 1);
    glPopMatrix();

    glPopAttrib();
}
