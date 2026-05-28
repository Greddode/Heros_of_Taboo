#include "inspector.h"
#include "entity/monster.h"
#include "core/inventory.h"
#include <stdio.h>
#include <string.h>

static int DrawWrappedLine(const char* text, int x, int y, int maxWidth, int fontSize, Color color) {
    char lineBuf[256] = "";
    const char* word = text;
    while (*word) {
        while (*word == ' ') word++;
        if (!*word) break;
        const char* wordEnd = word;
        while (*wordEnd && *wordEnd != ' ') wordEnd++;
        int wordLen = (int)(wordEnd - word);
        int curLen = (int)strlen(lineBuf);
        int sep = (curLen > 0) ? 1 : 0;
        char testBuf[256];
        memcpy(testBuf, lineBuf, curLen);
        if (sep) testBuf[curLen] = ' ';
        memcpy(testBuf + curLen + sep, word, wordLen);
        testBuf[curLen + sep + wordLen] = '\0';
        if (MeasureText(testBuf, fontSize) > maxWidth && curLen > 0) {
            DrawText(lineBuf, x, y, fontSize, color);
            y += fontSize + 4;
            memcpy(lineBuf, word, wordLen);
            lineBuf[wordLen] = '\0';
        } else {
            if (sep) lineBuf[curLen] = ' ';
            memcpy(lineBuf + curLen + sep, word, wordLen);
            lineBuf[curLen + sep + wordLen] = '\0';
        }
        word = wordEnd;
    }
    if (lineBuf[0]) {
        DrawText(lineBuf, x, y, fontSize, color);
        y += fontSize + 4;
    }
    return y;
}

static int DrawDesc(const char* desc, int x, int y, int maxWidth, int fontSize, Color color) {
    char buf[512];
    strncpy(buf, desc, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    int startY = y;
    char* para = buf;
    char* nl;
    while ((nl = strchr(para, '\n')) != NULL) {
        *nl = '\0';
        y = DrawWrappedLine(para, x, y, maxWidth, fontSize, color);
        para = nl + 1;
    }
    if (*para)
        y = DrawWrappedLine(para, x, y, maxWidth, fontSize, color);
    return y - startY;
}

void Inspector_Render(const Game* game, InspectorType type, int x, int y, int w, int h) {
    Monster* monsterArr = NULL;
    int monsterCount = 0;

    // Early-out: don't draw anything if there's nothing to inspect
    if (type == INSPECTOR_MONSTER) {
        if (game->selectedMonsterIdx < 0) return;
        monsterArr = Monster_GetArray();
        monsterCount = Monster_GetCount();
        if (game->selectedMonsterIdx >= monsterCount || !monsterArr[game->selectedMonsterIdx].alive) return;
    } else if (type == INSPECTOR_ITEM) {
        if (game->state != STATE_INVENTORY) return;
    }

    float scale = GetUIScale();
    int fs = (int)(14 * scale);
    int lh = fs + 4;
    int ly = y + (int)(8 * scale);
    int lx = x + (int)(10 * scale);
    int cx = x + w / 2;
    char buf[128];

    if (game->texUiSlot.id > 0)
        Draw9Slice(game->texUiSlot, (Rectangle){ (float)x, (float)y, (float)w, (float)h }, 8, 8, 8, 8);
    else {
        DrawRectangle(x, y, w, h, (Color){ 0, 0, 0, 200 });
        DrawRectangleLines(x, y, w, h, LIGHTGRAY);
    }

    if (type == INSPECTOR_MONSTER) {
        Monster* m = &monsterArr[game->selectedMonsterIdx];

        snprintf(buf, sizeof(buf), "%s", m->name);
        DrawText(buf, lx, ly, fs + 2, BLACK); ly += lh + 4;

        snprintf(buf, sizeof(buf), "Lv %d", m->level);
        DrawText(buf, lx, ly, fs, BLACK); ly += lh;

        snprintf(buf, sizeof(buf), "HP %d/%d", m->hp, m->maxHp);
        DrawText(buf, lx, ly, fs, BLACK); ly += lh;

        snprintf(buf, sizeof(buf), "ATK %d", m->attack);
        DrawText(buf, lx, ly, fs, BLACK); ly += lh;

        snprintf(buf, sizeof(buf), "DEF %d", m->defense);
        DrawText(buf, lx, ly, fs, BLACK); ly += lh;

        snprintf(buf, sizeof(buf), "EXP %d", m->expValue);
        DrawText(buf, lx, ly, fs, BLACK);

    } else if (type == INSPECTOR_ITEM) {
        bool isEquip = game->inventorySelection >= game->inventorySlotCount;

        if (isEquip) {
            EquipType eType = game->equipInventory[game->inventorySelection - game->inventorySlotCount];
            const EquipData* d = GetEquipData(eType);
            if (!d) return;

            char displayName[64];
            if (d->twoHanded)
                snprintf(displayName, sizeof(displayName), "%s (two-handed)", d->name);
            else
                snprintf(displayName, sizeof(displayName), "%s", d->name);
            int titleSize = (int)(16 * scale);
            int tw = MeasureText(displayName, titleSize);
            DrawText(displayName, cx - tw / 2, ly, titleSize, BLACK);
            ly += (int)(24 * scale);

            Texture2D sprite = GetEquipSprite(eType);
            if (sprite.id > 0) {
                float spriteSize = (int)(48 * scale);
                Rectangle src = { 0, 0, (float)sprite.width, (float)sprite.height };
                Rectangle dest = { cx - spriteSize / 2, (float)ly, spriteSize, spriteSize };
                DrawTexturePro(sprite, src, dest, (Vector2){ 0 }, 0, WHITE);
                ly += (int)(spriteSize + 8 * scale);
            }

            int descSize = (int)(14 * scale);
            const char* desc = d->description;
            int descH = DrawDesc(desc, lx, ly, w - (int)(20 * scale), descSize, BLACK);
            ly += descH + (int)(4 * scale);

            ly += (int)(40 * scale);

            int statSize = (int)(14 * scale);
            if (d->bonusAttack > 0) {
                snprintf(buf, sizeof(buf), "ATK+%d", d->bonusAttack);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
            if (d->bonusDefense != 0) {
                snprintf(buf, sizeof(buf), "DEF%+d", d->bonusDefense);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
            if (d->bonusMaxHp > 0) {
                snprintf(buf, sizeof(buf), "HP+%d", d->bonusMaxHp);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
            if (d->bonusStr != 0) {
                snprintf(buf, sizeof(buf), "STR%+d", d->bonusStr);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
            if (d->bonusDex != 0) {
                snprintf(buf, sizeof(buf), "DEX%+d", d->bonusDex);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
            if (d->bonusInt != 0) {
                snprintf(buf, sizeof(buf), "MGC%+d", d->bonusInt);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
            if (d->bonusCon != 0) {
                snprintf(buf, sizeof(buf), "CON%+d", d->bonusCon);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
            if (d->bonusLck != 0) {
                snprintf(buf, sizeof(buf), "LCK%+d", d->bonusLck);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
        } else {
            ItemType selType = game->inventory[game->inventorySelection].type;

            const char* iname = GetItemName(selType);
            int titleSize = (int)(16 * scale);
            int tw = MeasureText(iname, titleSize);
            DrawText(iname, cx - tw / 2, ly, titleSize, BLACK);
            ly += (int)(24 * scale);

            int texIdx = selType - 1;
            if (texIdx >= 0 && texIdx < 3 && game->potionTextures[texIdx].id > 0) {
                Texture2D tex = game->potionTextures[texIdx];
                float spriteSize = (int)(48 * scale);
                Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
                Rectangle dest = { cx - spriteSize / 2, (float)ly, spriteSize, spriteSize };
                DrawTexturePro(tex, src, dest, (Vector2){ 0 }, 0, WHITE);
                ly += (int)(spriteSize + 8 * scale);
            }

            const char* desc = GetItemDescription(selType);
            int descSize = (int)(14 * scale);
            int descH = DrawDesc(desc, lx, ly, w - (int)(20 * scale), descSize, BLACK);
            ly += descH + (int)(4 * scale);

            snprintf(buf, sizeof(buf), "Qty: %d", game->inventory[game->inventorySelection].quantity);
            DrawText(buf, lx, ly, (int)(14 * scale), BLACK);
            ly += (int)(22 * scale);

            int heal = GetItemHealAmount(selType);
            if (heal > 0) {
                snprintf(buf, sizeof(buf), "Heals %d HP", heal);
                DrawText(buf, lx, ly, (int)(14 * scale), BLACK);
            }
        }
    }
}