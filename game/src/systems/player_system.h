#ifndef PLAYER_SYSTEM_H
#define PLAYER_SYSTEM_H

#include "world.h"

// Create the player entity in the ECS with all components.
void PlayerSystem_Spawn(GameWorld* gw);

// Grant experience and handle level-ups. Operates on CStats.
void PlayerSystem_GainExperience(GameWorld* gw, int amount);

// Allocate one stat point (0=STR, 1=DEX, 2=INT, 3=CON, 4=LCK).
bool PlayerSystem_AllocateStatPoint(GameWorld* gw, int statIdx);

#endif
