#include "player.h"
#include "game.h"
#include "world.h"
#include "game_audio.h"
#include "ui/combat_log.h"
#include <stdio.h>

// Base experience needed: 20 + level * 10
int ExpForLevel(int level) {
    return 50 + level * 40;
}

// Apply stat bonuses for reaching a new level
static void ApplyLevelUp(GameWorld* game) {
    World* w = &game->ecs;
    EntityId pe = game->playerEntity;
    if (pe == ENTITY_NONE) return;
    CStats* ps = World_GetStats(w, pe);
    ps->level++;
    ps->statPoints += 2;
    ps->expToNext = ExpForLevel(ps->level);
    game->levelUpTimer = 3.0f;
    GameAudio_PlayLevelUpSound();
    TraceLog(LOG_INFO, "Level up! Now level %d (%d stat points available)", ps->level, ps->statPoints);
    CombatLog_Add(&game->combatLog, BLACK, "Level %d! +2 stat points to allocate!", ps->level);
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
    // Recalculate derived max HP from CON
    s->maxHp = 30 + s->con * 5;
    if (s->hp > s->maxHp) s->hp = s->maxHp;
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
