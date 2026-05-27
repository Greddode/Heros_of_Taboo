#ifndef INSPECTOR_H
#define INSPECTOR_H

#include "raylib.h"
#include "core/game.h"

typedef enum {
    INSPECTOR_MONSTER,
    INSPECTOR_ITEM
} InspectorType;

void Inspector_Render(const Game* game, InspectorType type, int x, int y, int w, int h);

#endif
