#include "game.h"
#include "debug_log.h"
#include "validation.h"
#include "ui/inspector.h"
#include "resources.h"
#include "game_balance.h"
#include "equipment_bonus.h"
#include "systems/spawner_system.h"
#include "systems/world_monster.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// ---- Potion data -----------------------------------------------------------
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
static const char* ITEM_SPRITES[ITEM_COUNT] = {
    "",
    "resources/sprites/items/health_potions/small_health_potion.png",
    "resources/sprites/items/health_potions/medium_health_potion.png",
    "resources/sprites/items/health_potions/large_health_potion.png"
};

// ---- Equipment data table --------------------------------------------------
static const EquipData EQUIP_TABLE[EQUIP_COUNT] = {
    // NONE
    { EQUIP_NONE,           "", "", NULL,                            EQUIP_CAT_ARMOR,    EQUIP_SLOT_HEAD,     0, 0, 0, 0,0,0,0,0, false, false, RARITY_COMMON },

    // Armor - Head
    { EQUIP_LEATHER_CAP,    "Leather Cap",     "A simple leather cap.\n+1 DEF, +1 CON",       "resources/sprites/items/equipment/armors/leather_cap.png",    EQUIP_CAT_ARMOR, EQUIP_SLOT_HEAD, 0, 1, 0, 0,0,0,1,0, false, false, RARITY_COMMON },
    { EQUIP_IRON_HELM,      "Iron Helm",       "An iron helmet forged for battle.\n+2 DEF, +2 CON", "resources/sprites/items/equipment/armors/iron_helm.png",      EQUIP_CAT_ARMOR, EQUIP_SLOT_HEAD, 0, 2, 0, 0,0,0,2,0, false, false, RARITY_UNCOMMON },
    { EQUIP_STEEL_HELM,     "Steel Helm",      "A sturdy steel helm.\n+3 DEF, +3 CON",         "resources/sprites/items/equipment/armors/steel_helm.png",     EQUIP_CAT_ARMOR, EQUIP_SLOT_HEAD, 0, 3, 0, 0,0,0,3,0, false, false, RARITY_RARE },

    // Armor - Chest
    { EQUIP_LEATHER_VEST,   "Leather Vest",    "A flexible leather vest.\n+2 DEF, +2 CON, +2 DEX",     "resources/sprites/items/equipment/armors/leather_vest.png",   EQUIP_CAT_ARMOR, EQUIP_SLOT_CHEST, 0, 2, 0, 0,2,0,2,0, false, false, RARITY_COMMON },
    { EQUIP_CHAIN_MAIL,     "Chain Mail",      "Interlocking rings of steel.\n+4 DEF, +4 CON", "resources/sprites/items/equipment/armors/chain_mail.png",     EQUIP_CAT_ARMOR, EQUIP_SLOT_CHEST, 0, 4, 0, 0,0,0,4,0, false, false, RARITY_UNCOMMON },
    { EQUIP_PLATE_MAIL,     "Plate Mail",      "Full plate armour.\n+6 DEF, +6 CON",          "resources/sprites/items/equipment/armors/plate_mail.png",     EQUIP_CAT_ARMOR, EQUIP_SLOT_CHEST, 0, 6, 0, 0,0,0,6,0, false, false, RARITY_RARE },

    // Weapons
    { EQUIP_SURVIVAL_KNIFE,"Survival Knife",   "A trusty blade for the dungeon.\n+2 ATK",     "resources/sprites/items/equipment/weapons/survival_knife.png", EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 2, 0, 0, 0,0,0,0,0, false, false, RARITY_COMMON },
    { EQUIP_DAGGER,         "Dagger",          "A swift dagger.\n+3 ATK, +1 DEX",              "resources/sprites/items/equipment/weapons/dagger.png",         EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 3, 0, 0, 0,1,0,0,0, false, false, RARITY_COMMON },
    { EQUIP_IRON_SWORD,     "Iron Sword",      "A well-balanced iron blade.\n+5 ATK",        "resources/sprites/items/equipment/weapons/iron_sword.png",    EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 5, 0, 0, 0,0,0,0,0, false, false, RARITY_UNCOMMON },
    { EQUIP_STEEL_SWORD,    "Steel Sword",     "A razor-sharp steel longsword.\n+8 ATK",     "resources/sprites/items/equipment/weapons/steel_sword.png",   EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 8, 0, 0, 0,0,0,0,0, false, false, RARITY_RARE },
    { EQUIP_WAR_HAMMER,     "War Hammer",      "A massive two-handed hammer.\n+10 ATK, +1 DEF, blocks off-hand", "resources/sprites/items/equipment/weapons/warhammer.png", EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 10, 1, 0, 0,0,0,0,0, true, false, RARITY_RARE },

    // Off-hand
    { EQUIP_WOODEN_SHIELD,  "Wooden Shield",   "A light wooden shield.\n+2 DEF, +2 LCK",             "resources/sprites/items/equipment/armors/wooden_shield.png",  EQUIP_CAT_ARMOR, EQUIP_SLOT_OFF_HAND, 0, 2, 0, 0,0,0,0,2, false, false, RARITY_COMMON },
    { EQUIP_IRON_SHIELD,    "Iron Shield",     "A sturdy iron shield.\n+4 DEF",               "resources/sprites/items/equipment/armors/iron_shield.png",    EQUIP_CAT_ARMOR, EQUIP_SLOT_OFF_HAND, 0, 4, 0, 0,0,0,0,0, false, false, RARITY_UNCOMMON },
    { EQUIP_STEEL_SHIELD,   "Steel Shield",    "A heavy steel tower shield.\n+6 DEF",        "resources/sprites/items/equipment/armors/steel_shield.png",   EQUIP_CAT_ARMOR, EQUIP_SLOT_OFF_HAND, 0, 6, 0, 0,0,0,0,0, false, false, RARITY_RARE },

    // Accessories
    { EQUIP_RING_OF_STRENGTH,  "Ring of Strength",   "A ring pulsing with power.\n+3 STR",           "resources/sprites/items/equipment/accessories/ring_of_strength.png", EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 3,0,0,0,0, false, false, RARITY_UNCOMMON },
    { EQUIP_AMULET_OF_WARDING, "Amulet of Warding",  "An enchanted amulet.\n+2 MGC, +2 CON",           "resources/sprites/items/equipment/accessories/amulet_of_warding.png", EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,0,2,2,0, false, false, RARITY_UNCOMMON },
    { EQUIP_BOOTS_OF_SWIFTNESS,"Boots of Swiftness","Light boots that aid movement.\n+3 DEX, +1 CON", "resources/sprites/items/equipment/accessories/boots_of_swiftness.png", EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,3,0,1,0, false, false, RARITY_UNCOMMON },
    { EQUIP_RING_OF_THE_HAWK,  "Ring of the Hawk",   "A hawk's eye in a ring.\n+4 DEX",               "resources/sprites/items/equipment/accessories/ring_of_hawk.png",     EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,4,0,0,0, false, false, RARITY_RARE },
    { EQUIP_SAGES_PENDANT,     "Sage's Pendant",     "The wisdom of ages.\n+4 MGC",                    "resources/sprites/items/equipment/accessories/sage_pendant.png",     EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,0,4,0,0, false, false, RARITY_RARE },
    { EQUIP_LUCKY_CHARM,       "Lucky Charm",        "A charm of fortune.\n+6 LCK",                    "resources/sprites/items/equipment/accessories/lucky_charm.png",      EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,0,0,0,6, false, false, RARITY_RARE },
    { EQUIP_BERSERKER_BAND,    "Berserker Band",     "Strength at a cost.\n+5 STR, -2 DEF",            "resources/sprites/items/equipment/accessories/fallback_ring.png",    EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, -2, 0, 5,0,0,0,0, false, false, RARITY_RARE },

    // Ranged weapons
    { EQUIP_SIMPLE_BOW,    "Simple Bow",       "A rough hunting bow.\n+3 DEX\nRange: 4",   "resources/sprites/items/equipment/weapons/simple_bow.png",   EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 0, 0, 0, 0,3,0,0,0, true, true, RARITY_UNCOMMON },
    { EQUIP_DWARVEN_BOW,   "Dwarven Bow",      "A stout dwarven shortbow.\n+5 DEX\nRange: 5", "resources/sprites/items/equipment/weapons/dwarven_bow.png", EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 0, 0, 0, 0,5,0,0,0, true, true, RARITY_UNCOMMON },
    { EQUIP_ELVEN_BOW,     "Elven Bow",        "A graceful elven longbow.\n+8 DEX\nRange: 7", "resources/sprites/items/equipment/weapons/elven_bow.png",   EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 0, 0, 0, 0,8,0,0,0, true, true, RARITY_RARE },
    { EQUIP_GREATBOW,      "Greatbow",         "A massive war bow.\n+9 DEX\nRange: 8",     "resources/sprites/items/equipment/weapons/greatbow.png",     EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 0, 0, 0, 0,9,0,0,0, true, true, RARITY_RARE },
    { EQUIP_CROSSBOW,      "Crossbow",         "A mechanical crossbow.\n+7 DEX\nRange: 6",   "resources/sprites/items/equipment/weapons/crossbow.png",     EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 0, 0, 0, 0,7,0,0,0, true, true, RARITY_RARE },
    { EQUIP_BAND_OF_GROWTH,"Band of Limitless Growth", "An ancient band. Your potential knows no bounds.\nRemoves all stat caps.", "resources/sprites/items/equipment/accessories/lucky_charm.png", EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,0,0,0,0, false, false, RARITY_LEGENDARY },
};

// ---- Helpers ---------------------------------------------------------------
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

// ---- Inventory (potions) ---------------------------------------------------
bool InventoryAdd(GameWorld* game, ItemType type) {
    if (type == ITEM_NONE) { TraceLog(LOG_WARNING, "InventoryAdd: ITEM_NONE"); return false; }
    if (type >= ITEM_COUNT) { TraceLog(LOG_WARNING, "InventoryAdd: type out of range [type=%d]", (int)type); return false; }
    for (int i = 0; i < game->inventorySlotCount; i++) {
        if (game->inventory[i].type == type) {
            game->inventory[i].quantity++;
            return true;
        }
    }
    if (game->inventorySlotCount >= MAX_INVENTORY_SLOTS) return false;
    game->inventory[game->inventorySlotCount].type = type;
    game->inventory[game->inventorySlotCount].quantity = 1;
    game->inventorySlotCount++;
    return true;
}

bool InventoryUse(GameWorld* game, int slot) {
    if (!game) { TraceLog(LOG_WARNING, "InventoryUse: NULL game"); return false; }
    if (game->playerEntity == ENTITY_NONE) { TraceLog(LOG_WARNING, "InventoryUse: player entity is ENTITY_NONE"); return false; }
    if (slot < 0 || slot >= game->inventorySlotCount) { TraceLog(LOG_WARNING, "InventoryUse: slot out of range [slot=%d count=%d]", slot, game->inventorySlotCount); return false; }
    InventorySlot* s = &game->inventory[slot];
    if (s->type == ITEM_NONE || s->quantity <= 0) { TraceLog(LOG_WARNING, "InventoryUse: slot empty [slot=%d]", slot); return false; }

    int healPercent = GetItemHealAmount(s->type);
    if (healPercent > 0) {
        CStats* ps = World_GetStats(&game->ecs, game->playerEntity);
        int oldHP = ps->hp;
        int heal = calc_potion_heal(ps->maxHp, healPercent, ps->intel);
        ps->hp += heal;
        if (ps->hp > ps->maxHp)
            ps->hp = ps->maxHp;
        DebugLog(DEBUG_INVENTORY, "UsePotion: type=%d HP=%d->%d heal=%d intelBonus=%d",
                 (int)s->type, oldHP, ps->hp, ps->hp - oldHP, (int)ps->intel);
        {
            CPosition* pp = World_GetPosition(&game->ecs, game->playerEntity);
            int tw = game->map->tileWidth;
            int th = game->map->tileHeight;
            DamageNumber_Spawn(&game->damageNumbers, heal, pp->x, pp->y, tw, th, GREEN);
        }
    }

    s->quantity--;
    if (s->quantity <= 0) {
        for (int i = slot; i < game->inventorySlotCount - 1; i++)
            game->inventory[i] = game->inventory[i + 1];
        game->inventorySlotCount--;
    }
    return true;
}

Texture2D* Inventory_LoadEquipTexture(EquipType type) {
    const EquipData* d = GetEquipData(type);
    if (!d || !d->spritePath || d->spritePath[0] == '\0') return NULL;
    return Resources_LoadTexture(d->spritePath);
}

Texture2D* Inventory_LoadPotionTexture(ItemType type) {
    if (type <= ITEM_NONE || type >= ITEM_COUNT) return NULL;
    if (!ITEM_SPRITES[type][0]) return NULL;
    return Resources_LoadTexture(ITEM_SPRITES[type]);
}

AbilityType Inventory_GetWeaponAbility(EquipType weapon) {
    switch (weapon) {
        case EQUIP_NONE:             return ABILITY_PUNCH;
        case EQUIP_SURVIVAL_KNIFE:
        case EQUIP_DAGGER:
        case EQUIP_IRON_SWORD:       return ABILITY_LIGHT_MELEE;
        case EQUIP_STEEL_SWORD:
        case EQUIP_WAR_HAMMER:       return ABILITY_HEAVY_MELEE;
        case EQUIP_SIMPLE_BOW:
        case EQUIP_ELVEN_BOW:        return ABILITY_RANGED;
        default:                     return ABILITY_PUNCH;
    }
}

void LoadPotionTextures(GameWorld* game) {
    (void)game;
    for (int i = 1; i < ITEM_COUNT; i++) {
        if (ITEM_SPRITES[i][0]) {
            Resources_LoadTexture(ITEM_SPRITES[i]);
        }
    }
    Resources_LoadTexture("resources/sprites/ui/UI_Flat_Frame01a.png");
    Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameSlot01b.png");
    Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameMarker01a.png");
    Resources_LoadTexture("resources/sprites/items/loot.png");
}



// ---- Equipment management --------------------------------------------------
bool EquipItem(GameWorld* game, EquipType type) {
    if (type == EQUIP_NONE) { TraceLog(LOG_WARNING, "EquipItem: EQUIP_NONE"); return false; }
    if (!game) { TraceLog(LOG_WARNING, "EquipItem: NULL game"); return false; }
    if (game->playerEntity == ENTITY_NONE) { TraceLog(LOG_WARNING, "EquipItem: player entity is ENTITY_NONE"); return false; }
    const EquipData* data = GetEquipData(type);
    if (!data) { TraceLog(LOG_WARNING, "EquipItem: unknown equip type [type=%d]", (int)type); return false; }

        int slotIdx = (int)data->slot;
    CStats* ps = World_GetStats(&game->ecs, game->playerEntity);

    // If a dual-wieldable weapon is equipped to the main-hand slot that's already occupied,
    // auto-redirect to off-hand slot if it's free.
    if (data->slot == EQUIP_SLOT_WEAPON && IsWeaponDualWieldable(type) &&
        game->equipped[EQUIP_SLOT_WEAPON] != EQUIP_NONE &&
        game->equipped[EQUIP_SLOT_OFF_HAND] == EQUIP_NONE) {
        slotIdx = EQUIP_SLOT_OFF_HAND;
    }

    if (slotIdx == EQUIP_SLOT_OFF_HAND && IsTwoHandedEquipped(game)) {
        return false;
    }

    if (game->equipped[slotIdx] != EQUIP_NONE) {
        UnequipSlot(game, data->slot);
    }

    if (data->twoHanded && game->equipped[EQUIP_SLOT_OFF_HAND] != EQUIP_NONE) {
        UnequipSlot(game, EQUIP_SLOT_OFF_HAND);
    }

    EquipmentBonus_Apply(&game->ecs, game->playerEntity, type);

    game->equipped[slotIdx] = type;

    if (type == EQUIP_BAND_OF_GROWTH)
        game->statCapsRemoved = true;

    if (data->category == EQUIP_CAT_WEAPON && World_HasComponents(&game->ecs, game->playerEntity, COMP_ABILITIES)) {
        CAbilities* a = World_GetAbilities(&game->ecs, game->playerEntity);
        a->abilities[0] = Inventory_GetWeaponAbility(type);
        if (a->count < 1) a->count = 1;
    }

    DebugLog(DEBUG_INVENTORY, "EquipItem: %s slot=%d atk=%d def=%d str=%d dex=%d int=%d con=%d lck=%d",
             data->name, slotIdx, data->bonusAttack, data->bonusDefense, data->bonusStr, data->bonusDex, data->bonusInt, data->bonusCon, data->bonusLck);
    return true;
}

void EquipItemSilent(GameWorld* game, EquipType type) {
    if (type == EQUIP_NONE) return;
    const EquipData* data = GetEquipData(type);
    if (!data) return;

        int slotIdx = (int)data->slot;

    if (game->equipped[slotIdx] != EQUIP_NONE) {
        EquipmentBonus_Remove(&game->ecs, game->playerEntity,
                              game->equipped[slotIdx]);
        game->equipped[slotIdx] = EQUIP_NONE;
    }

    EquipmentBonus_Apply(&game->ecs, game->playerEntity, type);

    game->equipped[slotIdx] = type;
}

void UnequipSlot(GameWorld* game, EquipSlot slot) {
        int slotIdx = (int)slot;
    EquipType oldType = game->equipped[slotIdx];
    if (oldType == EQUIP_NONE) return;

    const EquipData* data = GetEquipData(oldType);
    if (!data) return;

    DebugLog(DEBUG_INVENTORY, "UnequipSlot: %s slot=%d", data->name, slotIdx);
    EquipmentBonus_Remove(&game->ecs, game->playerEntity, oldType);

    game->equipped[slotIdx] = EQUIP_NONE;

    if (oldType == EQUIP_BAND_OF_GROWTH)
        game->statCapsRemoved = false;

    if (game->equipInventoryCount < MAX_INVENTORY_SLOTS) {
        AddEquipToInventory(game, oldType);
    } else {
        CPosition* pp = World_GetPosition(&game->ecs, game->playerEntity);
        int dropX = pp->x, dropY = pp->y;
        if (game->blocking[dropY][dropX] || World_FindMonsterAt(game, dropX, dropY, ENTITY_NONE) != ENTITY_NONE) {
            int nx[] = { 0, 0, -1, 1 };
            int ny[] = { -1, 1, 0, 0 };
            for (int d = 0; d < 4; d++) {
                int tx = pp->x + nx[d], ty = pp->y + ny[d];
                if (tx >= 0 && tx < game->map->width && ty >= 0 && ty < game->map->height &&
                    !game->blocking[ty][tx] && World_FindMonsterAt(game, tx, ty, ENTITY_NONE) == ENTITY_NONE) {
                    dropX = tx; dropY = ty; break;
                }
            }
        }
        SpawnerSystem_AddEquipAt(game, dropX, dropY, oldType, 1);
        FloatMsg_Spawn(game, dropX, dropY, YELLOW, "No room - item dropped!");
    }

}

bool IsEquipSlotOccupied(const GameWorld* game, EquipSlot slot) {
    return game->equipped[(int)slot] != EQUIP_NONE;
}

bool IsTwoHandedEquipped(const GameWorld* game) {
    EquipType weapon = game->equipped[EQUIP_SLOT_WEAPON];
    if (weapon == EQUIP_NONE) return false;
    const EquipData* data = GetEquipData(weapon);
    return data && data->twoHanded;
}

bool IsWeaponDualWieldable(EquipType type) {
    const EquipData* d = GetEquipData(type);
    if (!d || d->type == EQUIP_NONE) return false;
    return !d->twoHanded && !d->isRanged && d->category == EQUIP_CAT_WEAPON;
}

bool IsDualWielding(const GameWorld* game) {
    if (!game) return false;
    EquipType mainWeapon = game->equipped[EQUIP_SLOT_WEAPON];
    EquipType offWeapon = game->equipped[EQUIP_SLOT_OFF_HAND];
    if (mainWeapon == EQUIP_NONE || offWeapon == EQUIP_NONE) return false;
    const EquipData* md = GetEquipData(mainWeapon);
    if (!md || md->twoHanded || md->isRanged) return false;
    return IsWeaponDualWieldable(offWeapon);
}

bool AddEquipToInventory(GameWorld* game, EquipType type) {
    if (type == EQUIP_NONE) { TraceLog(LOG_WARNING, "AddEquipToInventory: EQUIP_NONE"); return false; }
    if (type >= EQUIP_COUNT) { TraceLog(LOG_WARNING, "AddEquipToInventory: type out of range [type=%d]", (int)type); return false; }
        if (game->equipInventoryCount >= MAX_INVENTORY_SLOTS) { TraceLog(LOG_WARNING, "AddEquipToInventory: inventory full [count=%d]", game->equipInventoryCount); return false; }
    game->equipInventory[game->equipInventoryCount++] = type;
    return true;
}

bool RemoveEquipFromInventory(GameWorld* game, int slot) {
        if (slot < 0 || slot >= game->equipInventoryCount) { TraceLog(LOG_WARNING, "RemoveEquipFromInventory: slot out of range [slot=%d count=%d]", slot, game->equipInventoryCount); return false; }
    for (int i = slot; i < game->equipInventoryCount - 1; i++)
        game->equipInventory[i] = game->equipInventory[i + 1];
    game->equipInventoryCount--;
    return true;
}
