#include "movement_system.h"
#include "debug_log.h"
#include "validation.h"
#include "game.h"
#include "systems/world_monster.h"
#include "systems/combat_system.h"
#include "systems/spawner_system.h"
#include "map/map_helpers.h"
#include "inventory.h"
#include "game_audio.h"

bool MovementSystem_IsWalkable(GameWorld* gw, int x, int y) {
    if (!gw || !gw->map) return false;
    if (x < 0 || x >= gw->map->width || y < 0 || y >= gw->map->height) return false;
    if (gw->blocking[y][x]) return false;
    if (World_FindMonsterAt(gw, x, y, ENTITY_NONE) != ENTITY_NONE) return false;
    return true;
}

void MovementSystem_PlayerMove(GameWorld* gw, Direction dir) {
    if (!gw || !gw->map) return;
    World* w = &gw->ecs;
    EntityId pe = gw->playerEntity;
    if (pe == ENTITY_NONE) { TraceLog(LOG_WARNING, "MovementSystem_PlayerMove: player entity is ENTITY_NONE"); return; }
    if (!Validate_EntityId(w, pe)) { TraceLog(LOG_ERROR, "MovementSystem_PlayerMove: player entity invalid [e=%d]", (int)pe); return; }

    CPosition* ppos = World_GetPosition(w, pe);
    CStats* ps = World_GetStats(w, pe);

    int nx = ppos->x, ny = ppos->y;
    switch (dir) {
        case DIR_UP:    ny--; break;
        case DIR_DOWN:  ny++; break;
        case DIR_LEFT:  nx--; break;
        case DIR_RIGHT: nx++; break;
        default: return;
    }

    bool inBounds = (nx >= 0 && nx < gw->map->width && ny >= 0 && ny < gw->map->height);
    bool hasMonster = inBounds && World_FindMonsterAt(gw, nx, ny, ENTITY_NONE) != ENTITY_NONE;
    bool canMove = inBounds && !gw->blocking[ny][nx] && !hasMonster;

    DebugLog(DEBUG_MOVEMENT, "PlayerMove: from=(%d,%d) to=(%d,%d) dir=%d walkable=%s",
             ppos->x, ppos->y, nx, ny, (int)dir, canMove ? "yes" : (hasMonster ? "blocked(monster)" : "no"));

    if (canMove) {
        int prevX = ppos->x, prevY = ppos->y;
        ppos->prevX = prevX;
        ppos->prevY = prevY;
        ppos->x = nx;
        ppos->y = ny;
        ppos->facingDir = dir;

        gw->selectedMonsterEntity = ENTITY_NONE;
        gw->turnCount++;
        gw->enemyTurnCooldown = 0.15f;

        gw->animTimer = MOVE_ANIM_DURATION;
        gw->animDuration = MOVE_ANIM_DURATION;
        RevealFOW(gw);
        gw->state = STATE_ENEMY_TURN;

        if (Validate_TilePos(gw, ppos->x, ppos->y) && gw->monsterGrid[ppos->y][ppos->x] != ENTITY_NONE) {
            TraceLog(LOG_WARNING, "MovementSystem_PlayerMove: player tile occupied in monsterGrid [pos=(%d,%d) occupant=%d]", ppos->x, ppos->y, (int)gw->monsterGrid[ppos->y][ppos->x]);
        }

        // Collect potions (list first, only collect if room exists)
        {
            ItemType pTypes[MAX_POTIONS]; int pQtys[MAX_POTIONS];
            int pCount = SpawnerSystem_ListPotionsAt(gw, ppos->x, ppos->y,
                             pTypes, pQtys, MAX_POTIONS);
            if (pCount > 0) {
                if (gw->inventorySlotCount >= MAX_INVENTORY_SLOTS) {
                    FloatMsg_Spawn(gw, ppos->x, ppos->y, RED, "Inventory full!");
                } else {
                    SpawnerSystem_CollectPickupsAt(gw, ppos->x, ppos->y,
                        pTypes, pQtys, MAX_POTIONS);
                    for (int i = 0; i < pCount; i++) {
                        int picked = 0;
                        for (int q = 0; q < pQtys[i]; q++) {
                            if (InventoryAdd(gw, pTypes[i])) picked++;
                            else break;
                        }
                        if (picked > 0) {
                            GameAudio_PlayPickupSound();
                            DebugLog(DEBUG_MOVEMENT, "PlayerMove: collected potion type=%d qty=%d", (int)pTypes[i], picked);
                        }
                    }
                }
            }
        }
        // Collect equipment (list first, only collect if room exists)
        {
            EquipType eTypes[MAX_EQUIP_ON_MAP]; int eQtys[MAX_EQUIP_ON_MAP];
            int eCount = SpawnerSystem_ListEquipAt(gw, ppos->x, ppos->y,
                            eTypes, eQtys, MAX_EQUIP_ON_MAP);
            if (eCount > 0) {
                if (gw->equipInventoryCount >= MAX_INVENTORY_SLOTS) {
                    FloatMsg_Spawn(gw, ppos->x, ppos->y, RED, "Inventory full!");
                } else {
                    SpawnerSystem_CollectEquipAt(gw, ppos->x, ppos->y,
                        eTypes, eQtys, MAX_EQUIP_ON_MAP);
                    for (int i = 0; i < eCount; i++) {
                        int picked = 0;
                        for (int q = 0; q < eQtys[i]; q++) {
                            if (AddEquipToInventory(gw, eTypes[i])) picked++;
                            else break;
                        }
                        if (picked > 0) {
                            const EquipData* d = GetEquipData(eTypes[i]);
                            GameAudio_PlayPickupSound();
                            SpawnerSystem_ReduceEquipAt(gw, ppos->x, ppos->y, eTypes[i], picked);
                            DebugLog(DEBUG_MOVEMENT, "PlayerMove: collected equip type=%d qty=%d",
                                     (int)eTypes[i], picked);
                        }
                    }
                }
            }
        }

        // Stair / escape checks
        if (ppos->x == gw->stairX && ppos->y == gw->stairY && gw->stairX >= 0 && gw->stairY >= 0) {
            DebugLog(DEBUG_MOVEMENT, "PlayerMove: stairs triggered at (%d,%d)", ppos->x, ppos->y);
            DescendFloor(gw);
        }
        if (gw->escapeSpawned && ppos->x == gw->escapeX && ppos->y == gw->escapeY) {
            DebugLog(DEBUG_MOVEMENT, "PlayerMove: escape triggered at (%d,%d)", ppos->x, ppos->y);
            gw->state = STATE_WIN;
        }

    } else if (inBounds && hasMonster) {
        // Attack on bump
        DebugLog(DEBUG_MOVEMENT, "PlayerMove: bump attack at (%d,%d)", nx, ny);
        if (CombatSystem_PlayerMeleeAttack(gw, pe, nx, ny)) {
            gw->turnCount++;
            gw->enemyTurnCooldown = 0.15f;
            gw->state = STATE_ENEMY_TURN;
            gw->selectedMonsterEntity = ENTITY_NONE;
        }
    }
}

void MovementSystem_UpdateAnimations(GameWorld* gw, float dt) {
    (void)gw;
    (void)dt;
    // Animation interpolation is handled in the render system using animTimer/monsterAnimTimer.
}
