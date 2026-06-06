#include "data/equip_table.h"
#include <stddef.h>
#include "raylib.h"

static const EquipData EQUIP_TABLE[EQUIP_COUNT] = {
    { EQUIP_NONE,           "", "", NULL,                            EQUIP_CAT_ARMOR,    EQUIP_SLOT_HEAD,     0, 0, 0, 0,0,0,0,0, false, false, RARITY_COMMON },

    { EQUIP_LEATHER_CAP,    "Leather Cap",     "A simple leather cap.\n+1 DEF, +1 CON",       "resources/sprites/items/equipment/armors/leather_cap.png",    EQUIP_CAT_ARMOR, EQUIP_SLOT_HEAD, 0, 1, 0, 0,0,0,1,0, false, false, RARITY_COMMON },
    { EQUIP_IRON_HELM,      "Iron Helm",       "An iron helmet forged for battle.\n+2 DEF, +2 CON", "resources/sprites/items/equipment/armors/iron_helm.png",      EQUIP_CAT_ARMOR, EQUIP_SLOT_HEAD, 0, 2, 0, 0,0,0,2,0, false, false, RARITY_UNCOMMON },
    { EQUIP_STEEL_HELM,     "Steel Helm",      "A sturdy steel helm.\n+3 DEF, +3 CON",         "resources/sprites/items/equipment/armors/steel_helm.png",     EQUIP_CAT_ARMOR, EQUIP_SLOT_HEAD, 0, 3, 0, 0,0,0,3,0, false, false, RARITY_RARE },

    { EQUIP_LEATHER_VEST,   "Leather Vest",    "A flexible leather vest.\n+2 DEF, +2 CON, +2 DEX",     "resources/sprites/items/equipment/armors/leather_vest.png",   EQUIP_CAT_ARMOR, EQUIP_SLOT_CHEST, 0, 2, 0, 0,2,0,2,0, false, false, RARITY_COMMON },
    { EQUIP_CHAIN_MAIL,     "Chain Mail",      "Interlocking rings of steel.\n+4 DEF, +4 CON", "resources/sprites/items/equipment/armors/chain_mail.png",     EQUIP_CAT_ARMOR, EQUIP_SLOT_CHEST, 0, 4, 0, 0,0,0,4,0, false, false, RARITY_UNCOMMON },
    { EQUIP_PLATE_MAIL,     "Plate Mail",      "Full plate armour.\n+6 DEF, +6 CON",          "resources/sprites/items/equipment/armors/plate_mail.png",     EQUIP_CAT_ARMOR, EQUIP_SLOT_CHEST, 0, 6, 0, 0,0,0,6,0, false, false, RARITY_RARE },

    { EQUIP_SURVIVAL_KNIFE,"Survival Knife",   "A trusty blade for the dungeon.\n+2 ATK",     "resources/sprites/items/equipment/weapons/survival_knife.png", EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 2, 0, 0, 0,0,0,0,0, false, false, RARITY_COMMON },
    { EQUIP_DAGGER,         "Dagger",          "A swift dagger.\n+3 ATK, +1 DEX",              "resources/sprites/items/equipment/weapons/dagger.png",         EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 3, 0, 0, 0,1,0,0,0, false, false, RARITY_COMMON },
    { EQUIP_IRON_SWORD,     "Iron Sword",      "A well-balanced iron blade.\n+5 ATK",        "resources/sprites/items/equipment/weapons/iron_sword.png",    EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 5, 0, 0, 0,0,0,0,0, false, false, RARITY_UNCOMMON },
    { EQUIP_STEEL_SWORD,    "Steel Sword",     "A razor-sharp steel longsword.\n+8 ATK",     "resources/sprites/items/equipment/weapons/steel_sword.png",   EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 8, 0, 0, 0,0,0,0,0, false, false, RARITY_RARE },
    { EQUIP_WAR_HAMMER,     "War Hammer",      "A massive two-handed hammer.\n+10 ATK, +1 DEF, blocks off-hand", "resources/sprites/items/equipment/weapons/warhammer.png", EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 10, 1, 0, 0,0,0,0,0, true, false, RARITY_RARE },

    { EQUIP_WOODEN_SHIELD,  "Wooden Shield",   "A light wooden shield.\n+2 DEF, +2 LCK",             "resources/sprites/items/equipment/armors/wooden_shield.png",  EQUIP_CAT_ARMOR, EQUIP_SLOT_OFF_HAND, 0, 2, 0, 0,0,0,0,2, false, false, RARITY_COMMON },
    { EQUIP_IRON_SHIELD,    "Iron Shield",     "A sturdy iron shield.\n+4 DEF",               "resources/sprites/items/equipment/armors/iron_shield.png",    EQUIP_CAT_ARMOR, EQUIP_SLOT_OFF_HAND, 0, 4, 0, 0,0,0,0,0, false, false, RARITY_UNCOMMON },
    { EQUIP_STEEL_SHIELD,   "Steel Shield",    "A heavy steel tower shield.\n+6 DEF",        "resources/sprites/items/equipment/armors/steel_shield.png",   EQUIP_CAT_ARMOR, EQUIP_SLOT_OFF_HAND, 0, 6, 0, 0,0,0,0,0, false, false, RARITY_RARE },

    { EQUIP_RING_OF_STRENGTH,  "Ring of Strength",   "A ring pulsing with power.\n+3 STR",           "resources/sprites/items/equipment/accessories/ring_of_strength.png", EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 3,0,0,0,0, false, false, RARITY_UNCOMMON },
    { EQUIP_AMULET_OF_WARDING, "Amulet of Warding",  "An enchanted amulet.\n+2 MGC, +2 CON",           "resources/sprites/items/equipment/accessories/amulet_of_warding.png", EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,0,2,2,0, false, false, RARITY_UNCOMMON },
    { EQUIP_BOOTS_OF_SWIFTNESS,"Boots of Swiftness","Light boots that aid movement.\n+3 DEX, +1 CON", "resources/sprites/items/equipment/accessories/boots_of_swiftness.png", EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,3,0,1,0, false, false, RARITY_UNCOMMON },
    { EQUIP_RING_OF_THE_HAWK,  "Ring of the Hawk",   "A hawk's eye in a ring.\n+4 DEX",               "resources/sprites/items/equipment/accessories/ring_of_hawk.png",     EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,4,0,0,0, false, false, RARITY_RARE },
    { EQUIP_SAGES_PENDANT,     "Sage's Pendant",     "The wisdom of ages.\n+4 MGC",                    "resources/sprites/items/equipment/accessories/sage_pendant.png",     EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,0,4,0,0, false, false, RARITY_RARE },
    { EQUIP_LUCKY_CHARM,       "Lucky Charm",        "A charm of fortune.\n+6 LCK",                    "resources/sprites/items/equipment/accessories/lucky_charm.png",      EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,0,0,0,6, false, false, RARITY_RARE },
    { EQUIP_BERSERKER_BAND,    "Berserker Band",     "Strength at a cost.\n+5 STR, -2 DEF",            "resources/sprites/items/equipment/accessories/fallback_ring.png",    EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, -2, 0, 5,0,0,0,0, false, false, RARITY_RARE },

    { EQUIP_SIMPLE_BOW,    "Simple Bow",       "A rough hunting bow.\n+3 DEX\nRange: 4",   "resources/sprites/items/equipment/weapons/simple_bow.png",   EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 0, 0, 0, 0,3,0,0,0, true, true, RARITY_UNCOMMON },
    { EQUIP_DWARVEN_BOW,   "Dwarven Bow",      "A stout dwarven shortbow.\n+5 DEX\nRange: 5", "resources/sprites/items/equipment/weapons/dwarven_bow.png", EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 0, 0, 0, 0,5,0,0,0, true, true, RARITY_UNCOMMON },
    { EQUIP_ELVEN_BOW,     "Elven Bow",        "A graceful elven longbow.\n+8 DEX\nRange: 7", "resources/sprites/items/equipment/weapons/elven_bow.png",   EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 0, 0, 0, 0,8,0,0,0, true, true, RARITY_RARE },
    { EQUIP_GREATBOW,      "Greatbow",         "A massive war bow.\n+9 DEX\nRange: 8",     "resources/sprites/items/equipment/weapons/greatbow.png",     EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 0, 0, 0, 0,9,0,0,0, true, true, RARITY_RARE },
    { EQUIP_CROSSBOW,      "Crossbow",         "A mechanical crossbow.\n+7 DEX\nRange: 6",   "resources/sprites/items/equipment/weapons/crossbow.png",     EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 0, 0, 0, 0,7,0,0,0, true, true, RARITY_RARE },
    { EQUIP_BAND_OF_GROWTH,"Band of Limitless Growth", "An ancient band. Your potential knows no bounds.\nRemoves all stat caps.", "resources/sprites/items/equipment/accessories/lucky_charm.png", EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,0,0,0,0, false, false, RARITY_LEGENDARY },
};

const EquipData* GetEquipData(EquipType type) {
    if (type < 0 || type >= EQUIP_COUNT) { TraceLog(LOG_WARNING, "GetEquipData: invalid type [type=%d]", (int)type); return NULL; }
    return &EQUIP_TABLE[type];
}

int GetEquipRangeBonus(EquipType type) {
    switch (type) {
        case EQUIP_SIMPLE_BOW:  return 4;
        case EQUIP_DWARVEN_BOW: return 5;
        case EQUIP_ELVEN_BOW:   return 7;
        case EQUIP_GREATBOW:    return 8;
        case EQUIP_CROSSBOW:    return 6;
        default: return 0;
    }
}
