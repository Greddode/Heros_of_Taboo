#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include "world.h"

// Visible map tile layers (fog-of-war aware). Call inside an active BeginMode2D.
void RenderSystem_DrawMap(const GameWorld* gw);

// ECS monsters with movement interpolation. Call inside an active BeginMode2D.
void RenderSystem_DrawMonsters(const GameWorld* gw, float monsterT);

// Map + entities + projectile
void RenderSystem_World(const GameWorld* gw);

// Health bar, floor label, combat log
void RenderSystem_HUD(const GameWorld* gw);

#endif
