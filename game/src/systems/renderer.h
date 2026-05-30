#ifndef RENDERER_H
#define RENDERER_H

#include "raylib.h"

typedef struct GameWorld GameWorld;
typedef struct InventoryUIState InventoryUIState;

float GetUIScale(void);
void Draw9Slice(Texture2D tex, Rectangle dest, int l, int t, int r, int b);
void RenderGame(GameWorld* game, const InventoryUIState* ui);

#endif
