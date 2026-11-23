#ifndef MATERIALS_H
#define MATERIALS_H

#include "scene.h"

/* ------------------------------------------------------------------ */
/* Surface materials for the cars                                      */
/*                                                                     */
/* GL_COLOR_MATERIAL already makes glColor set the ambient and diffuse */
/* colour. A material adds what glColor cannot: a specular highlight   */
/* (how strongly and how tightly it reflects the sun) and emission     */
/* (light the surface gives off itself, for lamps).                    */
/* ------------------------------------------------------------------ */
enum Material {
    MAT_PAINT,       /* main body colour (set per car)              */
    MAT_ACCENT,      /* second body colour: two-tone, wings         */
    MAT_CHROME,
    MAT_METAL,       /* brushed aluminium: modern rims, sills       */
    MAT_DARK_METAL,  /* blackened steel: early frames, brake parts  */
    MAT_GLASS,       /* semi-transparent, drawn in the glass pass   */
    MAT_RUBBER,      /* tyres, seals, wiper blades                  */
    MAT_PLASTIC,     /* dark grey trim, mirror bases, cladding      */
    MAT_GREY_TRIM,   /* lighter grey plastic (1980s cladding)       */
    MAT_GLOSS_BLACK, /* piano-black panels (EQS front)              */
    MAT_LEATHER,     /* upholstery (colour set per car)             */
    MAT_WOOD,
    MAT_BRASS,
    MAT_LAMP,        /* headlamp lens: glows                        */
    MAT_TAIL,        /* red tail lens: glows slightly               */
    MAT_INDICATOR,   /* amber lens                                  */
    MAT_MATTE,       /* black matte: arch liners, underbody         */
    MAT_SKIN,        /* driver figure                               */
    MAT_CLOTH,       /* driver's coat                               */
    MAT_PLATE,       /* blank number plate                          */
    MAT_COUNT
};

/* Per-car colours, used by MAT_PAINT, MAT_ACCENT and MAT_LEATHER.
 * Call before building a car's display lists. */
void setCarColors(const float paint[3], const float accent[3], const float leather[3]);

void setMaterial(int mat);
/* Same material with its colour multiplied by k (k > 1 lightens), for
 * highlight lines and shading variations. */
void setMaterialScaled(int mat, float k);

/* Back to "no specular, no emission", so scenery is unaffected. */
void resetMaterial(void);

#endif
