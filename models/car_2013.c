#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../cars.h"
#include "../geom.h"
#include "../materials.h"
#include "../carparts.h"
#include "models.h"

/* ------------------------------------------------------------------ */
/* 2013 S-Class (W222)                                                 */
/*                                                                     */
/* Long luxury saloon: long wheelbase and roofline, short boot, a      */
/* large upright chrome grille, slim headlamps with three LED strips,  */
/* a sculpted shoulder crease and a dropping lower line along the      */
/* doors, chrome around the windows and low on the doors, wide LED     */
/* tail lamps.                                                         */
/* ------------------------------------------------------------------ */
#define FX      1.5175f
#define RX     -1.5175f
#define WR      0.35f
#define WW      0.245f
#define ARCH    (WR + 0.06f)
#define HW      0.95f
#define SILL    0.30f
#define TRACK   (HW - 0.5f * WW - 0.04f)
#define NOSE_X  2.60f
#define TAIL_X -2.52f

static const Pt2 lowTop[] = {
    { TAIL_X, 0.80f }, { -2.47f, 0.97f }, { -2.25f, 1.03f }, { -1.90f, 1.03f },
    { -1.50f, 1.01f }, {  0.80f, 0.985f }, {  1.30f, 0.95f }, {  2.20f, 0.88f },
    {  2.50f, 0.84f }, { NOSE_X, 0.78f },
};
static const Pt2 lowBot[] = { { TAIL_X, 0.38f }, { -2.25f, 0.31f }, { -1.90f, SILL },
                              { 1.90f, SILL }, { 2.30f, 0.32f }, { NOSE_X, 0.36f } };
static const Pt2 lowHw[]  = { { TAIL_X, 0.82f }, { -2.25f, 0.93f }, { -1.60f, HW }, { 1.60f, HW },
                              { 2.25f, 0.92f }, { NOSE_X, 0.80f } };

static float beltY(float x) { return curveY(lowTop, COUNT(lowTop), x); }

static void lowerSection(float x, Section *s, const void *ctx)
{
    (void)ctx;
    s->yTop = beltY(x);
    float yb = curveY(lowBot, COUNT(lowBot), x);
    yb = archBottom(x, FX, ARCH, WR, yb);
    yb = archBottom(x, RX, ARCH, WR, yb);
    s->yBottom = yb < s->yTop - 0.06f ? yb : s->yTop - 0.06f;
    s->halfWidth = curveY(lowHw, COUNT(lowHw), x);
    s->topHalfWidth = 0.88f * s->halfWidth;
    s->roundness = 4.2f;
}

static const Pt2 glTop[] = {
    { -1.90f, 1.03f }, { -1.55f, 1.20f }, { -1.25f, 1.37f }, { -0.70f, 1.48f },
    {  0.00f, 1.50f }, {  0.35f, 1.46f }, {  0.70f, 1.24f }, {  1.02f, 0.99f },
};
static const Pt2 glHw[] = { { -1.90f, 0.78f }, { -1.25f, 0.87f }, { 1.02f, 0.86f } };

static void glassSection(float x, Section *s, const void *ctx)
{
    (void)ctx;
    s->yBottom = beltY(x) - 0.12f;
    s->yTop = curveY(glTop, COUNT(glTop), x);
    s->halfWidth = curveY(glHw, COUNT(glHw), x);
    s->topHalfWidth = 0.68f;
    s->roundness = 3.8f;
}

static const Loft lower = { lowerSection, NULL, TAIL_X, NOSE_X, 90, 32, 1, 1, NULL, NULL };
static const Loft glass = { glassSection, NULL, -1.90f, 1.02f, 58, 30, 0, 0, NULL, NULL };

static void setup(ClosedCar *c)
{
    memset(c, 0, sizeof *c);
    c->lower = &lower;
    c->glass = &glass;
    GlassLayout gl = { 0.32f, -1.25f, { -0.35f, -1.08f, 0.0f }, 2, 0.045f, 0.45f, 0.45f };
    c->layout = gl;
    c->beltY = beltY;
    c->axleX[0] = FX;   c->axleX[1] = RX;
    c->axleY[0] = WR;   c->axleY[1] = WR;
    c->archR[0] = ARCH; c->archR[1] = ARCH;
    c->hw = HW;
    c->sill = SILL;
    c->wellDepth = HW - (TRACK - 0.5f * WW - 0.06f);
    c->doorX[0] = 0.92f; c->doorX[1] = -0.32f; c->doorX[2] = -1.25f; c->nDoorX = 3;
    c->doorTopH = 0.96f; c->doorBottomH = 0.22f;
    c->roofMat = MAT_PAINT;
    c->beltMat = MAT_CHROME;
    c->roofTrimMat = MAT_CHROME;
    c->pillarTrimMat = MAT_GLOSS_BLACK;
    CabinSpec cab = { 1.00f, 0.55f, 0.74f, -0.32f, -1.30f, 0.40f, 0.42f, 1, MAT_WOOD, 0.19f };
    c->cabin = cab;
}

/* Strips drawn with lighting off look like light sources. */
static void glowQuad(float x0, float y0, float x1, float y1, float r, float g, float b)
{
    glDisable(GL_LIGHTING);
    glColor3f(r, g, b);
    quadXY(x0, y0, x1, y1);
    glEnable(GL_LIGHTING);
}

static void frontEnd(void)
{
    /* Large upright grille: chrome frame, dark backing, chrome slats. */
    setMaterial(MAT_CHROME);
    roundedBox(2.615f, 0.62f, 0.0f, 0.05f, 0.30f, 0.62f, 8.0f);
    pushFrame(v3(2.641f, 0.62f, 0.0f), v3(1, 0, 0));
        setMaterial(MAT_MATTE);
        roundRectDisc(0.56f, 0.24f, 0.02f);
        setMaterial(MAT_CHROME);
        for (int i = 0; i < 4; i++) {
            float y = -0.09f + 0.06f * i;
            tube(v3(-0.27f, y, 0.008f), v3(0.27f, y, 0.008f), 0.010f, 0.010f, 8);
        }
    glPopMatrix();

    /* Slim headlamps, each with three LED strips across it. */
    for (int s = -1; s <= 1; s += 2) {
        Vec3 n, p = loftTop(&lower, 2.48f, s * 0.66f, &n);
        pushFrameFacing(p, n, v3(0.0f, 0.0f, (float)s));
            glTranslatef(0.0f, 0.0f, 0.003f);
            setMaterial(MAT_CHROME);
            ellipseDisc(0.25f, 0.075f, 32);
            glTranslatef(0.0f, 0.0f, 0.002f);
            setMaterial(MAT_GLOSS_BLACK);
            ellipseDisc(0.235f, 0.062f, 32);
            glTranslatef(0.0f, 0.0f, 0.002f);
            for (int k = -1; k <= 1; k++) {
                float half = 0.17f - 0.03f * (float)abs(k);
                glowQuad(-half, k * 0.025f - 0.004f, half, k * 0.025f + 0.004f, 0.90f, 0.95f, 1.0f);
            }
        glPopMatrix();
    }

    /* Bumper with a chrome-barred lower intake. */
    setMaterial(MAT_PAINT);
    roundedBox(2.56f, 0.42f, 0.0f, 0.12f, 0.16f, 1.76f, 5.0f);
    pushFrame(v3(2.622f, 0.38f, 0.0f), v3(1, 0, 0));
        setMaterial(MAT_MATTE);
        roundRectDisc(1.00f, 0.07f, 0.03f);
        setMaterial(MAT_CHROME);
        tube(v3(-0.48f, 0.0f, 0.006f), v3(0.48f, 0.0f, 0.006f), 0.007f, 0.007f, 6);
    glPopMatrix();
    numberPlate(v3(2.626f, 0.41f, 0.0f), v3(1, 0, 0), 0.46f, 0.10f);   /* below the grille */
}

static void tail(void)
{
    for (int s = -1; s <= 1; s += 2) {
        /* Wide, flat LED lamps running round the corners. */
        lampRect(v3(TAIL_X, 0.70f, s * 0.55f), v3(-1, 0, 0), 0.44f, 0.10f, 0.02f,
                 MAT_TAIL, MAT_GLOSS_BLACK);
        pushFrame(v3(TAIL_X - 0.009f, 0.70f, s * 0.55f), v3(-1, 0, 0));
            glowQuad(-0.19f, 0.018f, 0.19f, 0.028f, 1.0f, 0.12f, 0.08f);
            glowQuad(-0.19f, -0.028f, 0.19f, -0.018f, 1.0f, 0.12f, 0.08f);
        glPopMatrix();
        Vec3 n, p = loftSide(&lower, -2.47f, loftH(&lower, -2.47f, 0.70f), (float)s, &n);
        lampRect(p, n, 0.14f, 0.09f, 0.02f, MAT_TAIL, MAT_GLOSS_BLACK);

        /* Exhaust outlets built into the rear apron. */
        lampRect(v3(TAIL_X - 0.02f, 0.40f, s * 0.55f), v3(-1, 0, 0), 0.22f, 0.06f, 0.02f,
                 MAT_MATTE, MAT_CHROME);
    }
    numberPlate(v3(TAIL_X - 0.002f, 0.56f, 0.0f), v3(-1, 0, 0), 0.46f, 0.10f);
    setMaterial(MAT_PAINT);
    roundedBox(-2.47f, 0.42f, 0.0f, 0.12f, 0.12f, 1.74f, 5.0f);
}

/* The shoulder crease and the lower line, shown as lighter highlight
 * lines (on black paint a crease shows as a streak of reflected sky),
 * and the chrome strip low on the doors. */
static void sculpting(void)
{
    for (int s = -1; s <= 1; s += 2) {
        setMaterialScaled(MAT_PAINT, 4.0f);
        Pt2 shoulder[4] = { { -2.30f, 0.86f }, { -1.00f, 0.85f }, { 1.00f, 0.85f }, { 2.30f, 0.86f } };
        loftStrip(&lower, shoulder, 4, 0, (float)s, 0.012f, 0.003f);
        Pt2 lowLine[3] = { { 0.95f, 0.42f }, { 0.00f, 0.38f }, { -1.05f, 0.30f } };
        loftStrip(&lower, lowLine, 3, 0, (float)s, 0.010f, 0.003f);

        setMaterial(MAT_CHROME);
        Pt2 sill[2] = { { -1.05f, 0.20f }, { 1.05f, 0.20f } };
        loftStrip(&lower, sill, 2, 0, (float)s, 0.014f, 0.005f);

        const float hx[2] = { 0.32f, -0.85f };
        for (int i = 0; i < 2; i++) {
            Vec3 n, p = loftSide(&lower, hx[i], 0.80f, (float)s, &n);
            doorHandle(p, n, 0.13f, MAT_CHROME);
        }
        Vec3 p = loftSide(&lower, 0.92f, 0.97f, (float)s, NULL);
        sideMirror(p, (float)s, MAT_PAINT, 1);
    }
    for (int i = 0; i < 2; i++) {
        float u0 = i ? 0.05f : -0.62f;
        Vec3 n, p = loftTop(&glass, 0.97f, u0, &n);
        Vec3 t = loftTop(&glass, 0.95f, u0 + 0.55f, NULL);
        wiper(p, t, n);
    }
}

static void buildBody(const CarModel *m)
{
    (void)m;
    ClosedCar c;
    setup(&c);
    drawClosedCarCommon(&c);
    frontEnd();
    tail();
    sculpting();
}

static void buildGlass(const CarModel *m)
{
    (void)m;
    ClosedCar c;
    setup(&c);
    drawClosedCarGlass(&c);
}

void defineCar2013(CarModel *m)
{
    static const float paint[3]   = { 0.04f, 0.04f, 0.05f };   /* black        */
    static const float accent[3]  = { 0.10f, 0.10f, 0.10f };
    static const float leather[3] = { 0.75f, 0.65f, 0.50f };   /* beige leather */

    m->year = "2013";
    m->name = "S-Class (W222)";
    m->length = 5.14f;
    memcpy(m->paint, paint, sizeof paint);
    memcpy(m->accent, accent, sizeof accent);
    memcpy(m->leather, leather, sizeof leather);

    m->numWheels = 4;
    m->axleX[0] = FX;     m->axleX[1] = RX;
    m->track[0] = TRACK;  m->track[1] = TRACK;

    WheelSpec w = { WR, WW, 0.245f, RIM_10SPOKE, MAT_METAL, 1 };
    m->wheel[0] = w;
    m->wheel[1] = w;

    m->buildBody    = buildBody;
    m->buildGlass   = buildGlass;
    m->drawAnimated = NULL;
}
