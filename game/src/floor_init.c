#include "floor_init.h"
#include "systems/spawner_system.h"
#include "data/monster_data.h"
#include "systems/world_monster.h"
#include "map/map_helpers.h"
#include "map/procedural.h"
#include "game_balance.h"

void Floor_InitNewFloor(GameWorld* game)
{
    if (!game || !game->map) return;

    BuildBlockingMap(game);

    Monster_InitTemplates();

    game->currentBiome = Biome_SelectForFloor(game->currentFloor);

    SpawnEntitiesFromObjects(game);

    SpawnMonstersForFloor(game);
    SpawnerSystem_SpawnPickups(game);

    game->stairX = GetStairX();
    game->stairY = GetStairY();

    game->state = STATE_PLAYER_TURN;
    game->turnCount = 0;
    game->enemyTurnCooldown = 0.0f;
    game->animTimer = 0.0f;
    game->monsterAnimTimer = 0.0f;
    game->animDuration = 0.0f;
    game->monsterAnimDuration = 0.0f;
    game->sprintBypassRoom = false;
    game->projectile.active = false;
    game->projectileTimer = 0.0f;
    game->projectileDuration = 0.0f;

    for (int y = 0; y < game->map->height; y++) {
        for (int x = 0; x < game->map->width; x++) {
            game->visibility[y][x] = 0;
        }
    }
    RevealFOW(game);

    if (game->playerEntity != ENTITY_NONE) {
        CPosition* pp = World_GetPosition(&game->ecs, game->playerEntity);
        game->camera.target = (Vector2){
            pp->x * game->map->tileWidth  + game->map->tileWidth  / 2,
            pp->y * game->map->tileHeight + game->map->tileHeight / 2
        };
    }
    game->camera.offset = (Vector2){ GetScreenWidth() / 2, GetScreenHeight() / 2 };
    game->camera.rotation = 0;
    game->camera.zoom = DEFAULT_CAMERA_ZOOM;
}
