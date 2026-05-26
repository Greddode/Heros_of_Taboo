#ifndef RENDERER_H
#define RENDERER_H

#include "raylib.h"

typedef struct Game Game;

void Draw9Slice(Texture2D tex, Rectangle dest, int l, int t, int r, int b);
void RenderGame(const Game* game);

#endif
