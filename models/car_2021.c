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
/* 2021 EQS                                                            */
/*                                                                     */
/* "One-bow" shape: a single arc from the nose over the roof to a     */
/* short tail with a small lip, with the cabin pushed far forward.     */
/* No grille, just a gloss-black panel; light bands join the lamps    */
/* across the front and across the rear. Flush handles, frameless      */
/* windows, a black lower section and large aero wheels. Electric, so  */
/* no exhaust.                                                         */
/* ------------------------------------------------------------------ */
#define FX      1.605f
#define RX     -1.605f
#define WR      0.37f
#define WW      0.255f
#define ARCH    (WR + 0.06f)
#define HW      0.965f
#define SILL    0.30f
#define TRACK   (HW - 0.5f * WW - 0.04f)
#define NOSE_X  2.61f
#define TAIL_X -2.60f

static const Pt2 lowTop[] = {
    { TAIL_X, 0.84f }, { -2.56f, 0.99f }, { -2.40f, 1.04f }, { -2.22f, 1.04f },
    { -1.80f, 1.02f }, { -1.00f, 1.00f }, {  1.40f, 0.99f }, {  1.70f, 0.96f },
    {  2.15f, 0.86f }, {  2.45f, 0.74f }, { NOSE_X, 0.56f },
};
static const Pt2 lowBot[] = { { TAIL_X, 0.40f }, { -2.30f, 0.32f }, { -1.90f, SILL },
                              { 1.90f, SILL }, { 2.35f, 0.33f }, { NOSE_X, 0.40f } };
static const Pt2 lowHw[]  = { { TAIL_X, 0.80f }, { -2.35f, 0.93f }, { -1.70f, HW }, { 1.70f, HW },
                              { 2.35f, 0.92f }, { NOSE_X, 0.74f } };

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
    s->topHalfWidth = 0.86f * s->halfWidth;
    s->roundness = 3.0f;                       /* very smooth surfaces */
}

/* The bow: windscreen base far forward, one long arc to the tail. */
static const Pt2 glTop[] = {
    { -2.20f, 1.04f }, { -1.90f, 1.17f }, { -1.50f, 1.33f }, { -1.00f, 1.45f },
    { -0.30f, 1.51f }, {  0.30f, 1.48f }, {  0.80f, 1.34f }, {  1.20f, 1.15f },
    {  1.50f, 1.00f },
};
static const Pt2 glHw[] = { { -2.20f, 0.80f }, { -1.40f, 0.88f }, { 1.50f, 0.86f } };

static void glassSection(float x, Section *s, const void *ctx)
{
    (void)ctx;
    s->yBottom = beltY(x) - 0.12f;
    s->yTop = curveY(glTop, COUNT(glTop), x);
    s->halfWidth = curveY(glHw, COUNT(glHw), x);
    s->topHalfWidth = 0.62f;
    s->roundness = 3.0f;
}

static const Loft lower = { lowerSection, NULL, TAIL_X, NOSE_X, 90, 32, 1, 1, NULL, NULL };
static const Loft glass = { glassSection, NULL, -2.20f, 1.50f, 64, 30, 0, 0, NULL, NULL };

static void setup(ClosedCar *c)
{
    memset(c, 0, sizeof *c);
    c->lower = &lower;
    c->glass = &glass;
    GlassLayout gl = { 0.55f, -1.40f, { -0.30f, 0.0f, 0.0f }, 1, 0.04f, 0.45f, 0.45f };
    c->layout = gl;
    c->beltY = beltY;
    c->axleX[0] = FX;   c->axleX[1] = RX;
    c->axleY[0] = WR;   c->axleY[1] = WR;
    c->archR[0] = ARCH; c->archR[1] = ARCH;
    c->hw = HW;
    c->sill = SILL;
    c->wellDepth = HW - (TRACK - 0.5f * WW - 0.06f);
    c->doorX[0] = 1.05f; c->doorX[1] = -0.25f; c->doorX[2] = -1.35f; c->nDoorX = 3;
    c->doorTopH = 0.97f; c->doorBottomH = 0.26f;
    c->roofMat = MAT_PAINT;
    c->beltMat = MAT_RUBBER;                  /* frameless: only thin seals */
    c->roofTrimMat = MAT_GLOSS_BLACK;
    c->pillarTrimMat = MAT_GLOSS_BLACK;
    CabinSpec cab = { 1.00f, 0.95f, 0.76f, 0.00f, -0.95f, 0.42f, 0.42f, 1, MAT_GLOSS_BLACK, 0.18f };
    c->cabin = cab;
}

/* A light band laid along a row of surface points: a thin tube drawn
 * with lighting off, so it reads as glowing. */
static void lightBand(const Vec3 *pts, int n, float r, float red, float green, float blue)
{
    glDisable(GL_LIGHTING);
    glColor3f(red, green, blue);
    sweepTube(pts, n, 4 * n, r, 6);
    glEnable(GL_LIGHTING);
}

static void frontEnd(void)
{
    /* Gloss-black panel where a grille would be. */
    setMaterial(MAT_GLOSS_BLACK);
    loftTopPatch(&lower, 2.38f, 2.57f, -0.55f, 0.55f, 6, 10, 0.004f);

    /* Light band across the front, ending in the slim headlamps. */
    Vec3 band[9];
    for (int i = 0; i < 9; i++) {
        float u = -0.80f + 1.60f * i / 8;
        Vec3 n, p = loftTop(&lower, 2.355f, u, &n);
        band[i] = vAdd(p, vScale(n, 0.008f));
    }
    lightBand(band, 9, 0.007f, 0.85f, 0.93f, 1.0f);

    for (int s = -1; s <= 1; s += 2) {
        Vec3 n, p = loftTop(&lower, 2.33f, s * 0.80f, &n);
        pushFrameFacing(p, n, v3(0.0f, 0.0f, (float)s));
            glTranslatef(0.0f, 0.0f, 0.003f);
            setMaterial(MAT_GLOSS_BLACK);
            ellipseDisc(0.20f, 0.05f, 28);
            glDisable(GL_LIGHTING);
            glColor3f(0.85f, 0.93f, 1.0f);
            glTranslatef(0.0f, 0.0f, 0.002f);
            quadXY(-0.15f, -0.006f, 0.15f, 0.006f);
            glEnable(GL_LIGHTING);
        glPopMatrix();
    }

    /* Black lower bumper with a blank plate. */
    setMaterial(MAT_GLOSS_BLACK);
    roundedBox(2.55f, 0.40f, 0.0f, 0.12f, 0.14f, 1.60f, 5.0f);
    numberPlate(v3(2.615f, 0.40f, 0.0f), v3(1, 0, 0), 0.46f, 0.10f);
}

static void tail(void)
{
    /* Continuous light band across the tail, joining the lamps. */
    Vec3 band[11];
    for (int i = 0; i < 11; i++) {
        float u = -0.92f + 1.84f * i / 10;
        Vec3 n, p = loftTop(&lower, -2.575f, u, &n);
        band[i] = vAdd(p, vScale(n, 0.008f));
    }
    for (int s = -1; s <= 1; s += 2) {
        Vec3 n, p = loftTop(&lower, -2.57f, s * 0.86f, &n);
        pushFrame(p, n);
            glTranslatef(0.0f, 0.0f, 0.003f);
            setMaterial(MAT_TAIL);
            ellipseDisc(0.16f, 0.05f, 28);
        glPopMatrix();
    }
    lightBand(band, 11, 0.010f, 1.0f, 0.10f, 0.08f);

    /* Small lip at the end of the bow. */
    setMaterialScaled(MAT_PAINT, 0.85f);
    Pt2 lip[2] = { { -2.42f, -0.90f }, { -2.42f, 0.90f } };
    loftStrip(&lower, lip, 2, 1, 1.0f, 0.03f, 0.008f);

    setMaterial(MAT_GLOSS_BLACK);
    roundedBox(-2.55f, 0.42f, 0.0f, 0.12f, 0.14f, 1.60f, 5.0f);
    numberPlate(v3(TAIL_X - 0.002f, 0.62f, 0.0f), v3(-1, 0, 0), 0.46f, 0.10f);
}

static void sides(void)
{
    /* Black lower section along the sides, between and around the arches. */
    const float xs[3][2] = {
        { TAIL_X + 0.05f, RX - ARCH - 0.02f },
        { RX + ARCH + 0.02f, FX - ARCH - 0.02f },
        { FX + ARCH + 0.02f, NOSE_X - 0.05f },
    };
    for (int s = -1; s <= 1; s += 2) {
        setMaterial(MAT_GLOSS_BLACK);
        for (int i = 0; i < 3; i++)
            loftSidePatch(&lower, xs[i][0], xs[i][1], 0.03f, 0.25f, (float)s, 12, 4, 0.006f);

        /* Flush door handles: just slots in the surface. */
        setMaterial(MAT_MATTE);
        const float hx[2] = { 0.40f, -0.70f };
        for (int i = 0; i < 2; i++) {
            Pt2 slot[2] = { { hx[i] - 0.08f, 0.86f }, { hx[i] + 0.08f, 0.86f } };
            loftStrip(&lower, slot, 2, 0, (float)s, 0.018f, 0.002f);
        }
        Vec3 p = loftSide(&lower, 1.05f, 0.97f, (float)s, NULL);
        sideMirror(p, (float)s, MAT_GLOSS_BLACK, 1);
    }
    for (int i = 0; i < 2; i++) {
        float u0 = i ? 0.05f : -0.62f;
        Vec3 n, p = loftTop(&glass, 1.45f, u0, &n);
        Vec3 t = loftTop(&glass, 1.43f, u0 + 0.55f, NULL);
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
    sides();
}

static void buildGlass(const CarModel *m)
{
    (void)m;
    ClosedCar c;
    setup(&c);
    drawClosedCarGlass(&c);
}

void defineCar2021(CarModel *m)
{
    static const float paint[3]   = { 0.88f, 0.89f, 0.90f };   /* silver-white */
    static const float accent[3]  = { 0.03f, 0.03f, 0.04f };
    static const float leather[3] = { 0.85f, 0.83f, 0.80f };

    m->year = "2021";
    m->name = "EQS";
    m->length = 5.22f;
    memcpy(m->paint, paint, sizeof paint);
    memcpy(m->accent, accent, sizeof accent);
    memcpy(m->leather, leather, sizeof leather);

    m->numWheels = 4;
    m->axleX[0] = FX;     m->axleX[1] = RX;
    m->track[0] = TRACK;  m->track[1] = TRACK;

    WheelSpec w = { WR, WW, 0.267f, RIM_AERO, MAT_METAL, 1 };
    m->wheel[0] = w;
    m->wheel[1] = w;

    m->buildBody    = buildBody;
    m->buildGlass   = buildGlass;
    m->drawAnimated = NULL;
}
