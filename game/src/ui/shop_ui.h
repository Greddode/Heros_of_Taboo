#ifndef SHOP_UI_H
#define SHOP_UI_H

#include "world.h"

typedef enum {
    SHOP_SECTION_BUY = 0,
    SHOP_SECTION_SELL
} ShopSection;

void ShopUI_Render(GameWorld* game, float scale);
void ShopUI_HandleInput(GameWorld* game);

#endif
