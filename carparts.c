#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "carparts.h"

static int roundRectPts(float w, float h, float rad, Pt2 *out)
{
    float hx = 0.5f * w - rad, hy = 0.5f * h - rad;
    if (hx < 0.0f) hx = 0.0f;
    if (hy < 0.0f) hy = 0.0f;
    const float cx[4] = { hx, -hx, -hx, hx }, cy[4] = { hy, hy, -hy, -hy };
    int k = 0;
    for (int q = 0; q < 4; q++)
        for (int s = 0; s <= 6; s++) {
            float a = (q * 90.0f + s * 15.0f) * (float)M_PI / 180.0f;
            out[k].x = cx[q] + rad * cosf(a);
            out[k].y = cy[q] + rad * sinf(a);
            k++;
        }
    return k;
}

void roundRectDisc(float w, float h, float rad)
{
    Pt2 p[28];
    int n = roundRectPts(w, h, rad, p);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0.0f, 0.0f, 0.0f);
        for (int i = 0; i <= n; i++) glVertex3f(p[i % n].x, p[i % n].y, 0.0f);
    glEnd();
}

void teardropDisc(float a, float b, float taper)
{
    glNormal3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0.0f, 0.0f, 0.0f);
        for (int i = 0; i <= 32; i++) {
            float t = 2.0f * (float)M_PI * i / 32;
            float k = 1.0f - taper * 0.5f * (1.0f - cosf(t));
            glVertex3f(a * cosf(t), b * sinf(t) * k, 0.0f);
        }
    glEnd();
}

void pushFrameFacing(Vec3 p, Vec3 n, Vec3 xWant)
{
    pushFrame(p, n);
    if (vDot(frameXAxis(n), xWant) < 0.0f) glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
}

void tubeLoop(const Vec3 *p, int n, float r)
{
    for (int i = 0; i < n; i++) {
        tube(p[i], p[(i + 1) % n], r, r, 8);
        ellipsoid(p[i].x, p[i].y, p[i].z, r, r, r);
    }
}

void tubeChain(const Vec3 *p, int n, float r)
{
    for (int i = 0; i + 1 < n; i++) tube(p[i], p[i + 1], r, r, 8);
    for (int i = 1; i + 1 < n; i++) ellipsoid(p[i].x, p[i].y, p[i].z, r, r, r);
}

void roundRectLoop(float w, float h, float rad, float tubeR)
{
    Pt2 p[28];
    Vec3 v[28];
    int n = roundRectPts(w, h, rad, p);
    for (int i = 0; i < n; i++) v[i] = v3(p[i].x, p[i].y, 0.0f);
    tubeLoop(v, n, tubeR);
}

void underbody(float x0, float x1, float y, float hw)
{
    setMaterial(MAT_MATTE);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex3f(x0, y, -hw); glVertex3f(x1, y, -hw);
        glVertex3f(x1, y,  hw); glVertex3f(x0, y,  hw);
    glEnd();
}

void archWell(float ax, float ay, float R, float zIn, float zOut, float sill)
{
    setMaterial(MAT_MATTE);
    float r = R - 0.008f;
    for (int s = -1; s <= 1; s += 2) {
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= 18; i++) {
            float a = (float)M_PI * i / 18, c = cosf(a), sn = sinf(a);
            glNormal3f(-c, -sn, 0.0f);
            glVertex3f(ax + r * c, ay + r * sn, s * zIn);
            glVertex3f(ax + r * c, ay + r * sn, s * zOut);
        }
        glEnd();
    }
    float top = ay + R - 0.012f;
    if (top > sill)
        boxAt(ax, 0.5f * (sill + top), 0.0f, 2.0f * R, top - sill, 2.0f * zIn);
}

void headlampRound(Vec3 p, Vec3 n, float r, float depth, int housingMat, int ringMat)
{
    pushFrame(p, n);
        Pt2 body[] = { { 0.0f, -1.05f * depth }, { 0.6f * r, -depth },
                       { 1.05f * r, -0.15f * depth }, { 1.05f * r, 0.0f } };
        setMaterial(housingMat);
        revolve(body, COUNT(body), 24);
        setMaterial(ringMat);
        glutSolidTorus(0.07 * r, r, 8, 28);
        Pt2 lens[] = { { 0.95f * r, 0.0f }, { 0.70f * r, 0.10f * r },
                       { 0.35f * r, 0.15f * r }, { 0.0f, 0.16f * r } };
        setMaterial(MAT_LAMP);
        revolve(lens, COUNT(lens), 24);
    glPopMatrix();
}

void lampRect(Vec3 p, Vec3 n, float w, float h, float rad, int lensMat, int frameMat)
{
    pushFrame(p, n);
        glTranslatef(0.0f, 0.0f, DECAL_LIFT);
        setMaterial(frameMat);
        roundRectDisc(w + 0.02f, h + 0.02f, rad + 0.01f);
        glTranslatef(0.0f, 0.0f, 0.002f);
        setMaterial(lensMat);
        roundRectDisc(w, h, rad);
    glPopMatrix();
}

void numberPlate(Vec3 p, Vec3 n, float w, float h)
{
    pushFrame(p, n);
        glTranslatef(0.0f, 0.0f, DECAL_LIFT);
        setMaterial(MAT_PLASTIC);
        roundRectDisc(w + 0.02f, h + 0.02f, 0.012f);
        glTranslatef(0.0f, 0.0f, 0.002f);
        setMaterial(MAT_PLATE);
        roundRectDisc(w, h, 0.008f);
    glPopMatrix();
}

void sideMirror(Vec3 base, float side, int housingMat, int indicator)
{
    float hz = base.z + side * 0.21f, hy = base.y + 0.08f;
    setMaterial(housingMat);
    tube(base, v3(base.x - 0.02f, base.y + 0.06f, base.z + side * 0.13f), 0.018f, 0.013f, 8);
    roundedBox(base.x - 0.01f, hy, hz, 0.10f, 0.10f, 0.17f, 3.0f);

    pushFrame(v3(base.x - 0.062f, hy, hz), v3(-1.0f, 0.0f, 0.0f));
        setMaterialScaled(MAT_CHROME, 0.55f);
        roundRectDisc(0.15f, 0.08f, 0.02f);
    glPopMatrix();

    if (indicator)
        lampRect(v3(base.x + 0.03f, hy - 0.02f, hz + side * 0.07f),
                 vNorm(v3(0.5f, 0.0f, side)), 0.09f, 0.014f, 0.006f,
                 MAT_INDICATOR, housingMat);
}

void doorHandle(Vec3 p, Vec3 n, float len, int mat)
{
    pushFrame(p, n);
        setMaterial(mat);
        roundedBox(0.0f, 0.0f, 0.012f, len, 0.026f, 0.024f, 3.0f);
    glPopMatrix();
}

void wiper(Vec3 pivot, Vec3 tip, Vec3 n)
{
    Vec3 u = vNorm(n);
    Vec3 lift = vScale(u, 0.014f);
    setMaterial(MAT_PLASTIC);
    tube(vAdd(pivot, lift), vAdd(tip, lift), 0.006f, 0.005f, 6);
    ellipsoid(pivot.x + lift.x, pivot.y + lift.y, pivot.z + lift.z, 0.016f, 0.016f, 0.016f);
    setMaterial(MAT_RUBBER);
    Vec3 a = vAdd(vAdd(pivot, vScale(vSub(tip, pivot), 0.12f)), vScale(u, 0.007f));
    Vec3 b = vAdd(tip, vScale(u, 0.007f));
    tube(a, b, 0.008f, 0.008f, 6);
}

void tailpipe(Vec3 p, float r, float len, int mat)
{
    setMaterial(mat);
    tube(v3(p.x + len, p.y, p.z), p, r, r, 12);
    pushFrame(v3(p.x - 0.001f, p.y, p.z), v3(-1.0f, 0.0f, 0.0f));
        setMaterial(MAT_MATTE);
        disc(0.8f * r, 12);
    glPopMatrix();
}

void louvre(Vec3 p, Vec3 n, float w, float h, int slatMat)
{
    pushFrame(p, n);
        setMaterial(MAT_MATTE);
        glTranslatef(0.0f, 0.0f, 0.002f);
        quadXY(-0.5f * w, -0.5f * h, 0.5f * w, 0.5f * h);
        setMaterial(slatMat);
        glTranslatef(-0.5f * w, 0.0f, 0.003f);
        glRotatef(-25.0f, 0.0f, 1.0f, 0.0f);
        roundedBox(0.5f * w, 0.0f, 0.0f, w, h, 0.006f, 3.0f);
    glPopMatrix();
}

void seat(float x, float y, float z, float w, float depth, float backH,
          float rakeDeg, int headrest)
{
    setMaterial(MAT_LEATHER);
    roundedBox(x + 0.5f * depth, y - 0.06f, z, depth, 0.13f, w, 4.0f);
    glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(rakeDeg, 0.0f, 0.0f, 1.0f);
        roundedBox(-0.05f, 0.5f * backH, 0.0f, 0.11f, backH, w, 4.0f);
        if (headrest)
            roundedBox(-0.05f, backH + 0.10f, 0.0f, 0.09f, 0.14f, 0.5f * w, 3.0f);
    glPopMatrix();
}

void steeringWheel(Vec3 c, Vec3 axis, float r, int rimMat, int spokes)
{
    pushFrame(c, axis);
        setMaterial(rimMat);
        glutSolidTorus(0.017, r, 8, 32);
        for (int i = 0; i < spokes; i++) {
            float a = (-90.0f + 360.0f * i / spokes) * (float)M_PI / 180.0f;
            tube(v3(0, 0, 0), v3(r * cosf(a), r * sinf(a), 0.0f), 0.011f, 0.011f, 6);
        }
        setMaterial(MAT_PLASTIC);
        ellipsoid(0.0f, 0.0f, 0.01f, 0.055f, 0.055f, 0.025f);
    glPopMatrix();
}

void driverFigure(Vec3 hip, Vec3 wc, float wr, int goggles)
{
    float x = hip.x, y = hip.y, z = hip.z;

    setMaterial(MAT_CLOTH);
    ellipsoid(x + 0.02f, y + 0.30f, z, 0.13f, 0.30f, 0.19f);
    for (int s = -1; s <= 1; s += 2) {
        Vec3 h0   = v3(x, y + 0.05f, z + s * 0.09f);
        Vec3 knee = v3(x + 0.42f, y + 0.08f, z + s * 0.10f);
        Vec3 foot = v3(x + 0.55f, y - 0.35f, z + s * 0.10f);
        tube(h0, knee, 0.075f, 0.065f, 10);
        tube(knee, foot, 0.06f, 0.05f, 10);

        Vec3 sh    = v3(x + 0.04f, y + 0.52f, z + s * 0.17f);
        Vec3 hand  = v3(wc.x, wc.y, wc.z + s * 0.85f * wr);
        Vec3 elbow = vAdd(vScale(vAdd(sh, hand), 0.5f), v3(0.0f, -0.10f, s * 0.05f));
        setMaterial(MAT_CLOTH);
        tube(sh, elbow, 0.05f, 0.045f, 10);
        tube(elbow, hand, 0.045f, 0.04f, 10);
        ellipsoid(elbow.x, elbow.y, elbow.z, 0.045f, 0.045f, 0.045f);
        setMaterial(MAT_SKIN);
        ellipsoid(hand.x, hand.y, hand.z, 0.04f, 0.04f, 0.04f);
    }

    setMaterial(MAT_SKIN);
    tube(v3(x + 0.03f, y + 0.56f, z), v3(x + 0.03f, y + 0.63f, z), 0.045f, 0.045f, 10);
    ellipsoid(x + 0.03f, y + 0.70f, z, 0.10f, 0.12f, 0.095f);

    setMaterialScaled(MAT_CLOTH, 0.5f);
    ellipsoid(x + 0.02f, y + 0.79f, z, 0.11f, 0.05f, 0.105f);
    roundedBox(x + 0.11f, y + 0.77f, z, 0.08f, 0.012f, 0.15f, 3.0f);

    if (goggles) {
        setMaterial(MAT_RUBBER);
        ellipsoid(x + 0.03f, y + 0.72f, z, 0.104f, 0.024f, 0.099f);
        for (int s = -1; s <= 1; s += 2) {
            pushFrame(v3(x + 0.128f, y + 0.72f, z + s * 0.04f), v3(1.0f, 0.0f, 0.0f));
                setMaterial(MAT_BRASS);
                annulus(0.020f, 0.029f, 14);
                glTranslatef(0.0f, 0.0f, 0.001f);
                setMaterialScaled(MAT_DARK_METAL, 1.4f);
                disc(0.021f, 14);
            glPopMatrix();
        }
    }
}

void drawCabin(const CabinSpec *c)
{
    float b = c->beltY;

    seat(c->seatX, b - 0.22f,  c->seatZ, 0.50f, 0.50f, 0.50f, 12.0f, c->headrests);
    seat(c->seatX, b - 0.22f, -c->seatZ, 0.50f, 0.50f, 0.50f, 12.0f, c->headrests);
    if (c->rearSeatX != 0.0f)
        seat(c->rearSeatX, b - 0.20f, 0.0f, 1.25f, 0.50f, c->rearBackH, 16.0f, 0);

    setMaterial(c->dashMat);
    roundedBox(c->dashX, b + 0.02f, 0.0f, 0.24f, 0.14f, 2.0f * c->dashHW, 4.0f);
    setMaterial(MAT_PLASTIC);
    roundedBox(c->dashX - 0.07f, b + 0.10f, -c->seatZ, 0.14f, 0.06f, 0.34f, 3.0f);

    Vec3 wc = v3(c->dashX - 0.30f, b + 0.10f, -c->seatZ);
    setMaterial(MAT_PLASTIC);
    tube(wc, v3(c->dashX - 0.08f, b - 0.02f, -c->seatZ), 0.025f, 0.03f, 8);
    steeringWheel(wc, v3(-0.85f, 0.53f, 0.0f), c->wheelR, MAT_PLASTIC, 3);
}

static int quadClass(Vec3 c, Vec3 n, const GlassLayout *g)
{
    if (n.y < -0.3f) return -1;
    if (c.x > g->wsX || c.x < g->rwX) return fabsf(n.z) < g->edgeNz ? 1 : 0;
    if (n.y > g->roofNy) return 0;
    for (int i = 0; i < g->nPillars; i++)
        if (fabsf(c.x - g->pillarX[i]) < g->pillarHalfW) return 0;
    return 1;
}

static int paintFilter(Vec3 c, Vec3 n, const void *ctx)
{
    return quadClass(c, n, (const GlassLayout *)ctx) == 0;
}

static int glassFilter(Vec3 c, Vec3 n, const void *ctx)
{
    return quadClass(c, n, (const GlassLayout *)ctx) == 1;
}

static float roofEdgeH(const Loft *g, float x, float side, float ny)
{
    float lo = 0.5f, hi = 0.995f;
    for (int i = 0; i < 16; i++) {
        float mid = 0.5f * (lo + hi);
        Vec3 n;
        loftSide(g, x, mid, side, &n);
        if (n.y < ny) lo = mid; else hi = mid;
    }
    return 0.5f * (lo + hi);
}

void drawClosedCarCommon(const ClosedCar *c)
{
    const GlassLayout *gl = &c->layout;

    setMaterial(MAT_PAINT);
    drawLoft(c->lower);

    Loft g = *c->glass;
    g.filter = paintFilter;
    g.filterCtx = gl;
    g.capRear = g.capFront = 0;
    setMaterial(c->roofMat);
    drawLoft(&g);

    for (int a = 0; a < 2; a++)
        archWell(c->axleX[a], c->axleY[a], c->archR[a],
                 c->hw - c->wellDepth, 0.98f * c->hw, c->sill);
    underbody(c->axleX[1] + c->archR[1], c->axleX[0] - c->archR[0],
              c->sill - 0.004f, 0.85f * c->hw);

    setMaterial(MAT_PLASTIC);
    loftTopPatch(c->lower, g.x0 + 0.15f, g.x1 - 0.15f, -0.8f, 0.8f, 14, 8, 0.004f);

    float xa = gl->rwX, xb = gl->wsX;
    for (int s = -1; s <= 1; s += 2) {
        Pt2 belt[12], roof[12];
        for (int i = 0; i < 12; i++) {
            float x = xa + (xb - xa) * i / 11;
            belt[i].x = roof[i].x = x;
            belt[i].y = loftH(c->glass, x, c->beltY(x) + 0.015f);
            roof[i].y = roofEdgeH(c->glass, x, (float)s, gl->roofNy);
        }
        setMaterial(c->beltMat);
        loftStrip(c->glass, belt, 12, 0, (float)s, 0.026f, 0.004f);
        setMaterial(c->roofTrimMat);
        loftStrip(c->glass, roof, 12, 0, (float)s, 0.020f, 0.004f);

        for (int i = 0; i < gl->nPillars; i++) {
            float px = gl->pillarX[i];
            Pt2 pl[2] = { { px, loftH(c->glass, px, c->beltY(px) + 0.01f) },
                          { px, roofEdgeH(c->glass, px, (float)s, gl->roofNy) } };
            setMaterial(c->pillarTrimMat);
            loftStrip(c->glass, pl, 2, 0, (float)s, 2.0f * gl->pillarHalfW + 0.01f, 0.003f);
        }

        setMaterial(MAT_MATTE);
        for (int i = 0; i < c->nDoorX; i++) {
            Pt2 d[2] = { { c->doorX[i], c->doorBottomH }, { c->doorX[i], c->doorTopH } };
            loftStrip(c->lower, d, 2, 0, (float)s, 0.006f, 0.002f);
        }
        if (c->nDoorX >= 2) {
            Pt2 sl[2] = { { c->doorX[0], c->doorBottomH },
                          { c->doorX[c->nDoorX - 1], c->doorBottomH } };
            loftStrip(c->lower, sl, 2, 0, (float)s, 0.006f, 0.002f);
        }
    }

    drawCabin(&c->cabin);
}

void drawClosedCarGlass(const ClosedCar *c)
{
    Loft g = *c->glass;
    g.filter = glassFilter;
    g.filterCtx = &c->layout;
    g.capRear = g.capFront = 0;
    setMaterial(MAT_GLASS);
    drawLoft(&g);
}
