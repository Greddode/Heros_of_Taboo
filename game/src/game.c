#include "game.h"
#include "systems/spawner_system.h"
#include "world.h"
#include "data/monster_data.h"
#include "systems/ai_system.h"
#include "systems/combat_system.h"
#include "systems/world_monster.h"
#include "systems/player_system.h"
#include "ui/combat_log.h"
#include "systems/input_system.h"
#include "audio.h"
#include "map/procedural.h"
#include "resources.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static float s_guiScaleSetting = 1.0f;



// UI scaling helper function
float GetUIScale(void) {
    const float baseWidth = 1024.0f;
    const float baseHeight = 768.0f;
    float widthScale = (float)GetScreenWidth() / baseWidth;
    float heightScale = (float)GetScreenHeight() / baseHeight;
    float scale = fminf(widthScale, heightScale);
    scale = fmaxf(scale, 0.75f);
    return scale * s_guiScaleSetting;
}

void SetGuiScale(float scale) {
    if (scale < 1.0f) scale = 1.0f;
    if (scale > 4.0f) scale = 4.0f;
    s_guiScaleSetting = scale;
}

float GetGuiScale(void) {
    return s_guiScaleSetting;
}

void HandleInput(GameWorld* game) {
    static InventoryUIState s_uiState = {0};
    static bool s_inited = false;
    if (!s_inited) {
        InventoryUI_Init(&s_uiState);
        s_inited = true;
    }
    InputSystem_Inventory(game, &s_uiState);
    InputSystem_PlayerTurn(game, &s_uiState);
}

void UpdateGame(GameWorld* game) {
    if (!game) return;

    World* w = &game->ecs;

    if (game->animTimer > 0.0f) {
        game->animTimer -= GetFrameTime();
        if (game->animTimer < 0.0f) game->animTimer = 0.0f;
    }
    if (game->monsterAnimTimer > 0.0f) {
        game->monsterAnimTimer -= GetFrameTime();
        if (game->monsterAnimTimer < 0.0f) game->monsterAnimTimer = 0.0f;
    }

    if (game->projectileTimer > 0.0f) {
        game->projectileTimer -= GetFrameTime();
        if (game->projectileTimer <= 0.0f) {
            game->projectileTimer = 0.0f;
            game->projectile.active = false;
        }
    }

    if (game->playerEntity != ENTITY_NONE && World_HasComponents(w, game->playerEntity, COMP_HIT_FLASH)) {
        CHitFlash* hf = World_GetHitFlash(w, game->playerEntity);
        if (hf->timer > 0.0f) {
            hf->timer -= GetFrameTime();
            if (hf->timer < 0.0f) hf->timer = 0.0f;
        }
    }

    if (game->playerEntity != ENTITY_NONE) {
        CStats* ps = World_GetStats(w, game->playerEntity);
        if (ps->alive && World_HasComponents(w, game->playerEntity, COMP_SPRITE_ANIM)) {
            CSpriteAnim* sa = World_GetSprite(w, game->playerEntity);
            if (sa->tex && sa->tex->id > 0) {
                static float s_playerAnimTimer = 0.0f;
                s_playerAnimTimer += GetFrameTime();
                if (s_playerAnimTimer >= 0.5f) {
                    s_playerAnimTimer -= 0.5f;
                    sa->frame = (sa->frame + 1) % 4;
                }
            }
        }
    }

    if (game->levelUpTimer > 0.0f) {
        game->levelUpTimer -= GetFrameTime();
        if (game->levelUpTimer < 0.0f) game->levelUpTimer = 0.0f;
    }

    World_UpdateMonsterAnimations(game, GetFrameTime());

    if (game->state == STATE_GAME_OVER || game->state == STATE_WIN) return;

    if (game->state == STATE_ENEMY_TURN) {
        if (game->enemyTurnCooldown > 0.0f) {
            game->enemyTurnCooldown -= GetFrameTime();
            return;
        }

        if (game->animatingEnemyTurn) {
            if (game->projectile.active && game->projectileTimer > 0.0f) return;
            if (game->monsterAnimTimer > 0.0f) return;
            game->animatingEnemyTurn = false;
            game->state = STATE_PLAYER_TURN;
            return;
        }

        bool playerAlive = AISystem_ProcessAll(game, game->timeWaited);

        if (!playerAlive) {
            if (game->playerEntity != ENTITY_NONE) {
                World_GetStats(w, game->playerEntity)->alive = false;
                World_GetStats(w, game->playerEntity)->hp = 0;
            }
            game->state = STATE_GAME_OVER;
            return;
        }

        if (World_CountAliveMonsters(game) > 0 && World_AreAllMonstersDead(game)) {
            if (game->currentFloor >= game->maxFloors) {
                if (!game->escapeSpawned) {
                    SpawnEscapeTile(game);
                    game->escapeSpawned = true;
                    CombatLog_Add(&game->combatLog, GREEN, "A teleport circle has appeared somewhere for you to escape!");
                }
            } else if (!game->floorClearedNotified) {
                game->floorClearedNotified = true;
                CombatLog_Add(&game->combatLog, YELLOW, "This floor sounds earily Quite now");
            }
        }

        game->monsterAnimTimer = MOVE_ANIM_DURATION;
        game->monsterAnimDuration = MOVE_ANIM_DURATION;
        game->animatingEnemyTurn = true;
        if (!game->projectile.active) {
            game->animatingEnemyTurn = false;
            game->state = STATE_PLAYER_TURN;
        }
    }
}

static void SavePlayerEcs(GameWorld* gw, CPosition* outPos, CStats* outStats,
                          CSpriteAnim* outSprite, CHitFlash* outHitFlash, bool* outHasHitFlash) {
    if (gw->playerEntity == ENTITY_NONE) return;
    World* w = &gw->ecs;
    *outPos = *World_GetPosition(w, gw->playerEntity);
    *outStats = *World_GetStats(w, gw->playerEntity);
    if (World_HasComponents(w, gw->playerEntity, COMP_SPRITE_ANIM))
        *outSprite = *World_GetSprite(w, gw->playerEntity);
    *outHasHitFlash = World_HasComponents(w, gw->playerEntity, COMP_HIT_FLASH);
    if (*outHasHitFlash)
        *outHitFlash = *World_GetHitFlash(w, gw->playerEntity);
}

static void RestorePlayerEcs(GameWorld* gw, const CPosition* pos, const CStats* stats,
                             const CSpriteAnim* sprite, const CHitFlash* hitFlash, bool hasHitFlash) {
    if (gw->playerEntity == ENTITY_NONE) {
        PlayerSystem_Spawn(gw);
    }
    if (gw->playerEntity == ENTITY_NONE) return;
    World* w = &gw->ecs;
    *World_GetPosition(w, gw->playerEntity) = *pos;
    *World_GetStats(w, gw->playerEntity) = *stats;
    if (World_HasComponents(w, gw->playerEntity, COMP_SPRITE_ANIM))
        *World_GetSprite(w, gw->playerEntity) = *sprite;
    if (hasHitFlash && World_HasComponents(w, gw->playerEntity, COMP_HIT_FLASH))
        *World_GetHitFlash(w, gw->playerEntity) = *hitFlash;
}

void DescendFloor(GameWorld* game) {
    BeginDrawing();
    ClearBackground(BLACK);
    const char* loadText = "LOADING...";
    int fontSize = 40;
    int tw = MeasureText(loadText, fontSize);
    DrawText(loadText, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() / 2 - fontSize / 2, fontSize, WHITE);
    EndDrawing();

    CPosition savedPos;
    CStats savedStats;
    CSpriteAnim savedSprite;
    CHitFlash savedHitFlash;
    bool hasHitFlash = false;
    bool hadPlayer = (game->playerEntity != ENTITY_NONE);
    if (hadPlayer) {
        SavePlayerEcs(game, &savedPos, &savedStats, &savedSprite, &savedHitFlash, &hasHitFlash);
    }

    InventorySlot savedInventory[MAX_INVENTORY_SLOTS];
    memcpy(savedInventory, game->inventory, sizeof(savedInventory));
    int savedInvCount = game->inventorySlotCount;
    EquipType savedEquipped[EQUIP_SLOT_COUNT];
    memcpy(savedEquipped, game->equipped, sizeof(savedEquipped));
    EquipType savedEquipInventory[MAX_INVENTORY_SLOTS];
    int savedEquipInvCount = game->equipInventoryCount;
    memcpy(savedEquipInventory, game->equipInventory, sizeof(savedEquipInventory));

    int savedPotionTileX = game->selectedPotionTileX;
    int savedPotionTileY = game->selectedPotionTileY;
    bool savedPotionTileActive = game->selectedPotionTileActive;

    GameWorld savedGw = *game;
    if (savedGw.map) {
        UnloadTMX(savedGw.map);
    }

    int floor = savedGw.currentFloor + 1;

    memset(game, 0, sizeof(GameWorld));
    game->selectedPotionTileX = savedPotionTileX;
    game->selectedPotionTileY = savedPotionTileY;
    game->selectedPotionTileActive = savedPotionTileActive;

    GameWorld_Init(game);

    {
        Texture2D* t = Resources_LoadTexture("resources/sprites/player.png");
        if (t) savedSprite.tex = t;
    }
    if (!savedSprite.tex || savedSprite.tex->id == 0) {
        TraceLog(LOG_WARNING, "Could not load player spritesheet during descend");
    }

    LoadPotionTextures(game);

    if (hadPlayer) {
        PlayerSystem_Spawn(game);
        RestorePlayerEcs(game, &savedPos, &savedStats, &savedSprite, &savedHitFlash, hasHitFlash);
    }

    memcpy(game->inventory, savedInventory, sizeof(savedInventory));
    game->inventorySlotCount = savedInvCount;
    memcpy(game->equipped, savedEquipped, sizeof(savedEquipped));
    memcpy(game->equipInventory, savedEquipInventory, sizeof(savedEquipInventory));
    game->equipInventoryCount = savedEquipInvCount;

    if (game->playerEntity != ENTITY_NONE) {
        World* w = &game->ecs;
        EntityId pe = game->playerEntity;
        for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
            if (game->equipped[i] != EQUIP_NONE) {
                const EquipData* d = GetEquipData(game->equipped[i]);
                if (d) {
                    CStats* s = World_GetStats(w, pe);
                    s->attack  += d->bonusAttack;
                    s->defense += d->bonusDefense;
                    s->str     += d->bonusStr;
                    s->dex     += d->bonusDex;
                    s->intel   += d->bonusInt;
                    s->con     += d->bonusCon;
                    s->lck     += d->bonusLck;
                }
            }
        }
        CStats* ps = World_GetStats(w, pe);
        ps->maxHp = 30 + ps->con * 5;
        ps->hp = ps->maxHp;
    }

    {
        Texture2D* t = Resources_LoadTexture("resources/tilesets/magic_attacks.png");
        if (!t || t->id == 0) {
            TraceLog(LOG_WARNING, "Could not load magic attacks tileset");
        }
    }

    game->map = GenerateProceduralMap(80, 50, game->currentFloor < game->maxFloors);
    if (!game->map) {
        TraceLog(LOG_ERROR, "Failed to generate map for floor %d", game->currentFloor);
        return;
    }

    game->currentFloor = floor;

    BuildBlockingMap(game);

    for (int t = 0; t < game->map->tilesetCount; t++) {
        char imgPath[512] = {0};
        if (imgPath[0] == '\0' || !FileExists(imgPath)) {
            snprintf(imgPath, sizeof(imgPath), "resources/%s", game->map->tilesets[t].imageSource);
        }
        {
            Texture2D* tex = Resources_LoadTexture(imgPath);
            if (tex) game->tilesetTextures[t] = *tex;
        }
        if (game->tilesetTextures[t].id == 0) {
            TraceLog(LOG_WARNING, "Could not load tileset texture: %s", imgPath);
            if (t == 0) {
                Image img = GenImageColor(game->map->tileWidth * 8, game->map->tileHeight * 8,
                                          (Color){ 100, 100, 100, 255 });
                for (int x = 0; x < 8; x++) {
                    for (int y = 0; y < 8; y++) {
                        Color c = ((x + y) % 2 == 0)
                            ? (Color){ 120, 120, 120, 255 }
                            : (Color){ 80, 80, 80, 255 };
                        ImageDrawRectangle(&img, x * game->map->tileWidth, y * game->map->tileHeight,
                                           game->map->tileWidth - 1, game->map->tileHeight - 1, c);
                    }
                }
                game->tilesetTextures[t] = LoadTextureFromImage(img);
                UnloadImage(img);
                game->map->tilesets[0].columns = 8;
                game->map->tilesets[0].imageWidth = game->map->tileWidth * 8;
                game->map->tilesets[0].imageHeight = game->map->tileHeight * 8;
            }
        }
    }

    Monster_InitTemplates();

    SpawnEntitiesFromObjects(game);

    ProceduralRoom spawnRooms[MAX_GENERATED_ROOMS];
    int spawnRoomCount = GetGeneratedRooms(spawnRooms, MAX_GENERATED_ROOMS);
    if (spawnRoomCount > 0) {
        int px = (game->playerEntity != ENTITY_NONE)
            ? World_GetPosition(&game->ecs, game->playerEntity)->x
            : 1;
        int py = (game->playerEntity != ENTITY_NONE)
            ? World_GetPosition(&game->ecs, game->playerEntity)->y
            : 1;
        SpawnerSystem_SpawnMonsters(game, spawnRooms, spawnRoomCount, px, py);
    }
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
    game->camera.zoom = 4.0f;

    char floorMsg[64];
    snprintf(floorMsg, sizeof(floorMsg), "You descend to floor %d", game->currentFloor);
    CombatLog_Add(&game->combatLog, BLACK, floorMsg);
    TraceLog(LOG_INFO, "%s", floorMsg);
}

bool InitGame(GameWorld* game, const char* tmxFile) {
    if (!game) return false;
    memset(game, 0, sizeof(GameWorld));
    GameWorld_Init(game);

    game->map = LoadTMX(tmxFile);
    if (!game->map) {
        TraceLog(LOG_INFO, "TMX load failed, generating procedural map...");
        game->map = GenerateProceduralMap(80, 50, 1);
    }
    if (!game->map) {
        TraceLog(LOG_ERROR, "Failed to create any map");
        return false;
    }

    BuildBlockingMap(game);

    for (int t = 0; t < game->map->tilesetCount; t++) {
        char imgPath[512] = {0};
        if (tmxFile) {
            const char* lastSlash = strrchr(tmxFile, '/');
            const char* lastBackslash = strrchr(tmxFile, '\\');
            const char* sep = (lastSlash > lastBackslash) ? lastSlash : lastBackslash;
            if (sep) {
                int dirLen = (int)(sep - tmxFile) + 1;
                snprintf(imgPath, sizeof(imgPath), "%.*s%s", dirLen, tmxFile, game->map->tilesets[t].imageSource);
            } else {
                snprintf(imgPath, sizeof(imgPath), "%s", game->map->tilesets[t].imageSource);
            }
        }
        if (imgPath[0] == '\0' || !FileExists(imgPath)) {
            snprintf(imgPath, sizeof(imgPath), "resources/%s", game->map->tilesets[t].imageSource);
        }

        TraceLog(LOG_INFO, "Loading tileset texture [%d]: %s", t, imgPath);
        {
            Texture2D* tex = Resources_LoadTexture(imgPath);
            if (tex) game->tilesetTextures[t] = *tex;
        }
        if (game->tilesetTextures[t].id == 0) {
            TraceLog(LOG_WARNING, "Could not load tileset texture: %s", imgPath);
            if (t == 0) {
                Image img = GenImageColor(game->map->tileWidth * 8, game->map->tileHeight * 8,
                                          (Color){ 100, 100, 100, 255 });
                for (int x = 0; x < 8; x++) {
                    for (int y = 0; y < 8; y++) {
                        Color c = ((x + y) % 2 == 0)
                            ? (Color){ 120, 120, 120, 255 }
                            : (Color){ 80, 80, 80, 255 };
                        ImageDrawRectangle(&img, x * game->map->tileWidth, y * game->map->tileHeight,
                                           game->map->tileWidth - 1, game->map->tileHeight - 1, c);
                    }
                }
                game->tilesetTextures[t] = LoadTextureFromImage(img);
                UnloadImage(img);
                game->map->tilesets[0].columns = 8;
                game->map->tilesets[0].imageWidth  = game->map->tileWidth * 8;
                game->map->tilesets[0].imageHeight = game->map->tileHeight * 8;
            }
        }
    }

    PlayerSystem_Spawn(game);
    if (game->playerEntity != ENTITY_NONE) {
        World* w = &game->ecs;
        EntityId pe = game->playerEntity;
        CPosition* pp = World_GetPosition(w, pe);
        pp->x = 1;
        pp->y = 1;
        pp->prevX = 1;
        pp->prevY = 1;
        pp->facingDir = DIR_DOWN;

        CStats* ps = World_GetStats(w, pe);
        ps->str = 3;
        ps->dex = 3;
        ps->intel = 3;
        ps->con = 3;
        ps->lck = 2;
        ps->statPoints = 0;
        ps->maxHp = 30 + ps->con * 5;
        ps->hp = ps->maxHp;
        ps->attack = 5;
        ps->defense = 1;
        ps->level = 1;
        ps->alive = true;
        ps->exp = 0;
        ps->expToNext = ExpForLevel(1);

        if (World_HasComponents(w, pe, COMP_SPRITE_ANIM)) {
            CSpriteAnim* sa = World_GetSprite(w, pe);
            {
                Texture2D* t = Resources_LoadTexture("resources/sprites/player.png");
                if (t) sa->tex = t;
            }
            if (!sa->tex || sa->tex->id == 0) {
                TraceLog(LOG_WARNING, "Could not load player spritesheet, using fallback rendering");
            }
            sa->row = 6;
            sa->frame = 0;
        }

        if (World_HasComponents(w, pe, COMP_FALLBACK_COLOR)) {
            World_GetColor(w, pe)->color = (Color){ 50, 200, 255, 255 };
        }
    }

    LoadPotionTextures(game);

    memset(game->equipped, 0, sizeof(game->equipped));
    EquipItemSilent(game, EQUIP_SURVIVAL_KNIFE);
    InventoryAdd(game, ITEM_SMALL_HP_POTION);

    {
        Texture2D* t = Resources_LoadTexture("resources/tilesets/magic_attacks.png");
        if (!t || t->id == 0) {
            TraceLog(LOG_WARNING, "Could not load magic attacks tileset");
        }
    }

    game->currentFloor = 1;

    Monster_InitTemplates();

    SpawnEntitiesFromObjects(game);

    ProceduralRoom spawnRooms[MAX_GENERATED_ROOMS];
    int spawnRoomCount = GetGeneratedRooms(spawnRooms, MAX_GENERATED_ROOMS);

    if (spawnRoomCount > 0) {
        int px = (game->playerEntity != ENTITY_NONE)
            ? World_GetPosition(&game->ecs, game->playerEntity)->x
            : 1;
        int py = (game->playerEntity != ENTITY_NONE)
            ? World_GetPosition(&game->ecs, game->playerEntity)->y
            : 1;
        SpawnerSystem_SpawnMonsters(game, spawnRooms, spawnRoomCount, px, py);
    }
    SpawnerSystem_SpawnPickups(game);

    game->selectedMonsterEntity = ENTITY_NONE;
    game->timeWaited = 0;
    game->escapeSpawned = false;
    game->maxFloors = 10;
    game->stairX = GetStairX();
    game->stairY = GetStairY();

    game->state = STATE_PLAYER_TURN;
    game->turnCount = 0;
    game->enemyTurnCooldown = 0.0f;
    game->animTimer = 0.0f;
    game->monsterAnimTimer = 0.0f;
    game->animDuration = 0.0f;
    game->monsterAnimDuration = 0.0f;

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
    game->camera.zoom = 4.0f;

    return true;
}

void CleanupGame(GameWorld* game) {
    if (!game) return;

    Resources_UnloadAll();

    if (game->map) {
        UnloadTMX(game->map);
        game->map = NULL;
    }

    memset(game, 0, sizeof(GameWorld));
}
