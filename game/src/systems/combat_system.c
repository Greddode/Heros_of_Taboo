#include "combat_system.h"
#include "world_monster.h"
#include <stddef.h>
#include "audio.h"
#include "game.h"
#include "systems/player.h"
#include "ui/combat_log.h"

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
    PlayHitSound();
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
