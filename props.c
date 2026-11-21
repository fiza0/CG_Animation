#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "props.h"

#define TURBINE_RATE  40.0f    /* rotor speed in degrees per second        */
#define SKY_HALF     450.0f    /* skyline spans followX - 450 .. + 450     */

static GLUquadric *quad = NULL;
static GLuint propLists[PROP_COUNT];
static GLuint skyLists[SKYLINE_COUNT];
static float  rotorAngle = 0.0f;

/* ------------------------------------------------------------------ */
/* Primitive helpers                                                   */
/* ------------------------------------------------------------------ */

/* Box from its centre and size; glutSolidCube supplies the normals. */
static void box(float cx, float cy, float cz, float sx, float sy, float sz)
{
    glPushMatrix();
        glTranslatef(cx, cy, cz);
        glScalef(sx, sy, sz);
        glutSolidCube(1.0);
    glPopMatrix();
}

/* Upright (optionally tapering) cylinder standing at height y0, with a
 * lid. GLU builds cylinders along +z; -90 degrees about x stands it up. */
static void column(float x, float z, float r0, float r1, float h, float y0)
{
    glPushMatrix();
        glTranslatef(x, y0, z);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(quad, r0, r1, h, 14, 1);
        glTranslatef(0.0f, 0.0f, h);
        gluDisk(quad, 0.0, r1, 14, 1);
    glPopMatrix();
}

static void sphereAt(float x, float y, float z, float r)
{
    glPushMatrix();
        glTranslatef(x, y, z);
        glutSolidSphere(r, 14, 10);
    glPopMatrix();
}

/* Cone with its base at height y and its tip pointing up. */
static void coneUp(float x, float y, float z, float r, float h)
{
    glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glutSolidCone(r, h, 14, 3);
    glPopMatrix();
}

/* Flat panel facing +z (doors, windows, posters), drawn just in front
 * of a wall so it does not z-fight with it. */
static void frontQuad(float x0, float y0, float x1, float y1, float z)
{
    glNormal3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
        glVertex3f(x0, y0, z);  glVertex3f(x1, y0, z);
        glVertex3f(x1, y1, z);  glVertex3f(x0, y1, z);
    glEnd();
}

/* Flat panel facing +x (nx = 1) or -x (nx = -1). */
static void sideQuad(float x, float z0, float z1, float y0, float y1, float nx)
{
    glNormal3f(nx, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex3f(x, y0, z0);  glVertex3f(x, y1, z0);
        glVertex3f(x, y1, z1);  glVertex3f(x, y0, z1);
    glEnd();
}

/* Unlit box for lamps and lights: with lighting off the pixel colour is
 * exactly the colour given, so it looks like it is glowing. */
static void glowBox(float cx, float cy, float cz, float sx, float sy, float sz,
                    float r, float g, float b)
{
    glDisable(GL_LIGHTING);
    glColor3f(r, g, b);
    box(cx, cy, cz, sx, sy, sz);
    glEnable(GL_LIGHTING);
}

static void glowSphere(float x, float y, float z, float rad,
                       float r, float g, float b)
{
    glDisable(GL_LIGHTING);
    glColor3f(r, g, b);
    sphereAt(x, y, z, rad);
    glEnable(GL_LIGHTING);
}

/* Pitched roof: a triangular prism along x, centred on x = z = 0.
 * Each sloping face's normal is perpendicular to the slope, leaning
 * outwards and upwards; the two gable ends face +x and -x. */
static void gableRoof(float lenX, float depthZ, float h, float y0)
{
    float hx = 0.5f * lenX, hz = 0.5f * depthZ, top = y0 + h;
    float len = sqrtf(hz * hz + h * h);
    float ny = hz / len, nz = h / len;

    glBegin(GL_QUADS);
        glNormal3f(0.0f, ny, nz);                       /* front slope */
        glVertex3f(-hx, y0, hz);  glVertex3f(hx, y0, hz);
        glVertex3f(hx, top, 0.0f); glVertex3f(-hx, top, 0.0f);
        glNormal3f(0.0f, ny, -nz);                      /* back slope  */
        glVertex3f(-hx, y0, -hz); glVertex3f(-hx, top, 0.0f);
        glVertex3f(hx, top, 0.0f); glVertex3f(hx, y0, -hz);
    glEnd();
    glBegin(GL_TRIANGLES);
        glNormal3f(1.0f, 0.0f, 0.0f);                   /* gable ends  */
        glVertex3f(hx, y0, hz);  glVertex3f(hx, y0, -hz); glVertex3f(hx, top, 0.0f);
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(-hx, y0, -hz); glVertex3f(-hx, y0, hz); glVertex3f(-hx, top, 0.0f);
    glEnd();
}

/* ------------------------------------------------------------------ */
/* Row props: each covers x = 0 .. 12 so the next slot continues it    */
/* ------------------------------------------------------------------ */
static void buildFence(void)
{
    glColor3f(0.50f, 0.36f, 0.22f);
    for (int i = 0; i < 4; i++)                         /* posts every 3 m */
        box(i * 3.0f, 0.55f, 0.0f, 0.12f, 1.1f, 0.12f);
    box(6.0f, 0.45f, 0.0f, 12.0f, 0.08f, 0.05f);        /* two rails */
    box(6.0f, 0.90f, 0.0f, 12.0f, 0.08f, 0.05f);
}

/* Telegraph pole with wires to the next pole. A hanging wire follows
 * a curve; a parabola y = top - sag * 4t(1 - t) is a close match. */
static void buildTelegraph(void)
{
    const float wireZ[4] = { -0.6f, -0.25f, 0.25f, 0.6f };
    const float top = 6.75f, sag = 0.45f;

    glColor3f(0.42f, 0.30f, 0.20f);
    column(0.0f, 0.0f, 0.13f, 0.10f, 7.0f, 0.0f);
    box(0.0f, 6.6f, 0.0f, 0.12f, 0.12f, 1.5f);          /* cross arm */

    glColor3f(0.85f, 0.85f, 0.80f);                     /* insulators */
    for (int i = 0; i < 4; i++) box(0.0f, 6.71f, wireZ[i], 0.07f, 0.1f, 0.07f);

    glDisable(GL_LIGHTING);                             /* thin lines, unlit */
    glColor3f(0.12f, 0.12f, 0.12f);
    for (int i = 0; i < 4; i++) {
        glBegin(GL_LINE_STRIP);
        for (int s = 0; s <= 12; s++) {
            float t = s / 12.0f;
            glVertex3f(SLOT_LENGTH * t, top - sag * 4.0f * t * (1.0f - t), wireZ[i]);
        }
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

static void buildStoneWall(void)
{
    glColor3f(0.55f, 0.52f, 0.46f);
    box(6.0f, 0.40f, 0.0f, 12.0f, 0.8f, 0.5f);
    glColor3f(0.45f, 0.43f, 0.38f);                     /* coping stones */
    box(6.0f, 0.85f, 0.0f, 12.0f, 0.1f, 0.62f);
}

/* Crash barrier: posts with a steel rail on the road side (+z). */
static void buildBarrier(void)
{
    glColor3f(0.45f, 0.45f, 0.47f);
    for (int i = 0; i < 3; i++) box(i * 4.0f, 0.38f, 0.28f, 0.12f, 0.76f, 0.12f);
    glColor3f(0.75f, 0.77f, 0.80f);
    box(6.0f, 0.58f, 0.40f, 12.0f, 0.32f, 0.06f);
}

/* ------------------------------------------------------------------ */
/* Countryside                                                         */
/* ------------------------------------------------------------------ */
static void buildHaystack(void)
{
    glColor3f(0.80f, 0.68f, 0.36f);
    column(0.0f, 0.0f, 1.2f, 1.2f, 1.0f, 0.0f);
    coneUp(0.0f, 1.0f, 0.0f, 1.3f, 1.5f);
}

static void buildCottage(void)
{
    glColor3f(0.86f, 0.82f, 0.72f);                     /* white-washed walls */
    box(0.0f, 1.3f, 0.0f, 5.0f, 2.6f, 4.0f);
    glColor3f(0.45f, 0.28f, 0.18f);
    gableRoof(5.6f, 4.6f, 1.8f, 2.6f);
    glColor3f(0.35f, 0.30f, 0.28f);
    box(1.5f, 3.6f, -0.6f, 0.5f, 1.4f, 0.5f);           /* chimney */
    glColor3f(0.30f, 0.20f, 0.12f);
    frontQuad(-0.4f, 0.0f, 0.4f, 1.8f, 2.01f);          /* door */
    glColor3f(0.20f, 0.24f, 0.30f);
    frontQuad(-1.9f, 1.1f, -1.0f, 1.9f, 2.01f);         /* windows */
    frontQuad( 1.0f, 1.1f,  1.9f, 1.9f, 2.01f);
}

static void buildRoundTree(void)
{
    glColor3f(0.40f, 0.26f, 0.13f);
    column(0.0f, 0.0f, 0.22f, 0.16f, 2.4f, 0.0f);
    glColor3f(0.22f, 0.44f, 0.16f);                     /* clustered leaves */
    sphereAt( 0.0f, 3.2f,  0.0f, 1.6f);
    sphereAt( 0.8f, 3.8f,  0.3f, 1.1f);
    sphereAt(-0.7f, 3.6f, -0.4f, 1.0f);
}

/* The original roadside tree: trunk plus a cone of foliage. */
static void buildConeTree(void)
{
    glColor3f(0.40f, 0.26f, 0.13f);
    column(0.0f, 0.0f, 0.25f, 0.20f, 2.0f, 0.0f);
    glColor3f(0.13f, 0.40f, 0.15f);
    coneUp(0.0f, 1.5f, 0.0f, 1.4f, 3.5f);
}

static void buildBush(void)
{
    glColor3f(0.22f, 0.42f, 0.18f);
    sphereAt(0.0f, 0.5f, 0.0f, 0.8f);                   /* lower half hides in the ground */
    sphereAt(0.6f, 0.4f, 0.2f, 0.6f);
}

/* ------------------------------------------------------------------ */
/* Street lights through the ages                                      */
/* ------------------------------------------------------------------ */
static void buildGasLamp(void)
{
    glColor3f(0.08f, 0.08f, 0.08f);
    column(0.0f, 0.0f, 0.06f, 0.05f, 3.2f, 0.0f);
    box(0.0f, 3.35f, 0.0f, 0.40f, 0.10f, 0.40f);        /* lantern base */
    box(0.0f, 3.85f, 0.0f, 0.44f, 0.08f, 0.44f);        /* lantern top  */
    coneUp(0.0f, 3.89f, 0.0f, 0.30f, 0.25f);
    for (int i = 0; i < 4; i++)                         /* corner frames */
        box((i & 1) ? 0.17f : -0.17f, 3.6f, (i & 2) ? 0.17f : -0.17f,
            0.03f, 0.42f, 0.03f);
    glowBox(0.0f, 3.6f, 0.0f, 0.30f, 0.42f, 0.30f, 1.0f, 0.82f, 0.45f);
}

static void buildElectricLamp(void)
{
    glColor3f(0.18f, 0.24f, 0.20f);
    column(0.0f, 0.0f, 0.10f, 0.07f, 5.0f, 0.0f);
    box(0.0f, 5.0f, 0.6f, 0.07f, 0.07f, 1.25f);         /* arm towards the road */
    coneUp(0.0f, 4.78f, 1.2f, 0.32f, 0.25f);            /* shade */
    glowSphere(0.0f, 4.72f, 1.2f, 0.12f, 1.0f, 0.93f, 0.75f);
}

/* The original lamp post: grey pole and a round head. */
static void buildLampPost(void)
{
    glColor3f(0.35f, 0.35f, 0.38f);
    column(0.0f, 0.0f, 0.08f, 0.08f, 5.0f, 0.0f);
    glColor3f(0.95f, 0.95f, 0.80f);
    sphereAt(0.0f, 5.0f, 0.0f, 0.25f);
}

/* Tall concrete lamp with an orange glowing head over the road. */
static void buildSodiumLamp(void)
{
    glColor3f(0.62f, 0.62f, 0.60f);
    column(0.0f, 0.0f, 0.14f, 0.09f, 9.0f, 0.0f);
    box(0.0f, 8.95f, 1.1f, 0.10f, 0.10f, 2.2f);
    glColor3f(0.35f, 0.35f, 0.35f);
    box(0.0f, 8.95f, 2.3f, 0.40f, 0.18f, 0.80f);
    glowBox(0.0f, 8.84f, 2.3f, 0.32f, 0.05f, 0.66f, 1.0f, 0.60f, 0.15f);
}

static void buildModernLamp(void)
{
    glColor3f(0.88f, 0.88f, 0.90f);
    column(0.0f, 0.0f, 0.11f, 0.08f, 8.0f, 0.0f);
    box(0.0f, 7.95f, 0.8f, 0.08f, 0.08f, 1.6f);
    box(0.0f, 7.95f, 1.75f, 0.30f, 0.12f, 0.70f);
    glowBox(0.0f, 7.88f, 1.75f, 0.24f, 0.03f, 0.60f, 0.95f, 0.97f, 1.0f);
}

static void buildLedLamp(void)
{
    glColor3f(0.30f, 0.31f, 0.33f);
    column(0.0f, 0.0f, 0.07f, 0.05f, 8.0f, 0.0f);
    box(0.0f, 7.98f, 0.7f, 0.06f, 0.06f, 1.4f);
    box(0.0f, 7.98f, 1.5f, 0.22f, 0.05f, 0.60f);
    glowBox(0.0f, 7.95f, 1.5f, 0.18f, 0.02f, 0.50f, 0.90f, 0.95f, 1.0f);
}

/* ------------------------------------------------------------------ */
/* Roadside furniture                                                  */
/* ------------------------------------------------------------------ */
static void buildPetrolPump(void)
{
    glColor3f(0.30f, 0.30f, 0.30f);
    box(0.0f, 0.08f, 0.0f, 0.9f, 0.16f, 0.8f);          /* plinth */
    glColor3f(0.75f, 0.15f, 0.12f);
    box(0.0f, 1.0f, 0.0f, 0.6f, 1.7f, 0.5f);            /* body   */
    box(0.0f, 2.0f, 0.0f, 0.3f, 0.3f, 0.3f);            /* neck   */
    glColor3f(0.95f, 0.95f, 0.90f);
    frontQuad(-0.2f, 1.3f, 0.2f, 1.7f, 0.251f);         /* dial   */
    glColor3f(0.10f, 0.10f, 0.10f);
    box(0.36f, 1.1f, 0.1f, 0.06f, 0.9f, 0.06f);         /* hose   */
    glowSphere(0.0f, 2.4f, 0.0f, 0.24f, 1.0f, 0.95f, 0.85f);   /* globe */
}

/* Billboard: plain coloured panels only, no text or logos. */
static void buildBillboard(void)
{
    glColor3f(0.35f, 0.35f, 0.36f);
    column(-2.0f, 0.0f, 0.1f, 0.1f, 3.2f, 0.0f);
    column( 2.0f, 0.0f, 0.1f, 0.1f, 3.2f, 0.0f);
    glColor3f(0.92f, 0.92f, 0.90f);
    box(0.0f, 4.6f, 0.0f, 6.2f, 3.2f, 0.15f);           /* frame */
    glColor3f(0.95f, 0.55f, 0.15f);
    frontQuad(-2.9f, 3.2f, 2.9f, 6.0f, 0.08f);
    glColor3f(0.15f, 0.55f, 0.70f);
    frontQuad(-2.4f, 3.7f, 0.2f, 5.5f, 0.09f);
    glColor3f(0.98f, 0.95f, 0.85f);
    frontQuad(0.6f, 4.3f, 2.5f, 4.9f, 0.09f);
}

/* Diner sign: a red panel on a pole, edged with glowing "neon" bars. */
static void buildDinerSign(void)
{
    glColor3f(0.70f, 0.70f, 0.72f);
    column(0.0f, 0.0f, 0.15f, 0.15f, 6.0f, 0.0f);
    glColor3f(0.85f, 0.15f, 0.15f);
    box(0.0f, 6.8f, 0.0f, 3.4f, 1.6f, 0.3f);
    glowBox(0.0f, 7.65f, 0.0f, 3.6f, 0.10f, 0.34f, 1.0f, 0.9f, 0.3f);
    glowBox(0.0f, 5.95f, 0.0f, 3.6f, 0.10f, 0.34f, 1.0f, 0.9f, 0.3f);
    glowBox(0.0f, 6.8f, 0.17f, 2.2f, 0.20f, 0.04f, 1.0f, 1.0f, 0.95f);
}

/* Green direction sign; the white border is a slightly larger board
 * behind the green one. */
static void buildRoadSign(void)
{
    glColor3f(0.60f, 0.60f, 0.62f);
    column(-1.0f, 0.0f, 0.05f, 0.05f, 3.4f, 0.0f);
    column( 1.0f, 0.0f, 0.05f, 0.05f, 3.4f, 0.0f);
    glColor3f(0.95f, 0.95f, 0.95f);
    box(0.0f, 2.85f, 0.06f, 2.75f, 1.45f, 0.04f);
    glColor3f(0.10f, 0.45f, 0.25f);
    box(0.0f, 2.85f, 0.09f, 2.6f, 1.3f, 0.04f);
    glColor3f(0.95f, 0.95f, 0.95f);                     /* simple arrow */
    box(0.40f, 2.85f, 0.115f, 1.2f, 0.12f, 0.01f);
    box(1.05f, 2.85f, 0.115f, 0.12f, 0.40f, 0.01f);
}

static void buildPhoneBox(void)
{
    glColor3f(0.90f, 0.75f, 0.12f);
    box(0.0f, 1.2f, 0.0f, 1.0f, 2.4f, 1.0f);
    glColor3f(0.30f, 0.40f, 0.45f);
    frontQuad(-0.38f, 0.3f, 0.38f, 2.0f, 0.501f);       /* glass door */
    glColor3f(0.20f, 0.20f, 0.20f);
    box(0.0f, 2.47f, 0.0f, 1.1f, 0.14f, 1.1f);          /* roof */
    glowBox(0.0f, 2.25f, 0.51f, 0.7f, 0.14f, 0.02f, 1.0f, 1.0f, 0.95f);
}

/* Overhead sign gantry spanning both lanes. Its signs face -x, towards
 * the traffic coming from behind. Plain panels, no lettering. */
static void buildGantry(void)
{
    glColor3f(0.55f, 0.56f, 0.58f);
    box(0.0f, 3.25f, -5.4f, 0.35f, 6.5f, 0.35f);        /* legs */
    box(0.0f, 3.25f,  5.4f, 0.35f, 6.5f, 0.35f);
    box(0.0f, 6.3f, 0.0f, 0.5f, 0.6f, 11.2f);           /* beam */
    for (int i = 0; i < 2; i++) {
        float z = i ? 2.0f : -2.0f;
        glColor3f(0.95f, 0.95f, 0.95f);
        box(-0.32f, 6.3f, z, 0.06f, 1.75f, 3.15f);
        glColor3f(0.10f, 0.42f, 0.25f);
        box(-0.36f, 6.3f, z, 0.06f, 1.6f, 3.0f);
        glColor3f(0.95f, 0.95f, 0.95f);
        box(-0.40f, 6.0f, z, 0.01f, 0.6f, 0.12f);       /* arrow shaft */
    }
}

static void buildCharger(void)
{
    glColor3f(0.40f, 0.40f, 0.42f);
    box(0.0f, 0.06f, 0.0f, 0.8f, 0.12f, 0.6f);
    glColor3f(0.92f, 0.93f, 0.95f);
    box(0.0f, 0.9f, 0.0f, 0.45f, 1.6f, 0.35f);
    glColor3f(0.10f, 0.12f, 0.15f);
    frontQuad(-0.16f, 1.15f, 0.16f, 1.45f, 0.176f);     /* screen */
    box(0.26f, 0.8f, 0.10f, 0.04f, 0.6f, 0.04f);        /* cable  */
    glowBox(0.0f, 0.75f, 0.18f, 0.06f, 0.70f, 0.02f, 0.2f, 1.0f, 0.4f);
    glowBox(0.0f, 1.66f, 0.0f, 0.46f, 0.05f, 0.36f, 0.2f, 1.0f, 0.4f);
}

/* Three solar panels tilted towards the road and the sky. The panel
 * normal (0, 0.9, 0.6) is perpendicular to its slope (0, 0.6, -0.9). */
static void buildSolar(void)
{
    const float n = sqrtf(0.9f * 0.9f + 0.6f * 0.6f);
    for (int i = -1; i <= 1; i++) {
        float x = i * 1.9f;
        glColor3f(0.60f, 0.60f, 0.62f);
        box(x, 0.25f,  0.35f, 0.06f, 0.5f, 0.06f);      /* legs */
        box(x, 0.50f, -0.35f, 0.06f, 1.0f, 0.06f);

        glColor3f(0.10f, 0.16f, 0.38f);                 /* panel, top */
        glNormal3f(0.0f, 0.9f / n, 0.6f / n);
        glBegin(GL_QUADS);
            glVertex3f(x - 0.9f, 0.5f,  0.45f); glVertex3f(x + 0.9f, 0.5f,  0.45f);
            glVertex3f(x + 0.9f, 1.1f, -0.45f); glVertex3f(x - 0.9f, 1.1f, -0.45f);
        glEnd();
        glColor3f(0.45f, 0.45f, 0.48f);                 /* panel, back */
        glNormal3f(0.0f, -0.9f / n, -0.6f / n);
        glBegin(GL_QUADS);
            glVertex3f(x - 0.9f, 0.48f,  0.44f); glVertex3f(x - 0.9f, 1.08f, -0.46f);
            glVertex3f(x + 0.9f, 1.08f, -0.46f); glVertex3f(x + 0.9f, 0.48f,  0.44f);
        glEnd();
    }
}

/* ------------------------------------------------------------------ */
/* Buildings                                                           */
/* ------------------------------------------------------------------ */
static void buildHouse(void)
{
    glColor3f(0.85f, 0.78f, 0.62f);
    box(0.0f, 1.5f, 0.0f, 8.0f, 3.0f, 6.0f);
    glColor3f(0.55f, 0.25f, 0.20f);
    gableRoof(8.6f, 6.6f, 2.2f, 3.0f);
    glColor3f(0.25f, 0.30f, 0.38f);
    frontQuad(-3.2f, 1.2f, -1.6f, 2.3f, 3.01f);
    frontQuad( 1.6f, 1.2f,  3.2f, 2.3f, 3.01f);
    glColor3f(0.30f, 0.50f, 0.55f);
    frontQuad(-0.5f, 0.0f, 0.5f, 2.1f, 3.01f);
}

static void buildFlatHouse(void)
{
    glColor3f(0.92f, 0.92f, 0.88f);
    box(0.0f, 1.6f, 0.0f, 10.0f, 3.2f, 6.0f);
    glColor3f(0.35f, 0.35f, 0.36f);
    box(0.0f, 3.35f, 0.0f, 10.8f, 0.3f, 6.8f);          /* flat roof slab */
    glColor3f(0.25f, 0.35f, 0.45f);
    frontQuad(-4.5f, 1.0f, 2.0f, 2.6f, 3.01f);          /* picture window */
    glColor3f(0.10f, 0.55f, 0.55f);
    frontQuad(3.0f, 0.0f, 4.0f, 2.2f, 3.01f);
}

/* Concrete block with a regular grid of darker windows. */
static void buildApartment(void)
{
    glColor3f(0.66f, 0.64f, 0.60f);
    box(0.0f, 10.5f, 0.0f, 16.0f, 21.0f, 10.0f);
    glColor3f(0.25f, 0.27f, 0.30f);
    for (int f = 0; f < 7; f++)
        for (int c = 0; c < 6; c++) {
            float x = -6.25f + 2.5f * c, y = 1.2f + 3.0f * f;
            frontQuad(x - 0.7f, y, x + 0.7f, y + 1.4f, 5.01f);
        }
    glColor3f(0.50f, 0.49f, 0.46f);
    box(0.0f, 21.2f, 0.0f, 16.4f, 0.4f, 10.4f);
}

/* Office block with bands of blue glass on the front and both ends. */
static void buildOffice(void)
{
    glColor3f(0.70f, 0.72f, 0.75f);
    box(0.0f, 7.5f, 0.0f, 14.0f, 15.0f, 10.0f);
    glColor3f(0.25f, 0.45f, 0.70f);
    for (int f = 0; f < 5; f++) {
        float y = 1.0f + 3.0f * f;
        frontQuad(-6.6f, y, 6.6f, y + 2.0f, 5.01f);
        sideQuad( 7.01f, -4.6f, 4.6f, y, y + 2.0f,  1.0f);
        sideQuad(-7.01f, -4.6f, 4.6f, y, y + 2.0f, -1.0f);
    }
}

static void buildGlassTower(void)
{
    glColor3f(0.45f, 0.62f, 0.72f);
    box(0.0f, 21.0f, 0.0f, 16.0f, 42.0f, 14.0f);
    glColor3f(0.30f, 0.38f, 0.45f);                     /* floor bands */
    for (int i = 1; i < 10; i++) box(0.0f, i * 4.2f, 0.0f, 16.1f, 0.25f, 14.1f);
    glColor3f(0.50f, 0.68f, 0.78f);
    box(-2.0f, 45.0f, 0.0f, 10.0f, 6.0f, 10.0f);        /* setback top */
}

/* Static part of a wind turbine; the rotor is drawn every frame. */
static void buildTurbine(void)
{
    glColor3f(0.92f, 0.92f, 0.92f);
    column(0.0f, 0.0f, 0.9f, 0.45f, 30.0f, 0.0f);
    box(0.0f, 30.4f, 0.4f, 1.2f, 1.2f, 3.4f);           /* nacelle */
}

/* Three blades around the hub, turning in the plane facing the road. */
static void drawTurbineRotor(float angle)
{
    glPushMatrix();
        glTranslatef(0.0f, 30.4f, 2.2f);
        glRotatef(angle, 0.0f, 0.0f, 1.0f);
        glColor3f(0.95f, 0.95f, 0.95f);
        glutSolidSphere(0.6, 12, 10);
        for (int b = 0; b < 3; b++) {
            glPushMatrix();
                glRotatef(b * 120.0f, 0.0f, 0.0f, 1.0f);
                glTranslatef(0.0f, 6.0f, 0.0f);
                glScalef(0.7f, 11.0f, 0.15f);
                glutSolidCube(1.0);
            glPopMatrix();
        }
    glPopMatrix();
}

/* ------------------------------------------------------------------ */
/* Skylines: flat silhouettes in the plane z = 0, facing +z            */
/* ------------------------------------------------------------------ */

/* Tiny pseudo-random generator with a fixed seed, so every run builds
 * exactly the same skyline. */
static unsigned rngState = 1u;
static float rnd(void)
{
    rngState = rngState * 1664525u + 1013904223u;
    return (float)(rngState >> 8) / 16777216.0f;
}

static void rect2(float x0, float x1, float y0, float y1)
{
    glBegin(GL_QUADS);
        glVertex3f(x0, y0, 0.0f); glVertex3f(x1, y0, 0.0f);
        glVertex3f(x1, y1, 0.0f); glVertex3f(x0, y1, 0.0f);
    glEnd();
}

static void tri2(float x0, float x1, float y0, float yTop)
{
    glBegin(GL_TRIANGLES);
        glVertex3f(x0, y0, 0.0f); glVertex3f(x1, y0, 0.0f);
        glVertex3f(0.5f * (x0 + x1), yTop, 0.0f);
    glEnd();
}

/* Rolling hills: a sum of a few sine waves of different lengths. */
static float hillHeight(float x, float base, float amp)
{
    return base + amp * (0.55f * sinf(x * 0.013f)
                       + 0.30f * sinf(x * 0.031f + 1.3f)
                       + 0.15f * sinf(x * 0.071f + 0.4f));
}

static void hills(float base, float amp)
{
    glBegin(GL_TRIANGLE_STRIP);
    for (float x = -SKY_HALF; x <= SKY_HALF; x += 10.0f) {
        glVertex3f(x, -5.0f, 0.0f);
        glVertex3f(x, hillHeight(x, base, amp), 0.0f);
    }
    glEnd();
}

/* A row of rectangular buildings of random width and height. With
 * roofs, some get a pitched top (roofRatio = roof height / width). */
static void blocks(float minW, float maxW, float minH, float maxH,
                   float maxGap, float roofChance, float roofRatio)
{
    for (float x = -SKY_HALF; x < SKY_HALF; ) {
        float w = minW + rnd() * (maxW - minW);
        float h = minH + rnd() * (maxH - minH);
        rect2(x, x + w, -5.0f, h);
        if (rnd() < roofChance) tri2(x, x + w, h, h + w * roofRatio);
        x += w + rnd() * maxGap;
    }
}

/* A few tall towers spaced roughly every `spacing` metres. */
static void towers(float spacing, float w, float minH, float maxH, float spireChance)
{
    for (float x = -SKY_HALF; x < SKY_HALF; x += spacing * (0.6f + 0.8f * rnd())) {
        float h = minH + rnd() * (maxH - minH);
        rect2(x, x + w, -5.0f, h);
        if (rnd() < spireChance) tri2(x, x + w, h, h + 0.8f * w);
    }
}

static void church(float x)
{
    float base = hillHeight(x, 14.0f, 10.0f) - 3.0f;
    rect2(x - 6.0f, x + 3.0f, base, base + 9.0f);       /* nave   */
    tri2(x - 6.0f, x + 3.0f, base + 9.0f, base + 13.0f);
    rect2(x + 3.0f, x + 6.5f, base, base + 18.0f);      /* tower  */
    tri2(x + 3.0f, x + 6.5f, base + 18.0f, base + 32.0f); /* spire */
}

static void buildSkyline(int type)
{
    rngState = 12345u + 977u * (unsigned)type;
    switch (type) {
        case SKY_HILLS:
            hills(14.0f, 10.0f);
            break;
        case SKY_HILLS_SPIRE:
            hills(14.0f, 10.0f);
            church(40.0f);
            church(-260.0f);
            break;
        case SKY_CHIMNEYS:
            hills(8.0f, 5.0f);
            blocks(20.0f, 45.0f, 8.0f, 16.0f, 30.0f, 0.3f, 0.15f);  /* sheds */
            for (int i = 0; i < 14; i++) {                         /* chimneys */
                float x = -SKY_HALF + 65.0f * i + rnd() * 30.0f;
                rect2(x, x + 3.0f, -5.0f, 30.0f + rnd() * 20.0f);
            }
            break;
        case SKY_TOWN:
            hills(6.0f, 4.0f);
            blocks(8.0f, 14.0f, 6.0f, 10.0f, 4.0f, 0.8f, 0.35f);
            break;
        case SKY_MIDRISE:
            blocks(15.0f, 30.0f, 14.0f, 26.0f, 6.0f, 0.0f, 0.0f);
            break;
        case SKY_TOWERS:
            blocks(14.0f, 22.0f, 8.0f, 14.0f, 4.0f, 0.0f, 0.0f);
            towers(50.0f, 14.0f, 40.0f, 60.0f, 0.0f);
            break;
        case SKY_OFFICES:
            blocks(16.0f, 30.0f, 20.0f, 45.0f, 5.0f, 0.0f, 0.0f);
            towers(60.0f, 10.0f, 50.0f, 70.0f, 0.0f);       /* stepped tops */
            break;
        default: /* SKY_GLASS */
            blocks(14.0f, 24.0f, 25.0f, 50.0f, 8.0f, 0.0f, 0.0f);
            towers(70.0f, 16.0f, 70.0f, 115.0f, 0.4f);
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Public interface                                                    */
/* ------------------------------------------------------------------ */
typedef void (*BuildFn)(void);

static const BuildFn builders[PROP_COUNT] = {
    [PROP_FENCE]         = buildFence,
    [PROP_TELEGRAPH]     = buildTelegraph,
    [PROP_STONE_WALL]    = buildStoneWall,
    [PROP_BARRIER]       = buildBarrier,
    [PROP_HAYSTACK]      = buildHaystack,
    [PROP_COTTAGE]       = buildCottage,
    [PROP_ROUND_TREE]    = buildRoundTree,
    [PROP_CONE_TREE]     = buildConeTree,
    [PROP_BUSH]          = buildBush,
    [PROP_GAS_LAMP]      = buildGasLamp,
    [PROP_ELECTRIC_LAMP] = buildElectricLamp,
    [PROP_PETROL_PUMP]   = buildPetrolPump,
    [PROP_BILLBOARD]     = buildBillboard,
    [PROP_LAMP_POST]     = buildLampPost,
    [PROP_HOUSE]         = buildHouse,
    [PROP_FLAT_HOUSE]    = buildFlatHouse,
    [PROP_DINER_SIGN]    = buildDinerSign,
    [PROP_SODIUM_LAMP]   = buildSodiumLamp,
    [PROP_APARTMENT]     = buildApartment,
    [PROP_ROAD_SIGN]     = buildRoadSign,
    [PROP_PHONE_BOX]     = buildPhoneBox,
    [PROP_MODERN_LAMP]   = buildModernLamp,
    [PROP_OFFICE]        = buildOffice,
    [PROP_GANTRY]        = buildGantry,
    [PROP_LED_LAMP]      = buildLedLamp,
    [PROP_GLASS_TOWER]   = buildGlassTower,
    [PROP_TURBINE]       = buildTurbine,
    [PROP_CHARGER]       = buildCharger,
    [PROP_SOLAR]         = buildSolar,
};

/* Static shapes never change, so each is recorded once into a display
 * list and replayed with one glCallList wherever it appears. */
void initProps(void)
{
    quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);

    for (int i = 0; i < PROP_COUNT; i++) {
        propLists[i] = 0;
        if (!builders[i]) continue;
        propLists[i] = glGenLists(1);
        glNewList(propLists[i], GL_COMPILE);
            builders[i]();
        glEndList();
    }

    /* Skyline lists hold geometry only; the colour and alpha are set
     * just before each call, so one list serves any colour grading. */
    for (int i = 0; i < SKYLINE_COUNT; i++) {
        skyLists[i] = glGenLists(1);
        glNewList(skyLists[i], GL_COMPILE);
            buildSkyline(i);
        glEndList();
    }
}

void updateProps(float dt)
{
    rotorAngle = fmodf(rotorAngle + TURBINE_RATE * dt, 360.0f);
}

static int isRowProp(int type)
{
    return type == PROP_FENCE || type == PROP_TELEGRAPH ||
           type == PROP_STONE_WALL || type == PROP_BARRIER;
}

void drawProp(int type, float x, float z, float scaleY)
{
    if (type <= PROP_NONE || type >= PROP_COUNT || scaleY <= 0.001f) return;

    glPushMatrix();
    if (type == PROP_GANTRY) {
        glTranslatef(x, 0.0f, 0.0f);            /* always spans the road */
    } else if (z > 0.0f) {
        /* Near side: turn 180 degrees so the front faces the road. That
         * also flips +x to -x, so a row prop starts 12 m further on to
         * cover the same stretch of road. */
        glTranslatef(isRowProp(type) ? x + SLOT_LENGTH : x, 0.0f, z);
        glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    } else {
        glTranslatef(x, 0.0f, z);
    }

    glScalef(1.0f, scaleY, 1.0f);               /* grow from the base */
    glCallList(propLists[type]);

    /* Each turbine gets its own phase from its x position. 3.75 degrees
     * per metre is exactly 360 degrees per 96 m scene period, so the
     * phase does not jump when the scene snaps forward. */
    if (type == PROP_TURBINE) drawTurbineRotor(rotorAngle + 3.75f * x);
    glPopMatrix();
}

void drawSkyline(int type, float followX, const float color[3], float alpha)
{
    if (type < 0 || type >= SKYLINE_COUNT || alpha <= 0.0f) return;
    glColor4f(color[0], color[1], color[2], alpha);
    glPushMatrix();
        glTranslatef(followX, 0.0f, SKYLINE_Z);
        glCallList(skyLists[type]);
    glPopMatrix();
}
