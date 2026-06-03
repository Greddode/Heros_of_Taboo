#include "movement_system.h"
#include "game.h"
#include "systems/world_monster.h"
#include "systems/combat_system.h"
#include "systems/spawner_system.h"
#include "map/map_helpers.h"
#include "inventory.h"
#include "game_audio.h"
#include "ui/combat_log.h"

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
    if (pe == ENTITY_NONE) return;

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
    bool canMove = inBounds && !gw->blocking[ny][nx]
        && World_FindMonsterAt(gw, nx, ny, ENTITY_NONE) == ENTITY_NONE;

    if (canMove) {
        ppos->prevX = ppos->x;
        ppos->prevY = ppos->y;
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

        // Collect potions
        {
            ItemType pTypes[MAX_POTIONS]; int pQtys[MAX_POTIONS];
            int pCount = SpawnerSystem_CollectPickupsAt(gw, ppos->x, ppos->y, pTypes, pQtys, MAX_POTIONS);
            for (int i = 0; i < pCount; i++) {
                int picked = 0;
                for (int q = 0; q < pQtys[i]; q++) {
                    if (InventoryAdd(gw, pTypes[i])) picked++;
                    else break;
                }
                if (picked > 0) {
                    CombatLog_Add(&gw->combatLog, BLACK, "Picked up %d x %s", picked, GetItemName(pTypes[i]));
                    GameAudio_PlayPickupSound();
                }
            }
        }
        // Collect equipment
        {
            EquipType eTypes[MAX_EQUIP_ON_MAP]; int eQtys[MAX_EQUIP_ON_MAP];
            int eCount = SpawnerSystem_CollectEquipAt(gw, ppos->x, ppos->y, eTypes, eQtys, MAX_EQUIP_ON_MAP);
            for (int i = 0; i < eCount; i++) {
                int picked = 0;
                for (int q = 0; q < eQtys[i]; q++) {
                    if (AddEquipToInventory(gw, eTypes[i])) picked++;
                    else break;
                }
                if (picked > 0) {
                    const EquipData* d = GetEquipData(eTypes[i]);
                    CombatLog_Add(&gw->combatLog, BLACK, "Picked up %s", d ? d->name : "equipment");
                    GameAudio_PlayPickupSound();
                    SpawnerSystem_ReduceEquipAt(gw, ppos->x, ppos->y, eTypes[i], picked);
                }
            }
        }

        // Stair / escape checks
        if (ppos->x == gw->stairX && ppos->y == gw->stairY && gw->stairX >= 0 && gw->stairY >= 0)
            DescendFloor(gw);
        if (gw->escapeSpawned && ppos->x == gw->escapeX && ppos->y == gw->escapeY)
            gw->state = STATE_WIN;

    } else if (inBounds && World_FindMonsterAt(gw, nx, ny, ENTITY_NONE) != ENTITY_NONE) {
        // Attack on bump
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