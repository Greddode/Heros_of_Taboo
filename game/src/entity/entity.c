#include "entity.h"
#include "core/game.h"
#include "world.h"
#include "core/audio.h"
#include "systems/spawner_system.h"
#include "data/monster_data.h"
#include "systems/combat_system.h"
#include "systems/world_monster.h"
#include "player.h"
#include "ui/combat_log.h"
#include <stdio.h>

Vector2 TileToScreen(int x, int y, int tileWidth, int tileHeight) {
    return (Vector2){ (float)(x * tileWidth), (float)(y * tileHeight) };
}

// Returns true if the tile at (x, y) is in bounds, not a wall, and unoccupied
bool IsWalkable(const Game* game, int x, int y) {
    if (!game->map) return false;
    if (x < 0 || x >= game->map->width || y < 0 || y >= game->map->height) return false;
    if (game->blocking[y][x]) return false;
    if (World_FindMonsterAt(&game->ecsWorld, x, y, ENTITY_NONE) != ENTITY_NONE) return false;
    return true;
}

// Return a pointer to the player Entity if they are at (x, y) and not excluded.
Entity* GetEntityAt(Game* game, int x, int y, Entity* exclude) {
    if (!exclude || !exclude->isPlayer) {
        if (game->player.ent.alive && game->player.ent.x == x && game->player.ent.y == y) {
            return &game->player.ent;
        }
    }
    return NULL;
}

// Return the direction this entity is facing
Direction GetFacingDirection(const Entity* entity) {
    return entity->facingDir;
}

// Attempt to move an entity one step in the given direction.
// If the target tile contains an enemy, the entity attacks instead.
// Returns true if the entity did something (moved or attacked).
bool MoveEntity(Game* game, Entity* entity, Direction dir) {
    if (!entity->alive) return false;

    if (dir != DIR_NONE) entity->facingDir = dir;

    int newX = entity->x;
    int newY = entity->y;

    switch (dir) {
        case DIR_UP:    newY--; break;
        case DIR_DOWN:  newY++; break;
        case DIR_LEFT:  newX--; break;
        case DIR_RIGHT: newX++; break;
        default: return false;
    }

    if (newX < 0 || newX >= game->map->width ||
        newY < 0 || newY >= game->map->height) return false;

    Entity* target = GetEntityAt(game, newX, newY, entity);
    if (target) {
        int damage = entity->attack - target->defense;
        if (damage < 1) damage = 1;
        target->hp -= damage;
        PlayHitSound();
        TraceLog(LOG_INFO, "%s attacks %s for %d damage! (HP: %d/%d)",
                 entity->name, target->name, damage, target->hp, target->maxHp);
        CombatLog_Add(&game->combatLog, BLACK, "%s hits %s for %d!", entity->name, target->name, damage);
        if (target->hp <= 0) {
            target->alive = false;
            target->hp = 0;
            TraceLog(LOG_INFO, "%s has been slain!", target->name);
            CombatLog_Add(&game->combatLog, BLACK, "%s is defeated!", target->name);
        }
        return true;
    }

    // Player attacks a monster at the target tile
    if (entity->isPlayer) {
        SyncGameWorldFromGame(game);
        if (CombatSystem_PlayerMeleeAttack(&game->ecsWorld, game, entity, newX, newY)) {
            return true;
        }
    }

    if (game->blocking[newY][newX]) return false;

    entity->prevX = entity->x;
    entity->prevY = entity->y;
    entity->x = newX;
    entity->y = newY;

    if (entity->isPlayer) {
        GameWorld* gw = &game->ecsWorld;
        for (EntityId eid = 1; eid < (EntityId)gw->ecs.count; eid++) {
            if (!gw->ecs.alive[eid]) continue;
            if (!World_HasComponents(&gw->ecs, eid, COMP_POSITION | COMP_PICKUP)) continue;
            CPosition* pos = World_GetPosition(&gw->ecs, eid);
            CPickup* pk = World_GetPickup(&gw->ecs, eid);
            if (pk->quantity <= 0 || pos->x != newX || pos->y != newY) continue;

            if (!pk->isEquip) {
                ItemType ptype = pk->itemType;
                int qty = pk->quantity;
                int picked = 0;
                for (int i = 0; i < qty; i++) {
                    if (InventoryAdd(game, ptype)) picked++;
                    else break;
                }
                pk->quantity = 0;
                if (picked > 0) {
                    TraceLog(LOG_INFO, "Picked up %d x %s", picked, GetItemName(ptype));
                    CombatLog_Add(&game->combatLog, BLACK, "Picked up %d x %s", picked, GetItemName(ptype));
                    PlayPickupSound();
                }
            } else {
                EquipType eType = pk->equipType;
                int qty = pk->quantity;
                int picked = 0;
                for (int i = 0; i < qty; i++) {
                    if (AddEquipToInventory(game, eType)) picked++;
                    else break;
                }
                pk->quantity -= picked;
                if (pk->quantity < 0) pk->quantity = 0;
                if (picked > 0) {
                    const EquipData* d = GetEquipData(eType);
                    TraceLog(LOG_INFO, "Picked up %s", d ? d->name : "equipment");
                    CombatLog_Add(&game->combatLog, BLACK, "Picked up %s", d ? d->name : "equipment");
                    PlayPickupSound();
                }
            }
        }
    }

    return true;
}

void DrawTileWorld(const GameWorld* gw, int x, int y, int layerIndex) {
    if (!gw || !gw->map || !gw->map->layers) return;

    int gid = GetTileGID(gw->map, layerIndex, x, y);
    if (gid <= 0) return;

    Rectangle src = GetTileSourceRect(gw->map, gid);
    if (src.width <= 0 || src.height <= 0) return;

    int tsIndex = FindTilesetForGID(gw->map, gid);
    if (tsIndex < 0 || tsIndex >= gw->map->tilesetCount) return;

    Texture2D tex = gw->tilesetTextures[tsIndex];
    if (tex.id == 0) return;

    Vector2 pos = TileToScreen(x, y, gw->map->tileWidth, gw->map->tileHeight);
    Rectangle dest = { pos.x, pos.y, (float)gw->map->tileWidth, (float)gw->map->tileHeight };
    DrawTexturePro(tex, src, dest, (Vector2){ 0, 0 }, 0, WHITE);
}

// Draw a single tile from the tileset at (x, y) on the given layer
void DrawTile(const Game* game, int x, int y, int layerIndex) {
    if (!game || !game->map || !game->map->layers) return;

    int gid = GetTileGID(game->map, layerIndex, x, y);
    if (gid <= 0) return;

    Rectangle src = GetTileSourceRect(game->map, gid);
    if (src.width <= 0 || src.height <= 0) return;

    int tsIndex = FindTilesetForGID(game->map, gid);
    if (tsIndex < 0 || tsIndex >= game->map->tilesetCount) return;

    Texture2D tex = game->tilesetTextures[tsIndex];
    if (tex.id == 0) return;

    Vector2 pos = TileToScreen(x, y, game->map->tileWidth, game->map->tileHeight);
    Rectangle dest = { pos.x, pos.y, (float)game->map->tileWidth, (float)game->map->tileHeight };
    DrawTexturePro(tex, src, dest, (Vector2){ 0, 0 }, 0, WHITE);
}
