#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"

// Forward declaration so GainExperience can accept Game* without circular includes
typedef struct Game Game;

// Player-specific data wrapping the base Entity
typedef struct {
    Entity ent;
    int exp;
    int expToNext;
} Player;

// Calculate the amount of exp needed to reach a given level
int ExpForLevel(int level);

// Grant experience to the player and handle level-ups
void GainExperience(Game* game, int amount);

// Allocate one stat point (0=STR, 1=DEX, 2=INT, 3=CON, 4=LCK). Returns true if allocated.
bool AllocateStatPoint(Entity* ent, int statIdx);

#endif
