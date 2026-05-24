#include "player.h"
#include "game.h"
#include "combat_log.h"
#include <stdio.h>

// Base experience needed: 20 + level * 10
int ExpForLevel(int level) {
    return 20 + level * 10;
}

// Apply stat bonuses for reaching a new level
static void ApplyLevelUp(Game* game) {
    Player* p = &game->player;
    p->ent.level++;
    p->ent.maxHp += 5;
    p->ent.hp += 5;
    if (p->ent.hp > p->ent.maxHp)
        p->ent.hp = p->ent.maxHp;
    if (p->ent.level % 2 == 0)
        p->ent.attack++;
    if (p->ent.level % 3 == 0)
        p->ent.defense++;
    p->expToNext = ExpForLevel(p->ent.level);
    TraceLog(LOG_INFO, "Level up! Now level %d", p->ent.level);
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
        CombatLog_Add(&game->combatLog, "Level %d! HP+5 ATK+%d DEF+%d",
                      p->ent.level,
                      (p->ent.level % 2 == 0) ? 1 : 0,
                      (p->ent.level % 3 == 0) ? 1 : 0);
    }
}
