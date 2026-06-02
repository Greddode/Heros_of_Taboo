#include "game.h"
#include "systems/spawner_system.h"
#include "world.h"
#include "data/monster_data.h"
#include "systems/ai_system.h"
#include "systems/combat_system.h"
#include "systems/world_monster.h"
#include "systems/player_system.h"
#include "systems/player.h"
#include "ui/combat_log.h"
#include "map/procedural.h"
#include "resources.h"
#include "game_balance.h"
#include "equipment_bonus.h"
#include "floor_init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static float s_guiScaleSetting = 1.0f;



// UI scaling helper function
float GetUIScale(void) {
    float widthScale = (float)GetScreenWidth() / UI_BASE_WIDTH;
    float heightScale = (float)GetScreenHeight() / UI_BASE_HEIGHT;
    float scale = fminf(widthScale, heightScale);
    scale = fmaxf(scale, UI_MIN_AUTO_SCALE);
    return scale * s_guiScaleSetting;
}

void SetGuiScale(float scale) {
    if (scale < UI_GUI_SCALE_MIN) scale = UI_GUI_SCALE_MIN;
    if (scale > UI_GUI_SCALE_MAX) scale = UI_GUI_SCALE_MAX;
    s_guiScaleSetting = scale;
}

float GetGuiScale(void) {
    return s_guiScaleSetting;
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

        if (game->monstersEverSpawned && World_AreAllMonstersDead(game)) {
            if (game->currentFloor >= game->maxFloors) {
                if (!game->escapeSpawned) {
                    SpawnEscapeTile(game);
                    game->escapeSpawned = true;
                    CombatLog_Add(&game->combatLog, GREEN, "A teleport circle has appeared somewhere for you to escape!");
                }
            } else if (!game->floorClearedNotified) {
                game->floorClearedNotified = true;
                CombatLog_Add(&game->combatLog, YELLOW, "This floor sounds eerily quiet now...");
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

static void LoadTilesets(GameWorld* game, const char* tmxFile) {
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
        game->tilesetTextures[t] = Resources_LoadTexture(imgPath);
        if (!game->tilesetTextures[t] || game->tilesetTextures[t]->id == 0) {
            TraceLog(LOG_WARNING, "Could not load tileset texture: %s", imgPath);
            game->tilesetTextures[t] = NULL;
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
                // fallbackTileset is owned locally (not in resource cache); cleaned up in CleanupGame
                game->fallbackTileset = LoadTextureFromImage(img);
                UnloadImage(img);
                game->tilesetTextures[0] = &game->fallbackTileset;
                game->map->tilesets[0].columns = 8;
                game->map->tilesets[0].imageWidth  = game->map->tileWidth * 8;
                game->map->tilesets[0].imageHeight = game->map->tileHeight * 8;
            }
        }
    }
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

    int savedPotionTileX = game->inspectedTileX;
    int savedPotionTileY = game->inspectedTileY;
    bool savedPotionTileActive = game->inspectedTileActive;

    GameWorld savedGw = *game;
    if (savedGw.map) {
        UnloadTMX(savedGw.map);
    }

    int floor = savedGw.currentFloor + 1;

    memset(game, 0, sizeof(GameWorld));
    game->inspectedTileX = savedPotionTileX;
    game->inspectedTileY = savedPotionTileY;
    game->inspectedTileActive = savedPotionTileActive;

    GameWorld_Init(game);

    game->currentFloor = floor;
    game->maxFloors    = savedGw.maxFloors;

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
                EquipmentBonus_Apply(w, pe, game->equipped[i]);
            }
        }
        CStats* ps = World_GetStats(w, pe);
        ps->maxHp = calc_max_hp(ps->con);
        if (savedStats.maxHp > 0) {
            float ratio = (float)savedStats.hp / (float)savedStats.maxHp;
            ps->hp = (int)(ratio * (float)ps->maxHp);
        } else {
            ps->hp = ps->maxHp;
        }
        if (ps->hp < 1) ps->hp = 1;
        if (ps->hp > ps->maxHp) ps->hp = ps->maxHp;
    }

    {
        Texture2D* t = Resources_LoadTexture("resources/tilesets/magic_attacks.png");
        if (!t || t->id == 0) {
            TraceLog(LOG_WARNING, "Could not load magic attacks tileset");
        }
    }

    game->map = GenerateProceduralMap(PROCEDURAL_MAP_WIDTH, PROCEDURAL_MAP_HEIGHT,
                                      game->currentFloor < game->maxFloors);
    if (!game->map) {
        TraceLog(LOG_ERROR, "Failed to generate map for floor %d", game->currentFloor);
        return;
    }

    LoadTilesets(game, NULL);

    Floor_InitNewFloor(game);

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
        game->map = GenerateProceduralMap(PROCEDURAL_MAP_WIDTH, PROCEDURAL_MAP_HEIGHT, 1);
    }
    if (!game->map) {
        TraceLog(LOG_ERROR, "Failed to create any map");
        return false;
    }

    LoadTilesets(game, tmxFile);

    PlayerSystem_Spawn(game);

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

    game->currentFloor = DEFAULT_START_FLOOR;

    Floor_InitNewFloor(game);

    game->selectedMonsterEntity = ENTITY_NONE;
    game->timeWaited = 0;
    game->escapeSpawned = false;
    game->maxFloors = DEFAULT_MAX_FLOORS;

    return true;
}

void CleanupGame(GameWorld* game) {
    if (!game) return;

    // Unload all ResourceManager-cached textures
    Resources_UnloadAll();

    // Unload locally-owned fallback tileset (generated via LoadTextureFromImage)
    if (game->fallbackTileset.id != 0) {
        UnloadTexture(game->fallbackTileset);
        game->fallbackTileset = (Texture2D){0};
    }

    if (game->map) {
        UnloadTMX(game->map);
        game->map = NULL;
    }

    memset(game, 0, sizeof(GameWorld));
}
