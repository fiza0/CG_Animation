#include <stddef.h>   /* NULL                         */
#include <stdio.h>    /* fprintf for tessellator errors */
#include <stdlib.h>
#include <math.h>     /* sqrtf, cosf, sinf, atan2f, fmod */
#include <string.h>
#include "cars.h"

/* ------------------------------------------------------------------ */
/* Shared colours                                                      */
/* ------------------------------------------------------------------ */
static const float GLASS_COL[3]      = { 0.18f, 0.22f, 0.30f };
static const float CHROME_COL[3]     = { 0.82f, 0.83f, 0.86f };
static const float TRIM_COL[3]       = { 0.20f, 0.20f, 0.22f };
static const float LAMP_FRONT_COL[3] = { 1.00f, 0.95f, 0.75f };
static const float LAMP_REAR_COL[3]  = { 0.80f, 0.08f, 0.06f };
static const float TYRE_COL[3]       = { 0.07f, 0.07f, 0.07f };

#define MAX_EXP       96     /* profile points after wheel arches are added */
#define ARCH_GAP      0.06f  /* clearance between tyre and wheel arch        */
#define ARCH_STEPS    8      /* the arch half-circle is cut into 8 edges     */
#define WINDOW_OFFSET 0.01f  /* windows sit this far outside the body side  */

/* Edges that meet at less than this angle share a blended normal, so
 * gentle curves look smooth while sharp corners stay crisp. */
#define CREASE_COS    0.82f  /* cos(35 degrees) */

static GLUquadric    *quad = NULL;   /* cylinders and discs for wheels */
static GLUtesselator *tess = NULL;   /* splits concave shapes into triangles */

/* ------------------------------------------------------------------ */
/* Small drawing helpers                                               */
/* ------------------------------------------------------------------ */

/* Axis-aligned box given its centre and size. glutSolidCube supplies
 * the face normals; GL_NORMALIZE fixes them after the scale. */
static void box(float cx, float cy, float cz, float sx, float sy, float sz)
{
    glPushMatrix();
        glTranslatef(cx, cy, cz);
        glScalef(sx, sy, sz);
        glutSolidCube(1.0);
    glPopMatrix();
}

/* A box stretched between two points in the side (xy) plane, used for
 * slanted posts such as a steering column or a fork. */
static void strut(float x0, float y0, float x1, float y1, float z,
                  float thickY, float depthZ)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    float deg = atan2f(dy, dx) * 180.0f / (float)M_PI;

    glPushMatrix();
        glTranslatef(0.5f * (x0 + x1), 0.5f * (y0 + y1), z);
        glRotatef(deg, 0.0f, 0.0f, 1.0f);   /* box x axis along the strut */
        glScalef(len, thickY, depthZ);
        glutSolidCube(1.0);
    glPopMatrix();
}

/* Lamps and light bars look bright when they ignore the sun: with
 * lighting off, the pixel colour is exactly the colour we ask for. */
static void glowBox(float cx, float cy, float cz,
                    float sx, float sy, float sz, float r, float g, float b)
{
    glDisable(GL_LIGHTING);
    glColor3f(r, g, b);
    box(cx, cy, cz, sx, sy, sz);
    glEnable(GL_LIGHTING);
}

/* ------------------------------------------------------------------ */
/* GLU tessellator                                                     */
/*                                                                     */
/* Car silhouettes are concave (wheel arches, windscreen dips), and    */
/* GL_POLYGON or a triangle fan would fill them wrongly. The GLU       */
/* tessellator cuts any outline into triangles and hands them back     */
/* through these callbacks, which simply forward them to OpenGL.       */
/* ------------------------------------------------------------------ */
#ifndef CALLBACK
#define CALLBACK
#endif

/* Extra vertices the tessellator invents where edges cross. They must
 * outlive the callback, so they live in a small static pool. */
#define COMBINE_POOL 64
static GLdouble combinePool[COMBINE_POOL][3];
static int      combineUsed = 0;

static void CALLBACK tessBegin(GLenum type)  { glBegin(type); }
static void CALLBACK tessEnd(void)           { glEnd(); }
static void CALLBACK tessVertex(void *data)  { glVertex3dv((const GLdouble *)data); }

static void CALLBACK tessCombine(GLdouble coords[3], void *data[4],
                                 GLfloat weight[4], void **out)
{
    (void)data; (void)weight;
    if (combineUsed >= COMBINE_POOL) { *out = combinePool[0]; return; }
    GLdouble *v = combinePool[combineUsed++];
    v[0] = coords[0]; v[1] = coords[1]; v[2] = coords[2];
    *out = v;
}

static void CALLBACK tessError(GLenum err)
{
    fprintf(stderr, "Tessellation error: %s\n", (const char *)gluErrorString(err));
}

/* The macOS header declares the callback parameter as GLvoid (*)(). */
typedef GLvoid (*TessFn)();

static void initTessellator(void)
{
    tess = gluNewTess();
    gluTessCallback(tess, GLU_TESS_BEGIN,   (TessFn)tessBegin);
    gluTessCallback(tess, GLU_TESS_END,     (TessFn)tessEnd);
    gluTessCallback(tess, GLU_TESS_VERTEX,  (TessFn)tessVertex);
    gluTessCallback(tess, GLU_TESS_COMBINE, (TessFn)tessCombine);
    gluTessCallback(tess, GLU_TESS_ERROR,   (TessFn)tessError);
    /* All our outlines lie in a plane facing z, so tell GLU that
     * instead of letting it guess the plane from the points. */
    gluTessNormal(tess, 0.0, 0.0, 1.0);
}

/* Fill a flat outline lying in the plane z = const. The current normal
 * set by the caller applies to every triangle produced. */
static void fillOutline(const ProfilePt *p, int n, float z)
{
    GLdouble v[MAX_EXP][3];   /* must stay alive until EndPolygon */
    if (n > MAX_EXP) n = MAX_EXP;

    combineUsed = 0;
    gluTessBeginPolygon(tess, NULL);
    gluTessBeginContour(tess);
    for (int i = 0; i < n; i++) {
        v[i][0] = p[i].x;  v[i][1] = p[i].y;  v[i][2] = z;
        gluTessVertex(tess, v[i], v[i]);
    }
    gluTessEndContour(tess);
    gluTessEndPolygon(tess);
}

/* ------------------------------------------------------------------ */
/* Wheel arches                                                        */
/*                                                                     */
/* Typing every arch point by hand is tedious, so an edge marked       */
/* SEG_ARCH is replaced by a half circle around the nearest wheel,     */
/* a little bigger than the tyre. Arc points below the sill are        */
/* skipped so the arch always opens downwards.                         */
/* ------------------------------------------------------------------ */
static int expandProfile(const CarModel *m, const ProfilePt *src, int n,
                         ProfilePt *out)
{
    int k = 0;
    for (int i = 0; i < n && k < MAX_EXP; i++) {
        ProfilePt p = src[i];
        if (p.seg != SEG_ARCH) { out[k++] = p; continue; }

        const ProfilePt *q = &src[(i + 1) % n];
        float mid  = 0.5f * (p.x + q->x);
        int   axle = fabsf(mid - m->wheelX[0]) < fabsf(mid - m->wheelX[1]) ? 0 : 1;
        float cx   = m->wheelX[axle];
        float cy   = m->wheelRadius[axle];             /* axle height = radius */
        float ra   = m->wheelRadius[axle] + ARCH_GAP;
        float sill = 0.5f * (p.y + q->y) + 0.02f;

        p.seg = SEG_BODY;
        out[k++] = p;
        for (int s = 1; s < ARCH_STEPS && k < MAX_EXP; s++) {
            float t  = (float)s / ARCH_STEPS;
            /* Walk the arc in the same direction as the edge: angle 0 is
             * the front of the wheel (+x), 180 degrees the rear. */
            float th = (p.x > q->x ? t : 1.0f - t) * (float)M_PI;
            float ax = cx + ra * cosf(th);
            float ay = cy + ra * sinf(th);
            if (ay > sill) {
                out[k].x = ax;  out[k].y = ay;  out[k].seg = SEG_BODY;
                k++;
            }
        }
    }
    return k;
}

/* ------------------------------------------------------------------ */
/* Extrusion                                                           */
/* ------------------------------------------------------------------ */

/* Twice the signed area (shoelace formula): negative for clockwise. */
static float signedArea(const ProfilePt *p, int n)
{
    float a = 0.0f;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        a += p[i].x * p[j].y - p[j].x * p[i].y;
    }
    return a;
}

static const float *segColor(int seg, float midX, const float body[3])
{
    switch (seg) {
        case SEG_GLASS:  return GLASS_COL;
        case SEG_CHROME: return CHROME_COL;
        case SEG_TRIM:   return TRIM_COL;
        case SEG_LAMP:   return midX > 0.0f ? LAMP_FRONT_COL : LAMP_REAR_COL;
        default:         return body;
    }
}

/* Normal at the vertex shared by edges a and b: blended across a gentle
 * bend, otherwise just edge a's own normal (a crease). */
static void vertexNormal(const float *nx, const float *ny, int a, int b,
                         float *ox, float *oy)
{
    float d = nx[a] * nx[b] + ny[a] * ny[b];
    if (d > CREASE_COS) {
        float x = nx[a] + nx[b], y = ny[a] + ny[b];
        float len = sqrtf(x * x + y * y);
        *ox = x / len;  *oy = y / len;
    } else {
        *ox = nx[a];  *oy = ny[a];
    }
}

/* Push a 2D outline out from z0 to z1: two flat caps plus one quad per
 * edge. Each quad's normal is perpendicular to its edge and to z. */
static void drawExtrusion(const ProfilePt *p, int n, float z0, float z1,
                          const float body[3])
{
    float nx[MAX_EXP], ny[MAX_EXP];

    /* Edge direction (dx, dy, 0) crossed with the z axis gives (dy, -dx).
     * Which of +/- points outward depends on the winding, so check it
     * once with the signed area. */
    float s = signedArea(p, n) < 0.0f ? -1.0f : 1.0f;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        float dx = p[j].x - p[i].x, dy = p[j].y - p[i].y;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 1e-6f) { nx[i] = ny[i] = 0.0f; continue; }
        nx[i] =  s * dy / len;
        ny[i] = -s * dx / len;
    }

    /* Side caps: the silhouette itself, on both sides of the car. */
    glColor3fv(body);
    glNormal3f(0.0f, 0.0f, 1.0f);
    fillOutline(p, n, z1);
    glNormal3f(0.0f, 0.0f, -1.0f);
    fillOutline(p, n, z0);

    /* Side walls: one strip across the car for every edge. */
    glBegin(GL_QUADS);
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        if (nx[i] == 0.0f && ny[i] == 0.0f) continue;   /* zero-length edge */

        float ax, ay, bx, by;
        vertexNormal(nx, ny, i, (i + n - 1) % n, &ax, &ay);  /* start vertex */
        vertexNormal(nx, ny, i, j, &bx, &by);                /* end vertex   */

        glColor3fv(segColor(p[i].seg, 0.5f * (p[i].x + p[j].x), body));
        glNormal3f(ax, ay, 0.0f);
        glVertex3f(p[i].x, p[i].y, z1);
        glVertex3f(p[i].x, p[i].y, z0);
        glNormal3f(bx, by, 0.0f);
        glVertex3f(p[j].x, p[j].y, z0);
        glVertex3f(p[j].x, p[j].y, z1);
    }
    glEnd();
}

/* Side window: a second, smaller outline in glass colour sitting just
 * outside each side cap so the two surfaces do not z-fight. */
static void drawWindows(const CarModel *m)
{
    float z = m->halfWidth + WINDOW_OFFSET;

    glColor3fv(GLASS_COL);
    glNormal3f(0.0f, 0.0f, 1.0f);
    fillOutline(m->window, m->numWindow, z);
    glNormal3f(0.0f, 0.0f, -1.0f);
    fillOutline(m->window, m->numWindow, -z);

    if (m->pillarW <= 0.0f) return;

    /* B pillar: a body-coloured strip splitting the glass in two. */
    float y0 = m->window[0].y, y1 = y0;
    for (int i = 1; i < m->numWindow; i++) {
        if (m->window[i].y < y0) y0 = m->window[i].y;
        if (m->window[i].y > y1) y1 = m->window[i].y;
    }
    float x0 = m->pillarX - 0.5f * m->pillarW, x1 = m->pillarX + 0.5f * m->pillarW;
    float zp = z + 0.005f;

    glColor3fv(m->bodyColor);
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(x0, y0, zp);  glVertex3f(x1, y0, zp);
        glVertex3f(x1, y1, zp);  glVertex3f(x0, y1, zp);
        glNormal3f(0.0f, 0.0f, -1.0f);
        glVertex3f(x0, y0, -zp); glVertex3f(x0, y1, -zp);
        glVertex3f(x1, y1, -zp); glVertex3f(x1, y0, -zp);
    glEnd();
}

/* Tessellating every frame would be wasteful, because the shapes never
 * change. A display list records the finished GL commands once; each
 * frame then replays them with a single glCallList. */
static void buildBodyList(CarModel *m)
{
    ProfilePt pts[MAX_EXP];

    m->bodyList = glGenLists(1);
    glNewList(m->bodyList, GL_COMPILE);

        int n = expandProfile(m, m->profile, m->numPoints, pts);
        drawExtrusion(pts, n, -m->halfWidth, m->halfWidth, m->bodyColor);

        if (m->fender) {   /* separate fenders, one extrusion per side */
            n = expandProfile(m, m->fender, m->numFender, pts);
            drawExtrusion(pts, n,  m->fenderIn,   m->fenderOut, m->fenderColor);
            drawExtrusion(pts, n, -m->fenderOut, -m->fenderIn,  m->fenderColor);
        }

        if (m->window) drawWindows(m);

    glEndList();
}

/* ------------------------------------------------------------------ */
/* Wheels                                                              */
/* All wheel parts are built around the axle, which is the z axis.     */
/* ------------------------------------------------------------------ */

/* Open cylinder of radius r and width w, centred on z = 0. */
static void ring(float r, float w)
{
    glPushMatrix();
        glTranslatef(0.0f, 0.0f, -0.5f * w);
        gluCylinder(quad, r, r, w, 28, 1);
    glPopMatrix();
}

/* Flat ring (or full disc when inner = 0) on both faces z = +-halfW.
 * GLU_INSIDE flips the normals of the -z face so it is lit correctly. */
static void faces(float inner, float outer, float halfW)
{
    glPushMatrix();
        glTranslatef(0.0f, 0.0f, -halfW);
        gluQuadricOrientation(quad, GLU_INSIDE);
        gluDisk(quad, inner, outer, 28, 1);
        gluQuadricOrientation(quad, GLU_OUTSIDE);
        glTranslatef(0.0f, 0.0f, 2.0f * halfW);
        gluDisk(quad, inner, outer, 28, 1);
    glPopMatrix();
}

/* count bars from radius r0 to r1, evenly spread around the axle. */
static void spokes(int count, float r0, float r1, float thick, float depth)
{
    float len = r1 - r0;
    for (int i = 0; i < count; i++) {
        glPushMatrix();
            glRotatef(i * 360.0f / count, 0.0f, 0.0f, 1.0f);
            glTranslatef(r0 + 0.5f * len, 0.0f, 0.0f);
            glScalef(len, thick, depth);
            glutSolidCube(1.0);
        glPopMatrix();
    }
}

/* Thin rim, narrow tyre and spokes, as on the earliest cars. */
static void spokedWheel(float r, float w, int wood)
{
    float hw = 0.5f * w;

    glColor3fv(TYRE_COL);                         /* narrow tyre */
    ring(r, w);
    faces(0.86f * r, r, hw);

    if (wood) glColor3f(0.55f, 0.36f, 0.18f);     /* rim (felloe) */
    else      glColor3f(0.45f, 0.45f, 0.48f);
    ring(0.78f * r, w);
    faces(0.78f * r, 0.86f * r, hw);

    if (wood) spokes(10, 0.10f * r, 0.80f * r, 0.09f * r, 0.5f * w);
    else      spokes(12, 0.10f * r, 0.80f * r, 0.02f,     0.02f);

    ring(0.12f * r, 1.6f * w);                    /* hub */
    faces(0.0f, 0.12f * r, 0.8f * w);
}

/* The original placeholder wheel: discs plus a cross bar. */
static void hubcapWheel(float r, float w)
{
    float hw = 0.5f * w;

    glColor3fv(TYRE_COL);
    ring(r, w);
    faces(0.7f * r, r, hw);

    glColor3f(0.75f, 0.75f, 0.78f);               /* hub caps */
    faces(0.0f, 0.7f * r, hw);

    glColor3f(0.15f, 0.15f, 0.15f);               /* cross bar shows rotation */
    box(0.0f, 0.0f, 0.0f, 1.3f * r, 0.07f, w + 0.02f);
    box(0.0f, 0.0f, 0.0f, 0.07f, 1.3f * r, w + 0.02f);
}

/* Modern alloy: dark tyre, silver rim ring and five silver spokes over
 * a dark centre, so the gaps between the spokes read clearly. */
static void alloyWheel(float r, float w)
{
    float hw = 0.5f * w;

    glColor3fv(TYRE_COL);
    ring(r, w);
    faces(0.68f * r, r, hw);

    glColor3f(0.22f, 0.22f, 0.24f);               /* dark wheel centre */
    faces(0.0f, 0.68f * r, hw);

    glColor3f(0.80f, 0.81f, 0.84f);               /* silver */
    faces(0.58f * r, 0.68f * r, hw + 0.003f);     /* rim lip */
    spokes(5, 0.08f * r, 0.62f * r, 0.16f * r, w + 0.012f);
    faces(0.0f, 0.14f * r, hw + 0.008f);          /* centre cap */
}

/* Draw one wheel centred on the origin, turned by angle degrees.
 * Rolling forward (+x) is a clockwise turn when seen from +z, which is
 * a NEGATIVE rotation about the axle. */
static void drawWheel(float r, float w, int style, float angle)
{
    glPushMatrix();
        glRotatef(-angle, 0.0f, 0.0f, 1.0f);
        switch (style) {
            case WIRE_SPOKES: spokedWheel(r, w, 0); break;
            case WOOD_SPOKES: spokedWheel(r, w, 1); break;
            case ALLOY:       alloyWheel(r, w);     break;
            default:          hubcapWheel(r, w);    break;
        }
    glPopMatrix();
}

/* Rolling without slipping: the tyre's arc length equals the distance
 * travelled, so angle (radians) = distance / radius. Each radius gets
 * its own angle, so small wheels spin faster than large ones. The
 * distance is a double so it stays precise during long runs; fmod
 * keeps the result between 0 and 360 degrees. */
static float wheelAngle(double distance, float radius)
{
    return (float)fmod(distance / radius * 180.0 / M_PI, 360.0);
}

static void placeWheel(const CarModel *m, int axle, float z, double distance)
{
    float r = m->wheelRadius[axle];
    glPushMatrix();
        glTranslatef(m->wheelX[axle], r, z);   /* wheel touches y = 0 */
        drawWheel(r, m->wheelWidth, m->wheelStyle, wheelAngle(distance, r));
    glPopMatrix();
}

/* Mudguard: a thin curved strip over a wheel, built from short boxes
 * laid along an arc (each box is turned to follow the curve). */
static void mudguard(float cx, float r, float z, float width)
{
    const int steps = 9;
    float R = r + 0.07f;
    float dTheta = (float)M_PI * 0.85f / steps;
    float seg = R * dTheta * 1.08f;               /* slight overlap, no gaps */

    for (int i = 0; i < steps; i++) {
        float th = (float)M_PI * 0.075f + (i + 0.5f) * dTheta;
        glPushMatrix();
            glTranslatef(cx + R * cosf(th), r + R * sinf(th), z);
            glRotatef(th * 180.0f / (float)M_PI, 0.0f, 0.0f, 1.0f);
            glScalef(0.03f, seg, width);          /* x is now radial */
            glutSolidCube(1.0);
        glPopMatrix();
    }
}

/* Chrome bumper bar across the full width of the car. */
static void bumper(const CarModel *m, float x, float y)
{
    glColor3fv(CHROME_COL);
    box(x, y, 0.0f, 0.07f, 0.08f, 2.0f * m->halfWidth + 0.06f);
}

/* ------------------------------------------------------------------ */
/* Era details: small extras built from simple primitives              */
/* ------------------------------------------------------------------ */

/* 1886: bench seat, tiller, front fork, rear axle and the big flat
 * flywheel of the single-cylinder engine at the back. */
static void details1886(const CarModel *m)
{
    glColor3f(0.60f, 0.10f, 0.10f);                    /* red leather seat */
    box(-0.30f, 0.92f, 0.0f, 0.55f, 0.12f, 0.80f);
    box(-0.58f, 1.12f, 0.0f, 0.08f, 0.48f, 0.80f);

    glColor3f(0.25f, 0.25f, 0.27f);                    /* dark metal */
    box(0.42f, 1.10f, 0.0f, 0.035f, 0.60f, 0.035f);    /* tiller post */
    box(0.36f, 1.40f, 0.0f, 0.14f, 0.035f, 0.035f);    /* tiller handle */

    float fx = m->wheelX[0], fr = m->wheelRadius[0];   /* front fork */
    strut(fx, fr, 0.86f, 0.98f,  0.06f, 0.03f, 0.03f);
    strut(fx, fr, 0.86f, 0.98f, -0.06f, 0.03f, 0.03f);

    box(m->wheelX[1], m->wheelRadius[1], 0.0f,          /* rear axle */
        0.05f, 0.05f, 2.0f * m->wheelTrack);

    glPushMatrix();                                    /* flywheel */
        glTranslatef(-0.95f, 0.88f, 0.0f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);           /* lie flat */
        gluCylinder(quad, 0.28, 0.28, 0.05, 24, 1);
        glTranslatef(0.0f, 0.0f, 0.05f);
        gluDisk(quad, 0.0, 0.28, 24, 1);
    glPopMatrix();
}

/* 1901: tall upright seats, steering column with a wheel, mudguards,
 * radiator core and two brass side lamps. */
static void details1901(const CarModel *m)
{
    glColor3f(0.40f, 0.15f, 0.10f);                    /* leather seats */
    box(-0.30f, 1.00f, 0.0f, 0.45f, 0.14f, 1.00f);     /* front cushion */
    box(-0.55f, 1.22f, 0.0f, 0.10f, 0.62f, 1.00f);     /* front back    */
    box(-1.40f, 1.22f, 0.0f, 0.45f, 0.12f, 0.95f);     /* rear cushion  */
    box(-1.75f, 1.36f, 0.0f, 0.12f, 0.48f, 1.00f);     /* rear back     */

    glColor3f(0.15f, 0.15f, 0.15f);
    strut(0.45f, 0.95f, 0.10f, 1.42f, 0.25f, 0.04f, 0.04f);  /* column */
    box(1.885f, 0.98f, 0.0f, 0.02f, 0.50f, 0.85f);     /* radiator core */

    /* Steering wheel: a torus lies in the xy plane around z. Turn z
     * onto x (90 degrees about y), then x onto the column direction. */
    glPushMatrix();
        glTranslatef(0.10f, 1.42f, 0.25f);
        glRotatef(atan2f(0.47f, -0.35f) * 180.0f / (float)M_PI, 0.0f, 0.0f, 1.0f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        glutSolidTorus(0.015, 0.17, 8, 20);
    glPopMatrix();

    glColor3f(0.05f, 0.05f, 0.05f);                    /* mudguards */
    for (int a = 0; a < 2; a++) {
        mudguard(m->wheelX[a], m->wheelRadius[a],  m->wheelTrack, 0.16f);
        mudguard(m->wheelX[a], m->wheelRadius[a], -m->wheelTrack, 0.16f);
    }

    glColor3f(0.80f, 0.65f, 0.30f);                    /* brass lamps */
    glPushMatrix(); glTranslatef(1.70f, 1.05f,  0.62f); glutSolidSphere(0.08, 12, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(1.70f, 1.05f, -0.62f); glutSolidSphere(0.08, 12, 10); glPopMatrix();
}

/* 1936: spare wheels resting on the running boards, big round
 * headlamps, a chrome radiator shell and bumpers. */
static void details1936(const CarModel *m)
{
    float spareR = 0.36f, spareW = 0.16f;
    float z = m->fenderOut + 0.5f * spareW;
    glPushMatrix(); glTranslatef(0.70f, 0.52f + spareR,  z); drawWheel(spareR, spareW, HUBCAP, 0.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.70f, 0.52f + spareR, -z); drawWheel(spareR, spareW, HUBCAP, 0.0f); glPopMatrix();

    glColor3f(0.95f, 0.93f, 0.80f);                    /* headlamps */
    glPushMatrix(); glTranslatef(2.30f, 0.95f,  0.50f); glutSolidSphere(0.13, 16, 12); glPopMatrix();
    glPushMatrix(); glTranslatef(2.30f, 0.95f, -0.50f); glutSolidSphere(0.13, 16, 12); glPopMatrix();

    glColor3fv(CHROME_COL);
    box(2.24f, 0.86f, 0.0f, 0.06f, 0.48f, 0.42f);      /* radiator shell */
    box( 2.62f, 0.48f, 0.0f, 0.07f, 0.08f, 1.50f);     /* bumpers */
    box(-2.66f, 0.50f, 0.0f, 0.07f, 0.08f, 1.30f);
}

/* 1954: chrome bumpers, a dark grille and two bulges on the bonnet. */
static void details1954(const CarModel *m)
{
    bumper(m,  2.27f, 0.44f);
    bumper(m, -2.30f, 0.40f);

    glColor3f(0.12f, 0.12f, 0.13f);
    box(2.22f, 0.56f, 0.0f, 0.05f, 0.14f, 0.70f);      /* wide low grille */

    glColor3fv(m->bodyColor);
    strut(0.95f, 0.94f, 1.70f, 0.855f,  0.33f, 0.06f, 0.16f);
    strut(0.95f, 0.94f, 1.70f, 0.855f, -0.33f, 0.06f, 0.16f);
}

/* 1963: chrome bumpers, upright grille and the raised roof edges that
 * make the hardtop look slightly concave (the "pagoda" roof). */
static void details1963(const CarModel *m)
{
    bumper(m,  2.17f, 0.53f);
    bumper(m, -2.18f, 0.40f);

    glColor3fv(CHROME_COL);
    box(2.12f, 0.70f, 0.0f, 0.06f, 0.16f, 0.55f);
    glColor3f(0.12f, 0.12f, 0.13f);
    box(2.15f, 0.70f, 0.0f, 0.02f, 0.12f, 0.45f);

    glColor3fv(m->bodyColor);
    float z = m->halfWidth - 0.06f;
    box(-0.42f, 1.315f,  z, 1.05f, 0.04f, 0.10f);
    box(-0.42f, 1.315f, -z, 1.05f, 0.04f, 0.10f);
}

/* 1976: chunky chrome bumpers and a tall chrome grille. */
static void details1976(const CarModel *m)
{
    bumper(m,  2.40f, 0.58f);
    bumper(m, -2.38f, 0.40f);
    glColor3fv(CHROME_COL);
    box(2.35f, 0.77f, 0.0f, 0.06f, 0.17f, 0.50f);
}

/* 1982: a smaller, lower grille. */
static void details1982(const CarModel *m)
{
    (void)m;
    glColor3fv(CHROME_COL);
    box(2.205f, 0.66f, 0.0f, 0.04f, 0.10f, 0.45f);
}

/* 1995: the twin oval headlamps, made from squashed spheres. */
static void details1995(const CarModel *m)
{
    (void)m;
    glColor3fv(CHROME_COL);
    box(2.385f, 0.69f, 0.0f, 0.04f, 0.12f, 0.40f);     /* grille */

    const float zs[4] = { 0.48f, 0.72f, -0.48f, -0.72f };
    glColor3f(0.95f, 0.95f, 0.85f);
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
            glTranslatef(2.373f, 0.68f, zs[i]);
            glScalef(0.04f, 0.06f, 0.11f);
            glutSolidSphere(1.0, 14, 10);
        glPopMatrix();
    }
}

/* 2002: grille between the headlamps. */
static void details2002(const CarModel *m)
{
    (void)m;
    glColor3fv(CHROME_COL);
    box(2.355f, 0.68f, 0.0f, 0.05f, 0.13f, 0.50f);
}

/* 2013: large upright grille. */
static void details2013(const CarModel *m)
{
    (void)m;
    glColor3fv(CHROME_COL);
    box(2.54f, 0.64f, 0.0f, 0.06f, 0.24f, 0.62f);
}

/* 2021: thin light strips across the nose and tail, drawn unlit so
 * they glow like LEDs. */
static void details2021(const CarModel *m)
{
    float w = 2.0f * m->halfWidth - 0.12f;
    glowBox( 2.36f, 0.765f, 0.0f, 0.06f, 0.035f, w, 0.85f, 0.95f, 1.00f);
    glowBox(-2.47f, 0.955f, 0.0f, 0.06f, 0.035f, w, 1.00f, 0.10f, 0.08f);
}

/* ------------------------------------------------------------------ */
/* Profile data                                                        */
/* Each list runs clockwise when seen from +z: up the tail, over the   */
/* roof to the nose, then back along the underside. The third value   */
/* styles the edge from that point to the next one.                    */
/* ------------------------------------------------------------------ */
#define BOD SEG_BODY
#define GLS SEG_GLASS
#define CHR SEG_CHROME
#define TRM SEG_TRIM
#define LMP SEG_LAMP
#define ARC SEG_ARCH

#define COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define PROFILE(a) .profile = (a), .numPoints = COUNT(a)
#define WINDOW(a)  .window  = (a), .numWindow = COUNT(a)
#define FENDER(a)  .fender  = (a), .numFender = COUNT(a)

/* 1886 Benz Patent-Motorwagen: a small open carriage tub. */
static const ProfilePt p1886[] = {
    {-1.12f, 0.48f, BOD}, {-1.18f, 0.58f, BOD}, {-1.20f, 0.80f, BOD},
    {-1.10f, 0.86f, BOD}, {-0.60f, 0.86f, BOD}, {-0.30f, 0.80f, BOD},
    { 0.15f, 0.78f, BOD}, { 0.45f, 0.80f, BOD}, { 0.68f, 0.88f, BOD},
    { 0.82f, 1.00f, BOD}, { 0.90f, 1.04f, BOD}, { 0.88f, 0.96f, BOD},
    { 0.76f, 0.78f, BOD}, { 0.60f, 0.60f, BOD}, { 0.38f, 0.50f, BOD},
    {-0.30f, 0.46f, BOD},
};

/* 1901 Mercedes 35 HP: low seat wells, long bonnet, radiator at the front. */
static const ProfilePt p1901[] = {
    {-1.85f, 0.60f, BOD}, {-1.92f, 0.80f, BOD}, {-1.88f, 1.02f, BOD},
    {-1.70f, 1.14f, BOD}, {-1.15f, 1.16f, BOD}, {-0.90f, 1.08f, BOD},
    {-0.80f, 0.92f, BOD}, { 0.10f, 0.92f, BOD}, { 0.40f, 0.98f, BOD},
    { 0.52f, 1.20f, BOD}, { 0.66f, 1.24f, BOD}, { 1.62f, 1.20f, BOD},
    { 1.72f, 1.30f, CHR}, { 1.86f, 1.30f, CHR}, { 1.88f, 0.66f, BOD},
    { 1.75f, 0.58f, BOD}, { 0.60f, 0.58f, BOD}, {-1.60f, 0.58f, BOD},
};

/* 1936 540K: long bonnet, low cabin set far back, rounded tail. */
static const ProfilePt p1936[] = {
    {-2.40f, 0.50f, BOD}, {-2.58f, 0.62f, LMP}, {-2.62f, 0.78f, BOD},
    {-2.50f, 0.96f, BOD}, {-2.20f, 1.10f, BOD}, {-1.75f, 1.17f, GLS},
    {-1.35f, 1.44f, BOD}, {-0.65f, 1.50f, BOD}, {-0.18f, 1.47f, GLS},
    { 0.02f, 1.12f, BOD}, { 0.60f, 1.10f, BOD}, { 2.05f, 1.06f, BOD},
    { 2.16f, 1.10f, CHR}, { 2.26f, 0.62f, BOD}, { 2.20f, 0.50f, BOD},
    { 0.00f, 0.48f, BOD},
};
static const ProfilePt w1936[] = {
    {-1.62f, 1.18f, 0}, {-1.32f, 1.39f, 0}, {-0.65f, 1.44f, 0},
    {-0.24f, 1.42f, 0}, {-0.06f, 1.16f, 0},
};
/* Its sweeping fenders and running boards, as a separate outline. */
static const ProfilePt f1936[] = {
    {-2.25f, 0.48f, BOD}, {-2.30f, 0.56f, BOD}, {-2.05f, 0.82f, BOD},
    {-1.70f, 0.96f, BOD}, {-1.35f, 0.99f, BOD}, {-1.05f, 0.90f, BOD},
    {-0.85f, 0.66f, BOD}, {-0.70f, 0.52f, BOD}, { 0.75f, 0.52f, BOD},
    { 1.05f, 0.72f, BOD}, { 1.35f, 0.92f, BOD}, { 1.70f, 0.98f, BOD},
    { 2.05f, 0.88f, BOD}, { 2.38f, 0.68f, BOD}, { 2.55f, 0.52f, BOD},
    { 2.52f, 0.46f, BOD}, { 2.11f, 0.46f, ARC}, { 1.19f, 0.46f, BOD},
    { 0.90f, 0.40f, BOD}, {-0.70f, 0.40f, BOD}, {-0.94f, 0.46f, ARC},
    {-1.86f, 0.46f, BOD},
};

/* 1954 300 SL: low coupe, long bonnet, short rounded tail. */
static const ProfilePt p1954[] = {
    {-2.25f, 0.32f, CHR}, {-2.28f, 0.45f, BOD}, {-2.22f, 0.62f, LMP},
    {-2.12f, 0.75f, BOD}, {-1.90f, 0.86f, BOD}, {-1.45f, 0.92f, GLS},
    {-0.85f, 1.25f, BOD}, {-0.15f, 1.30f, BOD}, { 0.30f, 1.26f, GLS},
    { 0.75f, 0.95f, BOD}, { 1.40f, 0.88f, BOD}, { 1.95f, 0.78f, BOD},
    { 2.18f, 0.66f, LMP}, { 2.25f, 0.52f, CHR}, { 2.24f, 0.36f, BOD},
    { 2.10f, 0.30f, BOD}, { 1.59f, 0.30f, ARC}, { 0.81f, 0.30f, BOD},
    {-0.81f, 0.30f, ARC}, {-1.59f, 0.30f, BOD}, {-2.10f, 0.30f, BOD},
};
static const ProfilePt w1954[] = {
    {-1.30f, 0.95f, 0}, {-0.85f, 1.19f, 0}, {-0.15f, 1.23f, 0},
    { 0.22f, 1.20f, 0}, { 0.62f, 0.96f, 0},
};

/* 1963 230 SL: flat hardtop, upright windscreen and nose. */
static const ProfilePt p1963[] = {
    {-2.08f, 0.30f, CHR}, {-2.16f, 0.42f, BOD}, {-2.15f, 0.56f, LMP},
    {-2.10f, 0.74f, BOD}, {-1.70f, 0.80f, BOD}, {-1.15f, 0.84f, GLS},
    {-0.95f, 1.28f, BOD}, { 0.10f, 1.30f, BOD}, { 0.25f, 1.28f, GLS},
    { 0.55f, 0.88f, BOD}, { 1.70f, 0.82f, BOD}, { 2.05f, 0.78f, LMP},
    { 2.12f, 0.62f, CHR}, { 2.15f, 0.48f, BOD}, { 2.10f, 0.32f, BOD},
    { 1.59f, 0.30f, ARC}, { 0.81f, 0.30f, BOD}, {-0.81f, 0.30f, ARC},
    {-1.59f, 0.30f, BOD},
};
static const ProfilePt w1963[] = {
    {-0.95f, 0.88f, 0}, {-0.88f, 1.22f, 0}, { 0.08f, 1.24f, 0},
    { 0.20f, 1.22f, 0}, { 0.45f, 0.90f, 0},
};

/* 1976 W123: upright three-box saloon with a tall glasshouse. */
static const ProfilePt p1976[] = {
    {-2.30f, 0.30f, CHR}, {-2.37f, 0.48f, BOD}, {-2.35f, 0.58f, LMP},
    {-2.33f, 0.86f, BOD}, {-1.85f, 0.92f, BOD}, {-1.45f, 0.94f, GLS},
    {-1.05f, 1.40f, BOD}, { 0.25f, 1.44f, BOD}, { 0.45f, 1.42f, GLS},
    { 0.95f, 0.98f, BOD}, { 2.20f, 0.90f, BOD}, { 2.32f, 0.86f, LMP},
    { 2.33f, 0.66f, CHR}, { 2.38f, 0.50f, BOD}, { 2.30f, 0.32f, BOD},
    { 1.81f, 0.30f, ARC}, { 1.03f, 0.30f, BOD}, {-0.99f, 0.30f, ARC},
    {-1.77f, 0.30f, BOD},
};
static const ProfilePt w1976[] = {
    {-1.35f, 0.98f, 0}, {-1.00f, 1.34f, 0}, { 0.25f, 1.38f, 0},
    { 0.40f, 1.36f, 0}, { 0.85f, 1.00f, 0},
};

/* 1982 190E: compact wedge, low nose and high tail. */
static const ProfilePt p1982[] = {
    {-2.15f, 0.32f, TRM}, {-2.20f, 0.52f, BOD}, {-2.20f, 0.66f, LMP},
    {-2.17f, 0.92f, BOD}, {-1.70f, 0.96f, BOD}, {-1.35f, 0.98f, GLS},
    {-0.95f, 1.35f, BOD}, { 0.15f, 1.38f, BOD}, { 0.35f, 1.36f, GLS},
    { 0.90f, 0.92f, BOD}, { 2.05f, 0.80f, BOD}, { 2.18f, 0.74f, LMP},
    { 2.20f, 0.58f, TRM}, { 2.20f, 0.40f, BOD}, { 2.12f, 0.30f, BOD},
    { 1.73f, 0.28f, ARC}, { 0.97f, 0.28f, BOD}, {-0.93f, 0.28f, ARC},
    {-1.69f, 0.28f, BOD},
};
static const ProfilePt w1982[] = {
    {-1.25f, 1.00f, 0}, {-0.92f, 1.30f, 0}, { 0.15f, 1.32f, 0},
    { 0.32f, 1.30f, 0}, { 0.80f, 0.96f, 0},
};

/* 1995 W210: rounder saloon with a smooth roofline. */
static const ProfilePt p1995[] = {
    {-2.30f, 0.34f, TRM}, {-2.40f, 0.55f, BOD}, {-2.40f, 0.70f, LMP},
    {-2.34f, 0.90f, BOD}, {-2.10f, 0.98f, BOD}, {-1.65f, 1.02f, GLS},
    {-1.20f, 1.36f, BOD}, {-0.70f, 1.43f, BOD}, { 0.10f, 1.44f, BOD},
    { 0.30f, 1.41f, GLS}, { 0.95f, 0.98f, BOD}, { 1.80f, 0.88f, BOD},
    { 2.22f, 0.80f, BOD}, { 2.36f, 0.72f, LMP}, { 2.40f, 0.60f, TRM},
    { 2.38f, 0.40f, BOD}, { 2.28f, 0.32f, BOD}, { 1.79f, 0.30f, ARC},
    { 1.01f, 0.30f, BOD}, {-1.04f, 0.30f, ARC}, {-1.82f, 0.30f, BOD},
};
static const ProfilePt w1995[] = {
    {-1.55f, 1.05f, 0}, {-1.15f, 1.31f, 0}, {-0.65f, 1.38f, 0},
    { 0.10f, 1.39f, 0}, { 0.25f, 1.37f, 0}, { 0.85f, 1.02f, 0},
};

/* 2002 W211: smoother, with a long sloping rear window. */
static const ProfilePt p2002[] = {
    {-2.32f, 0.34f, TRM}, {-2.40f, 0.56f, BOD}, {-2.38f, 0.74f, LMP},
    {-2.30f, 0.94f, BOD}, {-2.05f, 1.00f, BOD}, {-1.80f, 1.02f, GLS},
    {-1.05f, 1.40f, BOD}, {-0.40f, 1.45f, BOD}, { 0.10f, 1.43f, GLS},
    { 0.95f, 0.96f, BOD}, { 1.80f, 0.86f, BOD}, { 2.25f, 0.76f, LMP},
    { 2.38f, 0.64f, TRM}, { 2.40f, 0.44f, BOD}, { 2.28f, 0.32f, BOD},
    { 1.82f, 0.30f, ARC}, { 1.02f, 0.30f, BOD}, {-1.03f, 0.30f, ARC},
    {-1.83f, 0.30f, BOD},
};
static const ProfilePt w2002[] = {
    {-1.65f, 1.05f, 0}, {-1.02f, 1.36f, 0}, {-0.40f, 1.40f, 0},
    { 0.05f, 1.38f, 0}, { 0.80f, 1.01f, 0},
};

/* 2013 S-Class W222: long and elegant, long roofline. */
static const ProfilePt p2013[] = {
    {-2.45f, 0.36f, TRM}, {-2.55f, 0.58f, BOD}, {-2.53f, 0.76f, LMP},
    {-2.45f, 0.98f, BOD}, {-2.15f, 1.04f, BOD}, {-1.85f, 1.06f, GLS},
    {-1.20f, 1.44f, BOD}, {-0.40f, 1.50f, BOD}, { 0.20f, 1.48f, GLS},
    { 1.00f, 1.00f, BOD}, { 2.00f, 0.92f, BOD}, { 2.38f, 0.86f, LMP},
    { 2.50f, 0.76f, BOD}, { 2.55f, 0.52f, TRM}, { 2.52f, 0.38f, BOD},
    { 2.40f, 0.33f, BOD}, { 1.97f, 0.32f, ARC}, { 1.13f, 0.32f, BOD},
    {-1.06f, 0.32f, ARC}, {-1.90f, 0.32f, BOD},
};
static const ProfilePt w2013[] = {
    {-1.72f, 1.09f, 0}, {-1.17f, 1.40f, 0}, {-0.40f, 1.45f, 0},
    { 0.15f, 1.43f, 0}, { 0.86f, 1.05f, 0},
};

/* 2021 EQS: one continuous arc from nose to tail, very short bonnet. */
static const ProfilePt p2021[] = {
    {-2.50f, 0.38f, TRM}, {-2.60f, 0.62f, BOD}, {-2.60f, 0.80f, BOD},
    {-2.52f, 0.92f, LMP}, {-2.40f, 0.98f, BOD}, {-2.05f, 1.08f, GLS},
    {-1.20f, 1.42f, BOD}, {-0.40f, 1.50f, BOD}, { 0.20f, 1.47f, GLS},
    { 1.35f, 0.98f, BOD}, { 1.95f, 0.86f, BOD}, { 2.35f, 0.76f, LMP},
    { 2.52f, 0.66f, TRM}, { 2.60f, 0.52f, BOD}, { 2.58f, 0.40f, TRM},
    { 2.45f, 0.33f, BOD}, { 2.11f, 0.32f, ARC}, { 1.25f, 0.32f, BOD},
    {-1.10f, 0.32f, ARC}, {-1.96f, 0.32f, BOD},
};
static const ProfilePt w2021[] = {
    {-1.85f, 1.12f, 0}, {-1.15f, 1.38f, 0}, {-0.40f, 1.45f, 0},
    { 0.15f, 1.43f, 0}, { 1.15f, 1.04f, 0},
};

/* ------------------------------------------------------------------ */
/* The model table: one entry per era, all drawn by the same code      */
/* ------------------------------------------------------------------ */
static CarModel models[] = {
    { .year = "1886", .name = "Benz Patent-Motorwagen",
      PROFILE(p1886), .halfWidth = 0.42f, .bodyColor = {0.08f, 0.08f, 0.08f},
      .numWheels = 3, .wheelX = {1.00f, -0.70f}, .wheelRadius = {0.33f, 0.56f},
      .wheelWidth = 0.06f, .wheelTrack = 0.64f, .wheelStyle = WIRE_SPOKES,
      .drawDetails = details1886 },

    { .year = "1901", .name = "Mercedes 35 HP",
      PROFILE(p1901), .halfWidth = 0.55f, .bodyColor = {0.08f, 0.30f, 0.18f},
      .numWheels = 4, .wheelX = {1.25f, -1.10f}, .wheelRadius = {0.45f, 0.47f},
      .wheelWidth = 0.10f, .wheelTrack = 0.75f, .wheelStyle = WOOD_SPOKES,
      .drawDetails = details1901 },

    { .year = "1936", .name = "540K",
      PROFILE(p1936), .halfWidth = 0.70f, .bodyColor = {0.90f, 0.86f, 0.72f},
      WINDOW(w1936),
      FENDER(f1936), .fenderIn = 0.70f, .fenderOut = 0.97f,
      .fenderColor = {0.06f, 0.06f, 0.07f},
      .numWheels = 4, .wheelX = {1.65f, -1.40f}, .wheelRadius = {0.40f, 0.40f},
      .wheelWidth = 0.18f, .wheelTrack = 0.82f, .wheelStyle = HUBCAP,
      .drawDetails = details1936 },

    { .year = "1954", .name = "300 SL Gullwing",
      PROFILE(p1954), .halfWidth = 0.89f, .bodyColor = {0.75f, 0.76f, 0.78f},
      WINDOW(w1954),
      .numWheels = 4, .wheelX = {1.20f, -1.20f}, .wheelRadius = {0.33f, 0.33f},
      .wheelWidth = 0.20f, .wheelTrack = 0.80f, .wheelStyle = HUBCAP,
      .drawDetails = details1954 },

    { .year = "1963", .name = "230 SL Pagoda",
      PROFILE(p1963), .halfWidth = 0.88f, .bodyColor = {0.93f, 0.93f, 0.90f},
      WINDOW(w1963),
      .numWheels = 4, .wheelX = {1.20f, -1.20f}, .wheelRadius = {0.33f, 0.33f},
      .wheelWidth = 0.20f, .wheelTrack = 0.79f, .wheelStyle = HUBCAP,
      .drawDetails = details1963 },

    { .year = "1976", .name = "W123 E-Class",
      PROFILE(p1976), .halfWidth = 0.89f, .bodyColor = {0.82f, 0.74f, 0.58f},
      WINDOW(w1976), .pillarX = -0.30f, .pillarW = 0.10f,
      .numWheels = 4, .wheelX = {1.42f, -1.38f}, .wheelRadius = {0.33f, 0.33f},
      .wheelWidth = 0.21f, .wheelTrack = 0.80f, .wheelStyle = HUBCAP,
      .drawDetails = details1976 },

    { .year = "1982", .name = "190E",
      PROFILE(p1982), .halfWidth = 0.85f, .bodyColor = {0.10f, 0.15f, 0.35f},
      WINDOW(w1982), .pillarX = -0.30f, .pillarW = 0.10f,
      .numWheels = 4, .wheelX = {1.35f, -1.31f}, .wheelRadius = {0.32f, 0.32f},
      .wheelWidth = 0.20f, .wheelTrack = 0.76f, .wheelStyle = ALLOY,
      .drawDetails = details1982 },

    { .year = "1995", .name = "E-Class W210",
      PROFILE(p1995), .halfWidth = 0.90f, .bodyColor = {0.58f, 0.66f, 0.60f},
      WINDOW(w1995), .pillarX = -0.40f, .pillarW = 0.10f,
      .numWheels = 4, .wheelX = {1.40f, -1.43f}, .wheelRadius = {0.33f, 0.33f},
      .wheelWidth = 0.22f, .wheelTrack = 0.80f, .wheelStyle = ALLOY,
      .drawDetails = details1995 },

    { .year = "2002", .name = "E-Class W211",
      PROFILE(p2002), .halfWidth = 0.91f, .bodyColor = {0.72f, 0.73f, 0.75f},
      WINDOW(w2002), .pillarX = -0.40f, .pillarW = 0.10f,
      .numWheels = 4, .wheelX = {1.42f, -1.43f}, .wheelRadius = {0.34f, 0.34f},
      .wheelWidth = 0.23f, .wheelTrack = 0.81f, .wheelStyle = ALLOY,
      .drawDetails = details2002 },

    { .year = "2013", .name = "S-Class W222",
      PROFILE(p2013), .halfWidth = 0.95f, .bodyColor = {0.10f, 0.10f, 0.11f},
      WINDOW(w2013), .pillarX = -0.45f, .pillarW = 0.10f,
      .numWheels = 4, .wheelX = {1.55f, -1.48f}, .wheelRadius = {0.36f, 0.36f},
      .wheelWidth = 0.25f, .wheelTrack = 0.84f, .wheelStyle = ALLOY,
      .drawDetails = details2013 },

    { .year = "2021", .name = "EQS",
      PROFILE(p2021), .halfWidth = 0.97f, .bodyColor = {0.88f, 0.89f, 0.91f},
      WINDOW(w2021), .pillarX = -0.45f, .pillarW = 0.10f,
      .numWheels = 4, .wheelX = {1.68f, -1.53f}, .wheelRadius = {0.37f, 0.37f},
      .wheelWidth = 0.26f, .wheelTrack = 0.85f, .wheelStyle = ALLOY,
      .drawDetails = details2021 },
};

/* ------------------------------------------------------------------ */
/* Public interface                                                    */
/* ------------------------------------------------------------------ */
void initCars(void)
{
    quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    initTessellator();

    for (int i = 0; i < COUNT(models); i++)
        buildBodyList(&models[i]);
}

int carModelCount(void) { return COUNT(models); }

const CarModel *getCarModel(int index)
{
    if (index < 0 || index >= COUNT(models)) index = 0;
    return &models[index];
}

void drawCar(const CarModel *m, double distance)
{
    glCallList(m->bodyList);   /* body, fenders and windows in one call */

    /* Rear axle always has a wheel on each side. */
    placeWheel(m, 1,  m->wheelTrack, distance);
    placeWheel(m, 1, -m->wheelTrack, distance);

    if (m->numWheels == 3) {
        placeWheel(m, 0, 0.0f, distance);   /* single centre front wheel */
    } else {
        placeWheel(m, 0,  m->wheelTrack, distance);
        placeWheel(m, 0, -m->wheelTrack, distance);
    }

    if (m->drawDetails) m->drawDetails(m);
}
