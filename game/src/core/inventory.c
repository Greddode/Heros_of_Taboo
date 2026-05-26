#include "game.h"
#include "ui/combat_log.h"
#include <stdio.h>
#include <string.h>

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
        CombatLog_Add(&game->combatLog, BLACK, "Used %s — restores %d HP!", GetItemName(s->type), heal);
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

void Inventory_Render(const Game* game) {
    if (game->state != STATE_INVENTORY) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int iw = 640;
    int ih = 360;
    int ix = (sw - iw) / 2;
    int iy = (sh - ih) / 2;

    DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 160 });
    if (game->texUiFrame.id > 0)
        Draw9Slice(game->texUiFrame, (Rectangle){ (float)ix, (float)iy, (float)iw, (float)ih }, 16, 16, 16, 16);
    else {
        DrawRectangle(ix, iy, iw, ih, (Color){ 30, 30, 40, 255 });
        DrawRectangleLines(ix, iy, iw, ih, LIGHTGRAY);
    }

    // Left column: item list
    int lx = ix + 16;
    int ly = iy + 12;
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

    // Right column: info panel
    int rx = ix + lw + 24;
    int rw = iw - lw - 40;
    int rtop = iy + 40;
    int rh = ih - 70;
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
    } else {
        const char* msg = "(empty)";
        int tw = MeasureText(msg, 16);
        DrawText(msg, rcx - tw / 2, (iy + ih) / 2 - 8, 16, DARKGRAY);
    }

    // Context-sensitive footer
    char footer[128];
    if (game->inventorySlotCount == 0) {
        snprintf(footer, sizeof(footer), "I / ESC to close");
    } else if (game->invSubState == INV_BROWSE) {
        snprintf(footer, sizeof(footer), "ENTER to select action  |  I / ESC to close");
    } else {
        snprintf(footer, sizeof(footer), "UP/DOWN to choose  |  ENTER to confirm  |  ESC to go back");
    }
    DrawText(footer, ix + 16, iy + ih - 28, 14, DARKGRAY);

    // Action-menu popup
    if (game->invSubState == INV_ACTION_MENU && game->inventorySlotCount > 0) {
        static const char* actions[] = { "Use", "Drop", "Drop All", "Back" };
        int mx = ix + 16 + lw + 2;
        int my = iy + 40 + game->inventorySelection * 22 + 30 - 2;
        if (my + 80 > iy + ih - 30) my = iy + ih - 30 - 80;
        if (my < iy + 40) my = iy + 40;
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
