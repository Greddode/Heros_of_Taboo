#include "shop_ui.h"
#include "event_bus.h"
#include "inventory.h"
#include "data/equip_data.h"
#include "systems/renderer.h"
#include "game_balance.h"
#include <stdio.h>
#include <string.h>

void ShopUI_HandleInput(GameWorld* game)
{
    if (!game) return;

    if (IsKeyPressed(KEY_ESCAPE)) {
        game->state = STATE_PLAYER_TURN;
        game->shopSelection = 0;
        game->shopSection = (int)SHOP_SECTION_BUY;
        return;
    }

    if (IsKeyPressed(KEY_TAB)) {
        game->shopSection = (game->shopSection == (int)SHOP_SECTION_BUY) ? (int)SHOP_SECTION_SELL : (int)SHOP_SECTION_BUY;
        game->shopSelection = 0;
        return;
    }

    int maxItems;
    if (game->shopSection == (int)SHOP_SECTION_BUY) {
        maxItems = 1;
    } else {
        maxItems = game->inventorySlotCount + game->equipInventoryCount;
    }
    if (maxItems < 1) maxItems = 1;

    if (IsKeyPressed(KEY_UP) && game->shopSelection > 0) game->shopSelection--;
    if (IsKeyPressed(KEY_DOWN) && game->shopSelection < maxItems - 1) game->shopSelection++;

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_E)) {
        CPosition* pp = World_GetPosition(&game->ecs, game->playerEntity);
        if (game->shopSection == (int)SHOP_SECTION_BUY) {
            int price = Equip_GetBasePrice(GetEquipData(EQUIP_BAND_OF_GROWTH));
            if (game->gold < price) {
                if (pp) FloatMsg_Spawn(game, pp->x, pp->y, RED, "Not enough gold!");
            } else if (game->equipInventoryCount >= MAX_INVENTORY_SLOTS) {
                if (pp) FloatMsg_Spawn(game, pp->x, pp->y, RED, "Inventory full!");
            } else {
                game->gold -= price;
                AddEquipToInventory(game, EQUIP_BAND_OF_GROWTH);
                if (pp) FloatMsg_Spawn(game, pp->x, pp->y, GREEN, "Purchased!");
            }
        } else {
            if (game->shopSelection < game->inventorySlotCount) {
                InventorySlot* slot = &game->inventory[game->shopSelection];
                if (slot->type != ITEM_NONE && slot->quantity > 0) {
                    int sellPrice = POTION_SELL_SMALL;
                    if (slot->type == ITEM_MEDIUM_HP_POTION) sellPrice = POTION_SELL_MEDIUM;
                    else if (slot->type == ITEM_LARGE_HP_POTION) sellPrice = POTION_SELL_LARGE;
                    game->gold += sellPrice;
                    {
                        GoldGainedEvent evt = { sellPrice, game->gold };
                        EventBus_Publish(EVT_GOLD_GAINED, &evt);
                    }
                    slot->quantity--;
                    if (slot->quantity <= 0) {
                        for (int i = game->shopSelection; i < game->inventorySlotCount - 1; i++)
                            game->inventory[i] = game->inventory[i + 1];
                        game->inventorySlotCount--;
                    }
                    if (pp) FloatMsg_Spawn(game, pp->x, pp->y, YELLOW, "Sold!");
                    if (game->shopSelection >= game->inventorySlotCount && game->inventorySlotCount > 0)
                        game->shopSelection = game->inventorySlotCount - 1;
                }
            } else {
                int equipIdx = game->shopSelection - game->inventorySlotCount;
                if (equipIdx >= 0 && equipIdx < game->equipInventoryCount) {
                    EquipType eType = game->equipInventory[equipIdx];
                    const EquipData* d = GetEquipData(eType);
                    if (d) {
                        bool equipped = false;
                        for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
                            if (game->equipped[i] == eType) { equipped = true; break; }
                        }
                        if (equipped) {
                            if (pp) FloatMsg_Spawn(game, pp->x, pp->y, YELLOW, "Unequip first!");
                        } else {
                            int sellPrice = Equip_GetSellPrice(d);
                            game->gold += sellPrice;
                    {
                        GoldGainedEvent evt = { sellPrice, game->gold };
                        EventBus_Publish(EVT_GOLD_GAINED, &evt);
                    }
                            RemoveEquipFromInventory(game, equipIdx);
                            if (pp) FloatMsg_Spawn(game, pp->x, pp->y, YELLOW, "Sold!");
                            if (game->shopSelection >= game->inventorySlotCount + game->equipInventoryCount
                                && game->inventorySlotCount + game->equipInventoryCount > 0)
                                game->shopSelection = game->inventorySlotCount + game->equipInventoryCount - 1;
                        }
                    }
                }
            }
        }
    }
}

void ShopUI_Render(GameWorld* game, float scale)
{
    if (!game) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int pw = (int)(400 * scale);
    int ph = (int)(500 * scale);
    int px = (sw - pw) / 2;
    int py = (sh - ph) / 2;

    DrawRectangle(px, py, pw, ph, (Color){ 20, 15, 30, 240 });
    DrawRectangleLines(px, py, pw, ph, GOLD);

    int fs = (int)(16 * scale);
    int lx = px + (int)(16 * scale);
    int ly = py + (int)(12 * scale);

    DrawText("Shop", px + pw / 2 - MeasureText("Shop", (int)(22 * scale)) / 2, ly, (int)(22 * scale), GOLD);
    ly += (int)(32 * scale);

    {
        int tw1 = MeasureText("[E] Buy", fs);
        int tw2 = MeasureText("[TAB] Sell", fs);
        Color buyC = (game->shopSection == (int)SHOP_SECTION_BUY) ? GOLD : LIGHTGRAY;
        Color sellC = (game->shopSection == (int)SHOP_SECTION_SELL) ? GOLD : LIGHTGRAY;
        DrawText("[E] Buy", lx, ly, fs, buyC);
        DrawText("[TAB] Sell", lx + tw1 + (int)(20 * scale), ly, fs, sellC);
    }
    ly += (int)(28 * scale);

    if (game->shopSection == (int)SHOP_SECTION_BUY) {
        int price = Equip_GetBasePrice(GetEquipData(EQUIP_BAND_OF_GROWTH));
        Color itemColor = (game->gold >= price) ? WHITE : DARKGRAY;
        if (game->shopSelection == 0) DrawText(">", lx - (int)(14 * scale), ly, fs, GOLD);
        char buf[128];
        snprintf(buf, sizeof(buf), "Band of Limitless Growth — %dg", price);
        DrawText(buf, lx, ly, fs, itemColor);
        ly += (int)(24 * scale);
        DrawText("Removes all stat caps.", lx + (int)(10 * scale), ly, (int)(13 * scale), GRAY);
    } else {
        int maxItems = game->inventorySlotCount + game->equipInventoryCount;
        if (maxItems < 1) {
            DrawText("Nothing to sell.", lx, ly, fs, GRAY);
        }
        for (int i = 0; i < maxItems; i++) {
            if (game->shopSelection == i) DrawText(">", lx - (int)(14 * scale), ly, fs, GOLD);
            char buf[128];
            if (i < game->inventorySlotCount) {
                const char* name = GetItemName(game->inventory[i].type);
                int displayPrice = POTION_SELL_SMALL;
                if (game->inventory[i].type == ITEM_MEDIUM_HP_POTION) displayPrice = POTION_SELL_MEDIUM;
                else if (game->inventory[i].type == ITEM_LARGE_HP_POTION) displayPrice = POTION_SELL_LARGE;
                snprintf(buf, sizeof(buf), "%s x%d — %dg", name, game->inventory[i].quantity, displayPrice);
            } else {
                int ei = i - game->inventorySlotCount;
                const EquipData* d = GetEquipData(game->equipInventory[ei]);
                int sellPrice = d ? Equip_GetSellPrice(d) : 0;
                snprintf(buf, sizeof(buf), "%s — %dg", d ? d->name : "???", sellPrice);
            }
            DrawText(buf, lx, ly, fs, WHITE);
            ly += (int)(24 * scale);
        }
    }

    ly = py + ph - (int)(50 * scale);
    char goldBuf[32];
    snprintf(goldBuf, sizeof(goldBuf), "Gold: %dg", game->gold);
    DrawText(goldBuf, lx, ly, fs, GOLD);

    ly += (int)(24 * scale);
    DrawText("[E] Buy/Sell  [Tab] Switch  [Esc] Leave", lx, ly, (int)(13 * scale), GRAY);
}
