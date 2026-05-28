#ifndef WORLD_MONSTER_H
#define WORLD_MONSTER_H

#include "world.h"

EntityId World_FindMonsterAt(const GameWorld* gw, int x, int y, EntityId exclude);
int World_CountAliveMonsters(const GameWorld* gw);
bool World_AreAllMonstersDead(const GameWorld* gw);
void World_UpdateMonsterAnimations(GameWorld* gw, float dt);

#endif
