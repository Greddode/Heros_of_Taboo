#include "data/item_data.h"
#include "raylib.h"
#include <string.h>

static const char* ITEM_NAMES[ITEM_COUNT] = {
    "",
    "Small HP Potion",
    "Medium HP Potion",
    "Large HP Potion"
};

static const int ITEM_HEALS[ITEM_COUNT] = {
    0,
    25,
    50,
    75
};

static const char* ITEM_DESCS[ITEM_COUNT] = {
    "",
    "A small vial of red liquid.\nRestores 25% Max HP.",
    "A hearty draught.\nRestores 50% Max HP.",
    "A potent elixir.\nRestores 75% Max HP."
};

const char* ITEM_SPRITES[ITEM_COUNT] = {
    "",
    "resources/sprites/items/health_potions/small_health_potion.png",
    "resources/sprites/items/health_potions/medium_health_potion.png",
    "resources/sprites/items/health_potions/large_health_potion.png"
};

const char* GetItemName(ItemType type) {
    if (type < 0 || type >= ITEM_COUNT) { TraceLog(LOG_WARNING, "GetItemName: invalid type [type=%d]", (int)type); return ""; }
    return ITEM_NAMES[type];
}

int GetItemHealAmount(ItemType type) {
    if (type < 0 || type >= ITEM_COUNT) { TraceLog(LOG_WARNING, "GetItemHealAmount: invalid type [type=%d]", (int)type); return 0; }
    return ITEM_HEALS[type];
}

const char* GetItemDescription(ItemType type) {
    if (type < 0 || type >= ITEM_COUNT) { TraceLog(LOG_WARNING, "GetItemDescription: invalid type [type=%d]", (int)type); return ""; }
    return ITEM_DESCS[type];
}

const char* GetItemSpritePath(ItemType type) {
    if (type < 0 || type >= ITEM_COUNT) return "";
    return ITEM_SPRITES[type];
}
