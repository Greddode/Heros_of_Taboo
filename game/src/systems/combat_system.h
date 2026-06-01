#ifndef COMBAT_SYSTEM_H
#define COMBAT_SYSTEM_H

#include "world.h"

typedef struct GameWorld GameWorld;

// Player melee attack on a tile. Returns true if a monster was targeted (hit or dodge).
bool CombatSystem_PlayerMeleeAttack(GameWorld* game, EntityId attackerId, int targetX, int targetY);

// Fire a ranged attack from the player in facingDir using the equipped bow.
// Returns true if the action consumed a turn (shot fired, even if it missed).
bool CombatSystem_PlayerRangedAttack(GameWorld* game, EntityId attackerId);

// Throw the equipped melee weapon in facingDir.
// Unequips weapon and consumes it regardless of hit/miss.
// Returns true if a weapon was thrown (turn consumed).
bool CombatSystem_PlayerThrowWeapon(GameWorld* game, EntityId attackerId);

#endif
