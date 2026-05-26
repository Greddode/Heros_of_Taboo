#include "entity.h"
#include "core/game.h"
#include "core/audio.h"
#include "monster.h"
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
    Monster* m = Monster_GetAt(x, y, NULL);
    if (m && m->alive) return false;
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

// Attempt to move an entity one step in the given direction.
// If the target tile contains an enemy, the entity attacks instead.
// Returns true if the entity did something (moved or attacked).
bool MoveEntity(Game* game, Entity* entity, Direction dir) {
    if (!entity->alive) return false;

    if (dir == DIR_LEFT)  entity->facingRight = false;
    if (dir == DIR_RIGHT) entity->facingRight = true;

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
        CombatLog_Add(&game->combatLog, LIGHTGRAY, "%s hits %s for %d!", entity->name, target->name, damage);
        if (target->hp <= 0) {
            target->alive = false;
            target->hp = 0;
            TraceLog(LOG_INFO, "%s has been slain!", target->name);
            CombatLog_Add(&game->combatLog, LIGHTGRAY, "%s is defeated!", target->name);
        }
        return true;
    }

    // Player attacks a monster at the target tile
    if (entity->isPlayer) {
        Monster* mon = Monster_GetAt(newX, newY, NULL);
        if (mon && mon->alive) {
            int damage = entity->attack - mon->defense;
            if (damage < 1) damage = 1;
            mon->hp -= damage;
            PlayHitSound();
            entity->hitFlashTimer = 0.15f;
            mon->hitFlashTimer = 0.15f;
            TraceLog(LOG_INFO, "%s attacks %s for %d damage! (HP: %d/%d)",
                     entity->name, mon->name, damage, mon->hp, mon->maxHp);
            CombatLog_Add(&game->combatLog, LIGHTGRAY, "%s hits %s for %d!", entity->name, mon->name, damage);
            if (mon->hp <= 0) {
                mon->alive = false;
                GainExperience(game, mon->expValue);
                TraceLog(LOG_INFO, "%s has been slain!", mon->name);
                CombatLog_Add(&game->combatLog, LIGHTGRAY, "%s defeated! (+%d exp)", mon->name, mon->expValue);
            }
            return true;
        }
    }

    if (game->blocking[newY][newX]) return false;

    entity->prevX = entity->x;
    entity->prevY = entity->y;
    entity->x = newX;
    entity->y = newY;

    if (entity->isPlayer) {
        for (int h = 0; h < game->potionCount; h++) {
            if (!game->potionCollected[h] &&
                game->potionTiles[h][0] == newX &&
                game->potionTiles[h][1] == newY) {
                game->potionCollected[h] = true;
                ItemType ptype = game->potionTypes[h];
                if (InventoryAdd(game, ptype)) {
                    TraceLog(LOG_INFO, "Picked up %s", GetItemName(ptype));
                    CombatLog_Add(&game->combatLog, LIGHTGRAY, "Picked up %s", GetItemName(ptype));
                } else {
                    TraceLog(LOG_INFO, "Inventory full, dropping %s", GetItemName(ptype));
                }
                PlayPickupSound();
            }
        }
    }

    return true;
}

// Draw a single tile from the tileset at (x, y) on the given layer
void DrawTile(const Game* game, int x, int y, int layerIndex) {
    if (!game->map || !game->map->layers) return;

    int gid = GetTileGID(game->map, layerIndex, x, y);
    if (gid <= 0) return;

    Rectangle src = GetTileSourceRect(game->map, gid);
    if (src.width <= 0 || src.height <= 0) return;

    int tsIndex = FindTilesetForGID(game->map, gid);
    if (tsIndex < 0) return;
    Texture2D tex = game->tilesetTextures[tsIndex];

    Vector2 pos = TileToScreen(x, y, game->map->tileWidth, game->map->tileHeight);
    Rectangle dest = { pos.x, pos.y, (float)game->map->tileWidth, (float)game->map->tileHeight };

    DrawTexturePro(tex, src, dest, (Vector2){0, 0}, 0, WHITE);
}
