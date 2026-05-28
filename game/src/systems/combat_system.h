#ifndef COMBAT_SYSTEM_H
#define COMBAT_SYSTEM_H

#include "world.h"
#include "entity/entity.h"

typedef struct Game Game;

// Player melee attack on a tile. Returns true if a monster was targeted (hit or dodge).
bool CombatSystem_PlayerMeleeAttack(GameWorld* gw, Game* game, Entity* attacker, int targetX, int targetY);

#endif
