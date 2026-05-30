#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

#include "world.h"
#include "ui/inventory_ui.h"

void InputSystem_Inventory(GameWorld* game, InventoryUIState* ui);
void InputSystem_PlayerTurn(GameWorld* game, InventoryUIState* ui);

#endif
