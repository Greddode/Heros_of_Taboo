#ifndef PLAYER_H
#define PLAYER_H

#include "components.h"

// Forward declaration for GainExperience
typedef struct GameWorld GameWorld;

// Calculate the amount of exp needed to reach a given level
int ExpForLevel(int level);

// Grant experience to the player and handle level-ups
void GainExperience(GameWorld* game, int amount);

// Allocate one stat point (0=STR, 1=DEX, 2=MGC, 3=CON, 4=LCK). Returns true if allocated.
bool AllocateStatPoint(GameWorld* game, CStats* s, int statIdx);

#endif
