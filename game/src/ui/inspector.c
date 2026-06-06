#include "inspector.h"
#include "systems/world_monster.h"
#include "systems/renderer.h"
#include "inventory.h"
#include "resources.h"
#include <stdio.h>
#include <string.h>

static int DrawWrappedLine(const char* text, int x, int y, int maxWidth, int fontSize, Color color) {
    char lineBuf[512] = "";
    const char* word = text;
    while (*word) {
        while (*word == ' ') word++;
        if (!*word) break;
        const char* wordEnd = word;
        while (*wordEnd && *wordEnd != ' ') wordEnd++;
        int wordLen = (int)(wordEnd - word);
        int curLen = (int)strlen(lineBuf);
        int sep = (curLen > 0) ? 1 : 0;
        char testBuf[512];
        memcpy(testBuf, lineBuf, curLen);
        if (sep) testBuf[curLen] = ' ';
        memcpy(testBuf + curLen + sep, word, wordLen);
        testBuf[curLen + sep + wordLen] = '\0';
        if (MeasureText(testBuf, fontSize) > maxWidth) {
            if (curLen > 0) {
                DrawText(lineBuf, x, y, fontSize, color);
                y += fontSize + 4;
                memcpy(lineBuf, word, wordLen);
                lineBuf[wordLen] = '\0';
            } else {
                memcpy(lineBuf, word, wordLen);
                lineBuf[wordLen] = '\0';
            }
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

void Inspector_Render(const GameWorld* game, InspectorType type, int x, int y, int w, int h, int selectedSlot) {
    // Early-out: don't draw anything if there's nothing to inspect
    if (type == INSPECTOR_MONSTER) {
        if (game->selectedMonsterEntity == ENTITY_NONE) return;
        if (!game->ecs.alive[game->selectedMonsterEntity]) return;
        if (!World_HasComponents(&game->ecs, game->selectedMonsterEntity, COMP_STATS)) return;
        if (!World_GetStats(&game->ecs, game->selectedMonsterEntity)->alive) return;
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

    Texture2D* uiSlot = Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameSlot01b.png");
    if (uiSlot && uiSlot->id > 0)
        Draw9Slice(*uiSlot, (Rectangle){ (float)x, (float)y, (float)w, (float)h }, 8, 8, 8, 8);
    else {
        DrawRectangle(x, y, w, h, (Color){ 0, 0, 0, 200 });
        DrawRectangleLines(x, y, w, h, LIGHTGRAY);
    }

    if (type == INSPECTOR_MONSTER) {
        EntityId m = game->selectedMonsterEntity;
        CStats* s = World_GetStats(&game->ecs, m);
        CName* n = World_HasComponents(&game->ecs, m, COMP_NAME)
            ? World_GetName(&game->ecs, m) : NULL;
        const char* name = n ? n->name : "Monster";

        snprintf(buf, sizeof(buf), "%s", name);
        DrawText(buf, lx, ly, fs + 2, BLACK); ly += lh + 4;

        snprintf(buf, sizeof(buf), "Lv %d", s->level);
        DrawText(buf, lx, ly, fs, BLACK); ly += lh;

        snprintf(buf, sizeof(buf), "HP %d/%d", s->hp, s->maxHp);
        DrawText(buf, lx, ly, fs, BLACK); ly += lh;

        snprintf(buf, sizeof(buf), "ATK %d", s->attack);
        DrawText(buf, lx, ly, fs, BLACK); ly += lh;

        snprintf(buf, sizeof(buf), "DEF %d", s->defense);
        DrawText(buf, lx, ly, fs, BLACK); ly += lh;

        snprintf(buf, sizeof(buf), "EXP %d", s->expValue);
        DrawText(buf, lx, ly, fs, BLACK);

    } else if (type == INSPECTOR_ITEM) {
        bool isEquip = selectedSlot >= game->inventorySlotCount;

        if (isEquip) {
            EquipType eType = game->equipInventory[selectedSlot - game->inventorySlotCount];
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

            Texture2D* sprite = Inventory_LoadEquipTexture(eType);
            if (sprite && sprite->id > 0) {
                float spriteSize = (int)(48 * scale);
                Rectangle src = { 0, 0, (float)sprite->width, (float)sprite->height };
                Rectangle dest = { cx - spriteSize / 2, (float)ly, spriteSize, spriteSize };
                DrawTexturePro(*sprite, src, dest, (Vector2){ 0 }, 0, WHITE);
                ly += (int)(spriteSize + 8 * scale);
            }

            int descSize = (int)(14 * scale);
            const char* desc = d->description;
            int descH = DrawDesc(desc, lx, ly, (x + w) - lx - (int)(10 * scale), descSize, BLACK);
            ly += descH + (int)(4 * scale);

            int statSize = (int)(14 * scale);
            #define INSPECTOR_BOTTOM (y + h - (int)(12 * scale))
            if (d->bonusAttack > 0) {
                if (ly >= INSPECTOR_BOTTOM) return;
                snprintf(buf, sizeof(buf), "ATK+%d", d->bonusAttack);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
            if (d->bonusDefense != 0) {
                if (ly >= INSPECTOR_BOTTOM) return;
                snprintf(buf, sizeof(buf), "DEF%+d", d->bonusDefense);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
            if (d->bonusMaxHp > 0) {
                if (ly >= INSPECTOR_BOTTOM) return;
                snprintf(buf, sizeof(buf), "HP+%d", d->bonusMaxHp);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
            if (d->bonusStr != 0) {
                if (ly >= INSPECTOR_BOTTOM) return;
                snprintf(buf, sizeof(buf), "STR%+d", d->bonusStr);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
            if (d->bonusDex != 0) {
                if (ly >= INSPECTOR_BOTTOM) return;
                snprintf(buf, sizeof(buf), "DEX%+d", d->bonusDex);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
            if (d->bonusInt != 0) {
                if (ly >= INSPECTOR_BOTTOM) return;
                snprintf(buf, sizeof(buf), "MGC%+d", d->bonusInt);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
            if (d->bonusCon != 0) {
                if (ly >= INSPECTOR_BOTTOM) return;
                snprintf(buf, sizeof(buf), "CON%+d", d->bonusCon);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
            if (d->bonusLck != 0) {
                if (ly >= INSPECTOR_BOTTOM) return;
                snprintf(buf, sizeof(buf), "LCK%+d", d->bonusLck);
                DrawText(buf, lx, ly, statSize, BLACK); ly += (int)(18 * scale);
            }
        } else {
            ItemType selType = game->inventory[selectedSlot].type;

            const char* iname = GetItemName(selType);
            int titleSize = (int)(16 * scale);
            int tw = MeasureText(iname, titleSize);
            DrawText(iname, cx - tw / 2, ly, titleSize, BLACK);
            ly += (int)(24 * scale);

            Texture2D* pTex = Inventory_LoadPotionTexture(selType);
            if (pTex && pTex->id > 0) {
                float spriteSize = (int)(48 * scale);
                Rectangle src = { 0, 0, (float)pTex->width, (float)pTex->height };
                Rectangle dest = { cx - spriteSize / 2, (float)ly, spriteSize, spriteSize };
                DrawTexturePro(*pTex, src, dest, (Vector2){ 0 }, 0, WHITE);
                ly += (int)(spriteSize + 8 * scale);
            }

            const char* desc = GetItemDescription(selType);
            int descSize = (int)(14 * scale);
            int descH = DrawDesc(desc, lx, ly, (x + w) - lx - (int)(10 * scale), descSize, BLACK);
            ly += descH + (int)(4 * scale);

            snprintf(buf, sizeof(buf), "Qty: %d", game->inventory[selectedSlot].quantity);
            DrawText(buf, lx, ly, (int)(14 * scale), BLACK);
            ly += (int)(22 * scale);

            int heal = GetItemHealAmount(selType);
            if (heal > 0) {
                if (ly >= INSPECTOR_BOTTOM) return;
                snprintf(buf, sizeof(buf), "Heals %d HP", heal);
                DrawText(buf, lx, ly, (int)(14 * scale), BLACK);
            }
        }
    }
}