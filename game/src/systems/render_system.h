#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include "world.h"

// Map + entities + projectile
void RenderSystem_World(const GameWorld* gw);

// Health bar, floor label, combat log
void RenderSystem_HUD(const GameWorld* gw);

#endif
