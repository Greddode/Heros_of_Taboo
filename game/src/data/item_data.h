#ifndef ITEM_DATA_H
#define ITEM_DATA_H

#include "game_types.h"

const char* GetItemName(ItemType type);
int GetItemHealAmount(ItemType type);
const char* GetItemDescription(ItemType type);
const char* GetItemSpritePath(ItemType type);

#endif
