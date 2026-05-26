#include "game.h"
#include "entity/entity.h"
#include "entity/player.h"
#include "entity/monster.h"
#include "entity/spawner.h"
#include "ui/combat_log.h"
#include "ui/monster_info.h"
#include "core/audio.h"
#include "procedural.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void HandleInput(Game* game) {
    if (game->state == STATE_INVENTORY) {
        if (IsKeyPressed(KEY_I)) {
            game->state = STATE_PLAYER_TURN;
            return;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (game->invSubState == INV_ACTION_MENU)
                game->invSubState = INV_BROWSE;
            else
                game->state = STATE_PLAYER_TURN;
            return;
        }

        if (game->inventorySlotCount == 0) return;

        if (game->invSubState == INV_BROWSE) {
            if (IsKeyPressed(KEY_UP) && game->inventorySelection > 0)
                game->inventorySelection--;
            if (IsKeyPressed(KEY_DOWN) && game->inventorySelection < game->inventorySlotCount - 1)
                game->inventorySelection++;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                game->invSubState = INV_ACTION_MENU;
                game->invActionSelection = 0;
            }
        } else if (game->invSubState == INV_ACTION_MENU) {
            if (IsKeyPressed(KEY_UP) && game->invActionSelection > 0)
                game->invActionSelection--;
            if (IsKeyPressed(KEY_DOWN) && game->invActionSelection < 3)
                game->invActionSelection++;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (game->invActionSelection == 0) {
                    if (InventoryUse(game, game->inventorySelection)) {
                        game->turnCount++;
                        game->enemyTurnCooldown = 0.15f;
                        game->state = STATE_ENEMY_TURN;
                    } else {
                        game->invSubState = INV_BROWSE;
                    }
                } else if (game->invActionSelection == 1) {
                    InventorySlot* slot = &game->inventory[game->inventorySelection];
                    ItemType type = slot->type;
                    slot->quantity--;
                    bool stacked = false;
                    for (int p = 0; p < game->potionCount; p++) {
                        if (game->potionCollected[p]) continue;
                        if (game->potionTiles[p][0] == game->player.ent.x &&
                            game->potionTiles[p][1] == game->player.ent.y &&
                            game->potionTypes[p] == type) {
                            game->potionQuantities[p]++;
                            stacked = true;
                            break;
                        }
                    }
                    if (!stacked && game->potionCount < MAX_POTIONS) {
                        game->potionTiles[game->potionCount][0] = game->player.ent.x;
                        game->potionTiles[game->potionCount][1] = game->player.ent.y;
                        game->potionCollected[game->potionCount] = false;
                        game->potionTypes[game->potionCount] = type;
                        game->potionQuantities[game->potionCount] = 1;
                        game->potionCount++;
                    }
                    CombatLog_Add(&game->combatLog, BLACK, "Dropped %s", GetItemName(type));
                    if (slot->quantity <= 0) {
                        for (int i = game->inventorySelection; i < game->inventorySlotCount - 1; i++)
                            game->inventory[i] = game->inventory[i + 1];
                        game->inventorySlotCount--;
                        if (game->inventorySelection >= game->inventorySlotCount)
                            game->inventorySelection = game->inventorySlotCount - 1;
                    }
                    game->invSubState = INV_BROWSE;
                } else if (game->invActionSelection == 2) {
                    InventorySlot* slot = &game->inventory[game->inventorySelection];
                    ItemType type = slot->type;
                    int total = slot->quantity;
                    slot->quantity = 0;
                    bool stacked = false;
                    for (int p = 0; p < game->potionCount; p++) {
                        if (game->potionCollected[p]) continue;
                        if (game->potionTiles[p][0] == game->player.ent.x &&
                            game->potionTiles[p][1] == game->player.ent.y &&
                            game->potionTypes[p] == type) {
                            game->potionQuantities[p] += total;
                            stacked = true;
                            break;
                        }
                    }
                    if (!stacked && game->potionCount < MAX_POTIONS) {
                        game->potionTiles[game->potionCount][0] = game->player.ent.x;
                        game->potionTiles[game->potionCount][1] = game->player.ent.y;
                        game->potionCollected[game->potionCount] = false;
                        game->potionTypes[game->potionCount] = type;
                        game->potionQuantities[game->potionCount] = total;
                        game->potionCount++;
                    }
                    CombatLog_Add(&game->combatLog, BLACK, "Dropped %d x %s", total, GetItemName(type));
                    for (int i = game->inventorySelection; i < game->inventorySlotCount - 1; i++)
                        game->inventory[i] = game->inventory[i + 1];
                    game->inventorySlotCount--;
                    if (game->inventorySelection >= game->inventorySlotCount)
                        game->inventorySelection = game->inventorySlotCount - 1;
                    game->invSubState = INV_BROWSE;
                } else {
                    game->invSubState = INV_BROWSE;
                }
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                game->invSubState = INV_BROWSE;
            }
        }
        return;
    }

    if (game->state != STATE_PLAYER_TURN) return;
    if (game->animTimer > 0.0f) return;

    // Mouse click: select monster or potion tile
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        game->sprintBypassRoom = false;
        Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), game->camera);
        int tileX = (int)(worldPos.x / game->map->tileWidth);
        int tileY = (int)(worldPos.y / game->map->tileHeight);
        if (tileX >= 0 && tileX < game->map->width &&
            tileY >= 0 && tileY < game->map->height &&
            game->visibility[tileY][tileX] == 1) {
            Monster* mon = Monster_GetAt(tileX, tileY, NULL);
            if (mon) {
                Monster* arr = Monster_GetArray();
                int count = Monster_GetCount();
                game->selectedMonsterIdx = -1;
                for (int i = 0; i < count; i++) {
                    if (&arr[i] == mon) {
                        game->selectedMonsterIdx = i;
                        break;
                    }
                }
                game->selectedPotionTileActive = false;
            } else {
                game->selectedMonsterIdx = -1;
                game->selectedPotionTileActive = false;
                for (int p = 0; p < game->potionCount; p++) {
                    if (!game->potionCollected[p] &&
                        game->potionTiles[p][0] == tileX &&
                        game->potionTiles[p][1] == tileY) {
                        game->selectedPotionTileX = tileX;
                        game->selectedPotionTileY = tileY;
                        game->selectedPotionTileActive = true;
                        break;
                    }
                }
            }
        } else {
            game->selectedMonsterIdx = -1;
            game->selectedPotionTileActive = false;
        }
    }

    // Sprint: hold SHIFT + direction
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        Direction sprintDir = DIR_NONE;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) sprintDir = DIR_UP;
        else if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) sprintDir = DIR_DOWN;
        else if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) sprintDir = DIR_LEFT;
        else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) sprintDir = DIR_RIGHT;

        if (sprintDir != DIR_NONE) {
            bool stoppedAtRoom = false;
            int dx = 0, dy = 0;
            switch (sprintDir) {
                case DIR_UP:    dy = -1; break;
                case DIR_DOWN:  dy = 1;  break;
                case DIR_LEFT:  dx = -1; break;
                case DIR_RIGHT: dx = 1;  break;
                default: break;
            }

            int startX = game->player.ent.x;
            int startY = game->player.ent.y;
            int endX = startX, endY = startY;

            while (1) {
                int nx = endX + dx;
                int ny = endY + dy;
                if (nx < 0 || nx >= game->map->width || ny < 0 || ny >= game->map->height) break;
                if (game->blocking[ny][nx]) break;
                if (Monster_GetAt(nx, ny, NULL)) break;
                endX = nx;
                endY = ny;
            }

            int steps = abs(endX - startX) + abs(endY - startY);
            if (steps > 0) {
                Monster* monArray = Monster_GetArray();
                int monCount = Monster_GetCount();
                typedef struct { int x, y; float hitFlash; } MonSnap;
                MonSnap monSnap[64];
                int snapCount = monCount < 64 ? monCount : 64;
                for (int i = 0; i < snapCount; i++) {
                    monSnap[i].x = monArray[i].x;
                    monSnap[i].y = monArray[i].y;
                    monSnap[i].hitFlash = monArray[i].hitFlashTimer;
                }

                game->player.ent.prevX = startX;
                game->player.ent.prevY = startY;
                game->player.ent.x = startX;
                game->player.ent.y = startY;
                if (sprintDir == DIR_RIGHT) game->player.ent.facingRight = true;
                else if (sprintDir == DIR_LEFT) game->player.ent.facingRight = false;
                game->selectedMonsterIdx = -1;

                for (int s = 0; s < steps; s++) {
                    int nx = game->player.ent.x + dx;
                    int ny = game->player.ent.y + dy;

                    if (nx < 0 || nx >= game->map->width || ny < 0 || ny >= game->map->height) break;
                    if (game->blocking[ny][nx]) break;
                    if (Monster_GetAt(nx, ny, NULL)) break;

                    if (IsInRoom(game->player.ent.x, game->player.ent.y) != IsInRoom(nx, ny)) {
                        if (game->sprintBypassRoom) {
                            game->sprintBypassRoom = false;
                        } else {
                            game->sprintBypassRoom = true;
                            stoppedAtRoom = true;
                            break;
                        }
                    }

                    game->player.ent.x = nx;
                    game->player.ent.y = ny;
                    game->turnCount++;

                    int hpBefore = game->player.ent.hp;
                    bool alive = Monster_ProcessAllAI(game, game->timeWaited);

                    if (!alive) {
                        game->state = STATE_GAME_OVER;
                        return;
                    }

                    if (game->player.ent.hp < hpBefore) {
                        break;
                    }

                    // Pick up any potion at the current tile
                    for (int h = 0; h < game->potionCount; h++) {
                        if (!game->potionCollected[h] &&
                            game->potionTiles[h][0] == game->player.ent.x &&
                            game->potionTiles[h][1] == game->player.ent.y) {
                            game->potionCollected[h] = true;
                            ItemType ptype = game->potionTypes[h];
                            int qty = game->potionQuantities[h];
                            int picked = 0;
                            for (int i = 0; i < qty; i++) {
                                if (InventoryAdd(game, ptype)) picked++;
                                else break;
                            }
                            if (picked > 0) {
                                TraceLog(LOG_INFO, "Picked up %d x %s", picked, GetItemName(ptype));
                                CombatLog_Add(&game->combatLog, BLACK, "Picked up %d x %s", picked, GetItemName(ptype));
                                PlayPickupSound();
                            }
                        }
                    }
                }

                if (!stoppedAtRoom) game->sprintBypassRoom = false;

                for (int i = 0; i < snapCount; i++) {
                    monArray[i].prevX = monSnap[i].x;
                    monArray[i].prevY = monSnap[i].y;
                    monArray[i].hitFlashTimer = monSnap[i].hitFlash;
                }

                game->animTimer = 0.30f;
                game->animDuration = 0.30f;
                game->monsterAnimTimer = 0.30f;
                game->monsterAnimDuration = 0.30f;
                game->enemyTurnCooldown = 0.0f;
                game->state = STATE_ENEMY_TURN;
                game->animatingEnemyTurn = true;
                if (!game->projectile.active) {
                    game->animatingEnemyTurn = false;
                    game->state = STATE_PLAYER_TURN;
                }
                RevealFOW(game);

                if (game->player.ent.x == game->stairX && game->player.ent.y == game->stairY &&
                    game->stairX >= 0 && game->stairY >= 0) {
                    DescendFloor(game);
                }
                if (game->escapeSpawned &&
                    game->player.ent.x == game->escapeX && game->player.ent.y == game->escapeY) {
                    game->state = STATE_WIN;
                }
            }
            return;
        }
    }

    Direction dir = DIR_NONE;

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) dir = DIR_UP;
    else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) dir = DIR_DOWN;
    else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) dir = DIR_LEFT;
    else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) dir = DIR_RIGHT;
    else if (IsKeyPressed(KEY_PERIOD) || IsKeyPressed(KEY_SPACE)) {
        game->sprintBypassRoom = false;
        game->turnCount++;
        game->timeWaited++;
        if (game->timeWaited == 15 && game->currentFloor < game->maxFloors) {
            SpawnShadow(game);
            CombatLog_Add(&game->combatLog, RED, "You feel a presence come to this floor");
        }
        if (game->player.ent.hp < game->player.ent.maxHp) {
            int waitHeal = 1 + (game->player.ent.level / 5) * 2;
            game->player.ent.hp += waitHeal;
            if (game->player.ent.hp > game->player.ent.maxHp)
                game->player.ent.hp = game->player.ent.maxHp;
            CombatLog_Add(&game->combatLog, BLACK, "Wait heals %d HP", waitHeal);
        }
        game->enemyTurnCooldown = 0.08f;
        game->state = STATE_ENEMY_TURN;
        return;
    }

    if (dir != DIR_NONE) {
        game->sprintBypassRoom = false;
        int oldX = game->player.ent.x;
        int oldY = game->player.ent.y;
        bool moved = MoveEntity(game, &game->player.ent, dir);
        if (moved) {
            game->selectedMonsterIdx = -1;
            game->turnCount++;
            game->enemyTurnCooldown = 0.15f;
            if (game->player.ent.x != oldX || game->player.ent.y != oldY) {
                game->animTimer = MOVE_ANIM_DURATION;
                game->animDuration = MOVE_ANIM_DURATION;
                RevealFOW(game);
            }
            game->state = STATE_ENEMY_TURN;
        }
    }

    if (IsKeyPressed(KEY_I)) {
        game->state = STATE_INVENTORY;
        game->invSubState = INV_BROWSE;
        if (game->inventorySelection < 0 || game->inventorySelection >= game->inventorySlotCount)
            game->inventorySelection = 0;
        return;
    }

    if (game->player.ent.x == game->stairX && game->player.ent.y == game->stairY &&
        game->stairX >= 0 && game->stairY >= 0) {
        DescendFloor(game);
    }

    if (game->escapeSpawned &&
        game->player.ent.x == game->escapeX && game->player.ent.y == game->escapeY) {
        game->state = STATE_WIN;
    }
}

void UpdateGame(Game* game) {
    if (!game) return;

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

    if (game->player.ent.hitFlashTimer > 0.0f) {
        game->player.ent.hitFlashTimer -= GetFrameTime();
        if (game->player.ent.hitFlashTimer < 0.0f) game->player.ent.hitFlashTimer = 0.0f;
    }

    Monster_UpdateAnimations(GetFrameTime());

    if (game->state == STATE_GAME_OVER || game->state == STATE_WIN) return;

    if (game->state == STATE_ENEMY_TURN) {
        if (game->enemyTurnCooldown > 0.0f) {
            game->enemyTurnCooldown -= GetFrameTime();
            return;
        }

        // Wait for all animations (projectile, monster movement) before giving player control
        if (game->animatingEnemyTurn) {
            if (game->projectile.active && game->projectileTimer > 0.0f) return;
            if (game->monsterAnimTimer > 0.0f) return;
            game->animatingEnemyTurn = false;
            game->state = STATE_PLAYER_TURN;
            return;
        }

        bool playerAlive = Monster_ProcessAllAI(game, game->timeWaited);

        if (!playerAlive) {
            game->player.ent.alive = false;
            game->player.ent.hp = 0;
            game->state = STATE_GAME_OVER;
            return;
        }

        if (Monster_GetCount() > 0 && Monster_AreAllDead()) {
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
        // If no projectile, we can go straight to player turn
        if (!game->projectile.active) {
            game->animatingEnemyTurn = false;
            game->state = STATE_PLAYER_TURN;
        }
    }
}

void DescendFloor(Game* game) {
    Player savedPlayer = game->player;
    InventorySlot savedInventory[MAX_INVENTORY_SLOTS];
    memcpy(savedInventory, game->inventory, sizeof(savedInventory));
    int savedInvCount = game->inventorySlotCount;

    Monster_UnloadSprites();
    Monster_ResetAll();
    UnloadPotionTextures(game);

    if (game->map) {
        for (int t = 0; t < game->map->tilesetCount; t++) {
            if (game->tilesetTextures[t].id > 0)
                UnloadTexture(game->tilesetTextures[t]);
        }
        UnloadTMX(game->map);
    }

    int floor = game->currentFloor + 1;

    memset(game, 0, sizeof(Game));
    game->player = savedPlayer;
    memcpy(game->inventory, savedInventory, sizeof(savedInventory));
    game->inventorySlotCount = savedInvCount;
    game->selectedMonsterIdx = -1;
    game->currentFloor = floor;
    game->maxFloors = 10;
    game->player.ent.spriteSheet = LoadTexture("resources/sprites/roguelikeChar_transparent.png");
    if (game->player.ent.spriteSheet.id == 0) {
        TraceLog(LOG_WARNING, "Could not load player spritesheet during descend");
    }

    LoadPotionTextures(game);

    game->magicAttacksTexture = LoadTexture("resources/tilesets/magic_attacks.png");
    if (game->magicAttacksTexture.id == 0) {
        TraceLog(LOG_WARNING, "Could not load magic attacks tileset");
    }

    game->map = GenerateProceduralMap(80, 50, game->currentFloor < game->maxFloors);
    if (!game->map) {
        TraceLog(LOG_ERROR, "Failed to generate map for floor %d", game->currentFloor);
        return;
    }

    BuildBlockingMap(game);

    for (int t = 0; t < game->map->tilesetCount; t++) {
        char imgPath[512] = {0};
        if (imgPath[0] == '\0' || !FileExists(imgPath)) {
            snprintf(imgPath, sizeof(imgPath), "resources/%s", game->map->tilesets[t].imageSource);
        }
        game->tilesetTextures[t] = LoadTexture(imgPath);
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
        Spawner_Populate(game, spawnRooms, spawnRoomCount);
    }

    Monster_LoadSprites();

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

    game->camera.target = (Vector2){
        game->player.ent.x * game->map->tileWidth  + game->map->tileWidth  / 2,
        game->player.ent.y * game->map->tileHeight + game->map->tileHeight / 2
    };
    game->camera.offset = (Vector2){ GetScreenWidth() / 2, GetScreenHeight() / 2 };
    game->camera.rotation = 0;
    game->camera.zoom = 4.0f;

    char floorMsg[64];
    snprintf(floorMsg, sizeof(floorMsg), "You descend to floor %d", game->currentFloor);
    CombatLog_Add(&game->combatLog, BLACK, floorMsg);
    TraceLog(LOG_INFO, "%s", floorMsg);
}

bool InitGame(Game* game, const char* tmxFile) {
    if (!game) return false;
    memset(game, 0, sizeof(Game));

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
        game->tilesetTextures[t] = LoadTexture(imgPath);
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

    game->player.ent.x = 1;
    game->player.ent.y = 1;
    game->player.ent.prevX = 1;
    game->player.ent.prevY = 1;
    strcpy(game->player.ent.name, "Hero");
    game->player.ent.hp = 20;
    game->player.ent.maxHp = 20;
    game->player.ent.attack = 5;
    game->player.ent.defense = 1;
    game->player.ent.level = 1;
    game->player.ent.alive = true;
    game->player.ent.isPlayer = true;
    game->player.ent.facingRight = true;
    game->player.ent.color = (Color){ 50, 200, 255, 255 };
    game->player.ent.spriteRow = 6;

    game->player.ent.spriteSheet = LoadTexture("resources/sprites/roguelikeChar_transparent.png");
    if (game->player.ent.spriteSheet.id == 0) {
        TraceLog(LOG_WARNING, "Could not load player spritesheet, using fallback rendering");
    }

    LoadPotionTextures(game);

    game->magicAttacksTexture = LoadTexture("resources/tilesets/magic_attacks.png");
    if (game->magicAttacksTexture.id == 0) {
        TraceLog(LOG_WARNING, "Could not load magic attacks tileset");
    }

    game->player.exp = 0;
    game->player.expToNext = ExpForLevel(1);

    game->currentFloor = 1;

    Monster_InitTemplates();

    SpawnEntitiesFromObjects(game);

    ProceduralRoom spawnRooms[MAX_GENERATED_ROOMS];
    int spawnRoomCount = GetGeneratedRooms(spawnRooms, MAX_GENERATED_ROOMS);
    if (spawnRoomCount > 0) {
        Spawner_Populate(game, spawnRooms, spawnRoomCount);
    }

    Monster_LoadSprites();

    game->selectedMonsterIdx = -1;
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

    game->camera.target = (Vector2){
        game->player.ent.x * game->map->tileWidth  + game->map->tileWidth  / 2,
        game->player.ent.y * game->map->tileHeight + game->map->tileHeight / 2
    };
    game->camera.offset = (Vector2){ GetScreenWidth() / 2, GetScreenHeight() / 2 };
    game->camera.rotation = 0;
    game->camera.zoom = 4.0f;

    return true;
}

void CleanupGame(Game* game) {
    if (!game) return;

    Monster_UnloadSprites();
    Monster_ResetAll();

    if (game->map) {
        for (int t = 0; t < game->map->tilesetCount; t++) {
            if (game->tilesetTextures[t].id > 0)
                UnloadTexture(game->tilesetTextures[t]);
        }
    }
    if (game->player.ent.spriteSheet.id > 0) UnloadTexture(game->player.ent.spriteSheet);
    if (game->magicAttacksTexture.id > 0) UnloadTexture(game->magicAttacksTexture);
    UnloadPotionTextures(game);
    if (game->map) {
        UnloadTMX(game->map);
        game->map = NULL;
    }

    memset(game, 0, sizeof(Game));
}
