#ifndef ATLAS_H
#define ATLAS_H

#include "raylib.h"
#include "game_types.h"

typedef struct {
    Texture2D* texture;
    int x, y;
    int w, h;
    int frameCount;
} AtlasEntry;

void Atlas_BuildAll(void);
void Atlas_Destroy(void);

const AtlasEntry* Atlas_GetMonster(MonsterType type);
const AtlasEntry* Atlas_GetEquip(EquipType type);
const AtlasEntry* Atlas_GetItem(ItemType type);

#endif
