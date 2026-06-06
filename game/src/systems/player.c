#include "player.h"
#include "debug_log.h"
#include "game.h"
#include "world.h"
#include "game_audio.h"
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
    int oldLevel = ps->level;
    ps->level++;
    ps->statPoints += STAT_POINTS_PER_LEVEL;
    ps->expToNext = ExpForLevel(ps->level);
    game->levelUpTimer = LEVEL_UP_FLASH_SECONDS;
    GameAudio_PlayLevelUpSound();
    DebugLog(DEBUG_PLAYER, "LevelUp: %d -> %d statPoints=%d", oldLevel, ps->level, ps->statPoints);
    TraceLog(LOG_INFO, "Level up! Now level %d (%d stat points available)", ps->level, ps->statPoints);
    {
        CPosition* pp = World_GetPosition(w, pe);
        FloatMsg_Spawn(game, pp->x, pp->y, GOLD, "Level %d!", ps->level);
    }
}

// Allocate one stat point to a specific stat
bool AllocateStatPoint(GameWorld* game, CStats* s, int statIdx) {
    if (s->statPoints <= 0) return false;
    int cap = game && game->statCapsRemoved ? STAT_CAP_UNLIMITED : STAT_CAP_DEFAULT;
    int oldVal = 0, newVal = 0;
    const char* snames[5] = {"STR", "DEX", "INT", "CON", "LCK"};
    switch (statIdx) {
        case 0: oldVal = s->str;   if (s->str < cap)   s->str++;   else return false; newVal = s->str;   break;
        case 1: oldVal = s->dex;   if (s->dex < cap)   s->dex++;   else return false; newVal = s->dex;   break;
        case 2: oldVal = s->intel; if (s->intel < cap) s->intel++; else return false; newVal = s->intel; break;
        case 3: oldVal = s->con;   if (s->con < cap)   s->con++;   else return false; newVal = s->con;   break;
        case 4: oldVal = s->lck;   if (s->lck < cap)   s->lck++;   else return false; newVal = s->lck;   break;
        default: return false;
    }
    s->statPoints--;
    int oldMax = s->maxHp;
    s->maxHp = calc_max_hp(s->con);
    if (s->maxHp > oldMax && oldMax > 0)
        s->hp += (s->maxHp - oldMax);
    if (s->hp > s->maxHp) s->hp = s->maxHp;
    if (s->hp < 1)        s->hp = 1;
    DebugLog(DEBUG_PLAYER, "AllocStat: %s %d -> %d (points left=%d)",
             (statIdx >= 0 && statIdx < 5) ? snames[statIdx] : "?", oldVal, newVal, s->statPoints);
    return true;
}

// Add experience and trigger level-ups as needed
void GainExperience(GameWorld* game, int amount) {
    World* w = &game->ecs;
    EntityId pe = game->playerEntity;
    if (pe == ENTITY_NONE) return;
    CStats* ps = World_GetStats(w, pe);
    ps->exp += amount;
    DebugLog(DEBUG_PLAYER, "GainXP: +%d total=%d threshold=%d", amount, ps->exp, ps->expToNext);
    TraceLog(LOG_INFO, "Gained %d exp (total: %d, next: %d)", amount, ps->exp, ps->expToNext);
    while (ps->exp >= ps->expToNext) {
        ps->exp -= ps->expToNext;
        ApplyLevelUp(game);
    }
}
