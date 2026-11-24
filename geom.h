#ifndef GEOM_H
#define GEOM_H

#include <math.h>
#include "scene.h"

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
#define DECAL_LIFT 0.005f

void initGeom(void);

float curveY(const Pt2 *p, int n, float x);

Vec3 pathPoint(const Vec3 *p, int n, float t);

typedef struct {
    float yBottom, yTop;
    float halfWidth;
    float topHalfWidth;
    float roundness;
} Section;

typedef void (*SectionFn)(float x, Section *s, const void *ctx);

typedef int (*QuadFilter)(Vec3 centre, Vec3 normal, const void *ctx);

typedef struct {
    SectionFn   fn;
    const void *ctx;
    float       x0, x1;
    int         stations;
    int         ringPts;
    int         capRear, capFront;
    QuadFilter  filter;
    const void *filterCtx;
} Loft;

void drawLoft(const Loft *L);

Vec3 loftSide(const Loft *L, float x, float h, float side, Vec3 *normal);
Vec3 loftTop (const Loft *L, float x, float u, Vec3 *normal);

float loftH(const Loft *L, float x, float y);

void loftStrip(const Loft *L, const Pt2 *pts, int n, int topMode,
               float side, float width, float lift);

void loftTopPatch(const Loft *L, float x0, float x1, float u0, float u1,
                  int nx, int nu, float lift);

void loftSidePatch(const Loft *L, float x0, float x1, float h0, float h1,
                   float side, int nx, int nh, float lift);

float archBottom(float x, float axleX, float archR, float axleY, float base);

void sweep(const Vec3 *path, int n, int samples,
           const Pt2 *sec, int nSec, Vec3 across, int caps);

void sweepTube(const Vec3 *path, int n, int samples, float radius, int sides);

void tube(Vec3 a, Vec3 b, float r0, float r1, int sides);
void tubeCapped(Vec3 a, Vec3 b, float r0, float r1, int sides);

void revolve(const Pt2 *rz, int n, int segs);

void roundedBox(float cx, float cy, float cz, float sx, float sy, float sz,
                float roundness);
void boxAt(float cx, float cy, float cz, float sx, float sy, float sz);
void ellipsoid(float cx, float cy, float cz, float rx, float ry, float rz);

void disc(float r, int segs);
void ellipseDisc(float rx, float ry, int segs);
void annulus(float r0, float r1, int segs);
void quadXY(float x0, float y0, float x1, float y1);

void pushFrame(Vec3 p, Vec3 n);

Vec3 frameXAxis(Vec3 n);

#endif
