#include "monster_info.h"
#include "core/game.h"
#include "entity/monster.h"
#include <stdio.h>

void MonsterInfo_Render(const Game* game) {
    if (game->selectedMonsterIdx < 0) return;

    Monster* arr = Monster_GetArray();
    int count = Monster_GetCount();
    if (game->selectedMonsterIdx >= count || !arr[game->selectedMonsterIdx].alive) return;

    Monster* m = &arr[game->selectedMonsterIdx];
    float scale = GetUIScale();
    int pX = GetScreenWidth() - (int)(200 * scale);
    int pY = (int)(10 * scale);
    int pW = (int)(190 * scale);
    int pH = (int)(160 * scale);
    int ly = pY + (int)(6 * scale);
    int fs = (int)(14 * scale);
    int lh = fs + (int)(4 * scale);
    char buf[64];

    if (game->texUiSlot.id > 0)
        Draw9Slice(game->texUiSlot, (Rectangle){ (float)pX, (float)pY, (float)pW, (float)pH }, 8, 8, 8, 8);
    else {
        DrawRectangle(pX, pY, pW, pH, (Color){ 0, 0, 0, 200 });
        DrawRectangleLines(pX, pY, pW, pH, (Color){ 80, 80, 80, 255 });
    }

    snprintf(buf, sizeof(buf), "%s", m->name);
    DrawText(buf, pX + (int)(6 * scale), ly, fs, YELLOW); ly += lh;

    snprintf(buf, sizeof(buf), "Lv %d", m->level);
    DrawText(buf, pX + (int)(6 * scale), ly, fs, BLACK); ly += lh;

    snprintf(buf, sizeof(buf), "HP %d/%d", m->hp, m->maxHp);
    DrawText(buf, pX + (int)(6 * scale), ly, fs, RED); ly += lh;

    snprintf(buf, sizeof(buf), "ATK %d", m->attack);
    DrawText(buf, pX + (int)(6 * scale), ly, fs, BLACK); ly += lh;

    snprintf(buf, sizeof(buf), "DEF %d", m->defense);
    DrawText(buf, pX + (int)(6 * scale), ly, fs, BLACK); ly += lh;

    snprintf(buf, sizeof(buf), "EXP %d", m->expValue);
    DrawText(buf, pX + (int)(6 * scale), ly, fs, (Color){ 0, 0, 160, 255 });
}
