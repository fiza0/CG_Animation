#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "geom.h"

#define MAX_ROWS     96
#define MAX_COLS     40
#define MAX_PROFILE  64

static GLUquadric *gq = NULL;

void initGeom(void)
{
    if (gq) return;
    gq = gluNewQuadric();
    gluQuadricNormals(gq, GLU_SMOOTH);
}

static GLUquadric *quadric(void)
{
    if (!gq) initGeom();
    return gq;
}

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

float curveY(const Pt2 *p, int n, float x)
{
    if (n <= 0) return 0.0f;
    if (n == 1 || x <= p[0].x) return p[0].y;
    if (x >= p[n - 1].x)        return p[n - 1].y;

    int i = 0;
    while (i < n - 2 && x > p[i + 1].x) i++;

    float x0 = p[i].x, x1 = p[i + 1].x, h = x1 - x0;
    float y0 = p[i].y, y1 = p[i + 1].y;
    if (h < 1e-6f) return y1;
    float t = (x - x0) / h;

    float m0 = (i > 0)     ? (y1 - p[i - 1].y) / (x1 - p[i - 1].x) : (y1 - y0) / h;
    float m1 = (i + 2 < n) ? (p[i + 2].y - y0) / (p[i + 2].x - x0) : (y1 - y0) / h;

    float t2 = t * t, t3 = t2 * t;
    return (2*t3 - 3*t2 + 1) * y0 + (t3 - 2*t2 + t) * h * m0
         + (-2*t3 + 3*t2)    * y1 + (t3 - t2)       * h * m1;
}

Vec3 pathPoint(const Vec3 *p, int n, float t)
{
    if (n <= 1) return p[0];
    t = clampf(t, 0.0f, 1.0f);
    float f = t * (n - 1);
    int i = (int)floorf(f);
    if (i > n - 2) i = n - 2;
    float u = f - i, u2 = u * u, u3 = u2 * u;

    Vec3 p0 = p[i > 0 ? i - 1 : 0], p1 = p[i], p2 = p[i + 1];
    Vec3 p3 = p[i + 2 < n ? i + 2 : n - 1];
#define CR(c) (0.5f * (2.0f * p1.c + (-p0.c + p2.c) * u \
             + (2.0f * p0.c - 5.0f * p1.c + 4.0f * p2.c - p3.c) * u2 \
             + (-p0.c + 3.0f * p1.c - 3.0f * p2.c + p3.c) * u3))
    Vec3 r = v3(CR(x), CR(y), CR(z));
#undef CR
    return r;
}

static Vec3 gP[MAX_ROWS * MAX_COLS], gN[MAX_ROWS * MAX_COLS], gC[MAX_ROWS];

static void gridNormals(int rows, int cols)
{
    for (int r = 0; r < rows; r++) {
        int rp = r + 1 < rows ? r + 1 : r, rm = r > 0 ? r - 1 : r;
        for (int c = 0; c < cols; c++) {
            int cp = (c + 1) % cols, cm = (c + cols - 1) % cols;
            Vec3 along  = vSub(gP[rp * cols + c], gP[rm * cols + c]);
            Vec3 around = vSub(gP[r * cols + cp], gP[r * cols + cm]);
            Vec3 out    = vSub(gP[r * cols + c], gC[r]);
            Vec3 n      = vCross(along, around);
            if (vLen(n) < 1e-10f)
                n = (r == 0) ? vScale(along, -1.0f) : (r == rows - 1 ? along : out);
            if (vDot(n, out) < 0.0f) n = vScale(n, -1.0f);
            gN[r * cols + c] = vNorm(n);
        }
    }
}

static void emit(int i)
{
    glNormal3f(gN[i].x, gN[i].y, gN[i].z);
    glVertex3f(gP[i].x, gP[i].y, gP[i].z);
}

static void drawGrid(int rows, int cols, QuadFilter filter, const void *ctx)
{
    glBegin(GL_QUADS);
    for (int r = 0; r + 1 < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int c2 = (c + 1) % cols;
            int q[4] = { r * cols + c, (r + 1) * cols + c, (r + 1) * cols + c2, r * cols + c2 };
            if (filter) {
                Vec3 ctr = v3(0, 0, 0), nn = v3(0, 0, 0);
                for (int k = 0; k < 4; k++) {
                    ctr = vAdd(ctr, gP[q[k]]);
                    nn  = vAdd(nn,  gN[q[k]]);
                }
                if (!filter(vScale(ctr, 0.25f), vNorm(nn), ctx)) continue;
            }
            for (int k = 0; k < 4; k++) emit(q[k]);
        }
    }
    glEnd();
}

static void capFan(int row, int cols, Vec3 normal)
{
    glNormal3f(normal.x, normal.y, normal.z);
    glBegin(GL_TRIANGLE_FAN);
        glVertex3f(gC[row].x, gC[row].y, gC[row].z);
        for (int c = 0; c <= cols; c++) {
            const Vec3 *p = &gP[row * cols + (c % cols)];
            glVertex3f(p->x, p->y, p->z);
        }
    glEnd();
}

static float spow(float v, float e) { return v >= 0.0f ? powf(v, e) : -powf(-v, e); }

static void evalSection(const Loft *L, float x, Section *s)
{
    L->fn(x, s, L->ctx);
    if (s->yTop < s->yBottom + 0.02f) s->yTop = s->yBottom + 0.02f;
    if (s->roundness < 1.5f)          s->roundness = 1.5f;
    if (s->halfWidth < 0.005f)        s->halfWidth = 0.005f;
    if (s->topHalfWidth < 0.005f)     s->topHalfWidth = 0.005f;
}

static Vec3 sectionPoint(const Section *s, float x, float u, float v)
{
    float yc = 0.5f * (s->yBottom + s->yTop), a = 0.5f * (s->yTop - s->yBottom);
    float hw = s->halfWidth + (s->topHalfWidth - s->halfWidth) * 0.5f * (v + 1.0f);
    return v3(x, yc + a * v, hw * u);
}

void drawLoft(const Loft *L)
{
    int rows = L->stations < 2 ? 2 : (L->stations > MAX_ROWS ? MAX_ROWS : L->stations);
    int cols = L->ringPts  < 4 ? 4 : (L->ringPts  > MAX_COLS ? MAX_COLS : L->ringPts);

    for (int r = 0; r < rows; r++) {
        float x = L->x0 + (L->x1 - L->x0) * r / (rows - 1);
        Section s;
        evalSection(L, x, &s);
        float e = 2.0f / s.roundness;
        gC[r] = v3(x, 0.5f * (s.yBottom + s.yTop), 0.0f);
        for (int c = 0; c < cols; c++) {
            float th = 2.0f * (float)M_PI * c / cols;
            gP[r * cols + c] = sectionPoint(&s, x, spow(cosf(th), e), spow(sinf(th), e));
        }
    }

    gridNormals(rows, cols);
    drawGrid(rows, cols, L->filter, L->filterCtx);
    if (L->capRear)  capFan(0,        cols, v3(-1.0f, 0.0f, 0.0f));
    if (L->capFront) capFan(rows - 1, cols, v3( 1.0f, 0.0f, 0.0f));
}

static Vec3 surfacePoint(const Loft *L, float x, float a, int top, float side)
{
    Section s;
    evalSection(L, x, &s);
    float n = s.roundness, u, v;
    if (top) {
        u = clampf(a, -1.0f, 1.0f);
        v = powf(1.0f - powf(fabsf(u), n), 1.0f / n);
    } else {
        v = clampf(2.0f * a - 1.0f, -1.0f, 1.0f);
        u = (side < 0.0f ? -1.0f : 1.0f) * powf(1.0f - powf(fabsf(v), n), 1.0f / n);
    }
    return sectionPoint(&s, x, u, v);
}

static Vec3 surfaceNormal(const Loft *L, float x, float a, int top, float side)
{
    const float dx = 0.01f, da = 0.01f;
    float lo = top ? -1.0f : 0.0f, hi = 1.0f;
    float a0 = clampf(a - da, lo, hi), a1 = clampf(a + da, lo, hi);

    Vec3 tx = vSub(surfacePoint(L, x + dx, a, top, side), surfacePoint(L, x - dx, a, top, side));
    Vec3 ta = vSub(surfacePoint(L, x, a1, top, side),     surfacePoint(L, x, a0, top, side));
    Vec3 n  = vCross(tx, ta);

    Section s;
    evalSection(L, x, &s);
    Vec3 p   = surfacePoint(L, x, a, top, side);
    Vec3 out = vSub(p, v3(x, 0.5f * (s.yBottom + s.yTop), 0.0f));
    if (vLen(n) < 1e-10f) n = out;
    if (vDot(n, out) < 0.0f) n = vScale(n, -1.0f);
    return vNorm(n);
}

Vec3 loftSide(const Loft *L, float x, float h, float side, Vec3 *normal)
{
    if (normal) *normal = surfaceNormal(L, x, h, 0, side);
    return surfacePoint(L, x, h, 0, side);
}

Vec3 loftTop(const Loft *L, float x, float u, Vec3 *normal)
{
    if (normal) *normal = surfaceNormal(L, x, u, 1, 1.0f);
    return surfacePoint(L, x, u, 1, 1.0f);
}

float loftH(const Loft *L, float x, float y)
{
    Section s;
    evalSection(L, x, &s);
    return clampf((y - s.yBottom) / (s.yTop - s.yBottom), 0.0f, 1.0f);
}

static Vec3 surfAt(const Loft *L, float x, float a, int top, float side, Vec3 *n)
{
    return top ? loftTop(L, x, a, n) : loftSide(L, x, a, side, n);
}

void loftStrip(const Loft *L, const Pt2 *pts, int n, int topMode,
               float side, float width, float lift)
{
    const int sub = 8;
    if (n < 2) return;
    int total = (n - 1) * sub + 1;

    glBegin(GL_QUAD_STRIP);
    for (int k = 0; k < total; k++) {
        int   seg = k / sub;
        float f   = (float)(k % sub) / sub;
        if (seg >= n - 1) { seg = n - 2; f = 1.0f; }

        float dxs = pts[seg + 1].x - pts[seg].x, das = pts[seg + 1].y - pts[seg].y;
        float x = pts[seg].x + dxs * f, a = pts[seg].y + das * f;

        Vec3 nrm, p = surfAt(L, x, a, topMode, side, &nrm);
        Vec3 q = surfAt(L, x + dxs * 0.02f, a + das * 0.02f, topMode, side, NULL);
        Vec3 T = vSub(q, p);
        if (vLen(T) < 1e-7f) T = v3(1.0f, 0.0f, 0.0f);
        Vec3 O = vScale(vNorm(vCross(nrm, T)), 0.5f * width);
        Vec3 c = vAdd(p, vScale(nrm, lift));

        glNormal3f(nrm.x, nrm.y, nrm.z);
        glVertex3f(c.x + O.x, c.y + O.y, c.z + O.z);
        glVertex3f(c.x - O.x, c.y - O.y, c.z - O.z);
    }
    glEnd();
}

static void surfacePatch(const Loft *L, float x0, float x1, float a0, float a1,
                         int top, float side, int nx, int na, float lift)
{
    if (nx < 1) nx = 1;
    if (na < 1) na = 1;
    for (int i = 0; i < nx; i++) {
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= na; j++) {
            float a = a0 + (a1 - a0) * j / na;
            for (int k = 0; k < 2; k++) {
                float x = x0 + (x1 - x0) * (i + k) / nx;
                Vec3 n, p = surfAt(L, x, a, top, side, &n);
                p = vAdd(p, vScale(n, lift));
                glNormal3f(n.x, n.y, n.z);
                glVertex3f(p.x, p.y, p.z);
            }
        }
        glEnd();
    }
}

void loftTopPatch(const Loft *L, float x0, float x1, float u0, float u1,
                  int nx, int nu, float lift)
{
    surfacePatch(L, x0, x1, u0, u1, 1, 1.0f, nx, nu, lift);
}

void loftSidePatch(const Loft *L, float x0, float x1, float h0, float h1,
                   float side, int nx, int nh, float lift)
{
    surfacePatch(L, x0, x1, h0, h1, 0, side, nx, nh, lift);
}

float archBottom(float x, float axleX, float archR, float axleY, float base)
{
    float dx = x - axleX;
    if (fabsf(dx) >= archR) return base;
    float y = axleY + sqrtf(archR * archR - dx * dx);
    return y > base ? y : base;
}

void sweep(const Vec3 *path, int n, int samples,
           const Pt2 *sec, int nSec, Vec3 across, int caps)
{
    int rows = samples < 2 ? 2 : (samples > MAX_ROWS ? MAX_ROWS : samples);
    int cols = nSec < 3 ? 3 : (nSec > MAX_COLS ? MAX_COLS : nSec);
    Vec3 firstT = v3(1, 0, 0), lastT = v3(1, 0, 0);

    for (int r = 0; r < rows; r++) {
        float t  = (float)r / (rows - 1);
        Vec3  C  = pathPoint(path, n, t);
        Vec3  T  = vNorm(vSub(pathPoint(path, n, clampf(t + 0.01f, 0, 1)),
                              pathPoint(path, n, clampf(t - 0.01f, 0, 1))));
        Vec3 B = vSub(across, vScale(T, vDot(across, T)));
        if (vLen(B) < 1e-4f)
            B = fabsf(T.y) < 0.9f ? vCross(T, v3(0, 1, 0)) : vCross(T, v3(1, 0, 0));
        B = vNorm(B);
        Vec3 N = vCross(B, T);

        gC[r] = C;
        for (int c = 0; c < cols; c++)
            gP[r * cols + c] = vAdd(C, vAdd(vScale(B, sec[c].x), vScale(N, sec[c].y)));
        if (r == 0) firstT = T;
        lastT = T;
    }

    gridNormals(rows, cols);
    drawGrid(rows, cols, NULL, NULL);
    if (caps) {
        capFan(0,        cols, vScale(firstT, -1.0f));
        capFan(rows - 1, cols, lastT);
    }
}

void sweepTube(const Vec3 *path, int n, int samples, float radius, int sides)
{
    Pt2 sec[MAX_COLS];
    if (sides > MAX_COLS) sides = MAX_COLS;
    if (sides < 3) sides = 3;
    for (int i = 0; i < sides; i++) {
        float a = 2.0f * (float)M_PI * i / sides;
        sec[i].x = radius * cosf(a);
        sec[i].y = radius * sinf(a);
    }
    Vec3 d = vSub(path[n - 1], path[0]);
    Vec3 across = (fabsf(d.z) < 0.7f * vLen(d)) ? v3(0, 0, 1) : v3(0, 1, 0);
    sweep(path, n, samples, sec, sides, across, 1);
}

static int alignToSegment(Vec3 a, Vec3 b, float *len)
{
    Vec3 d = vSub(b, a);
    *len = vLen(d);
    if (*len < 1e-6f) return 0;
    glTranslatef(a.x, a.y, a.z);
    Vec3 axis = v3(-d.y, d.x, 0.0f);
    float ang = acosf(clampf(d.z / *len, -1.0f, 1.0f)) * 180.0f / (float)M_PI;
    if (vLen(axis) < 1e-6f) {
        if (d.z < 0.0f) glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
    } else {
        glRotatef(ang, axis.x, axis.y, axis.z);
    }
    return 1;
}

void tube(Vec3 a, Vec3 b, float r0, float r1, int sides)
{
    float len;
    glPushMatrix();
    if (alignToSegment(a, b, &len))
        gluCylinder(quadric(), r0, r1, len, sides, 1);
    glPopMatrix();
}

void tubeCapped(Vec3 a, Vec3 b, float r0, float r1, int sides)
{
    float len;
    glPushMatrix();
    if (alignToSegment(a, b, &len)) {
        GLUquadric *q = quadric();
        gluCylinder(q, r0, r1, len, sides, 1);
        gluQuadricOrientation(q, GLU_INSIDE);
        gluDisk(q, 0.0, r0, sides, 1);
        gluQuadricOrientation(q, GLU_OUTSIDE);
        glTranslatef(0.0f, 0.0f, len);
        gluDisk(q, 0.0, r1, sides, 1);
    }
    glPopMatrix();
}

void revolve(const Pt2 *rz, int n, int segs)
{
    float nr[MAX_PROFILE], nz[MAX_PROFILE];
    if (n < 2) return;
    if (n > MAX_PROFILE) n = MAX_PROFILE;
    if (segs < 3) segs = 3;

    for (int i = 0; i < n; i++) {
        float sr = 0.0f, sz = 0.0f;
        for (int k = -1; k <= 0; k++) {
            int a = i + k, b = i + k + 1;
            if (a < 0 || b >= n) continue;
            float dr = rz[b].x - rz[a].x, dz = rz[b].y - rz[a].y;
            float l = sqrtf(dr * dr + dz * dz);
            if (l < 1e-6f) continue;
            sr += dz / l;
            sz += -dr / l;
        }
        float l = sqrtf(sr * sr + sz * sz);
        if (l < 1e-6f) { sr = 1.0f; sz = 0.0f; l = 1.0f; }
        nr[i] = sr / l;
        nz[i] = sz / l;
    }

    for (int i = 0; i + 1 < n; i++) {
        float dr = rz[i + 1].x - rz[i].x, dz = rz[i + 1].y - rz[i].y;
        if (dr * dr + dz * dz < 1e-12f) continue;
        glBegin(GL_QUAD_STRIP);
        for (int s = 0; s <= segs; s++) {
            float ph = 2.0f * (float)M_PI * s / segs, c = cosf(ph), sn = sinf(ph);
            for (int k = i; k <= i + 1; k++) {
                glNormal3f(nr[k] * c, nr[k] * sn, nz[k]);
                glVertex3f(rz[k].x * c, rz[k].x * sn, rz[k].y);
            }
        }
        glEnd();
    }
}

typedef struct { float y0, y1, hw, n; } BoxCtx;

static void boxSection(float x, Section *s, const void *ctx)
{
    const BoxCtx *b = (const BoxCtx *)ctx;
    (void)x;
    s->yBottom = b->y0;
    s->yTop    = b->y1;
    s->halfWidth = s->topHalfWidth = b->hw;
    s->roundness = b->n;
}

void roundedBox(float cx, float cy, float cz, float sx, float sy, float sz,
                float roundness)
{
    BoxCtx b = { cy - 0.5f * sy, cy + 0.5f * sy, 0.5f * sz, roundness };
    Loft L = { boxSection, &b, cx - 0.5f * sx, cx + 0.5f * sx, 2, 24, 1, 1, NULL, NULL };
    glPushMatrix();
        glTranslatef(0.0f, 0.0f, cz);
        drawLoft(&L);
    glPopMatrix();
}

void boxAt(float cx, float cy, float cz, float sx, float sy, float sz)
{
    glPushMatrix();
        glTranslatef(cx, cy, cz);
        glScalef(sx, sy, sz);
        glutSolidCube(1.0);
    glPopMatrix();
}

void ellipsoid(float cx, float cy, float cz, float rx, float ry, float rz)
{
    glPushMatrix();
        glTranslatef(cx, cy, cz);
        glScalef(rx, ry, rz);
        glutSolidSphere(1.0, 18, 12);
    glPopMatrix();
}

void disc(float r, int segs)            { gluDisk(quadric(), 0.0, r, segs, 1); }
void annulus(float r0, float r1, int segs) { gluDisk(quadric(), r0, r1, segs, 1); }

void ellipseDisc(float rx, float ry, int segs)
{
    glPushMatrix();
        glScalef(rx, ry, 1.0f);
        gluDisk(quadric(), 0.0, 1.0, segs, 1);
    glPopMatrix();
}

void quadXY(float x0, float y0, float x1, float y1)
{
    glNormal3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
        glVertex3f(x0, y0, 0.0f); glVertex3f(x1, y0, 0.0f);
        glVertex3f(x1, y1, 0.0f); glVertex3f(x0, y1, 0.0f);
    glEnd();
}

static void frameAxes(Vec3 n, Vec3 *X, Vec3 *Y, Vec3 *Z)
{
    *Z = vNorm(n);
    Vec3 ref = fabsf(Z->y) > 0.9f ? v3(1.0f, 0.0f, 0.0f) : v3(0.0f, 1.0f, 0.0f);
    *Y = vNorm(vSub(ref, vScale(*Z, vDot(ref, *Z))));
    *X = vCross(*Y, *Z);
}

Vec3 frameXAxis(Vec3 n)
{
    Vec3 X, Y, Z;
    frameAxes(n, &X, &Y, &Z);
    return X;
}

void pushFrame(Vec3 p, Vec3 n)
{
    Vec3 X, Y, Z;
    frameAxes(n, &X, &Y, &Z);
    GLfloat m[16] = { X.x, X.y, X.z, 0.0f,
                      Y.x, Y.y, Y.z, 0.0f,
                      Z.x, Z.y, Z.z, 0.0f,
                      p.x, p.y, p.z, 1.0f };
    glPushMatrix();
    glMultMatrixf(m);
}
