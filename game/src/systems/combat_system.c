#include "combat_system.h"
#include "debug_log.h"
#include "validation.h"
#include "event_bus.h"
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
    if (attackerId == ENTITY_NONE) { TraceLog(LOG_WARNING, "CombatSystem_PlayerMeleeAttack: attackerId is ENTITY_NONE"); return false; }
    if (!World_HasComponents(&game->ecs, attackerId, COMP_STATS)) { TraceLog(LOG_WARNING, "CombatSystem_PlayerMeleeAttack: attacker missing COMP_STATS [e=%d]", (int)attackerId); return false; }
    if (!Validate_TilePos(game, targetX, targetY)) { TraceLog(LOG_WARNING, "CombatSystem_PlayerMeleeAttack: target out of bounds [%d,%d]", targetX, targetY); return false; }

    EntityId mon = World_FindMonsterAt(game, targetX, targetY, ENTITY_NONE);
    if (mon == ENTITY_NONE) { TraceLog(LOG_WARNING, "CombatSystem_PlayerMeleeAttack: no monster at target tile [%d,%d] despite attempt", targetX, targetY); return false; }

    CStats* as = World_GetStats(&game->ecs, attackerId);
    CStats* ms = World_GetStats(&game->ecs, mon);
    if (!ms->alive) return false;

    CName* name = World_HasComponents(&game->ecs, mon, COMP_NAME)
        ? World_GetName(&game->ecs, mon) : NULL;
    const char* monName = name ? name->name : "monster";

    int rawMelee = calc_melee_damage(as->attack, as->str);
    int dodgePct = calc_dodge_chance(ms->dex);
    bool dodged = (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct);
    DebugLog(DEBUG_COMBAT, "Melee: attacker=%d target=%s dodge=%d/%s", (int)attackerId, monName, dodgePct, dodged ? "DODGED" : "hit");
    if (dodged) {
        {
            CPosition* mp = World_GetPosition(&game->ecs, mon);
            if (mp) FloatMsg_Spawn(game, mp->x, mp->y, SKYBLUE, "Dodge!");
        }
        if (World_HasComponents(&game->ecs, attackerId, COMP_HIT_FLASH))
            World_GetHitFlash(&game->ecs, attackerId)->timer = HIT_FLASH_DURATION;
        return true;
    }

    int afterDef = calc_damage_after_defense(rawMelee, ms->defense);
    int damage = afterDef;
    Color dmgColor = WHITE;
    bool critted = (GetRandomValue(1, 100) <= as->lck);
    if (critted) {
        damage *= CRIT_MULT;
        dmgColor = ORANGE;
        {
            CPosition* ap = World_GetPosition(&game->ecs, attackerId);
            FloatMsg_Spawn(game, ap->x, ap->y, ORANGE, "Critical!");
        }
    }
    bool megaCritted = (damage > MEGA_CRIT_THRESHOLD && GetRandomValue(1, 100) <= MEGA_CRIT_CHANCE);
    if (megaCritted) {
        damage *= 2;
        dmgColor = RED;
    }
    DebugLog(DEBUG_COMBAT, "Melee: raw=%d afterDef=%d crit=%s mega=%s final=%d targetHP=%d/%d",
             rawMelee, afterDef, critted ? "yes" : "no", megaCritted ? "yes" : "no",
             damage, ms->hp, ms->maxHp);

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
        bool offDodged = (offDodgePct > 0 && GetRandomValue(1, 100) <= offDodgePct);
        DebugLog(DEBUG_COMBAT, "Offhand: target=%s dodge=%d/%s", monName, offDodgePct, offDodged ? "DODGED" : "hit");
        if (offDodged) {
            {
                CPosition* mp = World_GetPosition(&game->ecs, mon);
                if (mp) FloatMsg_Spawn(game, mp->x, mp->y, SKYBLUE, "Dodge!");
            }
        } else {
            int offRaw = calc_melee_damage(as->attack, as->str);
            int offAfterDef = calc_damage_after_defense(offRaw, ms->defense);
            int offDmg = (int)((float)offAfterDef * DUAL_WIELD_OFFHAND_MULT);
            if (offDmg < 1) offDmg = 1;
            Color offColor = WHITE;
            bool offCrit = (GetRandomValue(1, 100) <= as->lck);
            if (offCrit) {
                offDmg *= CRIT_MULT;
                offColor = ORANGE;
                {
                    CPosition* ap = World_GetPosition(&game->ecs, attackerId);
                    FloatMsg_Spawn(game, ap->x, ap->y, ORANGE, "Off-hand!");
                }
            }
            bool offMega = (offDmg > MEGA_CRIT_THRESHOLD && GetRandomValue(1, 100) <= MEGA_CRIT_CHANCE);
            if (offMega) {
                offDmg *= 2;
                offColor = RED;
            }
            DebugLog(DEBUG_COMBAT, "Offhand: raw=%d afterDef=%d mult=%.2f crit=%s mega=%s final=%d",
                     offRaw, offAfterDef, DUAL_WIELD_OFFHAND_MULT,
                     offCrit ? "yes" : "no", offMega ? "yes" : "no", offDmg);
            ms->hp -= offDmg;
            {
                int tw = game->map->tileWidth;
                int th = game->map->tileHeight;
                DamageNumber_Spawn(&game->damageNumbers, offDmg, targetX, targetY, tw, th, offColor);
            }
            if (ms->hp <= 0) {
                DebugLog(DEBUG_COMBAT, "Kill: %s XP=%d (off-hand)", monName, ms->expValue);
                GainExperience(game, ms->expValue);
                DropMonsterEquipment(game, mon);
                {
                    CPosition* dp = World_GetPosition(&game->ecs, mon);
                    CAI* oai = World_HasComponents(&game->ecs, mon, COMP_AI) ? World_GetAI(&game->ecs, mon) : NULL;
                    MonsterKilledEvent evt = { mon, oai ? oai->type : 0, dp->x, dp->y, ms->expValue, 0, oai ? oai->equippedWeapon : 0 };
                    SpatialHash_Remove(game, mon, dp->x, dp->y);
                    World_DestroyEntity(&game->ecs, mon);
                    EventBus_Publish(EVT_MONSTER_KILLED, &evt);
                }
                game->aliveMonsterCount--;
                ms = NULL;
                return true;
            }
        }
    }

    if (ms && ms->hp <= 0) {
        DebugLog(DEBUG_COMBAT, "Kill: %s XP=%d", monName, ms->expValue);
        GainExperience(game, ms->expValue);
        DropMonsterEquipment(game, mon);
        {
            CPosition* dp = World_GetPosition(&game->ecs, mon);
            CAI* mai = World_HasComponents(&game->ecs, mon, COMP_AI) ? World_GetAI(&game->ecs, mon) : NULL;
            MonsterKilledEvent evt = { mon, mai ? mai->type : 0, dp->x, dp->y, ms->expValue, 0, mai ? mai->equippedWeapon : 0 };
            SpatialHash_Remove(game, mon, dp->x, dp->y);
            World_DestroyEntity(&game->ecs, mon);
            EventBus_Publish(EVT_MONSTER_KILLED, &evt);
        }
        game->aliveMonsterCount--;
    }
    return true;
}

bool CombatSystem_PlayerRangedAttack(GameWorld* game, EntityId attackerId) {
    if (!game) return false;
    if (attackerId == ENTITY_NONE) { TraceLog(LOG_WARNING, "CombatSystem_PlayerRangedAttack: attackerId is ENTITY_NONE"); return false; }

    EquipType weapon = game->equipped[EQUIP_SLOT_WEAPON];
    const EquipData* wdata = GetEquipData(weapon);
    if (!wdata || !wdata->isRanged) {
        TraceLog(LOG_WARNING, "CombatSystem_PlayerRangedAttack: no ranged weapon equipped [weapon=%d]", (int)weapon);
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
    game->projectile.projectileVisual = PROJ_ARROW;
    game->projectile.color = (Color){ 139, 69, 19, 255 };
    game->projectile.startFrame = 0;
    game->projectile.startRow = 0;
    game->projectile.animFrameCount = 0;
    game->projectile.throwTex = NULL;
    game->projectile.throwRotation = 0.0f;
    game->projectileTimer = PROJECTILE_ANIM_DURATION;
    game->projectileDuration = PROJECTILE_ANIM_DURATION;

    int rawRanged = calc_ranged_damage(as->dex);
    int dodgePct = calc_dodge_chance(ms->dex);
    bool rDodged = (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct);
    DebugLog(DEBUG_COMBAT, "Ranged: attacker=%d target=%s dodge=%d/%s", (int)attackerId, monName, dodgePct, rDodged ? "DODGED" : "hit");
    if (rDodged) {
        {
            CPosition* mp = World_GetPosition(&game->ecs, target);
            if (mp) FloatMsg_Spawn(game, mp->x, mp->y, SKYBLUE, "Dodge!");
        }
        return true;
    }

    int afterDef = calc_damage_after_defense(rawRanged, ms->defense);
    int damage = afterDef;
    Color dmgColor = WHITE;
    bool rCrit = (GetRandomValue(1, 100) <= as->lck);
    if (rCrit) {
        damage *= CRIT_MULT;
        dmgColor = ORANGE;
        {
            CPosition* ap = World_GetPosition(&game->ecs, attackerId);
            FloatMsg_Spawn(game, ap->x, ap->y, ORANGE, "Critical!");
        }
    }
    bool rMega = (damage > MEGA_CRIT_THRESHOLD && GetRandomValue(1, 100) <= MEGA_CRIT_CHANCE);
    if (rMega) {
        damage *= 2;
        dmgColor = RED;
    }
    DebugLog(DEBUG_COMBAT, "Ranged: raw=%d afterDef=%d crit=%s mega=%s final=%d targetHP=%d/%d",
             rawRanged, afterDef, rCrit ? "yes" : "no", rMega ? "yes" : "no",
             damage, ms->hp, ms->maxHp);

    {
        DamageNumber_Spawn(&game->damageNumbers, damage, hitX, hitY, tw, th, dmgColor);
    }

    ms->hp -= damage;
    GameAudio_PlayRangedAttackSound();
    if (World_HasComponents(&game->ecs, target, COMP_HIT_FLASH))
        World_GetHitFlash(&game->ecs, target)->timer = HIT_FLASH_DURATION;

    if (ms->hp <= 0) {
        DebugLog(DEBUG_COMBAT, "Kill: %s XP=%d (ranged)", monName, ms->expValue);
        GainExperience(game, ms->expValue);
        DropMonsterEquipment(game, target);
        {
            CPosition* dp = World_GetPosition(&game->ecs, target);
            CAI* rtai = World_HasComponents(&game->ecs, target, COMP_AI) ? World_GetAI(&game->ecs, target) : NULL;
            MonsterKilledEvent evt = { target, rtai ? rtai->type : 0, dp->x, dp->y, ms->expValue, 0, rtai ? rtai->equippedWeapon : 0 };
            SpatialHash_Remove(game, target, dp->x, dp->y);
            World_DestroyEntity(&game->ecs, target);
            EventBus_Publish(EVT_MONSTER_KILLED, &evt);
        }
        game->aliveMonsterCount--;
    }

    return true;
}

bool CombatSystem_PlayerThrowWeapon(GameWorld* game, EntityId attackerId) {
    if (!game) return false;
    if (attackerId == ENTITY_NONE) { TraceLog(LOG_WARNING, "CombatSystem_PlayerThrowWeapon: attackerId is ENTITY_NONE"); return false; }

    EquipType weapon = game->equipped[EQUIP_SLOT_WEAPON];
    const EquipData* wdata = GetEquipData(weapon);
    if (!wdata || wdata->isRanged || weapon == EQUIP_NONE) {
        TraceLog(LOG_WARNING, "CombatSystem_PlayerThrowWeapon: no throwable weapon equipped [weapon=%d]", (int)weapon);
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
    game->projectile.projectileVisual = PROJ_ARROW;
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

            int rawThrow = calc_throw_damage(savedAttack, savedDex);
            int dodgePct = calc_dodge_chance(ms->dex);
            bool tDodged = (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct);
            DebugLog(DEBUG_COMBAT, "Throw: attacker=%d target=%s dodge=%d/%s weapon=%s",
                     (int)attackerId, monName, dodgePct, tDodged ? "DODGED" : "hit", weaponName);
            if (tDodged) {
                {
                    CPosition* mp = World_GetPosition(&game->ecs, target);
                    if (mp) FloatMsg_Spawn(game, mp->x, mp->y, SKYBLUE, "Dodge!");
                }
                return true;
            }

            int afterDef = calc_damage_after_defense(rawThrow, ms->defense);
            int damage = afterDef;
            Color dmgColor = WHITE;
            bool tCrit = (GetRandomValue(1, 100) <= savedLck);
            if (tCrit) {
                damage *= CRIT_MULT;
                if (damage < MIN_DAMAGE) damage = MIN_DAMAGE;
                dmgColor = ORANGE;
                {
                    CPosition* ap = World_GetPosition(&game->ecs, attackerId);
                    FloatMsg_Spawn(game, ap->x, ap->y, ORANGE, "Critical!");
                }
            }
            bool tMega = (damage > MEGA_CRIT_THRESHOLD && GetRandomValue(1, 100) <= MEGA_CRIT_CHANCE);
            if (tMega) {
                damage *= 2;
                dmgColor = RED;
            }
            DebugLog(DEBUG_COMBAT, "Throw: raw=%d afterDef=%d crit=%s mega=%s final=%d targetHP=%d/%d",
                     rawThrow, afterDef, tCrit ? "yes" : "no", tMega ? "yes" : "no",
                     damage, ms->hp, ms->maxHp);

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
                DebugLog(DEBUG_COMBAT, "Kill: %s XP=%d (throw)", monName, ms->expValue);
                GainExperience(game, ms->expValue);
                DropMonsterEquipment(game, target);
                {
                    CPosition* dp = World_GetPosition(&game->ecs, target);
                    CAI* thai = World_HasComponents(&game->ecs, target, COMP_AI) ? World_GetAI(&game->ecs, target) : NULL;
                    MonsterKilledEvent evt = { target, thai ? thai->type : 0, dp->x, dp->y, ms->expValue, 0, thai ? thai->equippedWeapon : 0 };
                    SpatialHash_Remove(game, target, dp->x, dp->y);
                    World_DestroyEntity(&game->ecs, target);
                    EventBus_Publish(EVT_MONSTER_KILLED, &evt);
                }
                game->aliveMonsterCount--;
            }
        }
    }

    return true;
}
