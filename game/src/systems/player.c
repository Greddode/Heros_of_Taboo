#include "player.h"
#include "game.h"
#include "world.h"
#include "game_audio.h"
#include "ui/combat_log.h"
#include "game_balance.h"
#include <stdio.h>

int ExpForLevel(int level) {
    return calc_xp_for_level(level);
}

// Apply stat bonuses for reaching a new level
static void ApplyLevelUp(GameWorld* game) {
    World* w = &game->ecs;
    EntityId pe = game->playerEntity;
    if (pe == ENTITY_NONE) return;
    CStats* ps = World_GetStats(w, pe);
    ps->level++;
    ps->statPoints += STAT_POINTS_PER_LEVEL;
    ps->expToNext = ExpForLevel(ps->level);
    game->levelUpTimer = LEVEL_UP_FLASH_SECONDS;
    GameAudio_PlayLevelUpSound();
    TraceLog(LOG_INFO, "Level up! Now level %d (%d stat points available)", ps->level, ps->statPoints);
    {
        CPosition* pp = World_GetPosition(w, pe);
        FloatMsg_Spawn(game, pp->x, pp->y, GOLD, "Level %d!", ps->level);
    }
}

// Allocate one stat point to a specific stat
bool AllocateStatPoint(CStats* s, int statIdx) {
    if (s->statPoints <= 0) return false;
    switch (statIdx) {
        case 0: s->str++;   break;
        case 1: s->dex++;   break;
        case 2: s->intel++; break;
        case 3: s->con++;   break;
        case 4: s->lck++;   break;
        default: return false;
    }
    s->statPoints--;
    int oldMax = s->maxHp;
    s->maxHp = calc_max_hp(s->con);
    if (s->maxHp > oldMax && oldMax > 0)
        s->hp += (s->maxHp - oldMax);
    if (s->hp > s->maxHp) s->hp = s->maxHp;
    if (s->hp < 1)        s->hp = 1;
    return true;
}

// Add experience and trigger level-ups as needed
void GainExperience(GameWorld* game, int amount) {
    World* w = &game->ecs;
    EntityId pe = game->playerEntity;
    if (pe == ENTITY_NONE) return;
    CStats* ps = World_GetStats(w, pe);
    ps->exp += amount;
    TraceLog(LOG_INFO, "Gained %d exp (total: %d, next: %d)", amount, ps->exp, ps->expToNext);
    while (ps->exp >= ps->expToNext) {
        ps->exp -= ps->expToNext;
        ApplyLevelUp(game);
    }
}
