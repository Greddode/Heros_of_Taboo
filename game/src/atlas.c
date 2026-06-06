#include "atlas.h"
#include "data/monster_data.h"
#include "data/item_data.h"
#include "data/equip_table.h"
#include "inventory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ATLAS_MAX_W 1024

static bool s_built = false;

static AtlasEntry s_monsterEntries[MONSTER_TYPE_COUNT];
static AtlasEntry s_equipEntries[EQUIP_COUNT];
static AtlasEntry s_itemEntries[ITEM_COUNT];

static Texture2D s_monsterAtlas;
static Texture2D s_equipAtlas;
static Texture2D s_itemAtlas;

static bool s_monsterAtlasReady, s_equipAtlasReady, s_itemAtlasReady;

static int PackSprite(Image* atlas, const char* path, int* cursorX, int* cursorY, int* maxRowH, AtlasEntry* out) {
    if (!path || !path[0] || !FileExists(path)) return 0;
    Image img = LoadImage(path);
    if (img.width == 0 || img.height == 0 || img.data == NULL) { UnloadImage(img); return 0; }

    if (atlas) {
        if (*cursorX + img.width > ATLAS_MAX_W) {
            *cursorX = 0;
            *cursorY += *maxRowH;
            *maxRowH = 0;
        }

        ImageDraw(atlas, img, (Rectangle){0,0,(float)img.width,(float)img.height},
                  (Rectangle){(float)*cursorX, (float)*cursorY, (float)img.width, (float)img.height}, WHITE);
    }

    out->texture = NULL;
    out->x = *cursorX;
    out->y = *cursorY;
    out->w = img.width;
    out->h = img.height;

    *cursorX += img.width;
    if (img.height > *maxRowH) *maxRowH = img.height;
    UnloadImage(img);
    return 1;
}

static int CalcAtlasHeight(int cursorY, int maxRowH) { return cursorY + maxRowH; }

static void BuildMonsterAtlas(void) {
    Monster_InitTemplates();

    int cursorX = 0, cursorY = 0, maxRowH = 0;
    int totalH = 0;
    for (int i = 0; i < MONSTER_TYPE_COUNT; i++) {
        const MonsterTemplate* tpl = Monster_GetTemplate((MonsterType)i);
        if (!tpl || !tpl->spritePath || !tpl->spritePath[0]) continue;
        int cX = cursorX, cY = cursorY, mRH = maxRowH;
        if (!PackSprite(NULL, tpl->spritePath, &cursorX, &cursorY, &maxRowH, &s_monsterEntries[i])) continue;
        s_monsterEntries[i].frameCount = tpl->frameCount;
        if (cursorY + maxRowH > totalH) totalH = cursorY + maxRowH;
    }
    if (totalH == 0) return;
    totalH = CalcAtlasHeight(cursorY, maxRowH);

    Image atlas = GenImageColor(ATLAS_MAX_W, totalH, BLANK);
    cursorX = 0; cursorY = 0; maxRowH = 0;
    for (int i = 0; i < MONSTER_TYPE_COUNT; i++) {
        const MonsterTemplate* tpl = Monster_GetTemplate((MonsterType)i);
        if (!tpl || !tpl->spritePath || !tpl->spritePath[0]) continue;
        if (s_monsterEntries[i].w == 0) continue;
        PackSprite(&atlas, tpl->spritePath, &cursorX, &cursorY, &maxRowH, &s_monsterEntries[i]);
    }

    s_monsterAtlas = LoadTextureFromImage(atlas);
    UnloadImage(atlas);
    for (int i = 0; i < MONSTER_TYPE_COUNT; i++)
        if (s_monsterEntries[i].w > 0) s_monsterEntries[i].texture = &s_monsterAtlas;
    s_monsterAtlasReady = true;
}

static void BuildEquipAtlas(void) {
    int cursorX = 0, cursorY = 0, maxRowH = 0;
    int totalH = 0;
    for (int i = 1; i < EQUIP_COUNT; i++) {
        const EquipData* d = GetEquipData((EquipType)i);
        if (!d || !d->spritePath || !d->spritePath[0]) continue;
        if (!PackSprite(NULL, d->spritePath, &cursorX, &cursorY, &maxRowH, &s_equipEntries[i])) continue;
        if (cursorY + maxRowH > totalH) totalH = cursorY + maxRowH;
    }
    if (totalH == 0) return;
    totalH = CalcAtlasHeight(cursorY, maxRowH);

    Image atlas = GenImageColor(ATLAS_MAX_W, totalH, BLANK);
    cursorX = 0; cursorY = 0; maxRowH = 0;
    for (int i = 1; i < EQUIP_COUNT; i++) {
        const EquipData* d = GetEquipData((EquipType)i);
        if (!d || !d->spritePath || !d->spritePath[0]) continue;
        if (s_equipEntries[i].w == 0) continue;
        PackSprite(&atlas, d->spritePath, &cursorX, &cursorY, &maxRowH, &s_equipEntries[i]);
    }

    s_equipAtlas = LoadTextureFromImage(atlas);
    UnloadImage(atlas);
    for (int i = 0; i < EQUIP_COUNT; i++)
        if (s_equipEntries[i].w > 0) s_equipEntries[i].texture = &s_equipAtlas;
    s_equipAtlasReady = true;
}

static void BuildItemAtlas(void) {
    int cursorX = 0, cursorY = 0, maxRowH = 0;
    int totalH = 0;
    for (int i = 1; i < ITEM_COUNT; i++) {
        const char* path = GetItemSpritePath((ItemType)i);
        if (!path || !path[0]) continue;
        if (!PackSprite(NULL, path, &cursorX, &cursorY, &maxRowH, &s_itemEntries[i])) continue;
        if (cursorY + maxRowH > totalH) totalH = cursorY + maxRowH;
    }
    if (totalH == 0) return;
    totalH = CalcAtlasHeight(cursorY, maxRowH);

    Image atlas = GenImageColor(ATLAS_MAX_W, totalH, BLANK);
    cursorX = 0; cursorY = 0; maxRowH = 0;
    for (int i = 1; i < ITEM_COUNT; i++) {
        const char* path = GetItemSpritePath((ItemType)i);
        if (!path || !path[0]) continue;
        if (s_itemEntries[i].w == 0) continue;
        PackSprite(&atlas, path, &cursorX, &cursorY, &maxRowH, &s_itemEntries[i]);
    }

    s_itemAtlas = LoadTextureFromImage(atlas);
    UnloadImage(atlas);
    for (int i = 0; i < ITEM_COUNT; i++)
        if (s_itemEntries[i].w > 0) s_itemEntries[i].texture = &s_itemAtlas;
    s_itemAtlasReady = true;
}

void Atlas_BuildAll(void) {
    if (s_built) return;
    s_built = true;
    memset(s_monsterEntries, 0, sizeof(s_monsterEntries));
    memset(s_equipEntries, 0, sizeof(s_equipEntries));
    memset(s_itemEntries, 0, sizeof(s_itemEntries));
    BuildMonsterAtlas();
    BuildEquipAtlas();
    BuildItemAtlas();
}

void Atlas_Destroy(void) {
    if (!s_built) return;
    s_built = false;
    if (s_monsterAtlasReady) { UnloadTexture(s_monsterAtlas); s_monsterAtlasReady = false; }
    if (s_equipAtlasReady)   { UnloadTexture(s_equipAtlas);   s_equipAtlasReady   = false; }
    if (s_itemAtlasReady)    { UnloadTexture(s_itemAtlas);    s_itemAtlasReady    = false; }
}

const AtlasEntry* Atlas_GetMonster(MonsterType type) {
    if (type >= MONSTER_TYPE_COUNT || s_monsterEntries[type].w == 0) return NULL;
    return &s_monsterEntries[type];
}

const AtlasEntry* Atlas_GetEquip(EquipType type) {
    if (type >= EQUIP_COUNT || s_equipEntries[type].w == 0) return NULL;
    return &s_equipEntries[type];
}

const AtlasEntry* Atlas_GetItem(ItemType type) {
    if (type >= ITEM_COUNT || s_itemEntries[type].w == 0) return NULL;
    return &s_itemEntries[type];
}
