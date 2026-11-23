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
/* 1936 540K Cabriolet, roof down                                      */
/*                                                                     */
/* Extremely long bonnet with outside exhaust pipes, a tall V-shaped   */
/* chrome grille, separate flowing wings that sweep into running       */
/* boards, free-standing chrome headlamps, side-mounted spare wheels,  */
/* a low cockpit far back, a split V windscreen and a folded hood.     */
/* Two-tone: cream body, black wings. Left-hand drive (driver at -z).  */
/* ------------------------------------------------------------------ */
#define FRONT_X   1.645f
#define REAR_X   -1.645f
#define WHEEL_R   0.40f
#define TRACK     0.78f
#define WING_Z    0.80f
#define BODY_HW   0.78f
#define DRIVER_Z -0.36f

/* Centre line of each wing, front tip to rear tip (y of its middle). */
static const Vec3 wingPath[] = {
    {  2.44f, 0.55f, 0 }, {  2.28f, 0.76f, 0 }, {  2.02f, 0.93f, 0 }, {  1.645f, 0.99f, 0 },
    {  1.28f, 0.93f, 0 }, {  1.02f, 0.74f, 0 }, {  0.78f, 0.52f, 0 }, {  0.45f, 0.45f, 0 },
    { -0.80f, 0.45f, 0 }, { -1.05f, 0.62f, 0 }, { -1.32f, 0.88f, 0 }, { -1.645f, 0.98f, 0 },
    { -1.98f, 0.90f, 0 }, { -2.25f, 0.70f, 0 }, { -2.40f, 0.52f, 0 },
};

/* Same line as y(x), ascending x, to find the wing height anywhere. */
static float wingY(float x)
{
    Pt2 p[COUNT(wingPath)];
    int n = COUNT(wingPath);
    for (int i = 0; i < n; i++) {
        p[i].x = wingPath[n - 1 - i].x;
        p[i].y = wingPath[n - 1 - i].y;
    }
    return curveY(p, n, x);
}

/* Long narrow bonnet. */
static void bonnetSection(float x, Section *s, const void *ctx)
{
    (void)ctx;
    float t = (x - 0.30f) / (2.17f - 0.30f);
    s->yBottom = 0.58f;
    s->yTop = 1.08f - 0.02f * t;
    s->halfWidth = 0.42f - 0.06f * t;
    s->topHalfWidth = 0.86f * s->halfWidth;
    s->roundness = 5.0f;
}
static const Loft bonnet = { bonnetSection, NULL, 0.30f, 2.17f, 40, 28, 1, 1, NULL, NULL };

/* Body: scuttle, cockpit dipping to the floor, rear deck, round tail. */
static const Pt2 bodyTop[] = {
    { -2.58f, 0.70f }, { -2.48f, 0.88f }, { -2.20f, 0.98f }, { -1.30f, 1.00f },
    { -1.05f, 0.99f }, { -0.95f, 0.74f }, { -0.85f, 0.68f }, {  0.10f, 0.68f },
    {  0.22f, 0.90f }, {  0.30f, 1.06f },
};
static const Pt2 bodyHw[]    = { { -2.58f, 0.55f }, { -2.35f, 0.74f }, { -1.80f, BODY_HW }, { 0.30f, BODY_HW } };
static const Pt2 bodyRound[] = { { -2.58f, 3.0f }, { -1.80f, 4.5f }, { 0.30f, 5.0f } };

static void bodySection(float x, Section *s, const void *ctx)
{
    (void)ctx;
    s->yTop = curveY(bodyTop, COUNT(bodyTop), x);
    float yb = archBottom(x, REAR_X, 0.50f, WHEEL_R, 0.52f);   /* rear wheel arch */
    s->yBottom = yb < s->yTop - 0.06f ? yb : s->yTop - 0.06f;
    s->halfWidth = curveY(bodyHw, COUNT(bodyHw), x);
    s->topHalfWidth = 0.93f * s->halfWidth;
    s->roundness = curveY(bodyRound, COUNT(bodyRound), x);
}
static const Loft body = { bodySection, NULL, -2.58f, 0.30f, 64, 30, 1, 1, NULL, NULL };

/* Frame rails, axles and the dark aprons under the bonnet sides. */
static void chassis(void)
{
    setMaterial(MAT_ACCENT);
    for (int s = -1; s <= 1; s += 2) {
        roundedBox(0.0f, 0.46f, s * 0.40f, 4.60f, 0.10f, 0.05f, 6.0f);
        boxAt(1.25f, 0.52f, s * 0.43f, 1.85f, 0.12f, 0.02f);
    }
    setMaterial(MAT_DARK_METAL);
    tube(v3(FRONT_X, WHEEL_R, -TRACK + 0.10f), v3(FRONT_X, WHEEL_R, TRACK - 0.10f), 0.03f, 0.03f, 10);
    tube(v3(REAR_X, WHEEL_R, -TRACK + 0.10f), v3(REAR_X, WHEEL_R, TRACK - 0.10f), 0.035f, 0.035f, 10);
    underbody(-2.30f, 2.30f, 0.405f, 0.42f);
}

/* Wings: a flattened rounded section swept along the wing line on each
 * side. The same sweep forms the front wing, the running board and the
 * rear wing, so they flow into each other. */
static void wings(void)
{
    Pt2 sec[20];
    for (int i = 0; i < 20; i++) {
        float t = 2.0f * (float)M_PI * i / 20, c = cosf(t), sn = sinf(t);
        sec[i].x = 0.18f  * (c  >= 0 ? sqrtf(c)  : -sqrtf(-c));
        sec[i].y = 0.032f * (sn >= 0 ? sqrtf(sn) : -sqrtf(-sn));
    }
    for (int s = -1; s <= 1; s += 2) {
        Vec3 path[COUNT(wingPath)];
        for (int i = 0; i < COUNT(wingPath); i++)
            path[i] = v3(wingPath[i].x, wingPath[i].y, s * WING_Z);
        setMaterial(MAT_ACCENT);
        sweep(path, COUNT(wingPath), 90, sec, 20, v3(0, 0, 1), 1);

        setMaterial(MAT_RUBBER);                 /* ribs on the running board */
        for (int k = -1; k <= 1; k++)
            boxAt(-0.17f, 0.486f, s * (WING_Z + 0.08f * k), 1.15f, 0.008f, 0.025f);
    }
}

/* Bonnet: chrome centre hinge, louvres, and three flexible chrome
 * exhaust pipes leaving each side and diving into the wing. */
static void bonnetDetails(void)
{
    setMaterial(MAT_CHROME);
    Pt2 hinge[2] = { { 0.32f, 0.0f }, { 2.15f, 0.0f } };
    loftStrip(&bonnet, hinge, 2, 1, 1.0f, 0.02f, 0.003f);

    for (int s = -1; s <= 1; s += 2) {
        for (int i = 0; i < 7; i++) {
            float x = 0.45f + 0.09f * i;
            Vec3 n, p = loftSide(&bonnet, x, 0.62f, (float)s, &n);
            louvre(p, n, 0.04f, 0.16f, MAT_PAINT);
        }
        Pt2 frameLines[4][2] = {
            { { 0.38f, 0.44f }, { 1.08f, 0.44f } }, { { 0.38f, 0.80f }, { 1.08f, 0.80f } },
            { { 0.38f, 0.44f }, { 0.38f, 0.80f } }, { { 1.08f, 0.44f }, { 1.08f, 0.80f } },
        };
        setMaterial(MAT_CHROME);
        for (int k = 0; k < 4; k++)
            loftStrip(&bonnet, frameLines[k], 2, 0, (float)s, 0.012f, 0.004f);

        for (int i = 0; i < 3; i++) {
            float x0 = 1.15f + 0.12f * i;
            Vec3 p0 = loftSide(&bonnet, x0, 0.30f, (float)s, NULL);
            Vec3 pipe[4] = {
                p0,
                v3(x0 - 0.05f, p0.y - 0.06f, s * 0.50f),
                v3(x0 - 0.18f, wingY(x0 - 0.18f) + 0.06f, s * 0.60f),
                v3(x0 - 0.30f, wingY(x0 - 0.30f) - 0.01f, s * 0.66f),
            };
            setMaterial(MAT_CHROME);
            sweepTube(pipe, 4, 20, 0.03f, 10);
        }
    }
}

/* Tall V-shaped grille: two chrome-framed panels meeting at a point at
 * the front, each with vertical chrome slats. */
static void grille(void)
{
    const float ax = 2.27f, ex = 2.17f, hw = 0.30f, y0 = 0.56f, y1 = 1.10f;
    for (int s = -1; s <= 1; s += 2) {
        Vec3 A0 = v3(ax, y0, 0.0f), A1 = v3(ax, y1, 0.0f);
        Vec3 E0 = v3(ex, y0, s * hw), E1 = v3(ex, y1, s * hw);
        /* Panel normal: up x (edge direction) points forward and out. */
        Vec3 n = vNorm(vCross(v3(0, 1, 0), vSub(E0, A0)));
        if (n.x < 0) n = vScale(n, -1.0f);

        setMaterial(MAT_MATTE);
        glNormal3f(n.x, n.y, n.z);
        glBegin(GL_QUADS);
            glVertex3f(A0.x, A0.y, A0.z); glVertex3f(E0.x, E0.y, E0.z);
            glVertex3f(E1.x, E1.y, E1.z); glVertex3f(A1.x, A1.y, A1.z);
        glEnd();

        setMaterial(MAT_CHROME);
        for (int i = 0; i < 7; i++) {
            float t = (i + 0.5f) / 7.0f;
            Vec3 b = vAdd(vAdd(A0, vScale(vSub(E0, A0), t)), vScale(n, 0.006f));
            tube(v3(b.x, y0 + 0.03f, b.z), v3(b.x, y1 - 0.03f, b.z), 0.008f, 0.008f, 6);
        }
        Vec3 loop[4] = { A0, E0, E1, A1 };
        tubeChain(loop, 4, 0.02f);
        tube(E1, A1, 0.03f, 0.03f, 10);                      /* heavier top edge */
    }
    setMaterial(MAT_CHROME);                                 /* top cover */
    glNormal3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_TRIANGLES);
        glVertex3f(ax, y1, 0.0f); glVertex3f(ex, y1, -hw); glVertex3f(ex, y1, hw);
    glEnd();
    tube(v3(ax, y0, 0.0f), v3(ax, y1 + 0.01f, 0.0f), 0.022f, 0.022f, 10);   /* nose spine */
    pushFrame(v3(2.23f, y1 + 0.01f, 0.0f), v3(0.0f, 1.0f, 0.0f));
        Pt2 cap[] = { { 0.045f, 0.0f }, { 0.045f, 0.03f }, { 0.045f, 0.03f },
                      { 0.03f, 0.05f }, { 0.0f, 0.055f } };
        revolve(cap, COUNT(cap), 16);
    glPopMatrix();
}

/* Headlamps on a cross bar, fog lamps, twin horns and the bumpers. */
static void frontEnd(void)
{
    setMaterial(MAT_CHROME);
    tube(v3(2.22f, 0.80f, -0.72f), v3(2.22f, 0.80f, 0.72f), 0.02f, 0.02f, 10);
    for (int s = -1; s <= 1; s += 2) {
        setMaterial(MAT_CHROME);
        tube(v3(2.22f, 0.80f, s * 0.56f), v3(2.18f, 0.87f, s * 0.56f), 0.02f, 0.02f, 8);
        headlampRound(v3(2.24f, 1.00f, s * 0.56f), v3(1, 0, 0), 0.15f, 0.22f, MAT_CHROME, MAT_CHROME);

        setMaterial(MAT_CHROME);                       /* fog lamps on stems */
        tube(v3(2.36f, 0.58f, s * 0.30f), v3(2.50f, 0.48f, s * 0.30f), 0.012f, 0.012f, 8);
        headlampRound(v3(2.40f, 0.62f, s * 0.30f), v3(1, 0, 0), 0.075f, 0.12f, MAT_CHROME, MAT_CHROME);

        /* Horn: a trumpet flaring towards the front. */
        pushFrame(v3(2.34f, 0.74f, s * 0.40f), v3(1, 0, 0));
            setMaterial(MAT_CHROME);
            Pt2 horn[] = { { 0.02f, -0.25f }, { 0.03f, -0.10f }, { 0.075f, 0.0f } };
            revolve(horn, COUNT(horn), 16);
            setMaterial(MAT_MATTE);
            glTranslatef(0.0f, 0.0f, -0.01f);
            disc(0.06f, 16);
        glPopMatrix();
    }

    /* Double-blade chrome bumpers, curving back at the ends. */
    setMaterial(MAT_CHROME);
    for (int k = 0; k < 2; k++) {
        float y = k ? 0.40f : 0.48f, r = k ? 0.028f : 0.035f;
        Vec3 fb[5] = { v3(2.46f, y, -0.92f), v3(2.54f, y, -0.80f), v3(2.56f, y, 0.0f),
                       v3(2.54f, y, 0.80f),  v3(2.46f, y, 0.92f) };
        sweepTube(fb, 5, 30, r, 10);
        Vec3 rb[5] = { v3(-2.52f, y + 0.02f, -0.88f), v3(-2.58f, y + 0.02f, -0.75f),
                       v3(-2.60f, y + 0.02f, 0.0f),  v3(-2.58f, y + 0.02f, 0.75f),
                       v3(-2.52f, y + 0.02f, 0.88f) };
        sweepTube(rb, 5, 30, r, 10);
    }
    for (int s = -1; s <= 1; s += 2) {
        tube(v3(2.42f, 0.46f, s * 0.40f), v3(2.55f, 0.46f, s * 0.40f), 0.018f, 0.018f, 8);
        tube(v3(-2.45f, 0.48f, s * 0.40f), v3(-2.59f, 0.48f, s * 0.40f), 0.018f, 0.018f, 8);
    }
}

/* Spare wheel standing against the outside of each front wing, behind
 * the front wheel, with a domed cover and a chrome ring. */
static void spareWheels(const CarModel *m)
{
    for (int s = -1; s <= 1; s += 2) {
        float z = s * 1.065f;
        glPushMatrix();
            glTranslatef(0.95f, 0.80f, z);
            drawWheel(&m->wheelLists[1], 0.0f, s < 0);
        glPopMatrix();
        pushFrame(v3(0.95f, 0.80f, z + s * 0.07f), v3(0.0f, 0.0f, (float)s));
            setMaterial(MAT_ACCENT);
            Pt2 cover[] = { { 0.31f, 0.0f }, { 0.26f, 0.05f }, { 0.14f, 0.09f }, { 0.0f, 0.10f } };
            revolve(cover, COUNT(cover), 32);
            setMaterial(MAT_CHROME);
            glutSolidTorus(0.022, 0.31, 8, 36);
        glPopMatrix();
    }
}

/* Cockpit: door walls with leather rolls, seats, wooden dashboard,
 * thin-rimmed steering wheel, folded hood, windscreen frame, driver. */
static void cockpit(void)
{
    setMaterialScaled(MAT_LEATHER, 0.5f);                         /* carpet */
    loftTopPatch(&body, -0.90f, 0.12f, -0.9f, 0.9f, 10, 8, 0.004f);

    for (int s = -1; s <= 1; s += 2) {
        setMaterial(MAT_PAINT);
        roundedBox(-0.365f, 0.83f, s * (BODY_HW - 0.04f), 1.17f, 0.34f, 0.08f, 6.0f);
        setMaterial(MAT_LEATHER);
        tube(v3(-0.95f, 1.00f, s * 0.72f), v3(0.22f, 1.00f, s * 0.72f), 0.03f, 0.03f, 10);
        pushFrame(v3(-0.365f, 0.84f, s * (BODY_HW - 0.081f)), v3(0, 0, (float)-s));
            roundRectDisc(1.10f, 0.28f, 0.04f);                       /* door lining */
        glPopMatrix();

        setMaterial(MAT_MATTE);                                       /* door shut lines */
        const float gx[2] = { -0.55f, 0.18f };
        for (int i = 0; i < 2; i++) {
            pushFrame(v3(gx[i], 0.83f, s * (BODY_HW + 0.002f)), v3(0, 0, (float)s));
                quadXY(-0.003f, -0.15f, 0.003f, 0.15f);
            glPopMatrix();
        }
        doorHandle(v3(0.08f, 0.93f, s * BODY_HW), v3(0, 0, (float)s), 0.10f, MAT_CHROME);
        seat(-0.55f, 0.82f, s * 0.36f, 0.58f, 0.55f, 0.45f, 12.0f, 0);
    }

    setMaterial(MAT_WOOD);                                            /* dashboard */
    roundedBox(0.16f, 0.95f, 0.0f, 0.10f, 0.20f, 1.40f, 6.0f);
    const float gz[3] = { -0.45f, -0.25f, 0.20f };
    for (int i = 0; i < 3; i++) {
        pushFrame(v3(0.108f, 0.97f, gz[i]), v3(-1, 0, 0));
            setMaterial(MAT_CHROME);
            annulus(0.045f, 0.055f, 20);
            setMaterialScaled(MAT_PLATE, 0.9f);
            disc(0.045f, 20);
        glPopMatrix();
    }

    Vec3 wc = v3(-0.05f, 1.08f, DRIVER_Z);
    setMaterial(MAT_DARK_METAL);
    tube(wc, v3(0.14f, 0.88f, DRIVER_Z), 0.022f, 0.022f, 8);
    steeringWheel(wc, vNorm(v3(-0.75f, 0.66f, 0.0f)), 0.22f, MAT_PLASTIC, 4);

    setMaterial(MAT_PLASTIC);                                         /* folded hood */
    ellipsoid(-1.12f, 1.06f, 0.0f, 0.26f, 0.15f, 0.74f);
    setMaterial(MAT_CHROME);
    for (int s = -1; s <= 1; s += 2)
        tube(v3(-1.02f, 1.00f, s * 0.74f), v3(-1.26f, 1.12f, s * 0.74f), 0.015f, 0.015f, 8);

    /* Windscreen frame: two panes forming a V, chrome all round. */
    for (int s = -1; s <= 1; s += 2) {
        Vec3 f[4] = { v3(0.32f, 1.06f, 0.0f), v3(0.22f, 1.06f, s * 0.72f),
                      v3(0.10f, 1.42f, s * 0.70f), v3(0.20f, 1.42f, 0.0f) };
        tubeLoop(f, 4, 0.015f);
        wiper(v3(0.30f, 1.08f, s * 0.20f), v3(0.27f, 1.09f, s * 0.50f), v3(0.95f, 0.3f, 0.0f));
    }

    driverFigure(v3(-0.42f, 0.86f, DRIVER_Z), wc, 0.22f, 0);
}

/* Tail: luggage trunk with chrome straps, small round tail lamps on the
 * rear wings, tailpipe. */
static void tail(void)
{
    setMaterial(MAT_ACCENT);
    roundedBox(-2.46f, 0.80f, 0.0f, 0.22f, 0.34f, 1.05f, 5.0f);
    setMaterial(MAT_CHROME);
    for (int s = -1; s <= 1; s += 2) {
        boxAt(-2.46f, 0.80f, s * 0.30f, 0.235f, 0.355f, 0.03f);
        roundedBox(-2.18f, 0.82f, s * WING_Z, 0.10f, 0.09f, 0.09f, 3.0f);
        lampRect(v3(-2.23f, 0.82f, s * WING_Z), v3(-1, 0, 0), 0.06f, 0.06f, 0.03f,
                 MAT_TAIL, MAT_CHROME);
    }
    tailpipe(v3(-2.50f, 0.40f, 0.45f), 0.03f, 0.8f, MAT_CHROME);
}

static void buildBody(const CarModel *m)
{
    setMaterial(MAT_PAINT);
    drawLoft(&body);
    drawLoft(&bonnet);
    chassis();
    wings();
    bonnetDetails();
    grille();
    frontEnd();
    spareWheels(m);
    cockpit();
    tail();
}

/* The two windscreen panes. */
static void buildGlass(const CarModel *m)
{
    (void)m;
    setMaterial(MAT_GLASS);
    for (int s = -1; s <= 1; s += 2) {
        Vec3 p0 = v3(0.32f, 1.06f, 0.0f),      p1 = v3(0.22f, 1.06f, s * 0.72f);
        Vec3 p2 = v3(0.10f, 1.42f, s * 0.70f), p3 = v3(0.20f, 1.42f, 0.0f);
        Vec3 n = vNorm(vCross(vSub(p1, p0), vSub(p3, p0)));
        if (n.x < 0) n = vScale(n, -1.0f);
        glNormal3f(n.x, n.y, n.z);
        glBegin(GL_QUADS);
            glVertex3f(p0.x, p0.y, p0.z); glVertex3f(p1.x, p1.y, p1.z);
            glVertex3f(p2.x, p2.y, p2.z); glVertex3f(p3.x, p3.y, p3.z);
        glEnd();
    }
}

void defineCar1936(CarModel *m)
{
    static const float paint[3]   = { 0.90f, 0.85f, 0.70f };   /* cream body  */
    static const float accent[3]  = { 0.04f, 0.04f, 0.05f };   /* black wings */
    static const float leather[3] = { 0.55f, 0.12f, 0.10f };   /* red leather */

    m->year = "1936";
    m->name = "540K Cabriolet";
    m->length = 5.23f;
    memcpy(m->paint, paint, sizeof paint);
    memcpy(m->accent, accent, sizeof accent);
    memcpy(m->leather, leather, sizeof leather);

    m->numWheels = 4;
    m->axleX[0] = FRONT_X;  m->axleX[1] = REAR_X;
    m->track[0] = TRACK;    m->track[1] = TRACK;

    /* Tall chrome wire wheels with fat, high-sidewall tyres. */
    WheelSpec w = { WHEEL_R, 0.17f, 0.23f, RIM_WIRE, MAT_CHROME, 0 };
    m->wheel[0] = w;
    m->wheel[1] = w;

    m->buildBody    = buildBody;
    m->buildGlass   = buildGlass;
    m->drawAnimated = NULL;
}
