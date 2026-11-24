#ifndef TRANSITION_H
#define TRANSITION_H

#include "scene.h"

void initTimeline(void);
void updateTimeline(float dt, float carX);

void timelineShiftWorld(float dx);

void timelineNext(void);
void timelineRestart(void);
void timelineToggleEffect(void);

void timelineSetInspect(int on);
int  timelineInspecting(void);
void timelineInspectStep(int dir);

int         timelineModelIndex(void);
int         timelineTargetIndex(void);
int         timelineTravelling(void);
float       timelineProgress(void);
float       timelineCarScale(void);
const char *timelineEffectName(void);

struct RippleState;
const struct RippleState *timelineRipple(void);

void drawTunnels(void);
void drawParticles(float carX);

#endif
