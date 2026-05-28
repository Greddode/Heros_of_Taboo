#include "game.h"
#include "entity/entity.h"
#include "entity/player.h"
#include "systems/spawner_system.h"
#include "entity/monster.h"
#include "entity/spawner.h"
#include "ui/combat_log.h"
#include "core/audio.h"
#include "procedural.h"
#include "resources.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static float s_guiScaleSetting = 1.0f; // GUI scale multiplier (default 1.0)

// UI scaling helper function
float GetUIScale(void) {
    // Base resolution for UI design (1024x768)
    const float baseWidth = 1024.0f;
    const float baseHeight = 768.0f;
    float widthScale = (float)GetScreenWidth() / baseWidth;
    float heightScale = (float)GetScreenHeight() / baseHeight;
    // Use the smaller scale to maintain aspect ratio, but with a minimum
    float scale = fminf(widthScale, heightScale);
    scale = fmaxf(scale, 0.75f); // Don't scale below 75% of base size
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

        // Tab switching (Q / E only)
        if (game->invSubState == INV_BROWSE) {
            int curTab = (int)game->inventoryTab;
            if (IsKeyPressed(KEY_Q)) {
                curTab--;
                if (curTab < 0) curTab = INV_TAB_COUNT - 1;
                game->inventoryTab = (InventoryTab)curTab;
                game->inventorySelection = 0;
                game->invScrollOffset = 0;
                return;
            }
            if (IsKeyPressed(KEY_E)) {
                curTab++;
                if (curTab >= INV_TAB_COUNT) curTab = 0;
                game->inventoryTab = (InventoryTab)curTab;
                game->inventorySelection = 0;
                game->invScrollOffset = 0;
                return;
            }

            // Mouse wheel scroll
            int wheel = (int)GetMouseWheelMove();
            if (wheel != 0) {
                game->invScrollOffset -= wheel * (int)(20 * GetUIScale());
                if (game->invScrollOffset < 0) game->invScrollOffset = 0;
            }
        }

        // Stats tab: selection-based navigation with column switching
        if (game->invSubState == INV_BROWSE && game->inventoryTab == INV_TAB_STATS) {
            float iscale = GetUIScale();
            int gap = (int)(22 * iscale);
            int textSize = (int)(16 * iscale);
            int derivedSize = (int)(14 * iscale);
            int viewH = (int)((400 - 24 - 40 - 60) * iscale); // ih - tabH - header_gap - footer

            int col1SelMax = 13; // max selection index in col1 (0-13 lines)
            int col2SelMax = (game->player.ent.statPoints > 0) ? 5 : 0;

            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                game->statsActiveCol = 0;
                game->statsSelection = 0;
            }
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                game->statsActiveCol = 1;
                game->statsSelection = 1;
                if (game->statsSelection > col2SelMax) game->statsSelection = col2SelMax;
            }

            if (game->statsActiveCol == 0) {
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                    game->statsSelection--;
                    if (game->statsSelection < 0) game->statsSelection = 0;
                }
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                    game->statsSelection++;
                    if (game->statsSelection > col1SelMax) game->statsSelection = col1SelMax;
                }
                // Scroll col1 to keep selection visible
                int selY = game->statsSelection * gap;
                int scrollGap = gap * 3;
                if (selY - game->statsScrollCol1 < 0) {
                    game->statsScrollCol1 = selY - scrollGap;
                    if (game->statsScrollCol1 < 0) game->statsScrollCol1 = 0;
                }
                if (selY - game->statsScrollCol1 + gap > viewH) {
                    game->statsScrollCol1 = selY + gap - viewH + scrollGap;
                }
            } else {
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                    game->statsSelection--;
                    if (game->statsSelection < 0) game->statsSelection = 0;
                }
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                    game->statsSelection++;
                    if (game->statsSelection > col2SelMax) game->statsSelection = col2SelMax;
                }
                // Scroll col2 to keep selection visible
                int selY = game->statsSelection * gap;
                int scrollGap = gap * 3;
                if (selY - game->statsScrollCol2 < 0) {
                    game->statsScrollCol2 = selY - scrollGap;
                    if (game->statsScrollCol2 < 0) game->statsScrollCol2 = 0;
                }
                if (selY - game->statsScrollCol2 + gap > viewH) {
                    game->statsScrollCol2 = selY + gap - viewH + scrollGap;
                }
            }

            if (game->player.ent.statPoints > 0) {
                // Enter allocates based on current selection
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                    int statIdx = -1;
                    if (game->statsActiveCol == 0) {
                        // Col1: selection indices 3-7 map to stats 0-4
                        if (game->statsSelection >= 3 && game->statsSelection <= 7) {
                            statIdx = game->statsSelection - 3;
                        }
                    } else {
                        // Col2: selection 1-5 maps to stats 0-4 (line 0 is "Unspent" header)
                        if (game->statsSelection >= 1 && game->statsSelection <= 5) {
                            statIdx = game->statsSelection - 1;
                        }
                    }
                    if (statIdx >= 0) {
                        AllocateStatPoint(&game->player.ent, statIdx);
                        return;
                    }
                }
            }
        }

        // Equipment tab item selection and action menu
        if (game->invSubState == INV_BROWSE && game->inventoryTab == INV_TAB_EQUIPMENT) {
            float iscale = GetUIScale();
            int slotH = (int)(44 * iscale); // slotH(36) + gap(8)
            int contentH = (int)((400 - 24 - 40 - 60) * iscale); // ih - tabH - header_gap - footer
            int topPad = (int)(40 * iscale);
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                if (game->inventorySelection > 0) {
                    game->inventorySelection--;
                    int t = topPad - game->invScrollOffset + game->inventorySelection * slotH;
                    if (t < 0) game->invScrollOffset += t;
                } else if (game->invScrollOffset > 0) {
                    game->invScrollOffset -= slotH;
                    if (game->invScrollOffset < 0) game->invScrollOffset = 0;
                }
            }
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                int maxSel = EQUIP_SLOT_COUNT - 1;
                if (game->inventorySelection < maxSel) {
                    int prevSel = game->inventorySelection;
                    game->inventorySelection++;
                    int b = topPad - game->invScrollOffset + (game->inventorySelection + 1) * slotH;
                    if (b > contentH) game->invScrollOffset += (b - contentH);
                }
            }
            if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) &&
                game->equipped[game->inventorySelection] != EQUIP_NONE) {
                game->invSubState = INV_ACTION_MENU;
                game->invActionSelection = 0;
            }
            return;
        }

        // Equipment action menu handling (Unequip / Drop / Back)
        if (game->invSubState == INV_ACTION_MENU && game->inventoryTab == INV_TAB_EQUIPMENT) {
            if (IsKeyPressed(KEY_UP) && game->invActionSelection > 0)
                game->invActionSelection--;
            if (IsKeyPressed(KEY_DOWN) && game->invActionSelection < 2)
                game->invActionSelection++;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                EquipSlot slot = (EquipSlot)game->inventorySelection;
                if (game->invActionSelection == 0) {
                    // Unequip
                    UnequipSlot(game, slot);
                    game->invSubState = INV_BROWSE;
                } else if (game->invActionSelection == 1) {
                    // Drop (unequip and place on map)
                    EquipType oldType = game->equipped[(int)slot];
                    if (oldType != EQUIP_NONE) {
                        const EquipData* data = GetEquipData(oldType);
                        if (data) {
                            game->player.ent.attack   -= data->bonusAttack;
                            game->player.ent.defense  -= data->bonusDefense;
                            game->player.ent.str      -= data->bonusStr;
                            game->player.ent.dex      -= data->bonusDex;
                            game->player.ent.intel    -= data->bonusInt;
                            game->player.ent.con      -= data->bonusCon;
                            game->player.ent.lck      -= data->bonusLck;
                            game->player.ent.maxHp    = 30 + game->player.ent.con * 5;
                            if (game->player.ent.hp > game->player.ent.maxHp)
                                game->player.ent.hp = game->player.ent.maxHp;
                        }
                        game->equipped[(int)slot] = EQUIP_NONE;
                        // Place on map at player position
                        {
                            bool stacked = false;
                            for (int e = 0; e < game->equipMapCount; e++) {
                                if (game->equipMapCollected[e]) continue;
                                if (game->equipMapTiles[e][0] == game->player.ent.x &&
                                    game->equipMapTiles[e][1] == game->player.ent.y &&
                                    game->equipMapTypes[e] == oldType) {
                                    game->equipMapQuantities[e]++;
                                    stacked = true;
                                    break;
                                }
                            }
                            if (!stacked && game->equipMapCount < MAX_EQUIP_ON_MAP) {
                                game->equipMapTiles[game->equipMapCount][0] = game->player.ent.x;
                                game->equipMapTiles[game->equipMapCount][1] = game->player.ent.y;
                                game->equipMapCollected[game->equipMapCount] = false;
                                game->equipMapTypes[game->equipMapCount] = oldType;
                                game->equipMapQuantities[game->equipMapCount] = 1;
                                game->equipMapCount++;
                            }
                        }
                        CombatLog_Add(&game->combatLog, BLACK, "Dropped %s", data ? data->name : "item");
                    }
                    game->invSubState = INV_BROWSE;
                } else {
                    // Back
                    game->invSubState = INV_BROWSE;
                }
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                game->invSubState = INV_BROWSE;
            }
            return;
        }

        // Inventory tab item selection (combined: potions + equipment)
        {
            int totalInv = game->inventorySlotCount + game->equipInventoryCount;
            if (totalInv == 0 && game->inventoryTab == INV_TAB_INVENTORY) {
                // No items at all, skip selection
            } else if (game->invSubState == INV_BROWSE && game->inventoryTab == INV_TAB_INVENTORY) {
                float iscale = GetUIScale();
                int itemH = (int)(22 * iscale);
            int contentH = (int)((400 - 24 - 40 - 60) * iscale); // ih - tabH - header_gap - footer
                int topPad = (int)(42 * iscale); // title + gap
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                    if (game->inventorySelection > 0) {
                        game->inventorySelection--;
                        int t = topPad - game->invScrollOffset + game->inventorySelection * itemH;
                        if (t < 0) game->invScrollOffset += t;
                    } else if (game->invScrollOffset > 0) {
                        game->invScrollOffset -= itemH;
                        if (game->invScrollOffset < 0) game->invScrollOffset = 0;
                    }
                }
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                    if (game->inventorySelection < totalInv - 1) {
                        game->inventorySelection++;
                        int b = topPad - game->invScrollOffset + (game->inventorySelection + 1) * itemH;
                        if (b > contentH) game->invScrollOffset += (b - contentH);
                    } else if (game->inventorySelection >= totalInv - 1) {
                        // Already at last item; try to scroll if more content exists below
                        // (handled by the else clause — no-op when at absolute end)
                    }
                }
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                    game->invSubState = INV_ACTION_MENU;
                    game->invActionSelection = 0;
                }
            } else if (game->invSubState == INV_ACTION_MENU && game->inventoryTab == INV_TAB_INVENTORY) {
                bool isEquip = game->inventorySelection >= game->inventorySlotCount;
                int maxAction = isEquip ? 2 : 3; // 0..2 for equip (Equip/Drop/Back), 0..3 for potions (Use/Drop/Drop All/Back)

                if (IsKeyPressed(KEY_UP) && game->invActionSelection > 0)
                    game->invActionSelection--;
                if (IsKeyPressed(KEY_DOWN) && game->invActionSelection < maxAction)
                    game->invActionSelection++;

                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                    if (isEquip) {
                        // Equipment item actions
                        if (game->invActionSelection == 0) {
                            // Equip
                            int equipIdx = game->inventorySelection - game->inventorySlotCount;
                            EquipType eType = game->equipInventory[equipIdx];
                            if (EquipItem(game, eType)) {
                                RemoveEquipFromInventory(game, equipIdx);
                            }
                            game->invSubState = INV_BROWSE;
                            if (game->inventorySelection >= game->inventorySlotCount + game->equipInventoryCount)
                                game->inventorySelection = game->inventorySlotCount + game->equipInventoryCount - 1;
                        } else if (game->invActionSelection == 1) {
                            // Drop equipment from inventory to map
                            int equipIdx = game->inventorySelection - game->inventorySlotCount;
                            EquipType eType = game->equipInventory[equipIdx];
                            const EquipData* d = GetEquipData(eType);
                            RemoveEquipFromInventory(game, equipIdx);
                            if (d) {
                                // Place on map at player position (stack same type)
                                bool stacked = false;
                                for (int e = 0; e < game->equipMapCount; e++) {
                                    if (game->equipMapCollected[e]) continue;
                                    if (game->equipMapTiles[e][0] == game->player.ent.x &&
                                        game->equipMapTiles[e][1] == game->player.ent.y &&
                                        game->equipMapTypes[e] == eType) {
                                        game->equipMapQuantities[e]++;
                                        stacked = true;
                                        break;
                                    }
                                }
                                if (!stacked && game->equipMapCount < MAX_EQUIP_ON_MAP) {
                                    game->equipMapTiles[game->equipMapCount][0] = game->player.ent.x;
                                    game->equipMapTiles[game->equipMapCount][1] = game->player.ent.y;
                                    game->equipMapCollected[game->equipMapCount] = false;
                                    game->equipMapTypes[game->equipMapCount] = eType;
                                    game->equipMapQuantities[game->equipMapCount] = 1;
                                    game->equipMapCount++;
                                }
                                CombatLog_Add(&game->combatLog, BLACK, "Dropped %s", d->name);
                            }
                            game->invSubState = INV_BROWSE;
                            if (game->inventorySelection >= game->inventorySlotCount + game->equipInventoryCount)
                                game->inventorySelection = game->inventorySlotCount + game->equipInventoryCount - 1;
                        } else {
                            // Back
                            game->invSubState = INV_BROWSE;
                        }
                    } else {
                        // Potion actions (original logic)
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
                }
                if (IsKeyPressed(KEY_ESCAPE)) {
                    game->invSubState = INV_BROWSE;
                }
            }
        }
        return;
    }

    if (game->state != STATE_PLAYER_TURN) return;
    if (game->animTimer > 0.0f) return;

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
                if (sprintDir != DIR_NONE) game->player.ent.facingDir = sprintDir;
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

                    // Pick up any equipment at the current tile
                    for (int e = 0; e < game->equipMapCount; e++) {
                        if (!game->equipMapCollected[e] &&
                            game->equipMapTiles[e][0] == game->player.ent.x &&
                            game->equipMapTiles[e][1] == game->player.ent.y) {
                            EquipType eType = game->equipMapTypes[e];
                            int qty = game->equipMapQuantities[e];
                            int picked = 0;
                            for (int i = 0; i < qty; i++) {
                                if (AddEquipToInventory(game, eType)) picked++;
                                else break;
                            }
                            if (picked >= qty) {
                                game->equipMapCollected[e] = true;
                            } else {
                                game->equipMapQuantities[e] -= picked;
                            }
                            if (picked > 0) {
                                const EquipData* d = GetEquipData(eType);
                                TraceLog(LOG_INFO, "Picked up %s", d ? d->name : "equipment");
                                CombatLog_Add(&game->combatLog, BLACK, "Picked up %s", d ? d->name : "equipment");
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

    if (dir != DIR_NONE) {
        bool ctrlHeld = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        game->player.ent.facingDir = dir;
        if (ctrlHeld) {
            // Ctrl + direction: just face that way, no turn cost
            game->selectedMonsterIdx = -1;
        } else {
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
    } else if (IsKeyPressed(KEY_PERIOD) || IsKeyPressed(KEY_SPACE)) {
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

    // Dedicated attack: F or Enter — attack in faced direction without moving
    if ((IsKeyPressed(KEY_F) || IsKeyPressed(KEY_ENTER)) && game->state == STATE_PLAYER_TURN) {
        Direction fdir = GetFacingDirection(&game->player.ent);
        int tx = game->player.ent.x;
        int ty = game->player.ent.y;
        switch (fdir) {
            case DIR_UP:    ty--; break;
            case DIR_DOWN:  ty++; break;
            case DIR_LEFT:  tx--; break;
            case DIR_RIGHT: tx++; break;
            default: break;
        }
        Monster* mon = Monster_GetAt(tx, ty, NULL);
        if (mon && mon->alive) {
            // Monster dodge
            int dodgePct = mon->dex * 2;
            if (dodgePct > 60) dodgePct = 60;
            if (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct) {
                game->player.ent.hitFlashTimer = 0.15f;
                CombatLog_Add(&game->combatLog, BLACK, "%s dodges your attack!", mon->name);
            } else {
                int damage = game->player.ent.attack + game->player.ent.str * 2 - mon->defense;
                if (damage < 1) damage = 1;
                if (GetRandomValue(1, 100) <= game->player.ent.lck) {
                    damage = damage * 2;
                    if (damage < 1) damage = 1;
                    CombatLog_Add(&game->combatLog, BLACK, "Critical hit!");
                }
                mon->hp -= damage;
                PlayHitSound();
                CombatLog_Add(&game->combatLog, BLACK, "You hit %s for %d!", mon->name, damage);
                if (mon->hp <= 0) {
                    mon->alive = false;
                    mon->hp = 0;
                    GainExperience(game, mon->expValue);
                    CombatLog_Add(&game->combatLog, BLACK, "%s defeated! (+%d exp)", mon->name, mon->expValue);
                }
            }
            game->turnCount++;
            game->enemyTurnCooldown = 0.15f;
            game->state = STATE_ENEMY_TURN;
            game->sprintBypassRoom = false;
            game->selectedMonsterIdx = -1;
        } else {
            CombatLog_Add(&game->combatLog, DARKGRAY, "Nothing to attack there");
        }
    }

    // Q: inspect tile in front of player
    if (IsKeyPressed(KEY_Q)) {
        Direction fdir = GetFacingDirection(&game->player.ent);
        int tx = game->player.ent.x;
        int ty = game->player.ent.y;
        switch (fdir) {
            case DIR_UP:    ty--; break;
            case DIR_DOWN:  ty++; break;
            case DIR_LEFT:  tx--; break;
            case DIR_RIGHT: tx++; break;
            default: break;
        }
        if (tx >= 0 && tx < game->map->width && ty >= 0 && ty < game->map->height &&
            game->visibility[ty][tx] == 1) {
            game->selectedPotionTileX = tx;
            game->selectedPotionTileY = ty;
            game->selectedPotionTileActive = true;
            // Also check for monster
            Monster* mon = Monster_GetAt(tx, ty, NULL);
            if (mon && mon->alive) {
                Monster* arr = Monster_GetArray();
                int count = Monster_GetCount();
                game->selectedMonsterIdx = -1;
                for (int i = 0; i < count; i++) {
                    if (&arr[i] == mon) {
                        game->selectedMonsterIdx = i;
                        break;
                    }
                }
            } else {
                game->selectedMonsterIdx = -1;
            }
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

    // Player sprite animation
    if (game->player.ent.alive && game->player.ent.spriteSheet.id > 0) {
        static float s_playerAnimTimer = 0.0f;
        s_playerAnimTimer += GetFrameTime();
        if (s_playerAnimTimer >= 0.5f) {
            s_playerAnimTimer -= 0.5f;
            game->player.ent.animFrame = (game->player.ent.animFrame + 1) % 4;
        }
    }

    if (game->levelUpTimer > 0.0f) {
        game->levelUpTimer -= GetFrameTime();
        if (game->levelUpTimer < 0.0f) game->levelUpTimer = 0.0f;
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
    // Show loading screen before heavy work
    BeginDrawing();
    ClearBackground(BLACK);
    const char* loadText = "LOADING...";
    int fontSize = 40;
    int tw = MeasureText(loadText, fontSize);
    DrawText(loadText, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() / 2 - fontSize / 2, fontSize, WHITE);
    EndDrawing();

    Player savedPlayer = game->player;
    InventorySlot savedInventory[MAX_INVENTORY_SLOTS];
    memcpy(savedInventory, game->inventory, sizeof(savedInventory));
    int savedInvCount = game->inventorySlotCount;
    EquipType savedEquipped[EQUIP_SLOT_COUNT];
    memcpy(savedEquipped, game->equipped, sizeof(savedEquipped));
    EquipType savedEquipInventory[MAX_INVENTORY_SLOTS];
    int savedEquipInvCount = game->equipInventoryCount;
    memcpy(savedEquipInventory, game->equipInventory, sizeof(savedEquipInventory));

    Monster_UnloadSprites();
    Monster_ResetAll();

    if (game->map) {
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
    {
        Texture2D* t = Resources_LoadTexture("resources/sprites/player.png");
        if (t) game->player.ent.spriteSheet = *t;
    }
    if (game->player.ent.spriteSheet.id == 0) {
        TraceLog(LOG_WARNING, "Could not load player spritesheet during descend");
    }

    LoadPotionTextures(game);

    // Restore equipped items (re-apply stat bonuses after memset)
    memcpy(game->equipped, savedEquipped, sizeof(savedEquipped));
    for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
        if (game->equipped[i] != EQUIP_NONE) {
            const EquipData* d = GetEquipData(game->equipped[i]);
            if (d) {
                game->player.ent.attack  += d->bonusAttack;
                game->player.ent.defense += d->bonusDefense;
                game->player.ent.str     += d->bonusStr;
                game->player.ent.dex     += d->bonusDex;
                game->player.ent.intel   += d->bonusInt;
                game->player.ent.con     += d->bonusCon;
                game->player.ent.lck     += d->bonusLck;
            }
        }
    }
    game->player.ent.maxHp = 30 + game->player.ent.con * 5;
    game->player.ent.hp = game->player.ent.maxHp;
    // Restore equipment inventory
    memcpy(game->equipInventory, savedEquipInventory, sizeof(savedEquipInventory));
    game->equipInventoryCount = savedEquipInvCount;

    {
        Texture2D* t = Resources_LoadTexture("resources/tilesets/magic_attacks.png");
        if (t) game->magicAttacksTexture = *t;
    }
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
        Spawner_Populate(game, spawnRooms, spawnRoomCount);
    }

    if (game->ecsWorld) SpawnerSystem_SpawnPickups(game->ecsWorld, game);

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

    game->player.ent.x = 1;
    game->player.ent.y = 1;
    game->player.ent.prevX = 1;
    game->player.ent.prevY = 1;
    game->player.ent.facingDir = DIR_DOWN;
    strcpy(game->player.ent.name, "Hero");
    game->player.ent.str = 3;
    game->player.ent.dex = 3;
    game->player.ent.intel = 3;
    game->player.ent.con = 3;
    game->player.ent.lck = 2;
    game->player.ent.statPoints = 0;
    game->player.ent.maxHp = 30 + game->player.ent.con * 5;
    game->player.ent.hp = game->player.ent.maxHp;
    game->player.ent.attack = 5;
    game->player.ent.defense = 1;
    game->player.ent.level = 1;
    game->player.ent.alive = true;
    game->player.ent.isPlayer = true;
    game->player.ent.color = (Color){ 50, 200, 255, 255 };
    game->player.ent.spriteRow = 6;

    {
        Texture2D* t = Resources_LoadTexture("resources/sprites/player.png");
        if (t) game->player.ent.spriteSheet = *t;
    }
    if (game->player.ent.spriteSheet.id == 0) {
        TraceLog(LOG_WARNING, "Could not load player spritesheet, using fallback rendering");
    }

    LoadPotionTextures(game);

    // Default starting equipment (silent — no combat log)
    memset(game->equipped, 0, sizeof(game->equipped));
    EquipItemSilent(game, EQUIP_SURVIVAL_KNIFE);
    InventoryAdd(game, ITEM_SMALL_HP_POTION);

    {
        Texture2D* t = Resources_LoadTexture("resources/tilesets/magic_attacks.png");
        if (t) game->magicAttacksTexture = *t;
    }
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

    if (game->ecsWorld) SpawnerSystem_SpawnPickups(game->ecsWorld, game);

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

    Resources_UnloadAll();

    if (game->map) {
        UnloadTMX(game->map);
        game->map = NULL;
    }

    memset(game, 0, sizeof(Game));
}
