#include "monster_info.h"
#include "entity/monster.h"
#include <stdio.h>

void MonsterInfo_Render(const Game* game) {
    if (game->selectedMonsterIdx < 0) return;

    Monster* arr = Monster_GetArray();
    int count = Monster_GetCount();
    if (game->selectedMonsterIdx >= count || !arr[game->selectedMonsterIdx].alive) return;

    Monster* m = &arr[game->selectedMonsterIdx];
    int pX = GetScreenWidth() - 200;
    int pY = 10;
    int pW = 190;
    int pH = 160;
    int ly = pY + 6;
    int fs = 14;
    int lh = fs + 4;
    char buf[64];

    DrawRectangle(pX, pY, pW, pH, (Color){ 0, 0, 0, 200 });
    DrawRectangleLines(pX, pY, pW, pH, (Color){ 80, 80, 80, 255 });

    snprintf(buf, sizeof(buf), "%s", m->name);
    DrawText(buf, pX + 6, ly, fs, YELLOW); ly += lh;

    snprintf(buf, sizeof(buf), "Lv %d", m->level);
    DrawText(buf, pX + 6, ly, fs, LIGHTGRAY); ly += lh;

    snprintf(buf, sizeof(buf), "HP %d/%d", m->hp, m->maxHp);
    DrawText(buf, pX + 6, ly, fs, RED); ly += lh;

    snprintf(buf, sizeof(buf), "ATK %d", m->attack);
    DrawText(buf, pX + 6, ly, fs, LIGHTGRAY); ly += lh;

    snprintf(buf, sizeof(buf), "DEF %d", m->defense);
    DrawText(buf, pX + 6, ly, fs, LIGHTGRAY); ly += lh;

    snprintf(buf, sizeof(buf), "EXP %d", m->expValue);
    DrawText(buf, pX + 6, ly, fs, (Color){ 80, 80, 255, 255 });
}
