#ifndef PROPS_H
#define PROPS_H

#include "scene.h"

/* ------------------------------------------------------------------ */
/* Roadside objects ("props") and distant skylines                     */
/*                                                                     */
/* Every prop is modelled standing on y = 0 at its local origin, with  */
/* its front (the side that faces the road) pointing towards +z. That  */
/* is correct as-is on the far side of the road; on the near side      */
/* drawProp turns it round by 180 degrees.                             */
/* ------------------------------------------------------------------ */
enum PropType {
    PROP_NONE,
    /* "Row" props run along the road for one whole 12 m slot, from     */
    /* their origin to x + 12, so neighbouring slots join up.           */
    PROP_FENCE, PROP_TELEGRAPH, PROP_STONE_WALL, PROP_BARRIER,
    /* Single objects. */
    PROP_HAYSTACK, PROP_COTTAGE, PROP_ROUND_TREE, PROP_CONE_TREE, PROP_BUSH,
    PROP_GAS_LAMP, PROP_ELECTRIC_LAMP, PROP_PETROL_PUMP, PROP_BILLBOARD,
    PROP_LAMP_POST, PROP_HOUSE, PROP_FLAT_HOUSE, PROP_DINER_SIGN,
    PROP_SODIUM_LAMP, PROP_APARTMENT, PROP_ROAD_SIGN, PROP_PHONE_BOX,
    PROP_MODERN_LAMP, PROP_OFFICE, PROP_GANTRY, PROP_LED_LAMP,
    PROP_GLASS_TOWER, PROP_TURBINE, PROP_CHARGER, PROP_SOLAR,
    PROP_COUNT
};

enum SkylineType {
    SKY_HILLS, SKY_HILLS_SPIRE, SKY_CHIMNEYS, SKY_TOWN,
    SKY_MIDRISE, SKY_TOWERS, SKY_OFFICES, SKY_GLASS,
    SKYLINE_COUNT
};

#define SKYLINE_Z  -150.0f   /* distance of the backdrop from the road */

void initProps(void);          /* builds all display lists; needs a GL context */
void updateProps(float dt);    /* advances animated parts (turbine rotors)     */

/* Draw one prop at world (x, z), stretched vertically by scaleY from its
 * base (used by the time ripple). Props with z > 0 are on the near side
 * and are turned to face the road. The gantry always spans the road at
 * z = 0, whatever z is given. */
void drawProp(int type, float x, float z, float scaleY);

/* The distant backdrop, kept centred on followX so it never slides past.
 * Drawn unlit in one flat colour; alpha < 1 for crossfades. */
void drawSkyline(int type, float followX, const float color[3], float alpha);

#endif
