#include "combat_system.h"
#include "world_monster.h"
#include <stddef.h>
#include "core/audio.h"
#include "core/game.h"
#include "entity/player.h"
#include "ui/combat_log.h"

bool CombatSystem_PlayerMeleeAttack(GameWorld* gw, Game* game, Entity* attacker, int targetX, int targetY) {
    if (!gw || !game || !attacker) return false;

    EntityId mon = World_FindMonsterAt(gw, targetX, targetY, ENTITY_NONE);
    if (mon == ENTITY_NONE) return false;

    CStats* ms = World_GetStats(&gw->ecs, mon);
    if (!ms->alive) return false;

    CName* name = World_HasComponents(&gw->ecs, mon, COMP_NAME)
        ? World_GetName(&gw->ecs, mon) : NULL;
    const char* monName = name ? name->name : "monster";

    int dodgePct = ms->dex * 2;
    if (dodgePct > 60) dodgePct = 60;
    if (dodgePct > 0 && GetRandomValue(1, 100) <= dodgePct) {
        attacker->hitFlashTimer = 0.15f;
        CombatLog_Add(&game->combatLog, BLACK, "%s dodges your attack!", monName);
        return true;
    }

    int damage = attacker->attack + attacker->str * 2 - ms->defense;
    if (damage < 1) damage = 1;
    if (GetRandomValue(1, 100) <= attacker->lck) {
        damage *= 2;
        if (damage < 1) damage = 1;
        CombatLog_Add(&game->combatLog, BLACK, "Critical hit!");
    }

    ms->hp -= damage;
    PlayHitSound();
    attacker->hitFlashTimer = 0.15f;
    if (World_HasComponents(&gw->ecs, mon, COMP_HIT_FLASH)) {
        World_GetHitFlash(&gw->ecs, mon)->timer = 0.15f;
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
