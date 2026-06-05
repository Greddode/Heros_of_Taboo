#include "game.h"
#include "ui/inventory_ui.h"
#include "ui/inspector.h"
#include "systems/renderer.h"
#include "systems/spawner_system.h"
#include "systems/ai_system.h"
#include "systems/combat_system.h"
#include "systems/player.h"
#include "map/procedural.h"
#include "data/monster_data.h"
#include "resources.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// ---- Tab labels -------------------------------------------------------------
static const char* TAB_LABELS[INV_TAB_COUNT] = {
    "Inventory",
    "Equipment",
    "Stats"
};

// Slot labels (must match EquipSlot order)
static const char* EQUIP_SLOT_LABELS[EQUIP_SLOT_COUNT] = {
    "Head",
    "Chest",
    "Weapon",
    "Off-hand",
    "Accessory"
};

// ---- Draw the tab bar at the top of the inventory panel --------------------
static void DrawTabBar(const GameWorld* game, const InventoryUIState* ui, int ix, int iy, int iw) {
    float scale = GetUIScale();
    int tabH = (int)(24 * scale);
    int tabW = iw / INV_TAB_COUNT;

    for (int t = 0; t < INV_TAB_COUNT; t++) {
        int tx = ix + t * tabW;
        int ty = iy;

        Texture2D* uiMarker = Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameMarker01a.png");
        if (uiMarker && uiMarker->id > 0) {
            Draw9Slice(*uiMarker, (Rectangle){ (float)tx, (float)ty, (float)tabW, (float)tabH },
                       (int)(8 * scale), (int)(8 * scale), (int)(8 * scale), 0);
        }

        if (t == (int)ui->tab)
            DrawRectangle(tx, ty, tabW, (int)(2 * scale), YELLOW);

        int fontSize = (int)(16 * scale);
        int tw = MeasureText(TAB_LABELS[t], fontSize);
        DrawText(TAB_LABELS[t], tx + (tabW - tw) / 2, ty + (int)(4 * scale), fontSize,
                 BLACK);
    }
}

// ---- Inventory tab content --------------------------------------------------
static int InventoryTabTotalCount(const GameWorld* game) {
    return game->inventorySlotCount + game->equipInventoryCount;
}

static bool IsInventorySlotEquip(const GameWorld* game, int i) {
    return i >= game->inventorySlotCount;
}

static void DrawInventoryTab(const GameWorld* game, const InventoryUIState* ui, int ix, int iy, int iw, int ih) {
    float scale = GetUIScale();
    int tabH = (int)(24 * scale);
    int scroll = ui->scrollOffset;
    int lx = ix + (int)(16 * scale);
    int ly = iy + tabH + (int)(42 * scale) - scroll;
    int lw = (int)(280 * scale);
    int total = InventoryTabTotalCount(game);

    if (total == 0) {
        DrawText("(empty)", lx, ly, (int)(16 * scale), GRAY);
    } else {
        // First: potions
        for (int i = 0; i < game->inventorySlotCount; i++) {
            Color c = (i == ui->selection) ? YELLOW : BLACK;
            char line[128];
            snprintf(line, sizeof(line), "%s x%d", GetItemName(game->inventory[i].type), game->inventory[i].quantity);
            if (i == ui->selection)
                DrawText(">", ix + (int)(4 * scale), ly, (int)(16 * scale), YELLOW);
            DrawText(line, lx + (int)(2 * scale), ly, (int)(16 * scale), c);
            ly += (int)(22 * scale);
        }
        // Then: equipment items
        for (int i = 0; i < game->equipInventoryCount; i++) {
            int idx = game->inventorySlotCount + i;
            Color c = (idx == ui->selection) ? YELLOW : BLACK;
            const EquipData* d = GetEquipData(game->equipInventory[i]);
            if (d) {
                char line[128];
                snprintf(line, sizeof(line), "[%s] %s%s%s",
                         EQUIP_SLOT_LABELS[(int)d->slot], d->name,
                         d->twoHanded ? " (two-handed)" : "",
                         IsWeaponDualWieldable(game->equipInventory[i]) ? " (dual)" : "");
                if (idx == ui->selection)
                    DrawText(">", ix + (int)(4 * scale), ly, (int)(16 * scale), YELLOW);
                DrawText(line, lx + (int)(2 * scale), ly, (int)(16 * scale), c);
                ly += (int)(22 * scale);
            }
        }
    }

    int rx = ix + lw + 24;
    int rw = iw - lw - 40;
    int rtop = iy + tabH + (int)(40 * scale);
    int rh = ih - tabH - (int)(70 * scale);

    if (total > 0) {
        Texture2D* uiSlot = Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameSlot01b.png");
        if (uiSlot && uiSlot->id > 0)
            Draw9Slice(*uiSlot, (Rectangle){ (float)rx, (float)rtop, (float)rw, (float)rh }, 8, 8, 8, 8);
        else
            DrawRectangleLines(rx, rtop, rw, rh, (Color){ 60, 60, 80, 255 });
        Inspector_Render(game, INSPECTOR_ITEM, rx, rtop, rw, rh, ui->selection);
    }

    // Action-menu popup
    if (ui->subState == INV_ACTION_MENU && total > 0) {
        bool isEquip = IsInventorySlotEquip(game, ui->selection);
        int equipIdx = isEquip ? ui->selection - game->inventorySlotCount : -1;
        EquipType eType = isEquip ? game->equipInventory[equipIdx] : EQUIP_NONE;
        bool canDual = isEquip && IsWeaponDualWieldable(eType);
        const char* actions[5];
        int actionCount;
        if (isEquip) {
            actions[0] = "Equip";
            actions[1] = canDual ? "Dual Wield" : "Drop";
            actions[2] = canDual ? "Drop" : "Back";
            actions[3] = canDual ? "Back" : NULL;
            actionCount = canDual ? 4 : 3;
        } else {
            actions[0] = "Use";
            actions[1] = "Drop";
            actions[2] = "Drop All";
            actions[3] = "Back";
            actionCount = 4;
        }

        int mx = ix + (int)(16 * scale) + lw + (int)(2 * scale);
        int my = iy + tabH + (int)(40 * scale) + ui->selection * (int)(22 * scale) + (int)(30 * scale) - (int)(2 * scale) - scroll;
        if (my + (int)(80 * scale) > iy + ih - (int)(30 * scale)) my = iy + ih - (int)(30 * scale) - (int)(80 * scale);
        if (my < iy + tabH + (int)(40 * scale)) my = iy + tabH + (int)(40 * scale);
        int mw = (int)(140 * scale);
        int mh = actionCount * (int)(26 * scale) + (int)(8 * scale);

        Texture2D* uiSlot = Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameSlot01b.png");
        if (uiSlot && uiSlot->id > 0)
            Draw9Slice(*uiSlot, (Rectangle){ (float)mx, (float)my, (float)mw, (float)mh }, 8, 8, 8, 8);
        else {
            DrawRectangle(mx, my, mw, mh, (Color){ 20, 20, 30, 240 });
            DrawRectangleLines(mx, my, mw, mh, YELLOW);
        }

        for (int a = 0; a < actionCount; a++) {
            Color ac = (a == ui->actionSelection) ? YELLOW : BLACK;
            if (a == ui->actionSelection)
                DrawText(">", mx + (int)(6 * scale), my + (int)(6 * scale) + a * (int)(26 * scale), (int)(16 * scale), YELLOW);
            DrawText(actions[a], mx + (int)(22 * scale), my + (int)(6 * scale) + a * (int)(26 * scale), (int)(16 * scale), ac);
        }
    }
}

// ---- Equipment tab content --------------------------------------------------
static void DrawEquipmentTab(const GameWorld* game, const InventoryUIState* ui, int ix, int iy, int iw, int ih) {
    float scale = GetUIScale();
    int tabH = (int)(24 * scale);
    int scroll = ui->scrollOffset;
    int cx = ix + iw / 2;
    int slotStartY = iy + tabH + (int)(40 * scale) - scroll;
    int slotH = (int)(36 * scale);
    int slotW = (int)(360 * scale);
    int slotGap = (int)(8 * scale);

    for (int s = 0; s < EQUIP_SLOT_COUNT; s++) {
        int sy = slotStartY + s * (slotH + slotGap);
        int sx = cx - slotW / 2;

        Texture2D* uiSlot = Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameSlot01b.png");
        if (uiSlot && uiSlot->id > 0)
            Draw9Slice(*uiSlot, (Rectangle){ (float)sx, (float)sy, (float)slotW, (float)slotH }, 8, 8, 8, 8);
        else {
            DrawRectangle(sx, sy, slotW, slotH, (Color){ 30, 30, 50, 255 });
            DrawRectangleLines(sx, sy, slotW, slotH, DARKGRAY);
        }

        Color labelColor = (s == ui->selection) ? YELLOW : LIGHTGRAY;
        if (s == ui->selection)
            DrawText(">", sx - (int)(18 * scale), sy + (int)(8 * scale), (int)(16 * scale), YELLOW);

        DrawText(EQUIP_SLOT_LABELS[s], sx + (int)(10 * scale), sy + (int)(8 * scale), (int)(16 * scale), labelColor);

        EquipType eType = game->equipped[s];
        if (eType != EQUIP_NONE) {
            const EquipData* data = GetEquipData(eType);
            if (data) {
                char itemStr[128];
                snprintf(itemStr, sizeof(itemStr), "%s%s", data->name, data->twoHanded ? " (two-handed)" : "");
                int titleSize = (int)(14 * scale);
                int tw = MeasureText(itemStr, titleSize);
                DrawText(itemStr, sx + slotW - tw - (int)(10 * scale), sy + (int)(5 * scale), titleSize, YELLOW);

                char bonusStr[64] = "";
                if (data->bonusAttack > 0) { char tmp[16]; snprintf(tmp, sizeof(tmp), "ATK+%d ", data->bonusAttack); strncat(bonusStr, tmp, sizeof(bonusStr) - strlen(bonusStr) - 1); }
                if (data->bonusDefense != 0) { char tmp[16]; snprintf(tmp, sizeof(tmp), "DEF%+d ", data->bonusDefense); strncat(bonusStr, tmp, sizeof(bonusStr) - strlen(bonusStr) - 1); }
                if (data->bonusStr != 0) { char tmp[16]; snprintf(tmp, sizeof(tmp), "STR%+d ", data->bonusStr); strncat(bonusStr, tmp, sizeof(bonusStr) - strlen(bonusStr) - 1); }
                if (data->bonusDex != 0) { char tmp[16]; snprintf(tmp, sizeof(tmp), "DEX%+d ", data->bonusDex); strncat(bonusStr, tmp, sizeof(bonusStr) - strlen(bonusStr) - 1); }
                if (data->bonusInt != 0) { char tmp[16]; snprintf(tmp, sizeof(tmp), "MGC%+d ", data->bonusInt); strncat(bonusStr, tmp, sizeof(bonusStr) - strlen(bonusStr) - 1); }
                if (data->bonusCon != 0) { char tmp[16]; snprintf(tmp, sizeof(tmp), "CON%+d ", data->bonusCon); strncat(bonusStr, tmp, sizeof(bonusStr) - strlen(bonusStr) - 1); }
                if (data->bonusLck != 0) { char tmp[16]; snprintf(tmp, sizeof(tmp), "LCK%+d ", data->bonusLck); strncat(bonusStr, tmp, sizeof(bonusStr) - strlen(bonusStr) - 1); }
                DrawText(bonusStr, sx + (int)(10 * scale), sy + (int)(22 * scale), (int)(10 * scale), BLACK);
            }
        } else {
            DrawText("(empty)", sx + slotW - (int)(80 * scale), sy + (int)(8 * scale), (int)(14 * scale), (Color){ 80, 80, 80, 255 });
        }
    }

    // Equipment action menu
    if (ui->subState == INV_ACTION_MENU && game->equipped[ui->selection] != EQUIP_NONE) {
        static const char* actions[] = { "Unequip", "Drop", "Back" };
        int selSlot = ui->selection;
        int sy = slotStartY + selSlot * (slotH + slotGap);
        int sx = cx - slotW / 2;
        int mx = sx + slotW + (int)(4 * scale);
        int my = sy;
        if (my + (int)(80 * scale) > iy + ih - (int)(30 * scale)) my = iy + ih - (int)(30 * scale) - (int)(80 * scale);
        int mw = (int)(130 * scale);
        int mh = (int)(90 * scale);

        Texture2D* uiSlot = Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameSlot01b.png");
        if (uiSlot && uiSlot->id > 0)
            Draw9Slice(*uiSlot, (Rectangle){ (float)mx, (float)my, (float)mw, (float)mh }, 8, 8, 8, 8);
        else {
            DrawRectangle(mx, my, mw, mh, (Color){ 20, 20, 30, 240 });
            DrawRectangleLines(mx, my, mw, mh, YELLOW);
        }

        for (int a = 0; a < 3; a++) {
            Color ac = (a == ui->actionSelection) ? YELLOW : BLACK;
            if (a == ui->actionSelection)
                DrawText(">", mx + (int)(6 * scale), my + (int)(6 * scale) + a * (int)(26 * scale), (int)(16 * scale), YELLOW);
            DrawText(actions[a], mx + (int)(22 * scale), my + (int)(6 * scale) + a * (int)(26 * scale), (int)(16 * scale), ac);
        }
    }
}

// ---- Stats tab content ------------------------------------------------------
static void DrawStatsTab(const GameWorld* game, const InventoryUIState* ui, int ix, int iy, int iw, int ih) {
    float scale = GetUIScale();
    int tabH = (int)(24 * scale);
    int headerGap = (int)(40 * scale);
    int sx = ix + (int)(40 * scale);
    int sy = iy + tabH + headerGap + (int)(12 * scale);
    int gap = (int)(22 * scale);
    int slotH = (int)(20 * scale);
    int slotW = (int)(260 * scale);

    const CStats* pStats = NULL;
    const CPosition* pPos = NULL;
    const char* pName = "";
    int pLevel = 0, pHp = 0, pMaxHp = 0;
    int pStr = 0, pDex = 0, pIntel = 0, pCon = 0, pLck = 0;
    int pAttack = 0, pStatPoints = 0;
    int pExp = 0, pExpToNext = 0;
    if (game->playerEntity != ENTITY_NONE) {
        const World* w = &game->ecs;
        EntityId pe = game->playerEntity;
        pStats = World_GetStats((World*)w, pe);
        pPos = World_GetPosition((World*)w, pe);
        pLevel = pStats->level; pHp = pStats->hp; pMaxHp = pStats->maxHp;
        pStr = pStats->str; pDex = pStats->dex; pIntel = pStats->intel; pCon = pStats->con; pLck = pStats->lck;
        pAttack = pStats->attack; pStatPoints = pStats->statPoints;
        pExp = pStats->exp; pExpToNext = pStats->expToNext;
        if (World_HasComponents((World*)w, pe, COMP_NAME))
            pName = World_GetName((World*)w, pe)->name;
    }

    char buf[128];
    int textSize = (int)(16 * scale);
    int col1x = sx;
    int col2x = sx + (int)(300 * scale);
    int c1y = sy - ui->statsScrollCol1;
    int c2y = sy - ui->statsScrollCol2;
    int derivedSize = (int)(14 * scale);
    bool isCol1 = (ui->statsActiveCol == 0);
    bool isCol2 = (ui->statsActiveCol == 1);
    static const Color cBlack = {0, 0, 0, 255};
    int lineIdx = 0;

    {
        Color selColor = cBlack;
        if (isCol1 && ui->statsSelection == lineIdx) {
            selColor = DARKGRAY;
            DrawText(">", col1x - (int)(18 * scale), c1y, (int)(16 * scale), selColor);
        }
        snprintf(buf, sizeof(buf), "Name:     %s", pName);
        DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, selColor); c1y += gap; lineIdx++;
    }
    {
        Color selColor = cBlack;
        if (isCol1 && ui->statsSelection == lineIdx) {
            selColor = DARKGRAY;
            DrawText(">", col1x - (int)(18 * scale), c1y, (int)(16 * scale), selColor);
        }
        snprintf(buf, sizeof(buf), "Level:    %d", pLevel);
        DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, selColor); c1y += gap; lineIdx++;
    }
    {
        Color selColor = cBlack;
        if (isCol1 && ui->statsSelection == lineIdx) {
            selColor = DARKGRAY;
            DrawText(">", col1x - (int)(18 * scale), c1y, (int)(16 * scale), selColor);
        }
        snprintf(buf, sizeof(buf), "HP:       %d / %d", pHp, pMaxHp);
        DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, selColor); c1y += gap; lineIdx++;
    }

    int baseStr = pStr, baseDex = pDex, baseInt = pIntel, baseCon = pCon, baseLck = pLck;
    for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
        const EquipData* d = GetEquipData(game->equipped[i]);
        if (d) { baseStr -= d->bonusStr; baseDex -= d->bonusDex; baseInt -= d->bonusInt; baseCon -= d->bonusCon; baseLck -= d->bonusLck; }
    }

    {
        Color selColor = cBlack;
        if (isCol1 && ui->statsSelection == lineIdx) {
            selColor = (pStatPoints > 0) ? YELLOW : DARKGRAY;
            DrawText(">", col1x - (int)(18 * scale), c1y, (int)(16 * scale), selColor);
        }
        snprintf(buf, sizeof(buf), "STR:      %d (base %d)", pStr, baseStr);
        DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, selColor); c1y += gap; lineIdx++;
    }
    {
        Color selColor = cBlack;
        if (isCol1 && ui->statsSelection == lineIdx) {
            selColor = (pStatPoints > 0) ? YELLOW : DARKGRAY;
            DrawText(">", col1x - (int)(18 * scale), c1y, (int)(16 * scale), selColor);
        }
        snprintf(buf, sizeof(buf), "DEX:      %d (base %d)", pDex, baseDex);
        DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, selColor); c1y += gap; lineIdx++;
    }
    {
        Color selColor = cBlack;
        if (isCol1 && ui->statsSelection == lineIdx) {
            selColor = (pStatPoints > 0) ? YELLOW : DARKGRAY;
            DrawText(">", col1x - (int)(18 * scale), c1y, (int)(16 * scale), selColor);
        }
        snprintf(buf, sizeof(buf), "MGC:      %d (base %d)", pIntel, baseInt);
        DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, selColor); c1y += gap; lineIdx++;
    }
    {
        Color selColor = cBlack;
        if (isCol1 && ui->statsSelection == lineIdx) {
            selColor = (pStatPoints > 0) ? YELLOW : DARKGRAY;
            DrawText(">", col1x - (int)(18 * scale), c1y, (int)(16 * scale), selColor);
        }
        snprintf(buf, sizeof(buf), "CON:      %d (base %d)", pCon, baseCon);
        DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, selColor); c1y += gap; lineIdx++;
    }
    {
        Color selColor = cBlack;
        if (isCol1 && ui->statsSelection == lineIdx) {
            selColor = (pStatPoints > 0) ? YELLOW : DARKGRAY;
            DrawText(">", col1x - (int)(18 * scale), c1y, (int)(16 * scale), selColor);
        }
        snprintf(buf, sizeof(buf), "LCK:      %d (base %d)", pLck, baseLck);
        DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, selColor); c1y += gap; lineIdx++;
    }

    c1y += (int)(8 * scale);
    int meleeDmg = pAttack + pStr * 2;
    int dodgePct = pDex * 2; if (dodgePct > 60) dodgePct = 60;
    int critPct = pLck;
    int magicRes = pIntel * 3;

    {
        Color selColor = cBlack;
        if (isCol1 && ui->statsSelection == lineIdx) {
            selColor = DARKGRAY;
            DrawText(">", col1x - (int)(18 * scale), c1y, (int)(16 * scale), selColor);
        }
        snprintf(buf, sizeof(buf), "Melee DMG:  %d (ATK %d + STRx2 %d)", meleeDmg, pAttack, pStr * 2);
        DrawText(buf, col1x + (int)(2 * scale), c1y, derivedSize, selColor); c1y += gap; lineIdx++;
    }
    {
        Color selColor = cBlack;
        if (isCol1 && ui->statsSelection == lineIdx) {
            selColor = DARKGRAY;
            DrawText(">", col1x - (int)(18 * scale), c1y, (int)(16 * scale), selColor);
        }
        snprintf(buf, sizeof(buf), "Dodge:      %d%%", dodgePct);
        DrawText(buf, col1x + (int)(2 * scale), c1y, derivedSize, selColor); c1y += gap; lineIdx++;
    }
    {
        Color selColor = cBlack;
        if (isCol1 && ui->statsSelection == lineIdx) {
            selColor = DARKGRAY;
            DrawText(">", col1x - (int)(18 * scale), c1y, (int)(16 * scale), selColor);
        }
        snprintf(buf, sizeof(buf), "Crit:       %d%%", critPct);
        DrawText(buf, col1x + (int)(2 * scale), c1y, derivedSize, selColor); c1y += gap; lineIdx++;
    }
    {
        Color selColor = cBlack;
        if (isCol1 && ui->statsSelection == lineIdx) {
            selColor = DARKGRAY;
            DrawText(">", col1x - (int)(18 * scale), c1y, (int)(16 * scale), selColor);
        }
        snprintf(buf, sizeof(buf), "Magic DEF:  %d", magicRes);
        DrawText(buf, col1x + (int)(2 * scale), c1y, derivedSize, selColor); c1y += gap; lineIdx++;
    }

    c1y += (int)(8 * scale);
    {
        Color selColor = cBlack;
        if (isCol1 && ui->statsSelection == lineIdx) {
            selColor = DARKGRAY;
            DrawText(">", col1x - (int)(18 * scale), c1y, (int)(16 * scale), selColor);
        }
        snprintf(buf, sizeof(buf), "Exp:      %d / %d", pExp, pExpToNext);
        DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, selColor); c1y += gap; lineIdx++;
    }
    {
        Color selColor = cBlack;
        if (isCol1 && ui->statsSelection == lineIdx) {
            selColor = DARKGRAY;
            DrawText(">", col1x - (int)(18 * scale), c1y, (int)(16 * scale), selColor);
        }
        snprintf(buf, sizeof(buf), "Floor:    %d / %d", game->currentFloor, game->maxFloors);
        DrawText(buf, col1x + (int)(2 * scale), c1y, textSize, selColor); c1y += gap; lineIdx++;
    }

    // Column 2: stat allocation
    int lineIdx2 = 0;
    if (pStatPoints > 0) {
        snprintf(buf, sizeof(buf), "Unspent: %d pt(s)", pStatPoints);
        DrawText(buf, col2x + (int)(2 * scale), c2y, (int)(18 * scale), YELLOW); c2y += gap + (int)(4 * scale); lineIdx2++;
        int allocSize = (int)(15 * scale);
        Color str2Color = (isCol2 && ui->statsSelection == lineIdx2) ? YELLOW : cBlack;
        if (isCol2 && ui->statsSelection == lineIdx2) DrawText(">", col2x - (int)(18 * scale), c2y, (int)(16 * scale), YELLOW);
        DrawText("STR (+1)", col2x + (int)(2 * scale), c2y, allocSize, str2Color); c2y += gap; lineIdx2++;
        Color dex2Color = (isCol2 && ui->statsSelection == lineIdx2) ? YELLOW : cBlack;
        if (isCol2 && ui->statsSelection == lineIdx2) DrawText(">", col2x - (int)(18 * scale), c2y, (int)(16 * scale), YELLOW);
        DrawText("DEX (+1)", col2x + (int)(2 * scale), c2y, allocSize, dex2Color); c2y += gap; lineIdx2++;
        Color mgc2Color = (isCol2 && ui->statsSelection == lineIdx2) ? YELLOW : cBlack;
        if (isCol2 && ui->statsSelection == lineIdx2) DrawText(">", col2x - (int)(18 * scale), c2y, (int)(16 * scale), YELLOW);
        DrawText("MGC (+1)", col2x + (int)(2 * scale), c2y, allocSize, mgc2Color); c2y += gap; lineIdx2++;
        Color con2Color = (isCol2 && ui->statsSelection == lineIdx2) ? YELLOW : cBlack;
        if (isCol2 && ui->statsSelection == lineIdx2) DrawText(">", col2x - (int)(18 * scale), c2y, (int)(16 * scale), YELLOW);
        DrawText("CON (+1)", col2x + (int)(2 * scale), c2y, allocSize, con2Color); c2y += gap; lineIdx2++;
        Color lck2Color = (isCol2 && ui->statsSelection == lineIdx2) ? YELLOW : cBlack;
        if (isCol2 && ui->statsSelection == lineIdx2) DrawText(">", col2x - (int)(18 * scale), c2y, (int)(16 * scale), YELLOW);
        DrawText("LCK (+1)", col2x + (int)(2 * scale), c2y, allocSize, lck2Color); c2y += gap; lineIdx2++;
    } else {
        DrawText("Gain stat points", col2x + (int)(2 * scale), c2y + (int)(2 * scale), (int)(14 * scale), cBlack);
        DrawText("on level up!", col2x + (int)(2 * scale), c2y + (int)(14 * scale) + (int)(4 * scale), (int)(14 * scale), cBlack);
    }
}

// ---- Main render entry point -----------------------------------------------
void InventoryUI_Render(GameWorld* game, const InventoryUIState* ui) {
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
    Texture2D* uiFrame = Resources_LoadTexture("resources/sprites/ui/UI_Flat_Frame01a.png");
    if (uiFrame && uiFrame->id > 0)
        Draw9Slice(*uiFrame, (Rectangle){ (float)ix, (float)iy, (float)iw, (float)ih }, 16, 16, 16, 16);
    else {
        DrawRectangle(ix, iy, iw, ih, (Color){ 30, 30, 40, 255 });
        DrawRectangleLines(ix, iy, iw, ih, LIGHTGRAY);
    }

    DrawTabBar(game, ui, ix, iy, iw);

    int titleSize = (int)(20 * scale);
    if (ui->tab == INV_TAB_INVENTORY)
        DrawText("INVENTORY", ix + (int)(16 * scale), iy + tabH + (int)(12 * scale), titleSize, YELLOW);
    else if (ui->tab == INV_TAB_EQUIPMENT)
        DrawText("EQUIPMENT", ix + (int)(16 * scale), iy + tabH + (int)(12 * scale), titleSize, YELLOW);
    else if (ui->tab == INV_TAB_STATS)
        DrawText("STATS", ix + (int)(16 * scale), iy + tabH + (int)(12 * scale), titleSize, YELLOW);

    int contentY = iy + tabH + (int)(12 * scale) + titleSize + (int)(4 * scale);
    int contentH = ih - (contentY - iy) - (int)(60 * scale);
    BeginScissorMode(ix + 1, contentY, iw - 2, contentH);
    switch (ui->tab) {
        case INV_TAB_INVENTORY: DrawInventoryTab(game, ui, ix, iy, iw, ih); break;
        case INV_TAB_EQUIPMENT: DrawEquipmentTab(game, ui, ix, iy, iw, ih); break;
        case INV_TAB_STATS:     DrawStatsTab(game, ui, ix, iy, iw, ih); break;
        default: break;
    }
    EndScissorMode();

    int footerSize = (int)(14 * scale);
    int footerY = iy + ih - (int)(28 * scale);
    BeginScissorMode(ix, footerY, iw, (int)(28 * scale));
    if (ui->tab == INV_TAB_STATS) {
        DrawText("< Q / E to switch tabs >", ix + (int)(16 * scale), footerY, footerSize, DARKGRAY);
    } else if (ui->tab == INV_TAB_INVENTORY) {
        if (game->inventorySlotCount == 0)
            DrawText("Q / E to switch tabs  |  I / ESC to close", ix + (int)(16 * scale), footerY, footerSize, DARKGRAY);
        else if (ui->subState == INV_BROWSE)
            DrawText("ENTER to select action  |  Q / E to switch tabs  |  I / ESC to close", ix + (int)(16 * scale), footerY, footerSize, DARKGRAY);
        else
            DrawText("UP/DOWN to choose  |  ENTER to confirm  |  ESC to go back", ix + (int)(16 * scale), footerY, footerSize, DARKGRAY);
    } else {
        DrawText("Q / E to switch tabs", ix + (int)(16 * scale), footerY, footerSize, DARKGRAY);
    }
    EndScissorMode();
}

// ---- Init ------------------------------------------------------------------
void InventoryUI_Init(InventoryUIState* ui) {
    ui->selection = 0;
    ui->scrollOffset = 0;
    ui->statsScrollCol1 = 0;
    ui->statsScrollCol2 = 0;
    ui->statsActiveCol = 0;
    ui->statsSelection = 0;
    ui->subState = INV_BROWSE;
    ui->tab = INV_TAB_INVENTORY;
    ui->actionSelection = 0;
}

