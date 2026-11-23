#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "materials.h"

typedef struct {
    float color[4];     /* diffuse and ambient (via glColor), with alpha */
    float specular;     /* grey level of the highlight                   */
    float shininess;    /* 0..128: higher = smaller, sharper highlight   */
    float emission[3];  /* light given off by the surface itself         */
} MaterialDef;

/* Colours for PAINT, ACCENT and LEATHER are filled in per car. */
static MaterialDef table[MAT_COUNT] = {
    [MAT_PAINT]       = { {0.7f, 0.7f, 0.7f, 1.0f},    0.60f,  64.0f, {0, 0, 0} },
    [MAT_ACCENT]      = { {0.1f, 0.1f, 0.1f, 1.0f},    0.60f,  64.0f, {0, 0, 0} },
    [MAT_CHROME]      = { {0.78f, 0.79f, 0.82f, 1.0f}, 1.00f, 120.0f, {0, 0, 0} },
    [MAT_METAL]       = { {0.66f, 0.67f, 0.70f, 1.0f}, 0.50f,  30.0f, {0, 0, 0} },
    [MAT_DARK_METAL]  = { {0.16f, 0.16f, 0.17f, 1.0f}, 0.35f,  25.0f, {0, 0, 0} },
    [MAT_GLASS]       = { {0.16f, 0.20f, 0.27f, 0.5f}, 0.90f, 100.0f, {0, 0, 0} },
    [MAT_RUBBER]      = { {0.06f, 0.06f, 0.06f, 1.0f}, 0.05f,   5.0f, {0, 0, 0} },
    [MAT_PLASTIC]     = { {0.13f, 0.13f, 0.14f, 1.0f}, 0.20f,  15.0f, {0, 0, 0} },
    [MAT_GREY_TRIM]   = { {0.42f, 0.43f, 0.45f, 1.0f}, 0.20f,  15.0f, {0, 0, 0} },
    [MAT_GLOSS_BLACK] = { {0.03f, 0.03f, 0.04f, 1.0f}, 0.90f, 100.0f, {0, 0, 0} },
    [MAT_LEATHER]     = { {0.35f, 0.20f, 0.12f, 1.0f}, 0.15f,  10.0f, {0, 0, 0} },
    [MAT_WOOD]        = { {0.42f, 0.25f, 0.12f, 1.0f}, 0.10f,  10.0f, {0, 0, 0} },
    [MAT_BRASS]       = { {0.78f, 0.60f, 0.25f, 1.0f}, 0.80f,  60.0f, {0, 0, 0} },
    [MAT_LAMP]        = { {0.95f, 0.95f, 0.88f, 1.0f}, 0.90f, 100.0f, {0.55f, 0.55f, 0.48f} },
    [MAT_TAIL]        = { {0.70f, 0.04f, 0.04f, 1.0f}, 0.90f, 100.0f, {0.25f, 0.01f, 0.01f} },
    [MAT_INDICATOR]   = { {0.95f, 0.55f, 0.08f, 1.0f}, 0.90f, 100.0f, {0.20f, 0.10f, 0.00f} },
    [MAT_MATTE]       = { {0.05f, 0.05f, 0.05f, 1.0f}, 0.00f,   1.0f, {0, 0, 0} },
    [MAT_SKIN]        = { {0.85f, 0.66f, 0.52f, 1.0f}, 0.10f,  10.0f, {0, 0, 0} },
    [MAT_CLOTH]       = { {0.30f, 0.26f, 0.20f, 1.0f}, 0.05f,   5.0f, {0, 0, 0} },
    [MAT_PLATE]       = { {0.92f, 0.92f, 0.90f, 1.0f}, 0.30f,  20.0f, {0, 0, 0} },
};

void setCarColors(const float paint[3], const float accent[3], const float leather[3])
{
    memcpy(table[MAT_PAINT].color,   paint,   3 * sizeof(float));
    memcpy(table[MAT_ACCENT].color,  accent,  3 * sizeof(float));
    memcpy(table[MAT_LEATHER].color, leather, 3 * sizeof(float));
}

void setMaterialScaled(int mat, float k)
{
    if (mat < 0 || mat >= MAT_COUNT) mat = MAT_PAINT;
    const MaterialDef *d = &table[mat];

    float c[3];
    for (int i = 0; i < 3; i++) {
        c[i] = d->color[i] * k;
        if (c[i] > 1.0f) c[i] = 1.0f;
    }
    glColor4f(c[0], c[1], c[2], d->color[3]);

    GLfloat spec[4] = { d->specular, d->specular, d->specular, 1.0f };
    GLfloat emis[4] = { d->emission[0], d->emission[1], d->emission[2], 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  spec);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, d->shininess);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  emis);
}

void setMaterial(int mat) { setMaterialScaled(mat, 1.0f); }

void resetMaterial(void)
{
    GLfloat zero[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
