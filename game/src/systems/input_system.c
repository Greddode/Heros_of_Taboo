#include "systems/input_system.h"
#include "game.h"
#include "audio.h"
#include "map/map_helpers.h"
#include "inventory.h"
#include "ui/combat_log.h"
#include "systems/spawner_system.h"
#include "systems/ai_system.h"
#include "systems/combat_system.h"
#include "systems/player.h"
#include "data/monster_data.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

void InputSystem_Inventory(GameWorld* game, InventoryUIState* ui) {
    if (game->state != STATE_INVENTORY) return;

    World* w = &game->ecs;
    EntityId pe = game->playerEntity;
    CPosition* ppos = (pe != ENTITY_NONE) ? World_GetPosition(w, pe) : NULL;
    CStats* ps = (pe != ENTITY_NONE) ? World_GetStats(w, pe) : NULL;

    if (IsKeyPressed(KEY_I)) { game->state = STATE_PLAYER_TURN; return; }
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (ui->subState == INV_ACTION_MENU) ui->subState = INV_BROWSE;
        else game->state = STATE_PLAYER_TURN;
        return;
    }

    if (ui->subState == INV_BROWSE) {
        int curTab = (int)ui->tab;
        if (IsKeyPressed(KEY_Q)) { curTab--; if (curTab < 0) curTab = INV_TAB_COUNT - 1; ui->tab = (InventoryTab)curTab; ui->selection = 0; ui->scrollOffset = 0; return; }
        if (IsKeyPressed(KEY_E)) { curTab++; if (curTab >= INV_TAB_COUNT) curTab = 0; ui->tab = (InventoryTab)curTab; ui->selection = 0; ui->scrollOffset = 0; return; }
        int wheel = (int)GetMouseWheelMove();
        if (wheel != 0) { ui->scrollOffset -= wheel * (int)(20 * GetUIScale()); if (ui->scrollOffset < 0) ui->scrollOffset = 0; }
    }

    if (ui->subState == INV_BROWSE && ui->tab == INV_TAB_STATS) {
        int col1SelMax = 13;
        int col2SelMax = (ps && ps->statPoints > 0) ? 5 : 0;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) { ui->statsActiveCol = 0; ui->statsSelection = 0; }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) { ui->statsActiveCol = 1; ui->statsSelection = 1; if (ui->statsSelection > col2SelMax) ui->statsSelection = col2SelMax; }
        if (ui->statsActiveCol == 0) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) { ui->statsSelection--; if (ui->statsSelection < 0) ui->statsSelection = 0; }
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) { ui->statsSelection++; if (ui->statsSelection > col1SelMax) ui->statsSelection = col1SelMax; }
        } else {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) { ui->statsSelection--; if (ui->statsSelection < 0) ui->statsSelection = 0; }
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) { ui->statsSelection++; if (ui->statsSelection > col2SelMax) ui->statsSelection = col2SelMax; }
        }
        if (ps && ps->statPoints > 0 && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))) {
            int statIdx = -1;
            if (ui->statsActiveCol == 0 && ui->statsSelection >= 3 && ui->statsSelection <= 7) statIdx = ui->statsSelection - 3;
            else if (ui->statsActiveCol == 1 && ui->statsSelection >= 1 && ui->statsSelection <= 5) statIdx = ui->statsSelection - 1;
            if (statIdx >= 0 && ps) { AllocateStatPoint(ps, statIdx); return; }
        }
    }

    if (ui->subState == INV_BROWSE && ui->tab == INV_TAB_EQUIPMENT) {
        float iscale = GetUIScale();
        int slotH = (int)(44 * iscale);
        int contentH = (int)((400 - 24 - 40 - 60) * iscale);
        int topPad = (int)(40 * iscale);
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) { if (ui->selection > 0) { ui->selection--; int t = topPad - ui->scrollOffset + ui->selection * slotH; if (t < 0) ui->scrollOffset += t; } }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) { if (ui->selection < EQUIP_SLOT_COUNT - 1) { ui->selection++; int b = topPad - ui->scrollOffset + (ui->selection + 1) * slotH; if (b > contentH) ui->scrollOffset += (b - contentH); } }
        if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) && game->equipped[ui->selection] != EQUIP_NONE) { ui->subState = INV_ACTION_MENU; ui->actionSelection = 0; }
        return;
    }

    if (ui->subState == INV_ACTION_MENU && ui->tab == INV_TAB_EQUIPMENT) {
        if (IsKeyPressed(KEY_UP) && ui->actionSelection > 0) ui->actionSelection--;
        if (IsKeyPressed(KEY_DOWN) && ui->actionSelection < 2) ui->actionSelection++;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            EquipSlot slot = (EquipSlot)ui->selection;
            if (ui->actionSelection == 0) { UnequipSlot(game, slot); ui->subState = INV_BROWSE; }
            else if (ui->actionSelection == 1) {
                EquipType oldType = game->equipped[(int)slot];
                if (oldType != EQUIP_NONE) {
                    const EquipData* data = GetEquipData(oldType);
                    if (data && ps) { ps->attack -= data->bonusAttack; ps->defense -= data->bonusDefense; ps->str -= data->bonusStr; ps->dex -= data->bonusDex; ps->intel -= data->bonusInt; ps->con -= data->bonusCon; ps->lck -= data->bonusLck; ps->maxHp = 30 + ps->con * 5; if (ps->hp > ps->maxHp) ps->hp = ps->maxHp; }
                    game->equipped[(int)slot] = EQUIP_NONE;
                    SpawnerSystem_AddEquipAt(game, ppos->x, ppos->y, oldType, 1);
                    CombatLog_Add(&game->combatLog, BLACK, "Dropped %s", data ? data->name : "item");
                }
                ui->subState = INV_BROWSE;
            } else { ui->subState = INV_BROWSE; }
        }
        if (IsKeyPressed(KEY_ESCAPE)) ui->subState = INV_BROWSE;
        return;
    }

    {
        int totalInv = game->inventorySlotCount + game->equipInventoryCount;
        if (ui->subState == INV_BROWSE && ui->tab == INV_TAB_INVENTORY && totalInv > 0) {
            float iscale = GetUIScale();
            int itemH = (int)(22 * iscale);
            int contentH = (int)((400 - 24 - 40 - 60) * iscale);
            int topPad = (int)(42 * iscale);
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) { if (ui->selection > 0) { ui->selection--; int t = topPad - ui->scrollOffset + ui->selection * itemH; if (t < 0) ui->scrollOffset += t; } }
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) { if (ui->selection < totalInv - 1) { ui->selection++; int b = topPad - ui->scrollOffset + (ui->selection + 1) * itemH; if (b > contentH) ui->scrollOffset += (b - contentH); } }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) { ui->subState = INV_ACTION_MENU; ui->actionSelection = 0; }
        } else if (ui->subState == INV_ACTION_MENU && ui->tab == INV_TAB_INVENTORY) {
            bool isEquip = ui->selection >= game->inventorySlotCount;
            int maxAction = isEquip ? 2 : 3;
            if (IsKeyPressed(KEY_UP) && ui->actionSelection > 0) ui->actionSelection--;
            if (IsKeyPressed(KEY_DOWN) && ui->actionSelection < maxAction) ui->actionSelection++;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (isEquip) {
                    if (ui->actionSelection == 0) {
                        int equipIdx = ui->selection - game->inventorySlotCount;
                        if (EquipItem(game, game->equipInventory[equipIdx])) RemoveEquipFromInventory(game, equipIdx);
                        ui->subState = INV_BROWSE;
                        if (ui->selection >= game->inventorySlotCount + game->equipInventoryCount) ui->selection = game->inventorySlotCount + game->equipInventoryCount - 1;
                    } else if (ui->actionSelection == 1) {
                        int equipIdx = ui->selection - game->inventorySlotCount;
                        EquipType eType = game->equipInventory[equipIdx];
                        const EquipData* d = GetEquipData(eType);
                        RemoveEquipFromInventory(game, equipIdx);
                        if (d) { SpawnerSystem_AddEquipAt(game, ppos->x, ppos->y, eType, 1); CombatLog_Add(&game->combatLog, BLACK, "Dropped %s", d->name); }
                        ui->subState = INV_BROWSE;
                        if (ui->selection >= game->inventorySlotCount + game->equipInventoryCount) ui->selection = game->inventorySlotCount + game->equipInventoryCount - 1;
                    } else { ui->subState = INV_BROWSE; }
                } else {
                    if (ui->actionSelection == 0) {
                        if (InventoryUse(game, ui->selection)) { game->turnCount++; game->enemyTurnCooldown = 0.15f; game->state = STATE_ENEMY_TURN; } else { ui->subState = INV_BROWSE; }
                    } else if (ui->actionSelection == 1) {
                        InventorySlot* slot = &game->inventory[ui->selection];
                        ItemType type = slot->type;
                        slot->quantity--;
                        SpawnerSystem_AddPotionAt(game, ppos->x, ppos->y, type, 1);
                        CombatLog_Add(&game->combatLog, BLACK, "Dropped %s", GetItemName(type));
                        if (slot->quantity <= 0) { for (int i = ui->selection; i < game->inventorySlotCount - 1; i++) game->inventory[i] = game->inventory[i + 1]; game->inventorySlotCount--; if (ui->selection >= game->inventorySlotCount) ui->selection = game->inventorySlotCount - 1; }
                        ui->subState = INV_BROWSE;
                    } else if (ui->actionSelection == 2) {
                        InventorySlot* slot = &game->inventory[ui->selection];
                        ItemType type = slot->type;
                        int total = slot->quantity;
                        slot->quantity = 0;
                        SpawnerSystem_AddPotionAt(game, ppos->x, ppos->y, type, total);
                        CombatLog_Add(&game->combatLog, BLACK, "Dropped %d x %s", total, GetItemName(type));
                        for (int i = ui->selection; i < game->inventorySlotCount - 1; i++) game->inventory[i] = game->inventory[i + 1];
                        game->inventorySlotCount--;
                        if (ui->selection >= game->inventorySlotCount) ui->selection = game->inventorySlotCount - 1;
                        ui->subState = INV_BROWSE;
                    } else { ui->subState = INV_BROWSE; }
                }
            }
            if (IsKeyPressed(KEY_ESCAPE)) ui->subState = INV_BROWSE;
        }
    }
}

void InputSystem_PlayerTurn(GameWorld* game, InventoryUIState* ui) {
    (void)ui;
    if (game->state != STATE_PLAYER_TURN) return;
    if (game->animTimer > 0.0f) return;

    World* w = &game->ecs;
    EntityId pe = game->playerEntity;
    CPosition* ppos = (pe != ENTITY_NONE) ? World_GetPosition(w, pe) : NULL;
    CStats* ps = (pe != ENTITY_NONE) ? World_GetStats(w, pe) : NULL;

    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        Direction sprintDir = DIR_NONE;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) sprintDir = DIR_UP;
        else if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) sprintDir = DIR_DOWN;
        else if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) sprintDir = DIR_LEFT;
        else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) sprintDir = DIR_RIGHT;

        if (sprintDir != DIR_NONE) {
            bool stoppedAtRoom = false;
            int dx = 0, dy = 0;
            switch (sprintDir) { case DIR_UP: dy = -1; break; case DIR_DOWN: dy = 1; break; case DIR_LEFT: dx = -1; break; case DIR_RIGHT: dx = 1; break; default: break; }

            int startX = ppos ? ppos->x : 0, startY = ppos ? ppos->y : 0;
            int endX = startX, endY = startY;
            while (1) { int nx = endX + dx; int ny = endY + dy; if (nx < 0 || nx >= game->map->width || ny < 0 || ny >= game->map->height) break; if (game->blocking[ny][nx]) break; if (World_FindMonsterAt(game, nx, ny, ENTITY_NONE) != ENTITY_NONE) break; endX = nx; endY = ny; }

            int steps = abs(endX - startX) + abs(endY - startY);
            if (steps > 0) {
                typedef struct { EntityId id; int prevX, prevY; float hitFlash; } MonSnap;
                MonSnap monSnap[64]; int snapCount = 0;
                for (EntityId e = 1; e < (EntityId)game->ecs.count && snapCount < 64; e++) {
                    if (!game->ecs.alive[e]) continue;
                    if (!World_HasComponents(&game->ecs, e, COMP_POSITION | COMP_STATS | COMP_AI)) continue;
                    if (!World_GetStats(&game->ecs, e)->alive) continue;
                    CPosition* mp = World_GetPosition(&game->ecs, e);
                    monSnap[snapCount].id = e; monSnap[snapCount].prevX = mp->x; monSnap[snapCount].prevY = mp->y;
                    monSnap[snapCount].hitFlash = World_HasComponents(&game->ecs, e, COMP_HIT_FLASH) ? World_GetHitFlash(&game->ecs, e)->timer : 0.0f;
                    snapCount++;
                }
                if (ppos) { ppos->prevX = startX; ppos->prevY = startY; ppos->x = startX; ppos->y = startY; }
                if (sprintDir != DIR_NONE && ppos) ppos->facingDir = sprintDir;
                game->selectedMonsterEntity = ENTITY_NONE;

                for (int s = 0; s < steps; s++) {
                    int nx = (ppos ? ppos->x : 0) + dx, ny = (ppos ? ppos->y : 0) + dy;
                    if (nx < 0 || nx >= game->map->width || ny < 0 || ny >= game->map->height) break;
                    if (game->blocking[ny][nx]) break;
                    if (World_FindMonsterAt(game, nx, ny, ENTITY_NONE) != ENTITY_NONE) break;
                    if (IsInRoom(ppos ? ppos->x : 0, ppos ? ppos->y : 0) != IsInRoom(nx, ny)) {
                        if (game->sprintBypassRoom) { game->sprintBypassRoom = false; } else { game->sprintBypassRoom = true; stoppedAtRoom = true; break; }
                    }
                    if (ppos) { ppos->x = nx; ppos->y = ny; } game->turnCount++;
                    int hpBefore = ps ? ps->hp : 0;
                    bool alive = AISystem_ProcessAll(game, game->timeWaited);
                    if (!alive) { game->state = STATE_GAME_OVER; return; }
                    if (ps && ps->hp < hpBefore) break;
                    { ItemType pTypes[MAX_POTIONS]; int pQtys[MAX_POTIONS]; int pCount = SpawnerSystem_CollectPickupsAt(game, ppos ? ppos->x : 0, ppos ? ppos->y : 0, pTypes, pQtys, MAX_POTIONS); for (int i = 0; i < pCount; i++) { int picked = 0; for (int q = 0; q < pQtys[i]; q++) { if (InventoryAdd(game, pTypes[i])) picked++; else break; } if (picked > 0) { CombatLog_Add(&game->combatLog, BLACK, "Picked up %d x %s", picked, GetItemName(pTypes[i])); PlayPickupSound(); } } }
                    { EquipType eTypes[MAX_EQUIP_ON_MAP]; int eQtys[MAX_EQUIP_ON_MAP]; int eCount = SpawnerSystem_CollectEquipAt(game, ppos ? ppos->x : 0, ppos ? ppos->y : 0, eTypes, eQtys, MAX_EQUIP_ON_MAP); for (int i = 0; i < eCount; i++) { int picked = 0; for (int q = 0; q < eQtys[i]; q++) { if (AddEquipToInventory(game, eTypes[i])) picked++; else break; } if (picked > 0) { const EquipData* d = GetEquipData(eTypes[i]); CombatLog_Add(&game->combatLog, BLACK, "Picked up %s", d ? d->name : "equipment"); PlayPickupSound(); SpawnerSystem_ReduceEquipAt(game, ppos ? ppos->x : 0, ppos ? ppos->y : 0, eTypes[i], picked); } } }
                }
                if (!stoppedAtRoom) game->sprintBypassRoom = false;
                for (int i = 0; i < snapCount; i++) { CPosition* mp = World_GetPosition(&game->ecs, monSnap[i].id); mp->prevX = monSnap[i].prevX; mp->prevY = monSnap[i].prevY; if (World_HasComponents(&game->ecs, monSnap[i].id, COMP_HIT_FLASH)) World_GetHitFlash(&game->ecs, monSnap[i].id)->timer = monSnap[i].hitFlash; }
                game->animTimer = 0.30f; game->animDuration = 0.30f; game->monsterAnimTimer = 0.30f; game->monsterAnimDuration = 0.30f;
                game->enemyTurnCooldown = 0.0f; game->state = STATE_ENEMY_TURN; game->animatingEnemyTurn = true;
                if (!game->projectile.active) { game->animatingEnemyTurn = false; game->state = STATE_PLAYER_TURN; }
                RevealFOW(game);
                if (ppos && ppos->x == game->stairX && ppos->y == game->stairY && game->stairX >= 0 && game->stairY >= 0) DescendFloor(game);
                if (game->escapeSpawned && ppos && ppos->x == game->escapeX && ppos->y == game->escapeY) game->state = STATE_WIN;
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
        if (ppos) ppos->facingDir = dir;
        if (ctrlHeld) { game->selectedMonsterEntity = ENTITY_NONE; }
        else {
            game->sprintBypassRoom = false;
            int oldX = ppos ? ppos->x : 0, oldY = ppos ? ppos->y : 0;
            bool moved = false;
            if (ppos) {
                int nx = ppos->x, ny = ppos->y;
                switch (dir) {
                    case DIR_UP: ny--; break;
                    case DIR_DOWN: ny++; break;
                    case DIR_LEFT: nx--; break;
                    case DIR_RIGHT: nx++; break;
                    default: break;
                }
                bool canMove = (nx >= 0 && nx < game->map->width && ny >= 0 && ny < game->map->height
                    && !game->blocking[ny][nx]
                    && World_FindMonsterAt(game, nx, ny, ENTITY_NONE) == ENTITY_NONE);
                if (canMove) {
                    ppos->prevX = ppos->x; ppos->prevY = ppos->y;
                    ppos->x = nx; ppos->y = ny;
                    moved = true;
                }
            }
            if (moved) {
                game->selectedMonsterEntity = ENTITY_NONE; game->turnCount++; game->enemyTurnCooldown = 0.15f;
                if (ppos && (ppos->x != oldX || ppos->y != oldY)) { game->animTimer = MOVE_ANIM_DURATION; game->animDuration = MOVE_ANIM_DURATION; RevealFOW(game); }
                game->state = STATE_ENEMY_TURN;
            }
        }
    } else if (IsKeyPressed(KEY_PERIOD) || IsKeyPressed(KEY_SPACE)) {
        game->sprintBypassRoom = false; game->turnCount++; game->timeWaited++;
        if (game->timeWaited == 15 && game->currentFloor < game->maxFloors) { SpawnShadow(game); CombatLog_Add(&game->combatLog, RED, "You feel a presence come to this floor"); }
        if (ps && ps->hp < ps->maxHp) { int waitHeal = 1 + (ps->level / 5) * 2; ps->hp += waitHeal; if (ps->hp > ps->maxHp) ps->hp = ps->maxHp; CombatLog_Add(&game->combatLog, BLACK, "Wait heals %d HP", waitHeal); }
        game->enemyTurnCooldown = 0.08f; game->state = STATE_ENEMY_TURN; return;
    }

    if ((IsKeyPressed(KEY_F) || IsKeyPressed(KEY_ENTER)) && game->state == STATE_PLAYER_TURN) {
        Direction fdir = ppos ? ppos->facingDir : DIR_NONE;
        int tx = ppos ? ppos->x : 0, ty = ppos ? ppos->y : 0;
        switch (fdir) { case DIR_UP: ty--; break; case DIR_DOWN: ty++; break; case DIR_LEFT: tx--; break; case DIR_RIGHT: tx++; break; default: break; }
        if (CombatSystem_PlayerMeleeAttack(game, game, pe, tx, ty)) { game->turnCount++; game->enemyTurnCooldown = 0.15f; game->state = STATE_ENEMY_TURN; game->sprintBypassRoom = false; game->selectedMonsterEntity = ENTITY_NONE; }
        else CombatLog_Add(&game->combatLog, DARKGRAY, "Nothing to attack there");
    }

    if (IsKeyPressed(KEY_Q)) {
        Direction fdir = ppos ? ppos->facingDir : DIR_NONE;
        int tx = ppos ? ppos->x : 0, ty = ppos ? ppos->y : 0;
        switch (fdir) { case DIR_UP: ty--; break; case DIR_DOWN: ty++; break; case DIR_LEFT: tx--; break; case DIR_RIGHT: tx++; break; default: break; }
        if (tx >= 0 && tx < game->map->width && ty >= 0 && ty < game->map->height && game->visibility[ty][tx] == 1) {
            game->selectedPotionTileX = tx; game->selectedPotionTileY = ty; game->selectedPotionTileActive = true;
            EntityId mon = World_FindMonsterAt(game, tx, ty, ENTITY_NONE);
            game->selectedMonsterEntity = (mon != ENTITY_NONE && World_GetStats(&game->ecs, mon)->alive) ? mon : ENTITY_NONE;
        }
    }

    if (IsKeyPressed(KEY_I)) { game->state = STATE_INVENTORY; ui->subState = INV_BROWSE; if (ui->selection < 0 || ui->selection >= game->inventorySlotCount) ui->selection = 0; return; }

    if (ppos && ppos->x == game->stairX && ppos->y == game->stairY && game->stairX >= 0 && game->stairY >= 0) DescendFloor(game);
    if (game->escapeSpawned && ppos && ppos->x == game->escapeX && ppos->y == game->escapeY) game->state = STATE_WIN;
}
