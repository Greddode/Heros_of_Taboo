#ifndef RESOURCES_H
#define RESOURCES_H

#include "raylib.h"

Texture2D* Resources_LoadTexture(const char* path);
void Resources_UnloadAll(void);

#endif
