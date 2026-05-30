#ifndef COMBAT_SYSTEM_H
#define COMBAT_SYSTEM_H

#include "world.h"

typedef struct GameWorld GameWorld;

// Player melee attack on a tile. Returns true if a monster was targeted (hit or dodge).
bool CombatSystem_PlayerMeleeAttack(GameWorld* game, EntityId attackerId, int targetX, int targetY);

#endif
