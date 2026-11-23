#ifndef GEOM_H
#define GEOM_H

#include <math.h>
#include "scene.h"

/* ------------------------------------------------------------------ */
/* Small vector maths                                                  */
/* ------------------------------------------------------------------ */
typedef struct { float x, y; }    Pt2;
typedef struct { float x, y, z; } Vec3;

static inline Vec3  v3(float x, float y, float z) { Vec3 r = { x, y, z }; return r; }
static inline Vec3  vAdd(Vec3 a, Vec3 b)   { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline Vec3  vSub(Vec3 a, Vec3 b)   { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline Vec3  vScale(Vec3 a, float s){ return v3(a.x * s, a.y * s, a.z * s); }
static inline float vDot(Vec3 a, Vec3 b)   { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline Vec3  vCross(Vec3 a, Vec3 b)
{
    return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
static inline float vLen(Vec3 a) { return sqrtf(vDot(a, a)); }
static inline Vec3  vNorm(Vec3 a)
{
    float l = vLen(a);
    return l > 1e-9f ? vScale(a, 1.0f / l) : v3(0.0f, 1.0f, 0.0f);
}

#define COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define DECAL_LIFT 0.005f   /* decals float this far above a surface */

void initGeom(void);   /* creates the shared GLU quadric */

/* ------------------------------------------------------------------ */
/* Splines                                                             */
/* ------------------------------------------------------------------ */

/* A smooth function y(x) through control points sorted by x. Between
 * two points it uses a cubic whose slopes come from the neighbours
 * (Catmull-Rom style), so 8 to 20 points describe a smooth profile.
 * Outside the range it holds the end values. */
float curveY(const Pt2 *p, int n, float x);

/* A smooth 3D path through points, t from 0 (first) to 1 (last). */
Vec3 pathPoint(const Vec3 *p, int n, float t);

/* ------------------------------------------------------------------ */
/* Lofted bodies                                                       */
/*                                                                     */
/* A loft is a chain of cross-sections along x, joined into one smooth */
/* skin. Each cross-section is a superellipse ("rounded rectangle")    */
/* between yBottom and yTop. Its half-width can differ at the bottom   */
/* and at the top (tumblehome: sides leaning inwards), and roundness   */
/* goes from 2 (an ellipse) to about 10 (almost a sharp box).          */
/* ------------------------------------------------------------------ */
typedef struct {
    float yBottom, yTop;
    float halfWidth;      /* at the bottom                       */
    float topHalfWidth;   /* at the top                          */
    float roundness;
} Section;

typedef void (*SectionFn)(float x, Section *s, const void *ctx);

/* Decides per quad whether it is drawn (centre point and normal). Used
 * to split one loft into painted parts and glass parts. */
typedef int (*QuadFilter)(Vec3 centre, Vec3 normal, const void *ctx);

typedef struct {
    SectionFn   fn;
    const void *ctx;
    float       x0, x1;          /* rear end, front end              */
    int         stations;        /* cross-sections along x (<= 96)   */
    int         ringPts;         /* points around each one (<= 40)   */
    int         capRear, capFront;
    QuadFilter  filter;          /* NULL: draw every quad            */
    const void *filterCtx;
} Loft;

void drawLoft(const Loft *L);

/* Points on a loft's surface, with the outward normal.
 * loftSide: h = 0 bottom centre, 0.5 halfway up the side, 1 top centre,
 *           on the side given by side = +1 (+z) or -1 (-z).
 * loftTop:  u = -1 .. 1 across the top (fraction of the half-width). */
Vec3 loftSide(const Loft *L, float x, float h, float side, Vec3 *normal);
Vec3 loftTop (const Loft *L, float x, float u, Vec3 *normal);

/* h for a given height y at station x (for use with loftSide). */
float loftH(const Loft *L, float x, float y);

/* A thin band laid on the surface through (x, h) points (side mode) or
 * (x, u) points (top mode), lifted slightly to avoid z-fighting. */
void loftStrip(const Loft *L, const Pt2 *pts, int n, int topMode,
               float side, float width, float lift);

/* A patch of surface between x0..x1 and u0..u1 across the top, lifted. */
void loftTopPatch(const Loft *L, float x0, float x1, float u0, float u1,
                  int nx, int nu, float lift);

/* Same on the side: x0..x1 and heights h0..h1. */
void loftSidePatch(const Loft *L, float x0, float x1, float h0, float h1,
                   float side, int nx, int nh, float lift);

/* Lower edge raised into a half-circle over a wheel (wheel arch). */
float archBottom(float x, float axleX, float archR, float axleY, float base);

/* ------------------------------------------------------------------ */
/* Sweeps, tubes, revolves, boxes                                      */
/* ------------------------------------------------------------------ */

/* Sweep a closed cross-section along a smooth path. Section points are
 * (a, b): a along `across` (made perpendicular to the path), b along
 * the third axis (across x tangent). The section should be convex
 * around (0, 0). */
void sweep(const Vec3 *path, int n, int samples,
           const Pt2 *sec, int nSec, Vec3 across, int caps);

/* A round tube along a smooth path. */
void sweepTube(const Vec3 *path, int n, int samples, float radius, int sides);

/* A cylinder (or cone, if r0 != r1) from point a to point b. */
void tube(Vec3 a, Vec3 b, float r0, float r1, int sides);
void tubeCapped(Vec3 a, Vec3 b, float r0, float r1, int sides);

/* Spin a profile of (r, z) points around the z axis. Normals point to
 * the right of the direction of travel along the profile, (dz, -dr).
 * Repeating a point makes a sharp crease there. */
void revolve(const Pt2 *rz, int n, int segs);

/* Box with rounded edges along x (a superellipse prism). */
void roundedBox(float cx, float cy, float cz, float sx, float sy, float sz,
                float roundness);
void boxAt(float cx, float cy, float cz, float sx, float sy, float sz);
void ellipsoid(float cx, float cy, float cz, float rx, float ry, float rz);

/* Flat shapes in the local xy plane, facing +z. */
void disc(float r, int segs);
void ellipseDisc(float rx, float ry, int segs);
void annulus(float r0, float r1, int segs);
void quadXY(float x0, float y0, float x1, float y1);

/* Push a matrix whose local z axis is the given normal and whose local
 * y axis points as close to world "up" as possible (or forward, on
 * horizontal surfaces). Anything drawn at local z = DECAL_LIFT then
 * lies flat on the surface. Pop with glPopMatrix. */
void pushFrame(Vec3 p, Vec3 n);

/* World direction of the local x axis that pushFrame would use for n.
 * Lets callers turn asymmetric shapes (teardrops) the right way. */
Vec3 frameXAxis(Vec3 n);

#endif
