#include "game.h"
#include "ui/combat_log.h"
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
    "resources/sprites/items/health_potions/big_health_potion.png",
    "resources/sprites/items/health_potions/large_health_potion.png"
};

// ---- Equipment data table --------------------------------------------------
static const EquipData EQUIP_TABLE[EQUIP_COUNT] = {
    // NONE
    { EQUIP_NONE,           "", "",                             EQUIP_CAT_ARMOR,    EQUIP_SLOT_HEAD,     0, 0, 0 },

    // Armor - Head
    { EQUIP_LEATHER_CAP,    "Leather Cap",     "A simple leather cap.\n+1 DEF, +2 HP",       EQUIP_CAT_ARMOR, EQUIP_SLOT_HEAD, 0, 1, 2 },
    { EQUIP_IRON_HELM,      "Iron Helm",       "An iron helmet forged for battle.\n+2 DEF, +4 HP", EQUIP_CAT_ARMOR, EQUIP_SLOT_HEAD, 0, 2, 4 },
    { EQUIP_STEEL_HELM,     "Steel Helm",      "A sturdy steel helm.\n+3 DEF, +6 HP",         EQUIP_CAT_ARMOR, EQUIP_SLOT_HEAD, 0, 3, 6 },

    // Armor - Chest
    { EQUIP_LEATHER_VEST,   "Leather Vest",    "A flexible leather vest.\n+2 DEF, +4 HP",     EQUIP_CAT_ARMOR, EQUIP_SLOT_CHEST, 0, 2, 4 },
    { EQUIP_CHAIN_MAIL,     "Chain Mail",      "Interlocking rings of steel.\n+4 DEF, +8 HP", EQUIP_CAT_ARMOR, EQUIP_SLOT_CHEST, 0, 4, 8 },
    { EQUIP_PLATE_MAIL,     "Plate Mail",      "Full plate armour.\n+6 DEF, +12 HP",          EQUIP_CAT_ARMOR, EQUIP_SLOT_CHEST, 0, 6, 12 },

    // Weapons
    { EQUIP_SURVIVAL_KNIFE,"Survival Knife",   "A trusty blade for the dungeon.\n+2 ATK",     EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 2, 0, 0 },
    { EQUIP_IRON_SWORD,     "Iron Sword",      "A well-balanced iron blade.\n+5 ATK",        EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 5, 0, 0 },
    { EQUIP_STEEL_SWORD,    "Steel Sword",     "A razor-sharp steel longsword.\n+8 ATK",     EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 8, 0, 0 },
    { EQUIP_WAR_HAMMER,     "War Hammer",      "A heavy hammer that crushes armor.\n+10 ATK, +1 DEF", EQUIP_CAT_WEAPON, EQUIP_SLOT_WEAPON, 10, 1, 0 },

    // Off-hand
    { EQUIP_WOODEN_SHIELD,  "Wooden Shield",   "A light wooden shield.\n+2 DEF",             EQUIP_CAT_ARMOR, EQUIP_SLOT_OFF_HAND, 0, 2, 0 },
    { EQUIP_IRON_SHIELD,    "Iron Shield",     "A sturdy iron shield.\n+4 DEF",               EQUIP_CAT_ARMOR, EQUIP_SLOT_OFF_HAND, 0, 4, 0 },
    { EQUIP_STEEL_SHIELD,   "Steel Shield",    "A heavy steel tower shield.\n+6 DEF",        EQUIP_CAT_ARMOR, EQUIP_SLOT_OFF_HAND, 0, 6, 0 },

    // Accessories
    { EQUIP_RING_OF_STRENGTH,  "Ring of Strength",   "A ring pulsing with power.\n+3 ATK",           EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 3, 0, 0 },
    { EQUIP_AMULET_OF_WARDING, "Amulet of Warding",  "An enchanted amulet.\n+2 DEF, +8 HP",           EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 2, 8 },
    { EQUIP_BOOTS_OF_SWIFTNESS,"Boots of Swiftness","Light boots that aid movement.\n+1 ATK, +1 DEF, +4 HP", EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 1, 1, 4 },
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
        int heal = (game->player.ent.maxHp * healPercent) / 100;
        game->player.ent.hp += heal;
        if (game->player.ent.hp > game->player.ent.maxHp)
            game->player.ent.hp = game->player.ent.maxHp;
        CombatLog_Add(&game->combatLog, BLACK, "Used %s - restores %d%% (%d HP)!", GetItemName(s->type), healPercent, heal);
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

void LoadPotionTextures(Game* game) {
    for (int i = 1; i < ITEM_COUNT; i++) {
        if (ITEM_SPRITES[i][0])
            game->potionTextures[i - 1] = LoadTexture(ITEM_SPRITES[i]);
    }
    game->texUiFrame = LoadTexture("resources/sprites/ui/UI_Flat_Frame01a.png");
    game->texUiSlot  = LoadTexture("resources/sprites/ui/UI_Flat_FrameSlot01b.png");
}

void UnloadPotionTextures(Game* game) {
    for (int i = 0; i < 3; i++) {
        if (game->potionTextures[i].id > 0)
            UnloadTexture(game->potionTextures[i]);
    }
    if (game->texUiFrame.id > 0) UnloadTexture(game->texUiFrame);
    if (game->texUiSlot.id > 0)  UnloadTexture(game->texUiSlot);
}

// ---- Equipment management --------------------------------------------------
void EquipItem(Game* game, EquipType type) {
    if (type == EQUIP_NONE) return;
    const EquipData* data = GetEquipData(type);
    if (!data) return;

    int slotIdx = (int)data->slot;

    if (game->equipped[slotIdx] != EQUIP_NONE) {
        UnequipSlot(game, data->slot);
    }

    game->player.ent.attack   += data->bonusAttack;
    game->player.ent.defense  += data->bonusDefense;
    game->player.ent.maxHp    += data->bonusMaxHp;
    game->player.ent.hp       += data->bonusMaxHp;

    game->equipped[slotIdx] = type;

    CombatLog_Add(&game->combatLog, BLACK, "Equipped %s", data->name);
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
            game->player.ent.maxHp    -= old->bonusMaxHp;
            if (game->player.ent.hp > game->player.ent.maxHp)
                game->player.ent.hp = game->player.ent.maxHp;
        }
        game->equipped[slotIdx] = EQUIP_NONE;
    }

    game->player.ent.attack   += data->bonusAttack;
    game->player.ent.defense  += data->bonusDefense;
    game->player.ent.maxHp    += data->bonusMaxHp;
    game->player.ent.hp       += data->bonusMaxHp;

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
    game->player.ent.maxHp    -= data->bonusMaxHp;
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

        Color bg = (t == (int)game->inventoryTab)
            ? (Color){ 50, 50, 70, 255 }
            : (Color){ 30, 30, 40, 255 };
        DrawRectangle(tx, ty, tabW, tabH, bg);

        if (t == (int)game->inventoryTab)
            DrawRectangle(tx, ty, tabW, (int)(2 * scale), YELLOW);

        int fontSize = (int)(16 * scale);
        int tw = MeasureText(TAB_LABELS[t], fontSize);
        DrawText(TAB_LABELS[t], tx + (tabW - tw) / 2, ty + (int)(4 * scale), fontSize,
                 (t == (int)game->inventoryTab) ? YELLOW : LIGHTGRAY);
    }
    DrawLine(ix, iy + tabH, ix + iw, iy + tabH, LIGHTGRAY);
}

// ---- Draw a multi-line description wrapped at a given width -----------------
static void DrawDescription(const char* desc, int x, int y, int maxWidth, int fontSize, Color color) {
    char buf[512];
    strncpy(buf, desc, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* line = buf;
    char* nl;
    while ((nl = strchr(line, '\n')) != NULL) {
        *nl = '\0';
        DrawText(line, x, y, fontSize, color);
        y += fontSize + 4;
        line = nl + 1;
    }
    if (*line) {
        DrawText(line, x, y, fontSize, color);
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

// Get the equip type at the given combined inventory index (or EQUIP_NONE if it's a potion)
static EquipType GetInventoryEquipType(const Game* game, int i) {
    if (!IsInventorySlotEquip(game, i)) return EQUIP_NONE;
    return game->equipInventory[i - game->inventorySlotCount];
}

static void DrawInventoryTab(const Game* game, int ix, int iy, int iw, int ih) {
    float scale = GetUIScale();
    int tabH = (int)(24 * scale);
    int lx = ix + (int)(16 * scale);
    int ly = iy + tabH + (int)(12 * scale);
    int lw = (int)(280 * scale);
    int total = InventoryTabTotalCount(game);

    int titleSize = (int)(20 * scale);
    DrawText("INVENTORY", lx, ly, titleSize, YELLOW);
    ly += (int)(30 * scale);

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
            DrawText(line, lx, ly, textSize, c);
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
                snprintf(line, sizeof(line), "[%s] %s",
                         EQUIP_SLOT_LABELS[(int)d->slot], d->name);
                if (idx == game->inventorySelection)
                    DrawText(">", lx - (int)(18 * scale), ly, (int)(16 * scale), YELLOW);
                int textSize = (int)(16 * scale);
                DrawText(line, lx, ly, textSize, c);
                ly += (int)(22 * scale);
                lineCount++;
            }
        }
    }

    int rx = ix + lw + 24;
    int rw = iw - lw - 40;
    int rtop = iy + tabH + 40;
    int rh = ih - tabH - 70;
    int rcx = rx + rw / 2;

    if (total > 0) {
        if (game->texUiSlot.id > 0)
            Draw9Slice(game->texUiSlot, (Rectangle){ (float)rx, (float)rtop, (float)rw, (float)rh }, 8, 8, 8, 8);
        else
            DrawRectangleLines(rx, rtop, rw, rh, (Color){ 60, 60, 80, 255 });

        int infoY = rtop + (int)(14 * scale);

        // Determine what's selected
        if (IsInventorySlotEquip(game, game->inventorySelection)) {
            // Equipment item selected
            EquipType eType = GetInventoryEquipType(game, game->inventorySelection);
            const EquipData* d = GetEquipData(eType);
            if (d) {
                int titleSize = (int)(16 * scale);
                int tw = MeasureText(d->name, titleSize);
                DrawText(d->name, rcx - tw / 2, infoY, titleSize, YELLOW);
                infoY += (int)(24 * scale);

                int descSize = (int)(14 * scale);
                const char* desc = d->description;
                DrawDescription(desc, rx + (int)(10 * scale), infoY, rw - (int)(20 * scale), descSize, DARKGRAY);
                 
                // Count lines for spacing
                int lines = 1;
                for (const char* p = desc; *p; p++) if (*p == '\n') lines++;
                infoY += lines * (descSize + 4) + (int)(4 * scale);

                infoY += (int)(40 * scale); // spacing

                if (d->bonusAttack > 0) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "ATK+%d", d->bonusAttack);
                    int statSize = (int)(14 * scale);
                    DrawText(buf, rx + (int)(10 * scale), infoY, statSize, GREEN);
                    infoY += (int)(18 * scale);
                }
                if (d->bonusDefense > 0) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "DEF+%d", d->bonusDefense);
                    int statSize = (int)(14 * scale);
                    DrawText(buf, rx + (int)(10 * scale), infoY, statSize, (Color){ 100, 150, 255, 255 });
                    infoY += (int)(18 * scale);
                }
                if (d->bonusMaxHp > 0) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "HP+%d", d->bonusMaxHp);
                    int statSize = (int)(14 * scale);
                    DrawText(buf, rx + (int)(10 * scale), infoY, statSize, RED);
                    infoY += (int)(18 * scale);
                }
            }
        } else if (game->inventorySlotCount > 0) {
            // Potion selected
            ItemType selType = game->inventory[game->inventorySelection].type;

            const char* iname = GetItemName(selType);
            int titleSize = (int)(16 * scale);
            int tw = MeasureText(iname, titleSize);
            DrawText(iname, rcx - tw / 2, infoY, titleSize, YELLOW);
            infoY += (int)(24 * scale);

            const char* desc = GetItemDescription(selType);
            int descSize = (int)(14 * scale);
            DrawDescription(desc, rx + (int)(10 * scale), infoY, rw - (int)(20 * scale), descSize, DARKGRAY);
            int lines = 1;
            for (const char* p = desc; *p; p++) if (*p == '\n') lines++;
            infoY += lines * (descSize + 4) + (int)(4 * scale);

            char qty[64];
            snprintf(qty, sizeof(qty), "Qty: %d", game->inventory[game->inventorySelection].quantity);
            int textSize = (int)(14 * scale);
            DrawText(qty, rx + (int)(10 * scale), infoY, textSize, BLACK);
            infoY += (int)(22 * scale);

            int heal = GetItemHealAmount(selType);
            if (heal > 0) {
                char healStr[64];
                snprintf(healStr, sizeof(healStr), "Heals %d HP", heal);
                int statSize = (int)(14 * scale);
                DrawText(healStr, rx + (int)(10 * scale), infoY, statSize, (Color){ 0, 90, 0, 255 });
            }
        }
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
        int my = iy + tabH + (int)(40 * scale) + game->inventorySelection * (int)(22 * scale) + (int)(30 * scale) - (int)(2 * scale);
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
    int cx = ix + iw / 2;
    int slotStartY = iy + tabH + (int)(40 * scale);
    int slotH = (int)(36 * scale);
    int slotW = (int)(360 * scale);
    int slotGap = (int)(8 * scale);

    int titleSize = (int)(20 * scale);
    DrawText("EQUIPMENT", ix + (int)(16 * scale), iy + tabH + (int)(12 * scale), titleSize, YELLOW);

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

        int labelSize = (int)(16 * scale);
        DrawText(EQUIP_SLOT_LABELS[s], sx + (int)(10 * scale), sy + (int)(8 * scale), labelSize, labelColor);

        EquipType eType = game->equipped[s];
        if (eType != EQUIP_NONE) {
            const EquipData* data = GetEquipData(eType);
            if (data) {
                char itemStr[128];
                snprintf(itemStr, sizeof(itemStr), "%s", data->name);
                int titleSize = (int)(14 * scale);
                int tw = MeasureText(itemStr, titleSize);
                DrawText(itemStr, sx + slotW - tw - (int)(10 * scale), sy + (int)(5 * scale), titleSize, YELLOW);

                int bonusY = sy + (int)(22 * scale);
                if (data->bonusAttack > 0) {
                    char atkStr[32];
                    snprintf(atkStr, sizeof(atkStr), "ATK+%d", data->bonusAttack);
                    int statSize = (int)(10 * scale);
                    DrawText(atkStr, sx + slotW - tw - (int)(10 * scale), bonusY, statSize, GREEN);
                    bonusY += (int)(12 * scale);
                }
                if (data->bonusDefense > 0) {
                    char defStr[32];
                    snprintf(defStr, sizeof(defStr), "DEF+%d", data->bonusDefense);
                    int statSize = (int)(10 * scale);
                    DrawText(defStr, sx + slotW - tw - (int)(10 * scale), bonusY, statSize, (Color){ 100, 150, 255, 255 });
                    bonusY += (int)(12 * scale);
                }
                if (data->bonusMaxHp > 0) {
                    char hpStr[32];
                    snprintf(hpStr, sizeof(hpStr), "HP+%d", data->bonusMaxHp);
                    int statSize = (int)(10 * scale);
                    DrawText(hpStr, sx + slotW - tw - (int)(10 * scale), bonusY, statSize, RED);
                }
            }
        } else {
            int emptySize = (int)(14 * scale);
            DrawText("(empty)", sx + slotW - (int)(80 * scale), sy + (int)(8 * scale), emptySize, (Color){ 80, 80, 80, 255 });
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
    int sx = ix + (int)(40 * scale);
    int sy = iy + tabH + (int)(40 * scale);
    int gap = (int)(22 * scale);

    int titleSize = (int)(20 * scale);
    DrawText("STATS", ix + (int)(16 * scale), iy + tabH + (int)(12 * scale), titleSize, YELLOW);

    const Entity* p = &game->player.ent;

    char buf[128];

    int textSize = (int)(16 * scale);
    snprintf(buf, sizeof(buf), "Name:     %s", p->name);
    DrawText(buf, sx, sy, textSize, BLACK); sy += gap;

    snprintf(buf, sizeof(buf), "Level:    %d", p->level);
    DrawText(buf, sx, sy, textSize, BLACK); sy += gap;

    snprintf(buf, sizeof(buf), "HP:       %d / %d", p->hp, p->maxHp);
    DrawText(buf, sx, sy, textSize, BLACK); sy += gap;

    int baseAtk = p->attack;
    int baseDef = p->defense;
    for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
        const EquipData* d = GetEquipData(game->equipped[i]);
        if (d) {
            baseAtk   -= d->bonusAttack;
            baseDef   -= d->bonusDefense;
        }
    }

    snprintf(buf, sizeof(buf), "Attack:   %d (base %d)", p->attack, baseAtk);
    DrawText(buf, sx, sy, textSize, BLACK); sy += gap;

    snprintf(buf, sizeof(buf), "Defense:  %d (base %d)", p->defense, baseDef);
    DrawText(buf, sx, sy, textSize, BLACK); sy += gap;

    sy += (int)(8 * scale);

    snprintf(buf, sizeof(buf), "Exp:      %d / %d", game->player.exp, game->player.expToNext);
    DrawText(buf, sx, sy, textSize, BLACK); sy += gap;

    snprintf(buf, sizeof(buf), "Floor:    %d / %d", game->currentFloor, game->maxFloors);
    DrawText(buf, sx, sy, textSize, BLACK); sy += gap;

    int footerSize = (int)(14 * scale);
    DrawText("< Q / E to switch tabs >", ix + (int)(16 * scale), iy + ih - (int)(28 * scale), footerSize, DARKGRAY);
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

    // Footer
    if (game->inventoryTab == INV_TAB_INVENTORY) {
        char footer[128];
        if (game->inventorySlotCount == 0) {
            snprintf(footer, sizeof(footer), "I / ESC to close");
        } else if (game->invSubState == INV_BROWSE) {
            snprintf(footer, sizeof(footer), "ENTER to select action  |  I / ESC to close");
        } else {
            snprintf(footer, sizeof(footer), "UP/DOWN to choose  |  ENTER to confirm  |  ESC to go back");
        }
        int footerSize = (int)(14 * scale);
        DrawText(footer, ix + (int)(16 * scale), iy + ih - (int)(28 * scale), footerSize, DARKGRAY);
    }
}