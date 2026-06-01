#include "data/loot_data.h"

const LootEntry LOOT_TABLE[] = {
    // Tier 1 (Common) — base weight 15
    { LOOT_TYPE_POTION, (int)ITEM_SMALL_HP_POTION,  1, 15 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_LEATHER_CAP,     1, 12 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_LEATHER_VEST,    1, 12 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_WOODEN_SHIELD,   1, 12 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_DAGGER,          1, 10 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_RING_OF_STRENGTH,1, 10 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_BOOTS_OF_SWIFTNESS,1, 10 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_SIMPLE_BOW,      1, 8 },

    // Tier 2 (Uncommon) — base weight 10
    { LOOT_TYPE_POTION, (int)ITEM_MEDIUM_HP_POTION, 2, 12 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_IRON_HELM,       2, 9 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_CHAIN_MAIL,      2, 9 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_IRON_SWORD,      2, 9 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_IRON_SHIELD,     2, 9 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_AMULET_OF_WARDING,2, 7 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_RING_OF_THE_HAWK,2, 7 },

    // Tier 3 (Rare) — base weight 6
    { LOOT_TYPE_POTION, (int)ITEM_LARGE_HP_POTION,  3, 9 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_STEEL_HELM,      3, 6 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_PLATE_MAIL,      3, 6 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_STEEL_SWORD,     3, 6 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_STEEL_SHIELD,    3, 6 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_SAGES_PENDANT,   3, 6 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_LUCKY_CHARM,     3, 5 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_DWARVEN_BOW,     3, 4 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_CROSSBOW,        3, 5 },

    // Tier 4 (Legendary) — base weight 3
    { LOOT_TYPE_EQUIP,  (int)EQUIP_WAR_HAMMER,      4, 4 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_BERSERKER_BAND,  4, 3 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_ELVEN_BOW,       4, 3 },
    { LOOT_TYPE_EQUIP,  (int)EQUIP_GREATBOW,        4, 2 },
};

const int LOOT_TABLE_COUNT = (int)(sizeof(LOOT_TABLE) / sizeof(LOOT_TABLE[0]));
