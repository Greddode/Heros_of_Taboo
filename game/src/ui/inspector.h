#ifndef INSPECTOR_H
#define INSPECTOR_H

#include "raylib.h"
#include "world.h"

typedef enum {
    INSPECTOR_MONSTER,
    INSPECTOR_ITEM
} InspectorType;

void Inspector_Render(const GameWorld* game, InspectorType type, int x, int y, int w, int h, int selectedSlot);

#endif
