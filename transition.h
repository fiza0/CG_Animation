#ifndef TRANSITION_H
#define TRANSITION_H

#include "scene.h"

/* ------------------------------------------------------------------ */
/* Timeline: a small state machine that decides which era is shown    */
/*                                                                     */
/*   DRIVE (show model i) -> TRANSITION (effect, swap to i+1) -> DRIVE */
/*   ... and after the last model it loops back to the first.          */
/* ------------------------------------------------------------------ */

void initTimeline(void);
void updateTimeline(float dt, float carX);   /* call every frame */

/* The car's x is pulled back now and then to keep floats precise.
 * Anything the timeline placed in the world must move by the same dx. */
void timelineShiftWorld(float dx);

/* Keyboard actions. */
void timelineNext(void);           /* next transition now, or queued   */
void timelineRestart(void);        /* back to the first era            */
void timelineToggleEffect(void);   /* tunnel <-> scale swap            */

/* Queries for drawing the car and the HUD. */
int         timelineModelIndex(void);   /* model to draw right now       */
int         timelineTargetIndex(void);  /* model being travelled to      */
int         timelineTravelling(void);   /* 1 between start and swap      */
float       timelineProgress(void);     /* 0..1 of the way to the target */
float       timelineCarScale(void);     /* 1, or less during scale swap  */
const char *timelineEffectName(void);

/* The background ripple started at the last model swap. The era theme
 * to draw is timelineModelIndex(); while ripple->active, the scene also
 * blends in the previous theme, ripple->fromEra. */
struct RippleState;
const struct RippleState *timelineRipple(void);

/* Drawing. Tunnels are opaque and drawn before the car; particles are
 * see-through and drawn after it. */
void drawTunnels(void);
void drawParticles(float carX);

#endif
