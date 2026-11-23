#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "wheels.h"
#include "geom.h"
#include "materials.h"

#define SEGS 40   /* points around a wheel */

/* All wheel parts are built around the axle (the z axis) with the
 * outer face towards +z. Revolved profiles are (r, z) pairs; walking
 * from the outer edge towards the centre gives normals facing +z. */

/* Height (z) of a profile at radius r, by straight-line interpolation,
 * so decals and ribs can sit exactly on a dished disc. */
static float profileZ(const Pt2 *p, int n, float r)
{
    for (int i = 0; i + 1 < n; i++) {
        float r0 = p[i].x, r1 = p[i + 1].x;
        float lo = r0 < r1 ? r0 : r1, hi = r0 < r1 ? r1 : r0;
        if (r >= lo && r <= hi && hi - lo > 1e-6f)
            return p[i].y + (p[i + 1].y - p[i].y) * (r - r0) / (r1 - r0);
    }
    return p[n - 1].y;
}

/* ------------------------------------------------------------------ */
/* Tyre: rounded shoulders and bulging sidewalls                       */
/* ------------------------------------------------------------------ */
static void tyre(const WheelSpec *w)
{
    float R = w->radius, Rr = w->rimRadius, hw = 0.5f * w->width;
    float side = R - Rr;
    float c = fminf(0.35f * side, 0.45f * hw);   /* shoulder rounding */

    /* Inner sidewall (-z) out to the tread and back down the outer
     * sidewall (+z). Normals (dz, -dr) then point out of the rubber. */
    Pt2 p[] = {
        { Rr,                -0.80f * hw },
        { Rr + 0.30f * side, -hw },
        { R - c,             -hw },
        { R - 0.3f * c,      -hw + 0.3f * c },
        { R,                 -hw + c },
        { R,                  hw - c },
        { R - 0.3f * c,       hw - 0.3f * c },
        { R - c,              hw },
        { Rr + 0.30f * side,  hw },
        { Rr,                 0.80f * hw },
    };
    setMaterial(MAT_RUBBER);
    revolve(p, COUNT(p), SEGS);

    if (w->treads) {
        /* Grooves: thin black bands just proud of the tread. */
        setMaterial(MAT_MATTE);
        const float zs[3] = { -0.32f, 0.0f, 0.32f };
        for (int i = 0; i < 3; i++) {
            float z = zs[i] * (hw - c), g = 0.006f;
            Pt2 band[] = { { R + 0.002f, z - g }, { R + 0.002f, z + g } };
            revolve(band, 2, SEGS);
        }
    }
}

/* Inside of the rim seen through spokes or around a disc. */
static void rimBarrel(float Rr, float hw, int mat)
{
    /* Travelling towards -z at constant r gives normals pointing in. */
    Pt2 p[] = { { Rr, 0.75f * hw }, { Rr - 0.01f, 0.65f * hw },
                { Rr - 0.01f, -0.65f * hw }, { Rr, -0.75f * hw } };
    setMaterial(mat);
    revolve(p, COUNT(p), SEGS);
}

/* Dark disc that closes the back of a wheel so it is not see-through. */
static void backPlate(float r, float z)
{
    setMaterial(MAT_MATTE);
    glPushMatrix();
        glTranslatef(0.0f, 0.0f, z);
        glRotatef(180.0f, 0.0f, 1.0f, 0.0f);   /* face -z */
        disc(r, SEGS);
        glRotatef(180.0f, 0.0f, 1.0f, 0.0f);   /* and +z, seen through gaps */
        disc(r, SEGS);
    glPopMatrix();
}

/* Hub drum with domed ends, length 2L along the axle. */
static void hubDrum(float hr, float L, int mat)
{
    Pt2 p[] = { { 0.0f, -L - 0.01f }, { hr, -L }, { hr, -L },
                { hr, L }, { hr, L }, { 0.0f, L + 0.012f } };
    setMaterial(mat);
    revolve(p, COUNT(p), 16);
}

/* ------------------------------------------------------------------ */
/* Rim styles                                                          */
/* ------------------------------------------------------------------ */

/* Wire wheel: thin rim, long hub, spokes laced from alternate ends of
 * the hub to the rim, each one leaning a little ahead or behind, which
 * gives the crossed pattern of a real wire wheel. */
static void wireRim(const WheelSpec *w)
{
    float R = w->radius, Rr = w->rimRadius, hw = 0.5f * w->width;
    float hr = fmaxf(0.035f, 0.07f * R), L = fmaxf(1.6f * hw, 0.06f);
    const int N = 32;

    Pt2 rim[] = { { Rr, 0.9f * hw }, { Rr - 0.02f, 0.7f * hw },
                  { Rr - 0.02f, -0.7f * hw }, { Rr, -0.9f * hw } };
    setMaterial(w->rimMat);
    revolve(rim, COUNT(rim), SEGS);
    hubDrum(hr, L, w->rimMat);

    float lean = 2.0f * (float)M_PI / N * 1.5f;
    for (int i = 0; i < N; i++) {
        float ph = 2.0f * (float)M_PI * i / N;
        float zh = (i % 2 == 0) ? 0.8f * L : -0.8f * L;
        float pr = ph + (((i / 2) % 2) ? lean : -lean);
        Vec3 a = v3(0.9f * hr * cosf(ph), 0.9f * hr * sinf(ph), zh);
        Vec3 b = v3((Rr - 0.02f) * cosf(pr), (Rr - 0.02f) * sinf(pr), 0.15f * zh);
        tube(a, b, 0.0045f, 0.0045f, 5);
    }

    /* Chrome centre-lock cap on the outer end of the hub. */
    setMaterial(MAT_CHROME);
    Pt2 cap[] = { { hr * 1.3f, L }, { hr * 1.2f, L + 0.02f }, { 0.0f, L + 0.03f } };
    revolve(cap, COUNT(cap), 16);
}

/* Artillery wheel: wooden felloe (outer ring), thick tapered spokes,
 * wooden hub and a brass cap. */
static void artilleryRim(const WheelSpec *w)
{
    float R = w->radius, Rr = w->rimRadius, hw = 0.5f * w->width;
    float fr = Rr - 0.06f, fw = 1.1f * hw;
    float hr = 0.17f * R;

    /* Felloe cross-section: front face, inner face, back face. */
    Pt2 felloe[] = { { Rr, fw }, { fr, fw }, { fr, fw }, { fr, -fw },
                     { fr, -fw }, { Rr, -fw } };
    setMaterial(MAT_WOOD);
    revolve(felloe, COUNT(felloe), SEGS);

    for (int i = 0; i < 12; i++) {
        float ph = 2.0f * (float)M_PI * i / 12;
        Vec3 a = v3(hr * 0.9f * cosf(ph), hr * 0.9f * sinf(ph), 0.0f);
        Vec3 b = v3((fr + 0.005f) * cosf(ph), (fr + 0.005f) * sinf(ph), 0.0f);
        tubeCapped(a, b, 0.065f * R, 0.04f * R, 8);   /* taper outwards */
    }

    hubDrum(hr, 1.3f * hw, MAT_WOOD);
    setMaterial(MAT_BRASS);
    Pt2 cap[] = { { 0.70f * hr, 1.3f * hw }, { 0.62f * hr, 1.3f * hw + 0.03f },
                  { 0.30f * hr, 1.3f * hw + 0.05f }, { 0.0f, 1.3f * hw + 0.055f } };
    revolve(cap, COUNT(cap), 16);
}

/* Painted steel disc, chrome trim ring, domed chrome hubcap. */
static void steelCapRim(const WheelSpec *w)
{
    float Rr = w->rimRadius, hw = 0.5f * w->width;

    rimBarrel(Rr, hw, MAT_DARK_METAL);
    backPlate(Rr, -0.5f * hw);

    Pt2 dish[] = { { 0.98f * Rr, 0.15f * hw }, { 0.76f * Rr, 0.35f * hw },
                   { 0.55f * Rr, 0.30f * hw }, { 0.0f, 0.30f * hw } };
    setMaterial(w->rimMat);
    revolve(dish, COUNT(dish), SEGS);

    Pt2 ring[] = { { 1.00f * Rr, 0.45f * hw }, { 0.90f * Rr, 0.52f * hw },
                   { 0.78f * Rr, 0.42f * hw } };
    setMaterial(MAT_CHROME);
    revolve(ring, COUNT(ring), SEGS);

    Pt2 cap[] = { { 0.52f * Rr, 0.33f * hw }, { 0.50f * Rr, 0.55f * hw },
                  { 0.38f * Rr, 0.80f * hw }, { 0.18f * Rr, 0.93f * hw },
                  { 0.0f, 0.96f * hw } };
    revolve(cap, COUNT(cap), SEGS);
}

/* Full chrome hubcap with raised radial ribs and a painted centre. */
static void ribbedCapRim(const WheelSpec *w)
{
    float Rr = w->rimRadius, hw = 0.5f * w->width;

    rimBarrel(Rr, hw, MAT_DARK_METAL);
    backPlate(Rr, -0.5f * hw);

    Pt2 dome[] = { { 1.00f * Rr, 0.30f * hw }, { 0.92f * Rr, 0.42f * hw },
                   { 0.60f * Rr, 0.55f * hw }, { 0.25f * Rr, 0.65f * hw },
                   { 0.0f, 0.67f * hw } };
    setMaterial(MAT_CHROME);
    revolve(dome, COUNT(dome), SEGS);

    for (int i = 0; i < 12; i++) {
        float ph = 2.0f * (float)M_PI * i / 12;
        float r0 = 0.30f * Rr, r1 = 0.88f * Rr;
        Vec3 a = v3(r0 * cosf(ph), r0 * sinf(ph), profileZ(dome, COUNT(dome), r0));
        Vec3 b = v3(r1 * cosf(ph), r1 * sinf(ph), profileZ(dome, COUNT(dome), r1));
        tube(a, b, 0.007f, 0.007f, 6);
    }

    setMaterial(MAT_PAINT);
    glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.672f * hw);
        disc(0.22f * Rr, SEGS);
    glPopMatrix();
}

/* Flat silver alloy disc with a ring of round dark holes. */
static void holeRim(const WheelSpec *w, int holes, float holeR, float ringR,
                    float stretch)
{
    float Rr = w->rimRadius, hw = 0.5f * w->width;

    rimBarrel(Rr, hw, MAT_DARK_METAL);
    backPlate(Rr, -0.5f * hw);

    Pt2 face[] = { { 1.00f * Rr, 0.36f * hw }, { 0.92f * Rr, 0.44f * hw },
                   { 0.30f * Rr, 0.41f * hw }, { 0.18f * Rr, 0.46f * hw },
                   { 0.0f, 0.47f * hw } };
    setMaterial(MAT_METAL);
    revolve(face, COUNT(face), SEGS);

    /* Holes: dark ovals sitting just on the face. A slightly larger,
     * darker grey ring around each one suggests the recess. */
    float z = profileZ(face, COUNT(face), ringR * Rr);
    for (int i = 0; i < holes; i++) {
        glPushMatrix();
            glRotatef(360.0f * i / holes, 0.0f, 0.0f, 1.0f);
            glTranslatef(ringR * Rr, 0.0f, z + 0.002f);
            setMaterialScaled(MAT_METAL, 0.55f);
            ellipseDisc(holeR * Rr * stretch * 1.12f, holeR * Rr * 1.12f, 16);
            glTranslatef(0.0f, 0.0f, 0.001f);
            setMaterial(MAT_MATTE);
            ellipseDisc(holeR * Rr * stretch, holeR * Rr, 16);
        glPopMatrix();
    }

    /* Five lug nuts and a centre cap. */
    float zc = profileZ(face, COUNT(face), 0.28f * Rr);
    setMaterial(MAT_DARK_METAL);
    for (int i = 0; i < 5; i++) {
        float ph = 2.0f * (float)M_PI * i / 5;
        Vec3 a = v3(0.28f * Rr * cosf(ph), 0.28f * Rr * sinf(ph), zc);
        tubeCapped(a, v3(a.x, a.y, zc + 0.012f), 0.012f, 0.012f, 6);
    }
    setMaterial(MAT_CHROME);
    Pt2 cap[] = { { 0.14f * Rr, 0.47f * hw }, { 0.12f * Rr, 0.47f * hw + 0.012f },
                  { 0.0f, 0.47f * hw + 0.016f } };
    revolve(cap, COUNT(cap), 20);
}

/* Spoked alloy: dark barrel, brake disc, then flat-topped spokes. */
static void spokeRim(const WheelSpec *w, int pairs, int perPair, float spread,
                     float spokeW)
{
    float Rr = w->rimRadius, hw = 0.5f * w->width;

    rimBarrel(Rr, hw, MAT_DARK_METAL);
    backPlate(Rr, -0.7f * hw);

    /* Brake disc: turns with the wheel, sits behind the spokes. */
    setMaterial(MAT_METAL);
    glPushMatrix();
        glTranslatef(0.0f, 0.0f, -0.15f * hw);
        annulus(0.25f * Rr, 0.80f * Rr, SEGS);
    glPopMatrix();
    setMaterial(MAT_DARK_METAL);
    hubDrum(0.24f * Rr, 0.15f * hw, MAT_DARK_METAL);

    /* Rim lip. */
    Pt2 lip[] = { { 1.00f * Rr, 0.55f * hw }, { 0.96f * Rr, 0.50f * hw },
                  { 0.90f * Rr, 0.42f * hw } };
    setMaterial(MAT_METAL);
    revolve(lip, COUNT(lip), SEGS);

    /* Spokes: rounded bars from the centre to the rim. */
    for (int k = 0; k < pairs; k++) {
        for (int j = 0; j < perPair; j++) {
            float off = (perPair == 2) ? (j ? spread : -spread) : 0.0f;
            float ang = 360.0f * k / pairs + off;
            glPushMatrix();
                glRotatef(ang, 0.0f, 0.0f, 1.0f);
                roundedBox(0.58f * Rr, 0.0f, 0.38f * hw, 0.74f * Rr,
                           spokeW * Rr, 0.06f, 3.0f);
            glPopMatrix();
        }
    }

    /* Centre: hub disc and a small cap. */
    Pt2 hub[] = { { 0.24f * Rr, 0.36f * hw }, { 0.20f * Rr, 0.46f * hw },
                  { 0.08f * Rr, 0.50f * hw }, { 0.0f, 0.51f * hw } };
    revolve(hub, COUNT(hub), 24);
}

/* Aero wheel: a nearly closed gloss black disc with a silver lip,
 * five silver blades and shallow dark slots between them. */
static void aeroRim(const WheelSpec *w)
{
    float Rr = w->rimRadius, hw = 0.5f * w->width;

    rimBarrel(Rr, hw, MAT_DARK_METAL);
    backPlate(Rr, -0.6f * hw);

    Pt2 face[] = { { 0.96f * Rr, 0.42f * hw }, { 0.86f * Rr, 0.48f * hw },
                   { 0.40f * Rr, 0.51f * hw }, { 0.0f, 0.53f * hw } };
    setMaterial(MAT_GLOSS_BLACK);
    revolve(face, COUNT(face), SEGS);

    Pt2 lip[] = { { 1.00f * Rr, 0.40f * hw }, { 0.98f * Rr, 0.47f * hw },
                  { 0.88f * Rr, 0.49f * hw } };
    setMaterial(MAT_METAL);
    revolve(lip, COUNT(lip), SEGS);

    float z = profileZ(face, COUNT(face), 0.65f * Rr) + 0.002f;
    for (int i = 0; i < 5; i++) {
        glPushMatrix();
            glRotatef(72.0f * i, 0.0f, 0.0f, 1.0f);
            /* Silver blade: a wedge from the centre towards the rim. */
            setMaterial(MAT_METAL);
            glNormal3f(0.0f, 0.0f, 1.0f);
            glBegin(GL_QUADS);
                glVertex3f(0.22f * Rr, -0.06f * Rr, z);
                glVertex3f(0.84f * Rr, -0.16f * Rr, z);
                glVertex3f(0.84f * Rr,  0.02f * Rr, z);
                glVertex3f(0.22f * Rr,  0.04f * Rr, z);
            glEnd();
            /* Shallow slot, turned 36 degrees to sit between blades. */
            glRotatef(36.0f, 0.0f, 0.0f, 1.0f);
            glTranslatef(0.66f * Rr, 0.0f, z + 0.001f);
            setMaterial(MAT_MATTE);
            ellipseDisc(0.16f * Rr, 0.035f * Rr, 16);
        glPopMatrix();
    }

    setMaterial(MAT_METAL);
    glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.535f * hw);
        disc(0.10f * Rr, 20);
    glPopMatrix();
}

/* Brake calliper: does not turn, sits over the disc at the upper rear. */
static void calliper(const WheelSpec *w)
{
    float Rr = w->rimRadius, hw = 0.5f * w->width;
    setMaterial(MAT_DARK_METAL);
    glPushMatrix();
        glRotatef(150.0f, 0.0f, 0.0f, 1.0f);
        /* Straddles the brake disc (z = -0.15 hw) but stays behind the
         * spokes, which pass in front of it as the wheel turns. */
        glTranslatef(0.70f * Rr, 0.0f, -0.20f * hw);
        glRotatef(90.0f, 0.0f, 0.0f, 1.0f);   /* long side runs round the disc */
        roundedBox(0.0f, 0.0f, 0.0f, 0.30f * Rr, 0.15f * Rr, 0.05f, 3.0f);
    glPopMatrix();
}

/* ------------------------------------------------------------------ */
void buildWheel(const WheelSpec *w, WheelLists *out)
{
    out->spin = glGenLists(1);
    glNewList(out->spin, GL_COMPILE);
        tyre(w);
        switch (w->style) {
            case RIM_WIRE:       wireRim(w);                              break;
            case RIM_ARTILLERY:  artilleryRim(w);                         break;
            case RIM_STEEL_CAP:  steelCapRim(w);                          break;
            case RIM_RIBBED_CAP: ribbedCapRim(w);                         break;
            case RIM_15HOLE:     holeRim(w, 15, 0.075f, 0.64f, 1.0f);     break;
            case RIM_8HOLE:      holeRim(w, 8, 0.085f, 0.58f, 1.9f);      break;
            case RIM_5TWIN:      spokeRim(w, 5, 2, 7.0f, 0.075f);         break;
            case RIM_10SPOKE:    spokeRim(w, 10, 1, 0.0f, 0.065f);        break;
            default:             aeroRim(w);                              break;
        }
        resetMaterial();
    glEndList();

    out->fixed = 0;
    if (w->style == RIM_5TWIN || w->style == RIM_10SPOKE || w->style == RIM_AERO) {
        out->fixed = glGenLists(1);
        glNewList(out->fixed, GL_COMPILE);
            calliper(w);
            resetMaterial();
        glEndList();
    }
}

void drawWheel(const WheelLists *wl, float angleDeg, int farSide)
{
    glPushMatrix();
        /* Mirroring in z keeps the turning direction (a mirror across
         * the axle's own axis does not reverse a spin about that axis)
         * and puts the outer face on the -z side. */
        if (farSide) glScalef(1.0f, 1.0f, -1.0f);
        glPushMatrix();
            /* Rolling forward (+x) is a clockwise turn seen from +z,
             * which is a negative rotation about the axle. */
            glRotatef(-angleDeg, 0.0f, 0.0f, 1.0f);
            glCallList(wl->spin);
        glPopMatrix();
        if (wl->fixed) glCallList(wl->fixed);
    glPopMatrix();
}
