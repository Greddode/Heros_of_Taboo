#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <stdbool.h>
#include <math.h>

#define FOG_RADIUS 7

#include "map/tmx/tmx.h"
#include "data/monster_data.h"
#include "inventory.h"
#include "systems/renderer.h"
#include "map/map_helpers.h"
#include "world.h"

void SetGuiScale(float scale);
float GetGuiScale(void);

bool InitGame(GameWorld* game, const char* tmxFile);
void CleanupGame(GameWorld* game);
void UpdateGame(GameWorld* game);
void DescendFloor(GameWorld* game);

#endif
