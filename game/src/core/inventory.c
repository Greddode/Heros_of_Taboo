#include "game.h"
#include "ui/combat_log.h"
#include "ui/inspector.h"
#include "resources.h"
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

static Texture2D s_equipSprites[EQUIP_COUNT]; // one per EquipType, [0] = empty

// ---- Equipment data table --------------------------------------------------
static const EquipData EQUIP_TABLE[EQUIP_COUNT] = {
    // NONE
    { EQUIP_NONE,           "", "", NULL,                            EQUIP_CAT_ARMOR,    EQUIP_SLOT_HEAD,     0, 0, 0, 0,0,0,0,0, false },

    // Armor - Head
    { EQUIP_LEATHER_CAP,    "Leather Cap",     "A simple leather cap.\n+1 DEF, +1 CON",       "resources/sprites/items/equipment/armors/leather_cap.png",    EQUIP_CAT_ARMOR, EQUIP_SLOT_HEAD, 0, 1, 0, 0,0,0,1,0, false },
    { EQUIP_IRON_HELM,      "Iron Helm",       "An iron helmet forged for battle.\n+2 DEF, +2 CON", "resources/sprites/items/equipment/armors/iron_helm.png",      EQUIP_CAT_ARMOR, EQUIP_SLOT_HEAD, 0, 2, 0, 0,0,0,2,0, false },
    { EQUIP_STEEL_HELM,     "Steel Helm",      "A sturdy steel helm.\n+3 DEF, +3 CON",         "resources/sprites/items/equipment/armors/steel_helm.png",     EQUIP_CAT_ARMOR, EQUIP_SLOT_HEAD, 0, 3, 0, 0,0,0,3,0, false },

    // Armor - Chest
    { EQUIP_LEATHER_VEST,   "Leather Vest",    "A flexible leather vest.\n+2 DEF, +2 CON",     "resources/sprites/items/equipment/armors/leather_vest.png",   EQUIP_CAT_ARMOR, EQUIP_SLOT_CHEST, 0, 2, 0, 0,0,0,2,0, false },
    { EQUIP_CHAIN_MAIL,     "Chain Mail",      "Interlocking rings of steel.\n+4 DEF, +4 CON", "resources/sprites/items/equipment/armors/chain_mail.png",     EQUIP_CAT_ARMOR, EQUIP_SLOT_CHEST, 0, 4, 0, 0,0,0,4,0, false },
    { EQUIP_PLATE_MAIL,     "Plate Mail",      "Full plate armour.\n+6 DEF, +6 CON",          "resources/sprites/items/equipment/armors/plate_mail.png",     EQUIP_CAT_ARMOR, EQUIP_SLOT_CHEST, 0, 6, 0, 0,0,0,6,0, false },

    // Weapons
    { EQUIP_SURVIVAL_KNIFE,"Survival Knife",   "A trusty blade for the dungeon.\n+2 ATK",     "resources/sprites/items/equipment/weapons/survival_knife.png", EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 2, 0, 0, 0,0,0,0,0, false },
    { EQUIP_DAGGER,         "Dagger",          "A swift dagger.\n+3 ATK, +1 DEX",              "resources/sprites/items/equipment/weapons/dagger.png",         EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 3, 0, 0, 0,1,0,0,0, false },
    { EQUIP_IRON_SWORD,     "Iron Sword",      "A well-balanced iron blade.\n+5 ATK",        "resources/sprites/items/equipment/weapons/iron_sword.png",    EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 5, 0, 0, 0,0,0,0,0, false },
    { EQUIP_STEEL_SWORD,    "Steel Sword",     "A razor-sharp steel longsword.\n+8 ATK",     "resources/sprites/items/equipment/weapons/steel_sword.png",   EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 8, 0, 0, 0,0,0,0,0, false },
    { EQUIP_WAR_HAMMER,     "War Hammer",      "A massive two-handed hammer.\n+10 ATK, +1 DEF, blocks off-hand", "resources/sprites/items/equipment/weapons/warhammer.png", EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 10, 1, 0, 0,0,0,0,0, true },

    // Off-hand
    { EQUIP_WOODEN_SHIELD,  "Wooden Shield",   "A light wooden shield.\n+2 DEF",             "resources/sprites/items/equipment/armors/wooden_shield.png",  EQUIP_CAT_ARMOR, EQUIP_SLOT_OFF_HAND, 0, 2, 0, 0,0,0,0,0, false },
    { EQUIP_IRON_SHIELD,    "Iron Shield",     "A sturdy iron shield.\n+4 DEF",               "resources/sprites/items/equipment/armors/iron_shield.png",    EQUIP_CAT_ARMOR, EQUIP_SLOT_OFF_HAND, 0, 4, 0, 0,0,0,0,0, false },
    { EQUIP_STEEL_SHIELD,   "Steel Shield",    "A heavy steel tower shield.\n+6 DEF",        "resources/sprites/items/equipment/armors/steel_shield.png",   EQUIP_CAT_ARMOR, EQUIP_SLOT_OFF_HAND, 0, 6, 0, 0,0,0,0,0, false },

    // Accessories
    { EQUIP_RING_OF_STRENGTH,  "Ring of Strength",   "A ring pulsing with power.\n+3 STR",           "resources/sprites/items/equipment/accessories/ring_of_strength.png", EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 3,0,0,0,0, false },
    { EQUIP_AMULET_OF_WARDING, "Amulet of Warding",  "An enchanted amulet.\n+2 MGC, +2 CON",           "resources/sprites/items/equipment/accessories/amulet_of_warding.png", EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,0,2,2,0, false },
    { EQUIP_BOOTS_OF_SWIFTNESS,"Boots of Swiftness","Light boots that aid movement.\n+3 DEX, +1 CON", "resources/sprites/items/equipment/accessories/boots_of_swiftness.png", EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,3,0,1,0, false },
    { EQUIP_RING_OF_THE_HAWK,  "Ring of the Hawk",   "A hawk's eye in a ring.\n+4 DEX",               "resources/sprites/items/equipment/accessories/ring_of_hawk.png",     EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,4,0,0,0, false },
    { EQUIP_SAGES_PENDANT,     "Sage's Pendant",     "The wisdom of ages.\n+4 MGC",                    "resources/sprites/items/equipment/accessories/sage_pendant.png",     EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,0,4,0,0, false },
    { EQUIP_LUCKY_CHARM,       "Lucky Charm",        "A charm of fortune.\n+6 LCK",                    "resources/sprites/items/equipment/accessories/lucky_charm.png",      EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 0, 0, 0,0,0,0,6, false },
    { EQUIP_BERSERKER_BAND,    "Berserker Band",     "Strength at a cost.\n+5 STR, -2 DEF",            "resources/sprites/items/equipment/accessories/fallback_ring.png",    EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, -2, 0, 5,0,0,0,0, false },
};

// ---- Helpers ---------------------------------------------------------------
const EquipData* GetEquipData(EquipType type) {
    if (type < 0 || type >= EQUIP_COUNT) return NULL;
    return &EQUIP_TABLE[type];
}

const char* GetItemName(ItemType type) {
    if (type < 0 || type >= ITEM_COUNT) return "";
    return ITEM_NAMES[type];
}

int GetItemHealAmount(ItemType type) {
    if (type < 0 || type >= ITEM_COUNT) return 0;
    return ITEM_HEALS[type];
}

const char* GetItemDescription(ItemType type) {
    if (type < 0 || type >= ITEM_COUNT) return "";
    return ITEM_DESCS[type];
}

// ---- Inventory (potions) ---------------------------------------------------
bool InventoryAdd(Game* game, ItemType type) {
    if (type == ITEM_NONE) return false;
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

bool InventoryUse(Game* game, int slot) {
    if (slot < 0 || slot >= game->inventorySlotCount) return false;
    InventorySlot* s = &game->inventory[slot];
    if (s->type == ITEM_NONE || s->quantity <= 0) return false;

    int healPercent = GetItemHealAmount(s->type);
    if (healPercent > 0) {
        // Base heal percentage scaled by MGC: base% * (1 + MGC * 0.02)
        float intMult = 1.0f + (float)game->player.ent.intel * 0.02f;
        int heal = (game->player.ent.maxHp * healPercent * (int)(intMult * 100)) / 10000;
        if (heal < 1) heal = 1;
        game->player.ent.hp += heal;
        if (game->player.ent.hp > game->player.ent.maxHp)
            game->player.ent.hp = game->player.ent.maxHp;
        CombatLog_Add(&game->combatLog, BLACK, "Used %s - restores %d%% (+%d HP, MGCx%.0f%%)!",
                      GetItemName(s->type), healPercent, heal, intMult * 100);
    }

    s->quantity--;
    if (s->quantity <= 0) {
        for (int i = slot; i < game->inventorySlotCount - 1; i++)
            game->inventory[i] = game->inventory[i + 1];
        game->inventorySlotCount--;
        if (game->inventorySelection >= game->inventorySlotCount)
            game->inventorySelection = game->inventorySlotCount - 1;
    }
    return true;
}

Texture2D GetEquipSprite(EquipType type) {
    if (type < 0 || type >= EQUIP_COUNT) return (Texture2D){ 0 };
    return s_equipSprites[type];
}

void LoadPotionTextures(Game* game) {
    for (int i = 1; i < ITEM_COUNT; i++) {
        if (ITEM_SPRITES[i][0]) {
            Texture2D* t = Resources_LoadTexture(ITEM_SPRITES[i]);
            if (t) game->potionTextures[i - 1] = *t;
        }
    }
    {
        Texture2D* t = Resources_LoadTexture("resources/sprites/ui/UI_Flat_Frame01a.png");
        if (t) game->texUiFrame = *t;
    }
    {
        Texture2D* t = Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameSlot01b.png");
        if (t) game->texUiSlot = *t;
    }
    {
        Texture2D* t = Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameMarker01a.png");
        if (t) game->texUiMarker = *t;
    }

    // Load equipment sprites
    for (int i = 0; i < EQUIP_COUNT; i++) {
        s_equipSprites[i] = (Texture2D){ 0 };
        const EquipData* d = GetEquipData((EquipType)i);
        if (d && d->spritePath) {
            Texture2D* t = Resources_LoadTexture(d->spritePath);
            if (t) {
                s_equipSprites[i] = *t;
                if (s_equipSprites[i].id == 0) {
                    TraceLog(LOG_WARNING, "Could not load equip sprite: %s", d->spritePath);
                }
            }
        }
    }

    {
        Texture2D* t = Resources_LoadTexture("resources/sprites/items/loot.png");
        if (t) game->texLoot = *t;
    }
}

void UnloadPotionTextures(Game* game) {
    // Handled by Resources_UnloadAll
}

// ---- Equipment management --------------------------------------------------
bool EquipItem(Game* game, EquipType type) {
    if (type == EQUIP_NONE) return false;
    const EquipData* data = GetEquipData(type);
    if (!data) return false;

    int slotIdx = (int)data->slot;

    // Block off-hand equipping if a two-handed weapon is equipped
    if (data->slot == EQUIP_SLOT_OFF_HAND && IsTwoHandedEquipped(game)) {
        CombatLog_Add(&game->combatLog, BLACK, "Cannot equip off-hand with a two-handed weapon!");
        return false;
    }

    if (game->equipped[slotIdx] != EQUIP_NONE) {
        UnequipSlot(game, data->slot);
    }

    // If equipping a two-handed weapon, also unequip off-hand
    if (data->twoHanded && game->equipped[EQUIP_SLOT_OFF_HAND] != EQUIP_NONE) {
        UnequipSlot(game, EQUIP_SLOT_OFF_HAND);
    }

    game->player.ent.attack   += data->bonusAttack;
    game->player.ent.defense  += data->bonusDefense;
    game->player.ent.str      += data->bonusStr;
    game->player.ent.dex      += data->bonusDex;
    game->player.ent.intel    += data->bonusInt;
    game->player.ent.con      += data->bonusCon;
    game->player.ent.lck      += data->bonusLck;
    // Recalculate derived HP from CON
    int oldMaxHp = game->player.ent.maxHp;
    game->player.ent.maxHp    = 30 + game->player.ent.con * 5;
    game->player.ent.hp      += (game->player.ent.maxHp - oldMaxHp);

    game->equipped[slotIdx] = type;

    CombatLog_Add(&game->combatLog, BLACK, "Equipped %s", data->name);
    return true;
}

// Same as EquipItem but doesn't log to combat log (for initial game setup)
void EquipItemSilent(Game* game, EquipType type) {
    if (type == EQUIP_NONE) return;
    const EquipData* data = GetEquipData(type);
    if (!data) return;

    int slotIdx = (int)data->slot;

    // Remove existing item from that slot silently, reverting its bonuses
    if (game->equipped[slotIdx] != EQUIP_NONE) {
        const EquipData* old = GetEquipData(game->equipped[slotIdx]);
        if (old) {
            game->player.ent.attack   -= old->bonusAttack;
            game->player.ent.defense  -= old->bonusDefense;
            game->player.ent.str      -= old->bonusStr;
            game->player.ent.dex      -= old->bonusDex;
            game->player.ent.intel    -= old->bonusInt;
            game->player.ent.con      -= old->bonusCon;
            game->player.ent.lck      -= old->bonusLck;
        }
        game->equipped[slotIdx] = EQUIP_NONE;
    }

    game->player.ent.attack   += data->bonusAttack;
    game->player.ent.defense  += data->bonusDefense;
    game->player.ent.str      += data->bonusStr;
    game->player.ent.dex      += data->bonusDex;
    game->player.ent.intel    += data->bonusInt;
    game->player.ent.con      += data->bonusCon;
    game->player.ent.lck      += data->bonusLck;

    // Recalculate derived HP from CON
    game->player.ent.maxHp    = 30 + game->player.ent.con * 5;
    if (game->player.ent.hp > game->player.ent.maxHp)
        game->player.ent.hp = game->player.ent.maxHp;

    game->equipped[slotIdx] = type;
}

void UnequipSlot(Game* game, EquipSlot slot) {
    int slotIdx = (int)slot;
    EquipType oldType = game->equipped[slotIdx];
    if (oldType == EQUIP_NONE) return;

    const EquipData* data = GetEquipData(oldType);
    if (!data) return;

    game->player.ent.attack   -= data->bonusAttack;
    game->player.ent.defense  -= data->bonusDefense;
    game->player.ent.str      -= data->bonusStr;
    game->player.ent.dex      -= data->bonusDex;
    game->player.ent.intel    -= data->bonusInt;
    game->player.ent.con      -= data->bonusCon;
    game->player.ent.lck      -= data->bonusLck;

    // Recalculate derived HP from CON
    game->player.ent.maxHp    = 30 + game->player.ent.con * 5;
    if (game->player.ent.hp > game->player.ent.maxHp)
        game->player.ent.hp = game->player.ent.maxHp;

    game->equipped[slotIdx] = EQUIP_NONE;

    // Move item to equipInventory instead of discarding it
    AddEquipToInventory(game, oldType);

    CombatLog_Add(&game->combatLog, BLACK, "Unequipped %s", data->name);
}

bool IsEquipSlotOccupied(const Game* game, EquipSlot slot) {
    return game->equipped[(int)slot] != EQUIP_NONE;
}

bool IsTwoHandedEquipped(const Game* game) {
    EquipType weapon = game->equipped[EQUIP_SLOT_WEAPON];
    if (weapon == EQUIP_NONE) return false;
    const EquipData* data = GetEquipData(weapon);
    return data && data->twoHanded;
}

bool AddEquipToInventory(Game* game, EquipType type) {
    if (type == EQUIP_NONE) return false;
    if (game->equipInventoryCount >= MAX_INVENTORY_SLOTS) return false;
    game->equipInventory[game->equipInventoryCount++] = type;
    return true;
}

bool RemoveEquipFromInventory(Game* game, int slot) {
    if (slot < 0 || slot >= game->equipInventoryCount) return false;
    for (int i = slot; i < game->equipInventoryCount - 1; i++)
        game->equipInventory[i] = game->equipInventory[i + 1];
    game->equipInventoryCount--;
    return true;
}

// ---- Tab rendering ---------------------------------------------------------

static const char* TAB_LABELS[INV_TAB_COUNT] = {
    "Inventory",
    "Equipment",
    "Stats"
};

static const char* EQUIP_SLOT_LABELS[EQUIP_SLOT_COUNT] = {
    "Head",
    "Chest",
    "Weapon",
    "Off-hand",
    "Accessory"
};

// Scaling helper function - defined in game.h
float GetUIScale(void);

// ---- Draw the tab bar at the top of the inventory panel --------------------
static void DrawTabBar(const Game* game, int ix, int iy, int iw) {
    float scale = GetUIScale();
    int tabH = (int)(24 * scale);
    int tabW = iw / INV_TAB_COUNT;

    for (int t = 0; t < INV_TAB_COUNT; t++) {
        int tx = ix + t * tabW;
        int ty = iy;

        // Draw marker texture as tab background (9-slice with flat bottom)
        if (game->texUiMarker.id > 0) {
            Draw9Slice(game->texUiMarker, (Rectangle){ (float)tx, (float)ty, (float)tabW, (float)tabH },
                       (int)(8 * scale), (int)(8 * scale), (int)(8 * scale), 0);
        }

        if (t == (int)game->inventoryTab)
            DrawRectangle(tx, ty, tabW, (int)(2 * scale), YELLOW);

        int fontSize = (int)(16 * scale);
        int tw = MeasureText(TAB_LABELS[t], fontSize);
        DrawText(TAB_LABELS[t], tx + (tabW - tw) / 2, ty + (int)(4 * scale), fontSize,
                 BLACK);
    }
}

// ---- Inventory tab content --------------------------------------------------
// Total items shown = potion inventory + equip inventory
static int InventoryTabTotalCount(const Game* game) {
    return game->inventorySlotCount + game->equipInventoryCount;
}

// Returns true if the slot at index i is an equipment item
static bool IsInventorySlotEquip(const Game* game, int i) {
    return i >= game->inventorySlotCount;
}

static void DrawInventoryTab(const Game* game, int ix, int iy, int iw, int ih) {
    float scale = GetUIScale();
    int tabH = (int)(24 * scale);
    int scroll = game->invScrollOffset;
    int lx = ix + (int)(16 * scale);
    int ly = iy + tabH + (int)(42 * scale) - scroll;
    int lw = (int)(280 * scale);
    int total = InventoryTabTotalCount(game);

    if (total == 0) {
        int emptySize = (int)(16 * scale);
        DrawText("(empty)", lx, ly, emptySize, GRAY);
    } else {
        int lineCount = 0;
        // First: potions
        for (int i = 0; i < game->inventorySlotCount; i++) {
            Color c = (i == game->inventorySelection) ? YELLOW : BLACK;
            char line[128];
            snprintf(line, sizeof(line), "%s x%d", GetItemName(game->inventory[i].type), game->inventory[i].quantity);
            if (i == game->inventorySelection)
                DrawText(">", lx - (int)(18 * scale), ly, (int)(16 * scale), YELLOW);
            int textSize = (int)(16 * scale);
            DrawText(line, lx + (int)(2 * scale), ly, textSize, c);
            ly += (int)(22 * scale);
            lineCount++;
        }
        // Then: equipment items
        for (int i = 0; i < game->equipInventoryCount; i++) {
            int idx = game->inventorySlotCount + i;
            Color c = (idx == game->inventorySelection) ? YELLOW : BLACK;
            const EquipData* d = GetEquipData(game->equipInventory[i]);
            if (d) {
                char line[128];
                if (d->twoHanded)
                    snprintf(line, sizeof(line), "[%s] %s (two-handed)",
                             EQUIP_SLOT_LABELS[(int)d->slot], d->name);
                else
                    snprintf(line, sizeof(line), "[%s] %s",
                             EQUIP_SLOT_LABELS[(int)d->slot], d->name);
                if (idx == game->inventorySelection)
                    DrawText(">", lx - (int)(18 * scale), ly, (int)(16 * scale), YELLOW);
                int textSize = (int)(16 * scale);
                DrawText(line, lx + (int)(2 * scale), ly, textSize, c);
                ly += (int)(22 * scale);
                lineCount++;
            }
        }
    }

    int rx = ix + lw + 24;
    int rw = iw - lw - 40;
    int rtop = iy + tabH + (int)(40 * scale);
    int rh = ih - tabH - (int)(70 * scale);

    if (total > 0) {
        if (game->texUiSlot.id > 0)
            Draw9Slice(game->texUiSlot, (Rectangle){ (float)rx, (float)rtop, (float)rw, (float)rh }, 8, 8, 8, 8);
        else
            DrawRectangleLines(rx, rtop, rw, rh, (Color){ 60, 60, 80, 255 });

        Inspector_Render(game, INSPECTOR_ITEM, rx, rtop, rw, rh);
    }

        // Action-menu popup
    if (game->invSubState == INV_ACTION_MENU && total > 0) {
        // Determine actions based on item type
        bool isEquip = IsInventorySlotEquip(game, game->inventorySelection);
        const char* actions[4];
        int actionCount;
        if (isEquip) {
            actions[0] = "Equip";
            actions[1] = "Drop";
            actions[2] = "Back";
            actionCount = 3;
        } else {
            actions[0] = "Use";
            actions[1] = "Drop";
            actions[2] = "Drop All";
            actions[3] = "Back";
            actionCount = 4;
        }

        float scale = GetUIScale();
        int mx = ix + (int)(16 * scale) + lw + (int)(2 * scale);
        int my = iy + tabH + (int)(40 * scale) + game->inventorySelection * (int)(22 * scale) + (int)(30 * scale) - (int)(2 * scale) - scroll;
        if (my + (int)(80 * scale) > iy + ih - (int)(30 * scale)) my = iy + ih - (int)(30 * scale) - (int)(80 * scale);
        if (my < iy + tabH + (int)(40 * scale)) my = iy + tabH + (int)(40 * scale);
        int mw = (int)(140 * scale);
        int mh = actionCount * (int)(26 * scale) + (int)(8 * scale);

        if (game->texUiSlot.id > 0)
            Draw9Slice(game->texUiSlot, (Rectangle){ (float)mx, (float)my, (float)mw, (float)mh }, 8, 8, 8, 8);
        else {
            DrawRectangle(mx, my, mw, mh, (Color){ 20, 20, 30, 240 });
            DrawRectangleLines(mx, my, mw, mh, YELLOW);
        }

        for (int a = 0; a < actionCount; a++) {
            Color ac = (a == game->invActionSelection) ? YELLOW : BLACK;
            if (a == game->invActionSelection)
                DrawText(">", mx + (int)(6 * scale), my + (int)(6 * scale) + a * (int)(26 * scale), (int)(16 * scale), YELLOW);
            DrawText(actions[a], mx + (int)(22 * scale), my + (int)(6 * scale) + a * (int)(26 * scale), (int)(16 * scale), ac);
        }
    }
}

// ---- Equipment tab content --------------------------------------------------
static void DrawEquipmentTab(const Game* game, int ix, int iy, int iw, int ih) {
    float scale = GetUIScale();
    int tabH = (int)(24 * scale);
    int scroll = game->invScrollOffset;
    int cx = ix + iw / 2;
    int slotStartY = iy + tabH + (int)(40 * scale) - scroll;
    int slotH = (int)(36 * scale);
    int slotW = (int)(360 * scale);
    int slotGap = (int)(8 * scale);

    for (int s = 0; s < EQUIP_SLOT_COUNT; s++) {
        int sy = slotStartY + s * (slotH + slotGap);
        int sx = cx - slotW / 2;

        if (game->texUiSlot.id > 0)
            Draw9Slice(game->texUiSlot, (Rectangle){ (float)sx, (float)sy, (float)slotW, (float)slotH }, 8, 8, 8, 8);
        else {
            DrawRectangle(sx, sy, slotW, slotH, (Color){ 30, 30, 50, 255 });
            DrawRectangleLines(sx, sy, slotW, slotH, DARKGRAY);
        }

        Color labelColor = (s == game->inventorySelection) ? YELLOW : LIGHTGRAY;
        if (s == game->inventorySelection)
            DrawText(">", sx - (int)(18 * scale), sy + (int)(8 * scale), (int)(16 * scale), YELLOW);

        EquipType eType = game->equipped[s];

        int labelSize = (int)(16 * scale);
        DrawText(EQUIP_SLOT_LABELS[s], sx + (int)(10 * scale), sy + (int)(8 * scale), labelSize, labelColor);

        if (eType != EQUIP_NONE) {
            const EquipData* data = GetEquipData(eType);
            if (data) {
                char itemStr[128];
                if (data->twoHanded)
                    snprintf(itemStr, sizeof(itemStr), "%s (two-handed)", data->name);
                else
                    snprintf(itemStr, sizeof(itemStr), "%s", data->name);
                int titleSize = (int)(14 * scale);
                int tw = MeasureText(itemStr, titleSize);
                DrawText(itemStr, sx + slotW - tw - (int)(10 * scale), sy + (int)(5 * scale), titleSize, YELLOW);

                int bonusY = sy + (int)(22 * scale);
                char bonusStr[64] = "";
                if (data->bonusAttack > 0) {
                    char tmp[16]; snprintf(tmp, sizeof(tmp), "ATK+%d ", data->bonusAttack);
                    strncat(bonusStr, tmp, sizeof(bonusStr) - strlen(bonusStr) - 1);
                }
                if (data->bonusDefense != 0) {
                    char tmp[16]; snprintf(tmp, sizeof(tmp), "DEF%+d ", data->bonusDefense);
                    strncat(bonusStr, tmp, sizeof(bonusStr) - strlen(bonusStr) - 1);
                }
                if (data->bonusStr != 0) {
                    char tmp[16]; snprintf(tmp, sizeof(tmp), "STR%+d ", data->bonusStr);
                    strncat(bonusStr, tmp, sizeof(bonusStr) - strlen(bonusStr) - 1);
                }
                if (data->bonusDex != 0) {
                    char tmp[16]; snprintf(tmp, sizeof(tmp), "DEX%+d ", data->bonusDex);
                    strncat(bonusStr, tmp, sizeof(bonusStr) - strlen(bonusStr) - 1);
                }
                if (data->bonusInt != 0) {
                    char tmp[16]; snprintf(tmp, sizeof(tmp), "MGC%+d ", data->bonusInt);
                    strncat(bonusStr, tmp, sizeof(bonusStr) - strlen(bonusStr) - 1);
                }
                if (data->bonusCon != 0) {
                    char tmp[16]; snprintf(tmp, sizeof(tmp), "CON%+d ", data->bonusCon);
                    strncat(bonusStr, tmp, sizeof(bonusStr) - strlen(bonusStr) - 1);
                }
                if (data->bonusLck != 0) {
                    char tmp[16]; snprintf(tmp, sizeof(tmp), "LCK%+d ", data->bonusLck);
                    strncat(bonusStr, tmp, sizeof(bonusStr) - strlen(bonusStr) - 1);
                }
                int statSize = (int)(10 * scale);
                DrawText(bonusStr, sx + (int)(10 * scale), bonusY, statSize, BLACK);
            }
        } else {
            int emptySize = (int)(14 * scale);
            if (s == EQUIP_SLOT_OFF_HAND && IsTwoHandedEquipped(game)) {
                DrawText("(blocked)", sx + slotW - (int)(80 * scale), sy + (int)(8 * scale), emptySize, (Color){ 120, 60, 60, 255 });
            } else {
                DrawText("(empty)", sx + slotW - (int)(80 * scale), sy + (int)(8 * scale), emptySize, (Color){ 80, 80, 80, 255 });
            }
        }
    }

    // Equipment action menu
    if (game->invSubState == INV_ACTION_MENU && game->equipped[game->inventorySelection] != EQUIP_NONE) {
        static const char* actions[] = { "Unequip", "Drop", "Back" };
        int selSlot = game->inventorySelection;
        int sy = slotStartY + selSlot * (slotH + slotGap);
        int sx = cx - slotW / 2;
        int mx = sx + slotW + (int)(4 * scale);
        int my = sy;
        if (my + (int)(80 * scale) > iy + ih - (int)(30 * scale)) my = iy + ih - (int)(30 * scale) - (int)(80 * scale);
        int mw = (int)(130 * scale);
        int mh = (int)(90 * scale);

        if (game->texUiSlot.id > 0)
            Draw9Slice(game->texUiSlot, (Rectangle){ (float)mx, (float)my, (float)mw, (float)mh }, 8, 8, 8, 8);
        else {
            DrawRectangle(mx, my, mw, mh, (Color){ 20, 20, 30, 240 });
            DrawRectangleLines(mx, my, mw, mh, YELLOW);
        }

        for (int a = 0; a < 3; a++) {
            Color ac = (a == game->invActionSelection) ? YELLOW : BLACK;
            if (a == game->invActionSelection)
                DrawText(">", mx + (int)(6 * scale), my + (int)(6 * scale) + a * (int)(26 * scale), (int)(16 * scale), YELLOW);
            DrawText(actions[a], mx + (int)(22 * scale), my + (int)(6 * scale) + a * (int)(26 * scale), (int)(16 * scale), ac);
        }
    }

    int footerSize = (int)(14 * scale);
    DrawText("< Q / E to switch tabs >", cx - MeasureText("< Q / E to switch tabs >", footerSize) / 2,
             iy + ih - (int)(28 * scale), footerSize, DARKGRAY);
}

// ---- Stats tab content ------------------------------------------------------
static void DrawStatsTab(const Game* game, int ix, int iy, int iw, int ih) {
    float scale = GetUIScale();
    int tabH = (int)(24 * scale);
    int headerGap = (int)(40 * scale);

    int sx = ix + (int)(40 * scale);
    int sy = iy + tabH + headerGap + (int)(12 * scale);

    int gap = (int)(22 * scale);
    int slotH = (int)(20 * scale);
    int slotW = (int)(260 * scale);

    const Entity* p = &game->player.ent;
    char buf[128];
    int textSize = (int)(16 * scale);

    // Column positions
    int col1x = sx;
    int col2x = sx + (int)(300 * scale);

    int c1y = sy - game->statsScrollCol1;
    int c2y = sy - game->statsScrollCol2;

    int derivedSize = (int)(14 * scale);
    bool isCol1 = (game->statsActiveCol == 0);
    bool isCol2 = (game->statsActiveCol == 1);

    // Helper to draw a slot + line
    #define STAT_SLOT(YY, XX, IS_SEL) do { \
        if (game->texUiSlot.id > 0) \
            Draw9Slice(game->texUiSlot, (Rectangle){ (float)(XX - (int)(4 * scale)), (float)(YY), (float)slotW, (float)slotH }, 8, 8, 8, 8); \
        if (IS_SEL) DrawText(">", (XX) - (int)(18 * scale), (YY), textSize, YELLOW); \
    } while(0)

    static const Color cBlack = {0, 0, 0, 255};
    int lineIdx = 0;

    snprintf(buf, sizeof(buf), "Name:     %s", p->name);
    STAT_SLOT(c1y, col1x, isCol1 && lineIdx == game->statsSelection); DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, cBlack); c1y += gap; lineIdx++;

    snprintf(buf, sizeof(buf), "Level:    %d", p->level);
    STAT_SLOT(c1y, col1x, isCol1 && lineIdx == game->statsSelection); DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, cBlack); c1y += gap; lineIdx++;

    snprintf(buf, sizeof(buf), "HP:       %d / %d", p->hp, p->maxHp);
    STAT_SLOT(c1y, col1x, isCol1 && lineIdx == game->statsSelection); DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, cBlack); c1y += gap; lineIdx++;

    int baseStr = p->str, baseDex = p->dex, baseInt = p->intel, baseCon = p->con, baseLck = p->lck;
    for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
        const EquipData* d = GetEquipData(game->equipped[i]);
        if (d) {
            baseStr -= d->bonusStr;
            baseDex -= d->bonusDex;
            baseInt -= d->bonusInt;
            baseCon -= d->bonusCon;
            baseLck -= d->bonusLck;
        }
    }

    snprintf(buf, sizeof(buf), "STR:      %d (base %d)", p->str, baseStr);
    STAT_SLOT(c1y, col1x, isCol1 && lineIdx == game->statsSelection); DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, cBlack); c1y += gap; lineIdx++;

    snprintf(buf, sizeof(buf), "DEX:      %d (base %d)", p->dex, baseDex);
    STAT_SLOT(c1y, col1x, isCol1 && lineIdx == game->statsSelection); DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, cBlack); c1y += gap; lineIdx++;

    snprintf(buf, sizeof(buf), "MGC:      %d (base %d)", p->intel, baseInt);
    STAT_SLOT(c1y, col1x, isCol1 && lineIdx == game->statsSelection); DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, cBlack); c1y += gap; lineIdx++;

    snprintf(buf, sizeof(buf), "CON:      %d (base %d)", p->con, baseCon);
    STAT_SLOT(c1y, col1x, isCol1 && lineIdx == game->statsSelection); DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, cBlack); c1y += gap; lineIdx++;

    snprintf(buf, sizeof(buf), "LCK:      %d (base %d)", p->lck, baseLck);
    STAT_SLOT(c1y, col1x, isCol1 && lineIdx == game->statsSelection); DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, cBlack); c1y += gap; lineIdx++;

    c1y += (int)(8 * scale);

    int meleeDmg = p->attack + p->str * 2;
    int dodgePct = p->dex * 2;
    if (dodgePct > 60) dodgePct = 60;
    int critPct = p->lck;
    int magicRes = p->intel * 3;

    snprintf(buf, sizeof(buf), "Melee DMG:  %d (ATK %d + STRx2 %d)", meleeDmg, p->attack, p->str * 2);
    STAT_SLOT(c1y, col1x, isCol1 && lineIdx == game->statsSelection); DrawText(buf, col1x + (int)(2 * scale), c1y, derivedSize, cBlack); c1y += gap; lineIdx++;

    snprintf(buf, sizeof(buf), "Dodge:      %d%%", dodgePct);
    STAT_SLOT(c1y, col1x, isCol1 && lineIdx == game->statsSelection); DrawText(buf, col1x + (int)(2 * scale), c1y, derivedSize, cBlack); c1y += gap; lineIdx++;

    snprintf(buf, sizeof(buf), "Crit:       %d%%", critPct);
    STAT_SLOT(c1y, col1x, isCol1 && lineIdx == game->statsSelection); DrawText(buf, col1x + (int)(2 * scale), c1y, derivedSize, cBlack); c1y += gap; lineIdx++;

    snprintf(buf, sizeof(buf), "Magic DEF:  %d", magicRes);
    STAT_SLOT(c1y, col1x, isCol1 && lineIdx == game->statsSelection); DrawText(buf, col1x + (int)(2 * scale), c1y, derivedSize, cBlack); c1y += gap; lineIdx++;

    c1y += (int)(8 * scale);

    snprintf(buf, sizeof(buf), "Exp:      %d / %d", game->player.exp, game->player.expToNext);
    STAT_SLOT(c1y, col1x, isCol1 && lineIdx == game->statsSelection); DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, cBlack); c1y += gap; lineIdx++;

    snprintf(buf, sizeof(buf), "Floor:    %d / %d", game->currentFloor, game->maxFloors);
    STAT_SLOT(c1y, col1x, isCol1 && lineIdx == game->statsSelection); DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, cBlack); c1y += gap; lineIdx++;

    // ---- Column 2: stat allocation ----
    int lineIdx2 = 0;

    if (p->statPoints > 0) {
        snprintf(buf, sizeof(buf), "Unspent: %d pt(s)", p->statPoints);
        int ptsSize = (int)(18 * scale);
        if (game->texUiSlot.id > 0)
            Draw9Slice(game->texUiSlot, (Rectangle){ (float)(col2x - (int)(4 * scale)), (float)c2y, (float)slotW, (float)slotH }, 8, 8, 8, 8);
        DrawText(buf, col2x + (int)(2 * scale), c2y, ptsSize, YELLOW); c2y += gap + (int)(4 * scale); lineIdx2++;

        int allocSize = (int)(15 * scale);
        STAT_SLOT(c2y, col2x, isCol2 && lineIdx2 == game->statsSelection); DrawText("STR (+1)", col2x + (int)(2 * scale), c2y, allocSize, cBlack); c2y += gap; lineIdx2++;
        STAT_SLOT(c2y, col2x, isCol2 && lineIdx2 == game->statsSelection); DrawText("DEX (+1)", col2x + (int)(2 * scale), c2y, allocSize, cBlack); c2y += gap; lineIdx2++;
        STAT_SLOT(c2y, col2x, isCol2 && lineIdx2 == game->statsSelection); DrawText("MGC (+1)", col2x + (int)(2 * scale), c2y, allocSize, cBlack); c2y += gap; lineIdx2++;
        STAT_SLOT(c2y, col2x, isCol2 && lineIdx2 == game->statsSelection); DrawText("CON (+1)", col2x + (int)(2 * scale), c2y, allocSize, cBlack); c2y += gap; lineIdx2++;
        STAT_SLOT(c2y, col2x, isCol2 && lineIdx2 == game->statsSelection); DrawText("LCK (+1)", col2x + (int)(2 * scale), c2y, allocSize, cBlack); c2y += gap; lineIdx2++;
    } else {
        int nsSize = (int)(14 * scale);
        int nsSlotH = slotH + gap;
        if (game->texUiSlot.id > 0)
            Draw9Slice(game->texUiSlot, (Rectangle){ (float)(col2x - (int)(4 * scale)), (float)c2y, (float)slotW, (float)nsSlotH }, 8, 8, 8, 8);
        if (isCol2) DrawText(">", col2x - (int)(18 * scale), c2y + (int)(6 * scale), textSize, YELLOW);
        DrawText("Gain stat points", col2x + (int)(2 * scale), c2y + (int)(2 * scale), nsSize, cBlack);
        DrawText("on level up!", col2x + (int)(2 * scale), c2y + nsSize + (int)(4 * scale), nsSize, cBlack);
    }
}

// ---- Main render entry point -----------------------------------------------
void Inventory_Render(const Game* game) {
    if (game->state != STATE_INVENTORY) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float scale = GetUIScale();
    int iw = (int)(640 * scale);
    int ih = (int)(400 * scale);
    int ix = (sw - iw) / 2;
    int iy = (sh - ih) / 2;
    int tabH = (int)(24 * scale);

    DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 160 });
    if (game->texUiFrame.id > 0)
        Draw9Slice(game->texUiFrame, (Rectangle){ (float)ix, (float)iy, (float)iw, (float)ih }, 16, 16, 16, 16);
    else {
        DrawRectangle(ix, iy, iw, ih, (Color){ 30, 30, 40, 255 });
        DrawRectangleLines(ix, iy, iw, ih, LIGHTGRAY);
    }

    DrawTabBar(game, ix, iy, iw);

    // Draw tab header (outside scissor)
    int titleSize = (int)(20 * scale);
    if (game->inventoryTab == INV_TAB_INVENTORY) {
        DrawText("INVENTORY", ix + (int)(16 * scale), iy + tabH + (int)(12 * scale), titleSize, YELLOW);
    } else if (game->inventoryTab == INV_TAB_EQUIPMENT) {
        DrawText("EQUIPMENT", ix + (int)(16 * scale), iy + tabH + (int)(12 * scale), titleSize, YELLOW);
    } else if (game->inventoryTab == INV_TAB_STATS) {
        DrawText("STATS", ix + (int)(16 * scale), iy + tabH + (int)(12 * scale), titleSize, YELLOW);
    }

    // Scissor for scrollable tab content (leave bottom ~60px for footer hints)
    int contentY = iy + tabH + (int)(40 * scale);
    int contentH = ih - tabH - (int)(100 * scale);
    BeginScissorMode(ix, contentY, iw, contentH);

    switch (game->inventoryTab) {
        case INV_TAB_INVENTORY:
            DrawInventoryTab(game, ix, iy, iw, ih);
            break;
        case INV_TAB_EQUIPMENT:
            DrawEquipmentTab(game, ix, iy, iw, ih);
            break;
        case INV_TAB_STATS:
            DrawStatsTab(game, ix, iy, iw, ih);
            break;
        default:
            break;
    }

    EndScissorMode();

    // Footer hints (drawn after scissor, always visible)
    int footerSize = (int)(14 * scale);
    if (game->inventoryTab == INV_TAB_STATS) {
        DrawText("< Q / E to switch tabs >", ix + (int)(16 * scale), iy + ih - (int)(28 * scale), footerSize, DARKGRAY);
    } else if (game->inventoryTab == INV_TAB_INVENTORY) {
        char footer[128];
        if (game->inventorySlotCount == 0) {
            snprintf(footer, sizeof(footer), "Q / E to switch tabs  |  I / ESC to close");
        } else if (game->invSubState == INV_BROWSE) {
            snprintf(footer, sizeof(footer), "ENTER to select action  |  Q / E to switch tabs  |  I / ESC to close");
        } else {
            snprintf(footer, sizeof(footer), "UP/DOWN to choose  |  ENTER to confirm  |  ESC to go back");
        }
        DrawText(footer, ix + (int)(16 * scale), iy + ih - (int)(28 * scale), footerSize, DARKGRAY);
    } else if (game->inventoryTab == INV_TAB_EQUIPMENT) {
        char footer[128];
        snprintf(footer, sizeof(footer), "Q / E to switch tabs");
        DrawText(footer, ix + (int)(16 * scale), iy + ih - (int)(28 * scale), footerSize, DARKGRAY);
    }
}