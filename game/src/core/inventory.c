#include "game.h"
#include "ui/combat_log.h"
#include <stdio.h>
#include <string.h>

// ---- Potion data -----------------------------------------------------------
static const char* ITEM_NAMES[ITEM_COUNT] = {
    "",
    "Small HP Potion",
    "Big HP Potion",
    "Large HP Potion"
};
static const int ITEM_HEALS[ITEM_COUNT] = {
    0,
    12,
    48,
    128
};
static const char* ITEM_DESCS[ITEM_COUNT] = {
    "",
    "A small vial of red liquid.\nRestores 12 HP.",
    "A hearty draught.\nRestores 48 HP.",
    "A potent elixir.\nRestores 128 HP."
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
    { EQUIP_NONE,           "",                  EQUIP_CAT_ARMOR,    EQUIP_SLOT_HEAD,     0, 0, 0 },

    // Armor - Head
    { EQUIP_LEATHER_CAP,    "Leather Cap",       EQUIP_CAT_ARMOR,    EQUIP_SLOT_HEAD,     0, 1, 2 },
    { EQUIP_IRON_HELM,      "Iron Helm",         EQUIP_CAT_ARMOR,    EQUIP_SLOT_HEAD,     0, 2, 4 },
    { EQUIP_STEEL_HELM,     "Steel Helm",        EQUIP_CAT_ARMOR,    EQUIP_SLOT_HEAD,     0, 3, 6 },

    // Armor - Chest
    { EQUIP_LEATHER_VEST,   "Leather Vest",      EQUIP_CAT_ARMOR,    EQUIP_SLOT_CHEST,    0, 2, 4 },
    { EQUIP_CHAIN_MAIL,     "Chain Mail",        EQUIP_CAT_ARMOR,    EQUIP_SLOT_CHEST,    0, 4, 8 },
    { EQUIP_PLATE_MAIL,     "Plate Mail",        EQUIP_CAT_ARMOR,    EQUIP_SLOT_CHEST,    0, 6, 12 },

    // Weapons
    { EQUIP_WOODEN_SWORD,   "Wooden Sword",      EQUIP_CAT_WEAPON,   EQUIP_SLOT_WEAPON,   3, 0, 0 },
    { EQUIP_IRON_SWORD,     "Iron Sword",        EQUIP_CAT_WEAPON,   EQUIP_SLOT_WEAPON,   5, 0, 0 },
    { EQUIP_STEEL_SWORD,    "Steel Sword",       EQUIP_CAT_WEAPON,   EQUIP_SLOT_WEAPON,   8, 0, 0 },
    { EQUIP_WAR_HAMMER,     "War Hammer",        EQUIP_CAT_WEAPON,   EQUIP_SLOT_WEAPON,  10, 1, 0 },

    // Off-hand
    { EQUIP_WOODEN_SHIELD,  "Wooden Shield",     EQUIP_CAT_ARMOR,    EQUIP_SLOT_OFF_HAND, 0, 2, 0 },
    { EQUIP_IRON_SHIELD,    "Iron Shield",       EQUIP_CAT_ARMOR,    EQUIP_SLOT_OFF_HAND, 0, 4, 0 },
    { EQUIP_STEEL_SHIELD,   "Steel Shield",      EQUIP_CAT_ARMOR,    EQUIP_SLOT_OFF_HAND, 0, 6, 0 },

    // Accessories
    { EQUIP_RING_OF_STRENGTH,  "Ring of Strength",   EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 3, 0, 0 },
    { EQUIP_AMULET_OF_WARDING, "Amulet of Warding",  EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 0, 2, 8 },
    { EQUIP_BOOTS_OF_SWIFTNESS,"Boots of Swiftness", EQUIP_CAT_ACCESSORY, EQUIP_SLOT_ACCESSORY, 1, 1, 4 },
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

    int heal = GetItemHealAmount(s->type);
    if (heal > 0) {
        game->player.ent.hp += heal;
        if (game->player.ent.hp > game->player.ent.maxHp)
            game->player.ent.hp = game->player.ent.maxHp;
        CombatLog_Add(&game->combatLog, BLACK, "Used %s - restores %d HP!", GetItemName(s->type), heal);
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

    // Unequip whatever is currently in that slot
    if (game->equipped[slotIdx] != EQUIP_NONE) {
        UnequipSlot(game, data->slot);
    }

    // Apply bonuses
    game->player.ent.attack   += data->bonusAttack;
    game->player.ent.defense  += data->bonusDefense;
    game->player.ent.maxHp    += data->bonusMaxHp;
    game->player.ent.hp       += data->bonusMaxHp;

    game->equipped[slotIdx] = type;

    CombatLog_Add(&game->combatLog, BLACK, "Equipped %s", data->name);
}

void UnequipSlot(Game* game, EquipSlot slot) {
    int slotIdx = (int)slot;
    EquipType oldType = game->equipped[slotIdx];
    if (oldType == EQUIP_NONE) return;

    const EquipData* data = GetEquipData(oldType);
    if (!data) return;

    // Remove bonuses
    game->player.ent.attack   -= data->bonusAttack;
    game->player.ent.defense  -= data->bonusDefense;
    game->player.ent.maxHp    -= data->bonusMaxHp;
    if (game->player.ent.hp > game->player.ent.maxHp)
        game->player.ent.hp = game->player.ent.maxHp;

    game->equipped[slotIdx] = EQUIP_NONE;

    CombatLog_Add(&game->combatLog, BLACK, "Unequipped %s", data->name);
}

bool IsEquipSlotOccupied(const Game* game, EquipSlot slot) {
    return game->equipped[(int)slot] != EQUIP_NONE;
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

// ---- Draw the tab bar at the top of the inventory panel --------------------
static void DrawTabBar(const Game* game, int ix, int iy, int iw) {
    int tabH = 24;
    int tabW = iw / INV_TAB_COUNT;

    for (int t = 0; t < INV_TAB_COUNT; t++) {
        int tx = ix + t * tabW;
        int ty = iy;

        Color bg = (t == (int)game->inventoryTab)
            ? (Color){ 50, 50, 70, 255 }
            : (Color){ 30, 30, 40, 255 };
        DrawRectangle(tx, ty, tabW, tabH, bg);

        if (t == (int)game->inventoryTab)
            DrawRectangle(tx, ty, tabW, 2, YELLOW);

        int tw = MeasureText(TAB_LABELS[t], 16);
        DrawText(TAB_LABELS[t], tx + (tabW - tw) / 2, ty + 4, 16,
                 (t == (int)game->inventoryTab) ? YELLOW : LIGHTGRAY);
    }
    DrawLine(ix, iy + tabH, ix + iw, iy + tabH, LIGHTGRAY);
}

// ---- Inventory tab content --------------------------------------------------
static void DrawInventoryTab(const Game* game, int ix, int iy, int iw, int ih) {
    int tabH = 24;
    int lx = ix + 16;
    int ly = iy + tabH + 12;
    int lw = 280;

    DrawText("INVENTORY", lx, ly, 20, YELLOW);
    ly += 30;

    if (game->inventorySlotCount == 0) {
        DrawText("(empty)", lx, ly, 16, GRAY);
    } else {
        for (int i = 0; i < game->inventorySlotCount; i++) {
            Color c = (i == game->inventorySelection) ? YELLOW : BLACK;
            char line[128];
            snprintf(line, sizeof(line), "%s x%d", GetItemName(game->inventory[i].type), game->inventory[i].quantity);
            if (i == game->inventorySelection)
                DrawText(">", lx - 18, ly, 16, YELLOW);
            DrawText(line, lx, ly, 16, c);
            ly += 22;
        }
    }

    int rx = ix + lw + 24;
    int rw = iw - lw - 40;
    int rtop = iy + tabH + 40;
    int rh = ih - tabH - 70;
    int rcx = rx + rw / 2;

    if (game->inventorySlotCount > 0) {
        ItemType selType = game->inventory[game->inventorySelection].type;

        if (game->texUiSlot.id > 0)
            Draw9Slice(game->texUiSlot, (Rectangle){ (float)rx, (float)rtop, (float)rw, (float)rh }, 8, 8, 8, 8);
        else
            DrawRectangleLines(rx, rtop, rw, rh, (Color){ 60, 60, 80, 255 });

        int infoY = rtop + 14;

        const char* iname = GetItemName(selType);
        int tw = MeasureText(iname, 16);
        DrawText(iname, rcx - tw / 2, infoY, 16, YELLOW);
        infoY += 24;

        const char* desc = GetItemDescription(selType);
        char descCopy[256];
        strncpy(descCopy, desc, sizeof(descCopy) - 1);
        descCopy[sizeof(descCopy) - 1] = '\0';
        char* line = descCopy;
        char* nl;
        while ((nl = strchr(line, '\n')) != NULL) {
            *nl = '\0';
            DrawText(line, rx + 10, infoY, 14, DARKGRAY);
            infoY += 18;
            line = nl + 1;
        }
        if (*line) {
            DrawText(line, rx + 10, infoY, 14, DARKGRAY);
            infoY += 18;
        }

        char qty[64];
        snprintf(qty, sizeof(qty), "Qty: %d", game->inventory[game->inventorySelection].quantity);
        DrawText(qty, rx + 10, infoY, 14, BLACK);
        infoY += 22;

        int heal = GetItemHealAmount(selType);
        if (heal > 0) {
            char healStr[64];
            snprintf(healStr, sizeof(healStr), "Heals %d HP", heal);
            DrawText(healStr, rx + 10, infoY, 14, (Color){ 0, 90, 0, 255 });
        }
    }

    // Action-menu popup
    if (game->invSubState == INV_ACTION_MENU && game->inventorySlotCount > 0) {
        static const char* actions[] = { "Use", "Drop", "Drop All", "Back" };
        int mx = ix + 16 + lw + 2;
        int my = iy + tabH + 40 + game->inventorySelection * 22 + 30 - 2;
        if (my + 80 > iy + ih - 30) my = iy + ih - 30 - 80;
        if (my < iy + tabH + 40) my = iy + tabH + 40;
        int mw = 140;
        int mh = 112;

        if (game->texUiSlot.id > 0)
            Draw9Slice(game->texUiSlot, (Rectangle){ (float)mx, (float)my, (float)mw, (float)mh }, 8, 8, 8, 8);
        else {
            DrawRectangle(mx, my, mw, mh, (Color){ 20, 20, 30, 240 });
            DrawRectangleLines(mx, my, mw, mh, YELLOW);
        }

        for (int a = 0; a < 4; a++) {
            Color ac = (a == game->invActionSelection) ? YELLOW : BLACK;
            if (a == game->invActionSelection)
                DrawText(">", mx + 6, my + 6 + a * 26, 16, YELLOW);
            DrawText(actions[a], mx + 22, my + 6 + a * 26, 16, ac);
        }
    }
}

// ---- Equipment tab content --------------------------------------------------
static void DrawEquipmentTab(const Game* game, int ix, int iy, int iw, int ih) {
    int tabH = 24;
    int cx = ix + iw / 2;
    int slotStartY = iy + tabH + 40;
    int slotH = 36;
    int slotW = 360;
    int slotGap = 8;

    DrawText("EQUIPMENT", ix + 16, iy + tabH + 12, 20, YELLOW);

    for (int s = 0; s < EQUIP_SLOT_COUNT; s++) {
        int sy = slotStartY + s * (slotH + slotGap);
        int sx = cx - slotW / 2;

        // Slot background
        if (game->texUiSlot.id > 0)
            Draw9Slice(game->texUiSlot, (Rectangle){ (float)sx, (float)sy, (float)slotW, (float)slotH }, 8, 8, 8, 8);
        else {
            DrawRectangle(sx, sy, slotW, slotH, (Color){ 30, 30, 50, 255 });
            DrawRectangleLines(sx, sy, slotW, slotH, DARKGRAY);
        }

        // Selection indicator
        Color labelColor = (s == game->inventorySelection) ? YELLOW : LIGHTGRAY;
        if (s == game->inventorySelection)
            DrawText(">", sx - 18, sy + 8, 16, YELLOW);

        // Slot label
        DrawText(EQUIP_SLOT_LABELS[s], sx + 10, sy + 8, 16, labelColor);

        // Equipped item or empty
        EquipType eType = game->equipped[s];
        if (eType != EQUIP_NONE) {
            const EquipData* data = GetEquipData(eType);
            if (data) {
                char itemStr[128];
                snprintf(itemStr, sizeof(itemStr), "%s", data->name);
                int tw = MeasureText(itemStr, 14);
                DrawText(itemStr, sx + slotW - tw - 10, sy + 5, 14, YELLOW);

                // Bonus stats
                int bonusY = sy + 22;
                if (data->bonusAttack > 0) {
                    char atkStr[32];
                    snprintf(atkStr, sizeof(atkStr), "ATK+%d", data->bonusAttack);
                    DrawText(atkStr, sx + slotW - tw - 10, bonusY, 10, GREEN);
                    bonusY += 12;
                }
                if (data->bonusDefense > 0) {
                    char defStr[32];
                    snprintf(defStr, sizeof(defStr), "DEF+%d", data->bonusDefense);
                    DrawText(defStr, sx + slotW - tw - 10, bonusY, 10, (Color){ 100, 150, 255, 255 });
                    bonusY += 12;
                }
                if (data->bonusMaxHp > 0) {
                    char hpStr[32];
                    snprintf(hpStr, sizeof(hpStr), "HP+%d", data->bonusMaxHp);
                    DrawText(hpStr, sx + slotW - tw - 10, bonusY, 10, RED);
                }
            }
        } else {
            DrawText("(empty)", sx + slotW - 80, sy + 8, 14, (Color){ 80, 80, 80, 255 });
        }
    }

    DrawText("< Q / E to switch tabs >", cx - MeasureText("< Q / E to switch tabs >", 14) / 2,
             iy + ih - 28, 14, DARKGRAY);
}

// ---- Stats tab content ------------------------------------------------------
static void DrawStatsTab(const Game* game, int ix, int iy, int iw, int ih) {
    int tabH = 24;
    int sx = ix + 40;
    int sy = iy + tabH + 40;
    int gap = 22;

    DrawText("STATS", ix + 16, iy + tabH + 12, 20, YELLOW);

    const Entity* p = &game->player.ent;

    char buf[128];

    snprintf(buf, sizeof(buf), "Name:     %s", p->name);
    DrawText(buf, sx, sy, 16, BLACK); sy += gap;

    snprintf(buf, sizeof(buf), "Level:    %d", p->level);
    DrawText(buf, sx, sy, 16, BLACK); sy += gap;

    snprintf(buf, sizeof(buf), "HP:       %d / %d", p->hp, p->maxHp);
    DrawText(buf, sx, sy, 16, BLACK); sy += gap;

    // Show base attack/defense with equipment contribution
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
    DrawText(buf, sx, sy, 16, BLACK); sy += gap;

    snprintf(buf, sizeof(buf), "Defense:  %d (base %d)", p->defense, baseDef);
    DrawText(buf, sx, sy, 16, BLACK); sy += gap;

    sy += 8;

    snprintf(buf, sizeof(buf), "Exp:      %d / %d", game->player.exp, game->player.expToNext);
    DrawText(buf, sx, sy, 16, BLACK); sy += gap;

    snprintf(buf, sizeof(buf), "Floor:    %d / %d", game->currentFloor, game->maxFloors);
    DrawText(buf, sx, sy, 16, BLACK); sy += gap;

    DrawText("< Q / E to switch tabs >", ix + 16, iy + ih - 28, 14, DARKGRAY);
}

// ---- Main render entry point -----------------------------------------------
void Inventory_Render(const Game* game) {
    if (game->state != STATE_INVENTORY) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int iw = 640;
    int ih = 400;
    int ix = (sw - iw) / 2;
    int iy = (sh - ih) / 2;
    int tabH = 24;

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

    if (game->inventoryTab == INV_TAB_INVENTORY) {
        char footer[128];
        if (game->inventorySlotCount == 0) {
            snprintf(footer, sizeof(footer), "I / ESC to close");
        } else if (game->invSubState == INV_BROWSE) {
            snprintf(footer, sizeof(footer), "ENTER to select action  |  I / ESC to close");
        } else {
            snprintf(footer, sizeof(footer), "UP/DOWN to choose  |  ENTER to confirm  |  ESC to go back");
        }
        DrawText(footer, ix + 16, iy + ih - 28, 14, DARKGRAY);
    }
}