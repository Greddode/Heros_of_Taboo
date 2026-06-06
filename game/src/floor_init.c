#include "floor_init.h"
#include "debug_log.h"
#include "validation.h"
#include "systems/spawner_system.h"
#include "data/monster_data.h"
#include "systems/world_monster.h"
#include "map/map_helpers.h"
#include "map/procedural.h"
#include "game_balance.h"

void Floor_InitNewFloor(GameWorld* game)
{
    if (!game) { TraceLog(LOG_WARNING, "Floor_InitNewFloor: NULL game"); return; }
    if (!game->map) { TraceLog(LOG_WARNING, "Floor_InitNewFloor: game->map is NULL"); return; }

    game->shopkeeperEntity = ENTITY_NONE;

    BuildBlockingMap(game);

    Monster_InitTemplates();

    game->currentBiome = Biome_SelectForFloor(game->currentFloor);
    ProceduralMap_SetBiome(game->currentBiome);

    SpawnEntitiesFromObjects(game);

    SpawnMonstersForFloor(game);
    SpawnerSystem_SpawnPickups(game);

    if (GetRandomValue(1, 100) <= 3)
        SpawnShopRoom(game);

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

    DebugLog(DEBUG_FLOOR, "Floor_Init: floor=%d biome=%d map=%dx%d",
             game->currentFloor, (int)game->currentBiome, game->map->width, game->map->height);
    {
        ProceduralRoom rooms[MAX_GENERATED_ROOMS];
        int rc = GetGeneratedRooms(rooms, MAX_GENERATED_ROOMS);
        if (rc == 0) TraceLog(LOG_WARNING, "Floor_InitNewFloor: no rooms generated [floor=%d]", game->currentFloor);
        DebugLog(DEBUG_FLOOR, "Floor_Init: rooms=%d", rc);
        for (int i = 0; i < rc; i++) {
            DebugLog(DEBUG_GENERATION, "Floor_Init: room[%d] pos=(%d,%d) size=%dx%d",
                     i, rooms[i].x, rooms[i].y, rooms[i].w, rooms[i].h);
        }
    }
    if (game->stairX == 0 && game->stairY == 0 && game->currentFloor > 0)
        TraceLog(LOG_WARNING, "Floor_InitNewFloor: stair position is (0,0) [floor=%d]", game->currentFloor);
    DebugLog(DEBUG_FLOOR, "Floor_Init: stairs=(%d,%d)", game->stairX, game->stairY);
    if (game->escapeSpawned)
        DebugLog(DEBUG_FLOOR, "Floor_Init: escape=(%d,%d)", game->escapeX, game->escapeY);
}
