#include "combat_system.h"
#include "world.h"
#include "world_monster.h"
#include "spatial_hash.h"
#include "inventory.h"
#include <stddef.h>
#include <math.h>
#include "game_audio.h"
#include "systems/player.h"
#include "resources.h"
#include "game_balance.h"
#include "systems/spawner_system.h"
#include "data/monster_data.h"
#include <string.h>

static void DropMonsterEquipment(GameWorld* game, EntityId mon) {
    CAI* mai = World_HasComponents(&game->ecs, mon, COMP_AI) ? World_GetAI(&game->ecs, mon) : NULL;
    if (!mai) return;
    const MonsterTemplate* mt = Monster_GetTemplate(mai->type);
    if (!mt || mt->equipDropChance <= 0) return;
    CPosition* dp = World_GetPosition(&game->ecs, mon);
    if (mai->equippedWeapon != EQUIP_NONE && GetRandomValue(1, 100) <= mt->equipDropChance)
        SpawnerSystem_AddEquipAt(game, dp->x, dp->y, mai->equippedWeapon, 1);
    if (mai->equippedArmor != EQUIP_NONE && GetRandomValue(1, 100) <= mt->equipDropChance)
        SpawnerSystem_AddEquipAt(game, dp->x, dp->y, mai->equippedArmor, 1);
}

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

    int dodgePct = calc_dodge_chance(ms->dex);
    if (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct) {
        if (World_HasComponents(&game->ecs, attackerId, COMP_HIT_FLASH))
            World_GetHitFlash(&game->ecs, attackerId)->timer = HIT_FLASH_DURATION;
        return true;
    }

    int damage = calc_damage_after_defense(calc_melee_damage(as->attack, as->str), ms->defense);
    Color dmgColor = WHITE;
    if (GetRandomValue(1, 100) <= as->lck) {
        damage *= CRIT_MULT;
        dmgColor = ORANGE;
        {
            CPosition* ap = World_GetPosition(&game->ecs, attackerId);
            FloatMsg_Spawn(game, ap->x, ap->y, ORANGE, "Critical!");
        }
    }
    if (damage > MEGA_CRIT_THRESHOLD && GetRandomValue(1, 100) <= MEGA_CRIT_CHANCE) {
        damage *= 2;
        dmgColor = RED;
    }

    {
        int tw = game->map->tileWidth;
        int th = game->map->tileHeight;
        DamageNumber_Spawn(&game->damageNumbers, damage, targetX, targetY, tw, th, dmgColor);
    }

    ms->hp -= damage;
    GameAudio_PlayHitSound();
    if (World_HasComponents(&game->ecs, attackerId, COMP_HIT_FLASH))
        World_GetHitFlash(&game->ecs, attackerId)->timer = HIT_FLASH_DURATION;
    if (World_HasComponents(&game->ecs, mon, COMP_HIT_FLASH)) {
        World_GetHitFlash(&game->ecs, mon)->timer = HIT_FLASH_DURATION;
    }

    // Off-hand follow-up strike (dual wield)
    if (ms->hp > 0 && IsDualWielding(game)) {
        int offDodgePct = calc_dodge_chance(ms->dex);
        if (offDodgePct > 0 && GetRandomValue(1, 100) <= offDodgePct) {
        } else {
            int offRaw = calc_melee_damage(as->attack, as->str);
            int offDmg = calc_damage_after_defense(offRaw, ms->defense);
            offDmg = (int)((float)offDmg * DUAL_WIELD_OFFHAND_MULT);
            if (offDmg < 1) offDmg = 1;
            Color offColor = WHITE;
            if (GetRandomValue(1, 100) <= as->lck) {
                offDmg *= CRIT_MULT;
                offColor = ORANGE;
                {
                    CPosition* ap = World_GetPosition(&game->ecs, attackerId);
                    FloatMsg_Spawn(game, ap->x, ap->y, ORANGE, "Off-hand!");
                }
            }
            if (offDmg > MEGA_CRIT_THRESHOLD && GetRandomValue(1, 100) <= MEGA_CRIT_CHANCE) {
                offDmg *= 2;
                offColor = RED;
            }
            ms->hp -= offDmg;
            {
                int tw = game->map->tileWidth;
                int th = game->map->tileHeight;
                DamageNumber_Spawn(&game->damageNumbers, offDmg, targetX, targetY, tw, th, offColor);
            }
            if (ms->hp <= 0) {
                ms->alive = false;
                ms->hp = 0;
                {
                    CPosition* dp = World_GetPosition(&game->ecs, mon);
                    SpatialHash_Remove(game, mon, dp->x, dp->y);
                }
                DropMonsterEquipment(game, mon);
                game->aliveMonsterCount--;
                GainExperience(game, ms->expValue);
            }
        }
    }

    if (ms->hp <= 0) {
        ms->alive = false;
        ms->hp = 0;
        {
            CPosition* dp = World_GetPosition(&game->ecs, mon);
            SpatialHash_Remove(game, mon, dp->x, dp->y);
        }
        DropMonsterEquipment(game, mon);
        game->aliveMonsterCount--;
        GainExperience(game, ms->expValue);
    }
    return true;
}

bool CombatSystem_PlayerRangedAttack(GameWorld* game, EntityId attackerId) {
    if (!game) return false;

    EquipType weapon = game->equipped[EQUIP_SLOT_WEAPON];
    const EquipData* wdata = GetEquipData(weapon);
    if (!wdata || !wdata->isRanged) {
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
        return false;
    }

    CStats* ms = World_GetStats(&game->ecs, target);
    if (!ms->alive) return false;

    CName* name = World_HasComponents(&game->ecs, target, COMP_NAME)
        ? World_GetName(&game->ecs, target) : NULL;
    const char* monName = name ? name->name : "monster";

    int dodgePct = calc_dodge_chance(ms->dex);
    if (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct) {
        return true;
    }

    int damage = calc_damage_after_defense(calc_ranged_damage(as->dex), ms->defense);
    Color dmgColor = WHITE;
    if (GetRandomValue(1, 100) <= as->lck) {
        damage *= CRIT_MULT;
        dmgColor = ORANGE;
        {
            CPosition* ap = World_GetPosition(&game->ecs, attackerId);
            FloatMsg_Spawn(game, ap->x, ap->y, ORANGE, "Critical!");
        }
    }
    if (damage > MEGA_CRIT_THRESHOLD && GetRandomValue(1, 100) <= MEGA_CRIT_CHANCE) {
        damage *= 2;
        dmgColor = RED;
    }

    {
        int tw = game->map->tileWidth;
        int th = game->map->tileHeight;
        DamageNumber_Spawn(&game->damageNumbers, damage, hitX, hitY, tw, th, dmgColor);
    }

    ms->hp -= damage;
    GameAudio_PlayRangedAttackSound();
    if (World_HasComponents(&game->ecs, target, COMP_HIT_FLASH))
        World_GetHitFlash(&game->ecs, target)->timer = HIT_FLASH_DURATION;

    if (ms->hp <= 0) {
        ms->alive = false;
        ms->hp = 0;
        {
            CPosition* dp = World_GetPosition(&game->ecs, target);
            SpatialHash_Remove(game, target, dp->x, dp->y);
        }
        DropMonsterEquipment(game, target);
        game->aliveMonsterCount--;
        GainExperience(game, ms->expValue);
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

    int throwRange = calc_throw_range(savedDex);

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


    if (target != ENTITY_NONE) {
        CStats* ms = World_GetStats(&game->ecs, target);
        if (ms->alive) {
            CName* name = World_HasComponents(&game->ecs, target, COMP_NAME)
                ? World_GetName(&game->ecs, target) : NULL;
            const char* monName = name ? name->name : "monster";

            int dodgePct = calc_dodge_chance(ms->dex);
            if (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct) {
                return true;
            }

            int damage = calc_damage_after_defense(calc_throw_damage(savedAttack, savedDex), ms->defense);
            Color dmgColor = WHITE;
            if (GetRandomValue(1, 100) <= savedLck) {
                damage *= CRIT_MULT;
                if (damage < MIN_DAMAGE) damage = MIN_DAMAGE;
                dmgColor = ORANGE;
                {
                    CPosition* ap = World_GetPosition(&game->ecs, attackerId);
                    FloatMsg_Spawn(game, ap->x, ap->y, ORANGE, "Critical!");
                }
            }
            if (damage > MEGA_CRIT_THRESHOLD && GetRandomValue(1, 100) <= MEGA_CRIT_CHANCE) {
                damage *= 2;
                dmgColor = RED;
            }

            {
                int tw = game->map->tileWidth;
                int th = game->map->tileHeight;
                DamageNumber_Spawn(&game->damageNumbers, damage, hitX, hitY, tw, th, dmgColor);
            }

            ms->hp -= damage;
            GameAudio_PlayRangedAttackSound();
            if (World_HasComponents(&game->ecs, target, COMP_HIT_FLASH))
                World_GetHitFlash(&game->ecs, target)->timer = HIT_FLASH_DURATION;

            if (ms->hp <= 0) {
                ms->alive = false;
                ms->hp = 0;
                {
                    CPosition* dp = World_GetPosition(&game->ecs, target);
                    SpatialHash_Remove(game, target, dp->x, dp->y);
                }
                DropMonsterEquipment(game, target);
                game->aliveMonsterCount--;
                GainExperience(game, ms->expValue);
            }
        }
    }

    return true;
}
