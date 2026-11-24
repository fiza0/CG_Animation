#ifndef MATERIALS_H
#define MATERIALS_H

#include "scene.h"

enum Material {
    MAT_PAINT,
    MAT_ACCENT,
    MAT_CHROME,
    MAT_METAL,
    MAT_DARK_METAL,
    MAT_GLASS,
    MAT_RUBBER,
    MAT_PLASTIC,
    MAT_GREY_TRIM,
    MAT_GLOSS_BLACK,
    MAT_LEATHER,
    MAT_WOOD,
    MAT_BRASS,
    MAT_LAMP,
    MAT_TAIL,
    MAT_INDICATOR,
    MAT_MATTE,
    MAT_SKIN,
    MAT_CLOTH,
    MAT_PLATE,
    MAT_COUNT
};

void setCarColors(const float paint[3], const float accent[3], const float leather[3]);

void setMaterial(int mat);
void setMaterialScaled(int mat, float k);

void resetMaterial(void);

#endif
