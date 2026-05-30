#ifndef WORLD_MONSTER_H
#define WORLD_MONSTER_H

#include "world.h"

EntityId World_FindMonsterAt(GameWorld* gw, int x, int y, EntityId exclude);
int World_CountAliveMonsters(GameWorld* gw);
bool World_AreAllMonstersDead(GameWorld* gw);
void World_UpdateMonsterAnimations(GameWorld* gw, float dt);

#endif
