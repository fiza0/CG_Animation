#ifndef SCENE_H
#define SCENE_H

/* Platform-specific OpenGL headers: macOS keeps them in frameworks. */
#ifdef __APPLE__
  #define GL_SILENCE_DEPRECATION
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
  #include <GLUT/glut.h>
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
  #include <GL/glut.h>
#endif

/* World layout (units are roughly metres).
 * The road runs along the x axis; the car drives in the +x direction.
 * y is up, z is across the road. */
#define ROAD_HALF_LENGTH 200.0f   /* road spans x = -200 .. +200        */
#define ROAD_HALF_WIDTH    4.0f   /* road spans z = -4 .. +4 (two lanes) */
#define GROUND_SIZE      400.0f   /* half-size of the grass plane        */

/* The roadside pattern repeats every 12 units (trees and posts every 12,
 * centre dashes every 6). Shifting the whole scene by a multiple of this
 * period is invisible, which is how the road becomes endless. */
#define SCENE_PERIOD      12.0f

/* Sky colour is shared by the clear colour and the fog, so distant
 * objects fade into the horizon instead of ending abruptly. */
#define SKY_R 0.62f
#define SKY_G 0.80f
#define SKY_B 0.95f

void initScene(void);       /* call once after the window is created */
void setupLighting(void);   /* call every frame, after the camera     */
void drawScene(float followX); /* ground, road, roadside items, kept
                                  centred on followX (the car)    */

#endif