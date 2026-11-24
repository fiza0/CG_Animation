#include <stddef.h>
#include <math.h>
#include <string.h>
#include "eras.h"

#define S(n, f, b) { PROP_##n, PROP_##f, PROP_##b }

static const float ROAD_COLORS[][3] = {
    [ROAD_DIRT]         = { 0.62f, 0.50f, 0.34f },
    [ROAD_GRAVEL]       = { 0.52f, 0.48f, 0.42f },
    [ROAD_CONCRETE]     = { 0.70f, 0.68f, 0.62f },
    [ROAD_ASPHALT_OLD]  = { 0.28f, 0.28f, 0.30f },
    [ROAD_ASPHALT]      = { 0.22f, 0.22f, 0.24f },
    [ROAD_ASPHALT_DARK] = { 0.12f, 0.12f, 0.13f },
};

static const EraTheme themes[] = {
    { .sky = {0.86f, 0.78f, 0.62f}, .ground = {0.55f, 0.52f, 0.34f},
      .sunColor = {1.00f, 0.88f, 0.68f}, .ambient = {0.42f, 0.38f, 0.32f},
      .fogStart = 45.0f, .fogEnd = 130.0f, .portal = {1.00f, 0.70f, 0.30f},
      .roadStyle = ROAD_DIRT, .markingStyle = MARK_RUTS,
      .rowProps = { PROP_FENCE, PROP_NONE },
      .slotProps = {
          S(HAYSTACK, ROUND_TREE, ROUND_TREE), S(NONE, COTTAGE, NONE),
          S(NONE, ROUND_TREE, ROUND_TREE),     S(HAYSTACK, NONE, COTTAGE),
          S(NONE, ROUND_TREE, NONE),           S(NONE, HAYSTACK, ROUND_TREE),
          S(HAYSTACK, COTTAGE, NONE),          S(NONE, ROUND_TREE, ROUND_TREE) },
      .skyline = SKY_HILLS },

    { .sky = {0.80f, 0.76f, 0.66f}, .ground = {0.50f, 0.50f, 0.36f},
      .sunColor = {0.95f, 0.88f, 0.74f}, .ambient = {0.40f, 0.38f, 0.34f},
      .fogStart = 45.0f, .fogEnd = 135.0f, .portal = {1.00f, 0.75f, 0.40f},
      .roadStyle = ROAD_GRAVEL, .markingStyle = MARK_NONE,
      .rowProps = { PROP_FENCE, PROP_TELEGRAPH },
      .slotProps = {
          S(GAS_LAMP, COTTAGE, ROUND_TREE),    S(GAS_LAMP, ROUND_TREE, NONE),
          S(GAS_LAMP, NONE, COTTAGE),          S(GAS_LAMP, COTTAGE, NONE),
          S(GAS_LAMP, ROUND_TREE, ROUND_TREE), S(GAS_LAMP, NONE, COTTAGE),
          S(GAS_LAMP, COTTAGE, NONE),          S(GAS_LAMP, ROUND_TREE, ROUND_TREE) },
      .skyline = SKY_HILLS_SPIRE },

    { .sky = {0.84f, 0.82f, 0.72f}, .ground = {0.48f, 0.52f, 0.36f},
      .sunColor = {0.98f, 0.92f, 0.80f}, .ambient = {0.40f, 0.39f, 0.36f},
      .fogStart = 50.0f, .fogEnd = 140.0f, .portal = {1.00f, 0.82f, 0.50f},
      .roadStyle = ROAD_CONCRETE, .markingStyle = MARK_SLAB_JOINTS,
      .rowProps = { PROP_STONE_WALL, PROP_TELEGRAPH },
      .slotProps = {
          S(ELECTRIC_LAMP, COTTAGE, ROUND_TREE), S(ELECTRIC_LAMP, CONE_TREE, NONE),
          S(ELECTRIC_LAMP, HOUSE, COTTAGE),      S(PETROL_PUMP, NONE, ROUND_TREE),
          S(ELECTRIC_LAMP, ROUND_TREE, NONE),    S(ELECTRIC_LAMP, COTTAGE, COTTAGE),
          S(ELECTRIC_LAMP, CONE_TREE, NONE),     S(ELECTRIC_LAMP, HOUSE, ROUND_TREE) },
      .skyline = SKY_CHIMNEYS },

    { .sky = {0.50f, 0.70f, 0.92f}, .ground = {0.36f, 0.62f, 0.26f},
      .sunColor = {1.00f, 0.95f, 0.85f}, .ambient = {0.36f, 0.36f, 0.40f},
      .fogStart = 60.0f, .fogEnd = 170.0f, .portal = {1.00f, 0.55f, 0.45f},
      .roadStyle = ROAD_ASPHALT_OLD, .markingStyle = MARK_WHITE_DASH,
      .rowProps = { PROP_LAMP_POST, PROP_NONE },
      .slotProps = {
          S(BUSH, HOUSE, ROUND_TREE),  S(NONE, CONE_TREE, HOUSE),
          S(NONE, BILLBOARD, NONE),    S(BUSH, HOUSE, CONE_TREE),
          S(NONE, ROUND_TREE, HOUSE),  S(NONE, HOUSE, NONE),
          S(BUSH, BILLBOARD, ROUND_TREE), S(NONE, CONE_TREE, HOUSE) },
      .skyline = SKY_TOWN },

    { .sky = {0.38f, 0.62f, 0.95f}, .ground = {0.30f, 0.64f, 0.24f},
      .sunColor = {1.00f, 0.97f, 0.88f}, .ambient = {0.36f, 0.37f, 0.42f},
      .fogStart = 60.0f, .fogEnd = 175.0f, .portal = {1.00f, 0.45f, 0.70f},
      .roadStyle = ROAD_ASPHALT_OLD, .markingStyle = MARK_WHITE_DASH_EDGES,
      .rowProps = { PROP_LAMP_POST, PROP_NONE },
      .slotProps = {
          S(BUSH, FLAT_HOUSE, ROUND_TREE), S(NONE, BILLBOARD, FLAT_HOUSE),
          S(NONE, DINER_SIGN, NONE),       S(BUSH, FLAT_HOUSE, CONE_TREE),
          S(NONE, CONE_TREE, FLAT_HOUSE),  S(NONE, FLAT_HOUSE, NONE),
          S(BUSH, BILLBOARD, ROUND_TREE),  S(NONE, ROUND_TREE, FLAT_HOUSE) },
      .skyline = SKY_MIDRISE },

    { .sky = {0.74f, 0.73f, 0.70f}, .ground = {0.38f, 0.50f, 0.28f},
      .sunColor = {1.00f, 0.92f, 0.78f}, .ambient = {0.40f, 0.38f, 0.38f},
      .fogStart = 55.0f, .fogEnd = 160.0f, .portal = {1.00f, 0.60f, 0.15f},
      .roadStyle = ROAD_ASPHALT, .markingStyle = MARK_YELLOW_DASH,
      .rowProps = { PROP_SODIUM_LAMP, PROP_NONE },
      .slotProps = {
          S(BUSH, CONE_TREE, APARTMENT), S(NONE, ROUND_TREE, NONE),
          S(NONE, NONE, APARTMENT),      S(BUSH, CONE_TREE, ROUND_TREE),
          S(NONE, ROUND_TREE, APARTMENT), S(NONE, NONE, NONE),
          S(BUSH, CONE_TREE, APARTMENT), S(NONE, ROUND_TREE, CONE_TREE) },
      .skyline = SKY_TOWERS },

    { .sky = {0.62f, 0.72f, 0.84f}, .ground = {0.36f, 0.52f, 0.30f},
      .sunColor = {0.98f, 0.95f, 0.86f}, .ambient = {0.38f, 0.39f, 0.42f},
      .fogStart = 55.0f, .fogEnd = 165.0f, .portal = {0.60f, 0.85f, 0.70f},
      .roadStyle = ROAD_ASPHALT, .markingStyle = MARK_YELLOW_DASH,
      .rowProps = { PROP_SODIUM_LAMP, PROP_NONE },
      .slotProps = {
          S(BUSH, CONE_TREE, APARTMENT),     S(ROAD_SIGN, ROUND_TREE, NONE),
          S(NONE, NONE, APARTMENT),          S(BUSH, CONE_TREE, ROUND_TREE),
          S(NONE, ROUND_TREE, APARTMENT),    S(PHONE_BOX, NONE, NONE),
          S(BUSH, CONE_TREE, APARTMENT),     S(NONE, ROUND_TREE, CONE_TREE) },
      .skyline = SKY_TOWERS },

    { .sky = {0.48f, 0.68f, 0.94f}, .ground = {0.32f, 0.58f, 0.28f},
      .sunColor = {1.00f, 0.98f, 0.92f}, .ambient = {0.37f, 0.38f, 0.42f},
      .fogStart = 60.0f, .fogEnd = 180.0f, .portal = {0.50f, 0.85f, 1.00f},
      .roadStyle = ROAD_ASPHALT, .markingStyle = MARK_CATS_EYES,
      .rowProps = { PROP_BARRIER, PROP_NONE },
      .slotProps = {
          S(MODERN_LAMP, ROUND_TREE, OFFICE), S(MODERN_LAMP, BUSH, ROUND_TREE),
          S(MODERN_LAMP, CONE_TREE, OFFICE),  S(MODERN_LAMP, NONE, NONE),
          S(MODERN_LAMP, ROUND_TREE, OFFICE), S(MODERN_LAMP, BUSH, CONE_TREE),
          S(MODERN_LAMP, CONE_TREE, OFFICE),  S(MODERN_LAMP, NONE, ROUND_TREE) },
      .skyline = SKY_OFFICES },

    { .sky = {0.46f, 0.68f, 0.96f}, .ground = {0.31f, 0.60f, 0.29f},
      .sunColor = {0.98f, 0.98f, 0.95f}, .ambient = {0.37f, 0.39f, 0.44f},
      .fogStart = 60.0f, .fogEnd = 180.0f, .portal = {0.55f, 0.90f, 1.00f},
      .roadStyle = ROAD_ASPHALT, .markingStyle = MARK_CATS_EYES,
      .rowProps = { PROP_BARRIER, PROP_NONE },
      .slotProps = {
          S(MODERN_LAMP, ROUND_TREE, OFFICE), S(MODERN_LAMP, BUSH, ROUND_TREE),
          S(MODERN_LAMP, CONE_TREE, OFFICE),  S(MODERN_LAMP, NONE, NONE),
          S(NONE, GANTRY, OFFICE),            S(MODERN_LAMP, BUSH, CONE_TREE),
          S(MODERN_LAMP, CONE_TREE, OFFICE),  S(MODERN_LAMP, NONE, ROUND_TREE) },
      .skyline = SKY_OFFICES },

    { .sky = {0.42f, 0.66f, 0.98f}, .ground = {0.30f, 0.62f, 0.30f},
      .sunColor = {0.96f, 0.98f, 1.00f}, .ambient = {0.36f, 0.39f, 0.45f},
      .fogStart = 65.0f, .fogEnd = 180.0f, .portal = {0.75f, 0.92f, 1.00f},
      .roadStyle = ROAD_ASPHALT_DARK, .markingStyle = MARK_MODERN,
      .rowProps = { PROP_LED_LAMP, PROP_NONE },
      .slotProps = {
          S(BUSH, ROUND_TREE, GLASS_TOWER), S(NONE, BUSH, TURBINE),
          S(NONE, CONE_TREE, GLASS_TOWER),  S(BUSH, NONE, TURBINE),
          S(NONE, ROUND_TREE, NONE),        S(NONE, BUSH, TURBINE),
          S(BUSH, CONE_TREE, GLASS_TOWER),  S(NONE, NONE, TURBINE) },
      .skyline = SKY_GLASS },

    { .sky = {0.40f, 0.65f, 1.00f}, .ground = {0.28f, 0.64f, 0.32f},
      .sunColor = {0.94f, 0.97f, 1.00f}, .ambient = {0.37f, 0.40f, 0.47f},
      .fogStart = 65.0f, .fogEnd = 180.0f, .portal = {0.85f, 0.95f, 1.00f},
      .roadStyle = ROAD_ASPHALT_DARK, .markingStyle = MARK_MODERN_CYCLE_LANE,
      .rowProps = { PROP_LED_LAMP, PROP_NONE },
      .slotProps = {
          S(CHARGER, SOLAR, GLASS_TOWER), S(SOLAR, ROUND_TREE, TURBINE),
          S(BUSH, SOLAR, GLASS_TOWER),    S(CHARGER, BUSH, TURBINE),
          S(SOLAR, ROUND_TREE, NONE),     S(NONE, SOLAR, TURBINE),
          S(CHARGER, CONE_TREE, GLASS_TOWER), S(BUSH, SOLAR, TURBINE) },
      .skyline = SKY_GLASS },
};

#define THEME_COUNT ((int)(sizeof themes / sizeof themes[0]))

int eraThemeCount(void) { return THEME_COUNT; }

const EraTheme *getEraTheme(int index)
{
    if (index < 0 || index >= THEME_COUNT) index = 0;
    return &themes[index];
}

const float *roadColor(int roadStyle)
{
    if (roadStyle < ROAD_DIRT || roadStyle > ROAD_ASPHALT_DARK) roadStyle = ROAD_ASPHALT;
    return ROAD_COLORS[roadStyle];
}

void lerpColor(float out[3], const float a[3], const float b[3], float t)
{
    for (int i = 0; i < 3; i++) out[i] = a[i] + (b[i] - a[i]) * t;
}

void skylineColor(float out[3], const EraTheme *t)
{
    static const float distant[3] = { 0.30f, 0.32f, 0.36f };
    lerpColor(out, t->sky, distant, 0.28f);
}

void blendThemeGlobals(EraTheme *out, const EraTheme *a, const EraTheme *b, float t)
{
    *out = *b;
    lerpColor(out->sky,      a->sky,      b->sky,      t);
    lerpColor(out->ground,   a->ground,   b->ground,   t);
    lerpColor(out->sunColor, a->sunColor, b->sunColor, t);
    lerpColor(out->ambient,  a->ambient,  b->ambient,  t);
    lerpColor(out->portal,   a->portal,   b->portal,   t);
    out->fogStart = a->fogStart + (b->fogStart - a->fogStart) * t;
    out->fogEnd   = a->fogEnd   + (b->fogEnd   - a->fogEnd)   * t;
}

float smoothstep01(float t)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

float rippleRadius(const RippleState *r)
{
    return RIPPLE_SPEED * r->time;
}

float rippleProgress(const RippleState *r, float distance)
{
    float p = (rippleRadius(r) - distance) / RIPPLE_RAMP;
    return p < 0.0f ? 0.0f : (p > 1.0f ? 1.0f : p);
}

float rippleGlobalFactor(const RippleState *r)
{
    return smoothstep01(r->time / RIPPLE_BLEND_TIME);
}
