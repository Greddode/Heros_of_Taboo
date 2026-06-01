#include "combat_system.h"
#include "world.h"
#include "world_monster.h"
#include "inventory.h"
#include <stddef.h>
#include <math.h>
#include "game_audio.h"
#include "systems/player.h"
#include "ui/combat_log.h"
#include "resources.h"
#include <string.h>

static bool HasLineOfSight(int x0, int y0, int x1, int y1,
                            const unsigned char blocking[][MAP_WIDTH], int maxDist) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int steps = dx > dy ? dx : dy;
    if (steps > maxDist) return false;

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int x = x0, y = y0;
    for (int i = 0; i < steps; i++) {
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 <  dx) { err += dx; y += sy; }
        if ((x != x1 || y != y1) && blocking[y][x]) return false;
    }
    return true;
}

bool CombatSystem_PlayerMeleeAttack(GameWorld* game, EntityId attackerId, int targetX, int targetY) {
    if (!game) return false;

    EntityId mon = World_FindMonsterAt(game, targetX, targetY, ENTITY_NONE);
    if (mon == ENTITY_NONE) return false;

    CStats* as = World_GetStats(&game->ecs, attackerId);
    CStats* ms = World_GetStats(&game->ecs, mon);
    if (!ms->alive) return false;

    CName* name = World_HasComponents(&game->ecs, mon, COMP_NAME)
        ? World_GetName(&game->ecs, mon) : NULL;
    const char* monName = name ? name->name : "monster";

    int dodgePct = ms->dex * 2;
    if (dodgePct > 60) dodgePct = 60;
    if (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct) {
        if (World_HasComponents(&game->ecs, attackerId, COMP_HIT_FLASH))
            World_GetHitFlash(&game->ecs, attackerId)->timer = 0.15f;
        CombatLog_Add(&game->combatLog, BLACK, "%s dodges your attack!", monName);
        return true;
    }

    int damage = as->attack + as->str * 2 - ms->defense;
    if (damage < 1) damage = 1;
    if (GetRandomValue(1, 100) <= as->lck) {
        damage *= 2;
        if (damage < 1) damage = 1;
        CombatLog_Add(&game->combatLog, BLACK, "Critical hit!");
    }

    ms->hp -= damage;
    GameAudio_PlayHitSound();
    if (World_HasComponents(&game->ecs, attackerId, COMP_HIT_FLASH))
        World_GetHitFlash(&game->ecs, attackerId)->timer = 0.15f;
    if (World_HasComponents(&game->ecs, mon, COMP_HIT_FLASH)) {
        World_GetHitFlash(&game->ecs, mon)->timer = 0.15f;
    }
    CombatLog_Add(&game->combatLog, BLACK, "You hit %s for %d!", monName, damage);

    if (ms->hp <= 0) {
        ms->alive = false;
        ms->hp = 0;
        GainExperience(game, ms->expValue);
        CombatLog_Add(&game->combatLog, BLACK, "%s defeated! (+%d exp)", monName, ms->expValue);
    }
    return true;
}

bool CombatSystem_PlayerRangedAttack(GameWorld* game, EntityId attackerId) {
    if (!game) return false;

    EquipType weapon = game->equipped[EQUIP_SLOT_WEAPON];
    const EquipData* wdata = GetEquipData(weapon);
    if (!wdata || !wdata->isRanged) {
        CombatLog_Add(&game->combatLog, DARKGRAY, "No ranged weapon equipped.");
        return false;
    }

    int range = GetEquipRangeBonus(weapon);
    CStats* as = World_GetStats(&game->ecs, attackerId);
    CPosition* apos = World_GetPosition(&game->ecs, attackerId);

    int dx = 0, dy = 0;
    switch (apos->facingDir) {
        case DIR_UP:    dy = -1; break;
        case DIR_DOWN:  dy = 1;  break;
        case DIR_LEFT:  dx = -1; break;
        case DIR_RIGHT: dx = 1;  break;
        default: return false;
    }

    if (apos->facingDir == DIR_UP || apos->facingDir == DIR_DOWN) {
        if (!HasLineOfSight(apos->x, apos->y, apos->x + dx * range, apos->y + dy * range,
                            game->blocking, range)) {
            CombatLog_Add(&game->combatLog, DARKGRAY, "Nothing in range.");
            return false;
        }
    }

    EntityId target = ENTITY_NONE;
    int hitX = apos->x, hitY = apos->y;
    for (int s = 1; s <= range; s++) {
        int tx = apos->x + dx * s;
        int ty = apos->y + dy * s;
        if (tx < 0 || tx >= game->map->width || ty < 0 || ty >= game->map->height) break;
        if (game->blocking[ty][tx]) break;
        EntityId mon = World_FindMonsterAt(game, tx, ty, ENTITY_NONE);
        if (mon != ENTITY_NONE) {
            target = mon;
            hitX = tx;
            hitY = ty;
            break;
        }
    }

    if (target == ENTITY_NONE) {
        CombatLog_Add(&game->combatLog, DARKGRAY, "Nothing in range.");
        return false;
    }

    CStats* ms = World_GetStats(&game->ecs, target);
    if (!ms->alive) return false;

    CName* name = World_HasComponents(&game->ecs, target, COMP_NAME)
        ? World_GetName(&game->ecs, target) : NULL;
    const char* monName = name ? name->name : "monster";

    int dodgePct = ms->dex * 2;
    if (dodgePct > 60) dodgePct = 60;
    if (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct) {
        CombatLog_Add(&game->combatLog, BLACK, "%s dodges your arrow!", monName);
        return true;
    }

    int damage = (int)(as->dex * 1.5f) - ms->defense;
    if (damage < 1) damage = 1;
    if (GetRandomValue(1, 100) <= as->lck) {
        damage *= 2;
        if (damage < 1) damage = 1;
        CombatLog_Add(&game->combatLog, BLACK, "Critical hit!");
    }

    ms->hp -= damage;
    GameAudio_PlayRangedAttackSound();
    if (World_HasComponents(&game->ecs, target, COMP_HIT_FLASH))
        World_GetHitFlash(&game->ecs, target)->timer = 0.15f;
    CombatLog_Add(&game->combatLog, BLACK, "You shoot %s for %d!", monName, damage);

    if (ms->hp <= 0) {
        ms->alive = false;
        ms->hp = 0;
        GainExperience(game, ms->expValue);
        CombatLog_Add(&game->combatLog, BLACK, "%s defeated! (+%d exp)", monName, ms->expValue);
    }

    int tw = game->map->tileWidth;
    int th = game->map->tileHeight;
    game->projectile.active = true;
    game->projectile.sx = apos->x * tw + tw / 2.0f;
    game->projectile.sy = apos->y * th + th / 2.0f;
    game->projectile.ex = hitX * tw + tw / 2.0f;
    game->projectile.ey = hitY * th + th / 2.0f;
    game->projectile.tileSX = apos->x;
    game->projectile.tileSY = apos->y;
    game->projectile.tileEX = hitX;
    game->projectile.tileEY = hitY;
    game->projectile.attackType = ATTACK_RANGED;
    game->projectile.color = (Color){ 139, 69, 19, 255 };
    game->projectile.startFrame = 0;
    game->projectile.startRow = 0;
    game->projectile.animFrameCount = 0;
    game->projectile.throwTex = NULL;
    game->projectile.throwRotation = 0.0f;
    game->projectileTimer = PROJECTILE_ANIM_DURATION;
    game->projectileDuration = PROJECTILE_ANIM_DURATION;

    return true;
}

bool CombatSystem_PlayerThrowWeapon(GameWorld* game, EntityId attackerId) {
    if (!game) return false;

    EquipType weapon = game->equipped[EQUIP_SLOT_WEAPON];
    const EquipData* wdata = GetEquipData(weapon);
    if (!wdata || wdata->isRanged || weapon == EQUIP_NONE) {
        CombatLog_Add(&game->combatLog, DARKGRAY, "Nothing to throw.");
        return false;
    }

    CStats* as = World_GetStats(&game->ecs, attackerId);
    CPosition* apos = World_GetPosition(&game->ecs, attackerId);

    char weaponName[32];
    strncpy(weaponName, wdata->name, sizeof(weaponName) - 1);
    weaponName[sizeof(weaponName) - 1] = '\0';

    int savedDex = as->dex;
    int savedAttack = as->attack;
    int savedLck = as->lck;

    Texture2D* throwTex = Resources_LoadTexture(wdata->spritePath);

    int throwRange = 3 + savedDex / 3;
    if (throwRange < 3) throwRange = 3;

    UnequipSlot(game, EQUIP_SLOT_WEAPON);
    int invSlot = -1;
    for (int i = 0; i < game->equipInventoryCount; i++) {
        if (game->equipInventory[i] == weapon) { invSlot = i; break; }
    }
    if (invSlot >= 0) RemoveEquipFromInventory(game, invSlot);

    int dx = 0, dy = 0;
    switch (apos->facingDir) {
        case DIR_UP:    dy = -1; break;
        case DIR_DOWN:  dy = 1;  break;
        case DIR_LEFT:  dx = -1; break;
        case DIR_RIGHT: dx = 1;  break;
        default: return true;
    }

    EntityId target = ENTITY_NONE;
    int hitX = apos->x, hitY = apos->y;
    for (int s = 1; s <= throwRange; s++) {
        int tx = apos->x + dx * s;
        int ty = apos->y + dy * s;
        if (tx < 0 || tx >= game->map->width || ty < 0 || ty >= game->map->height) break;
        if (game->blocking[ty][tx]) { hitX = tx; hitY = ty; break; }
        EntityId mon = World_FindMonsterAt(game, tx, ty, ENTITY_NONE);
        if (mon != ENTITY_NONE) {
            target = mon;
            hitX = tx;
            hitY = ty;
            break;
        }
        hitX = tx;
        hitY = ty;
    }

    int tw = game->map->tileWidth;
    int th = game->map->tileHeight;
    game->projectile.active = true;
    game->projectile.sx = apos->x * tw + tw / 2.0f;
    game->projectile.sy = apos->y * th + th / 2.0f;
    game->projectile.ex = hitX * tw + tw / 2.0f;
    game->projectile.ey = hitY * th + th / 2.0f;
    game->projectile.tileSX = apos->x;
    game->projectile.tileSY = apos->y;
    game->projectile.tileEX = hitX;
    game->projectile.tileEY = hitY;
    game->projectile.attackType = ATTACK_THROW;
    game->projectile.color = WHITE;
    game->projectile.startFrame = 0;
    game->projectile.startRow = 0;
    game->projectile.animFrameCount = 0;
    game->projectile.throwTex = throwTex;
    game->projectile.throwRotation = 0.0f;
    game->projectileTimer = PROJECTILE_ANIM_DURATION;
    game->projectileDuration = PROJECTILE_ANIM_DURATION;

    CombatLog_Add(&game->combatLog, BLACK, "You throw your %s!", weaponName);

    if (target != ENTITY_NONE) {
        CStats* ms = World_GetStats(&game->ecs, target);
        if (ms->alive) {
            CName* name = World_HasComponents(&game->ecs, target, COMP_NAME)
                ? World_GetName(&game->ecs, target) : NULL;
            const char* monName = name ? name->name : "monster";

            int dodgePct = ms->dex * 2;
            if (dodgePct > 60) dodgePct = 60;
            if (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct) {
                CombatLog_Add(&game->combatLog, BLACK, "%s dodges your %s!", monName, weaponName);
                return true;
            }

            int damage = (savedAttack + savedDex * 2) - ms->defense;
            if (damage < 1) damage = 1;
            if (GetRandomValue(1, 100) <= savedLck) {
                damage *= 2;
                if (damage < 1) damage = 1;
                CombatLog_Add(&game->combatLog, BLACK, "Critical hit!");
            }

            ms->hp -= damage;
            GameAudio_PlayRangedAttackSound();
            if (World_HasComponents(&game->ecs, target, COMP_HIT_FLASH))
                World_GetHitFlash(&game->ecs, target)->timer = 0.15f;
            CombatLog_Add(&game->combatLog, BLACK, "Your thrown %s hits %s for %d!", weaponName, monName, damage);

            if (ms->hp <= 0) {
                ms->alive = false;
                ms->hp = 0;
                GainExperience(game, ms->expValue);
                CombatLog_Add(&game->combatLog, BLACK, "%s defeated! (+%d exp)", monName, ms->expValue);
            }
        }
    }

    return true;
}
