#include "systems/overlay_renderer.h"
#include "game.h"
#include "systems/renderer.h"
#include "ui/inspector.h"
#include "ui/shop_ui.h"
#include "systems/spawner_system.h"
#include "inventory.h"
#include "resources.h"
#include <stdio.h>

void OverlayRenderer_Render(GameWorld* game, const InventoryUIState* ui) {
    float scale = GetUIScale();

    // Monster info (top-right)
    float iscale = GetUIScale();
    Inspector_Render(game, INSPECTOR_MONSTER, GetScreenWidth() - (int)(200 * iscale), (int)(10 * iscale), (int)(190 * iscale), (int)(160 * iscale), 0);

    // Potion / equipment tile info (top-right, below monster info)
    if (game->inspectedTileActive) {
        int ptx = game->inspectedTileX;
        int pty = game->inspectedTileY;
        ItemType tileTypes[MAX_POTIONS];
        int tileQtys[MAX_POTIONS];
        int tileCount = SpawnerSystem_ListPotionsAt(game, ptx, pty, tileTypes, tileQtys, MAX_POTIONS);
        EquipType equipInfos[MAX_EQUIP_ON_MAP];
        int equipQtys[MAX_EQUIP_ON_MAP];
        int equipCount = SpawnerSystem_ListEquipAt(game, ptx, pty, equipInfos, equipQtys, MAX_EQUIP_ON_MAP);

        if (tileCount > 0 || equipCount > 0) {
            int totalItems = tileCount + equipCount;
            int pX = GetScreenWidth() - (int)(200 * scale);
            int pY = (int)(180 * scale);
            int pW = (int)(190 * scale);
            int fs = (int)(14 * scale);
            int lh = fs + (int)(4 * scale);
            int itemH = (int)(2 * lh) + (int)(4 * scale);
            int pH = (int)(40 * scale) + totalItems * itemH;
            if (pH > GetScreenHeight() - pY - (int)(10 * scale)) pH = GetScreenHeight() - pY - (int)(10 * scale);
            int ly = pY + (int)(6 * scale);
            char buf[128];

            Texture2D* slotTex = Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameSlot01b.png");
            if (slotTex && slotTex->id > 0)
                Draw9Slice(*slotTex, (Rectangle){ (float)pX, (float)pY, (float)pW, (float)pH }, 8, 8, 8, 8);
            else {
                DrawRectangle(pX, pY, pW, pH, (Color){ 0, 0, 0, 200 });
                DrawRectangleLines(pX, pY, pW, pH, (Color){ 80, 80, 80, 255 });
            }

            for (int i = 0; i < tileCount; i++) {
                const char* name = GetItemName(tileTypes[i]);
                int heal = GetItemHealAmount(tileTypes[i]);
                int qty = tileQtys[i];

                snprintf(buf, sizeof(buf), "%s x%d", name, qty);
                DrawText(buf, pX + (int)(6 * scale), ly, fs, BLACK); ly += lh;

                snprintf(buf, sizeof(buf), "Heals ~%d%% HP", heal);
                DrawText(buf, pX + (int)(6 * scale), ly, fs, BLACK); ly += lh;

                ly += (int)(4 * scale);
            }
            for (int i = 0; i < equipCount; i++) {
                const EquipData* d = GetEquipData(equipInfos[i]);
                if (!d) continue;
                int qty = equipQtys[i];
                snprintf(buf, sizeof(buf), "%s%s", d->name, qty > 1 ? " x2" : "");
                DrawText(buf, pX + (int)(6 * scale), ly, fs, BLACK); ly += lh;

                snprintf(buf, sizeof(buf), "%s", d->description);
                DrawText(buf, pX + (int)(6 * scale), ly, fs, BLACK); ly += lh;

                ly += (int)(4 * scale);
            }
        }
    }

    // Inventory overlay
    InventoryUI_Render(game, ui);

    // Shop overlay
    if (game->state == STATE_SHOP)
        ShopUI_Render(game, scale);
}
