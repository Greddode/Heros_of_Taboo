#include "player.h"
#include "core/game.h"
#include "core/audio.h"
#include "ui/combat_log.h"
#include <stdio.h>

// Base experience needed: 20 + level * 10
int ExpForLevel(int level) {
    return 20 + level * 10;
}

// Apply stat bonuses for reaching a new level
static void ApplyLevelUp(Game* game) {
    Player* p = &game->player;
    p->ent.level++;
    p->ent.statPoints += 3;
    p->expToNext = ExpForLevel(p->ent.level);
    game->levelUpTimer = 3.0f;
    PlayLevelUpSound();
    TraceLog(LOG_INFO, "Level up! Now level %d (%d stat points available)", p->ent.level, p->ent.statPoints);
    CombatLog_Add(&game->combatLog, BLACK, "Level %d! +3 stat points to allocate!", p->ent.level);
}

// Allocate one stat point to a specific stat
bool AllocateStatPoint(Entity* ent, int statIdx) {
    if (ent->statPoints <= 0) return false;
    switch (statIdx) {
        case 0: ent->str++;   break;
        case 1: ent->dex++;   break;
        case 2: ent->intel++; break;
        case 3: ent->con++;   break;
        case 4: ent->lck++;   break;
        default: return false;
    }
    ent->statPoints--;
    // Recalculate derived max HP from CON
    ent->maxHp = 30 + ent->con * 5;
    if (ent->hp > ent->maxHp) ent->hp = ent->maxHp;
    return true;
}

// Add experience and trigger level-ups as needed
void GainExperience(Game* game, int amount) {
    Player* p = &game->player;
    p->ent.expValue += amount;
    p->exp += amount;
    TraceLog(LOG_INFO, "Gained %d exp (total: %d, next: %d)", amount, p->exp, p->expToNext);
    while (p->exp >= p->expToNext) {
        p->exp -= p->expToNext;
        ApplyLevelUp(game);
    }
}
