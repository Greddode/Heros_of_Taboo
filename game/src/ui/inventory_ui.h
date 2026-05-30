#ifndef INVENTORY_UI_H
#define INVENTORY_UI_H

#include "world.h"
#include "inventory.h"

typedef struct InventoryUIState {
    int selection;
    int scrollOffset;
    int statsScrollCol1, statsScrollCol2;
    int statsActiveCol;
    int statsSelection;
    InventorySubState subState;
    InventoryTab tab;
    int actionSelection;
} InventoryUIState;

void InventoryUI_Init(InventoryUIState* ui);
void InventoryUI_Render(GameWorld* game, const InventoryUIState* ui);

#endif
