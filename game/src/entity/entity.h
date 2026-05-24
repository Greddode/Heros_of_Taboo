#ifndef ENTITY_H
#define ENTITY_H

#include "raylib.h"
#include <stdbool.h>

// Direction an entity can move
typedef enum {
    DIR_NONE = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

// Shared entity type used for the player
typedef struct {
    char name[32];
    int x, y;              // Current tile position
    int prevX, prevY;      // Previous tile position (smooth interpolation)
    int hp, maxHp;
    int attack, defense;
    int level;
    int expValue;
    bool alive;
    bool isPlayer;
    Color color;
    bool facingRight;
    int animFrame;         // Animation frame index (spritesheet row offset)
    float hitFlashTimer;   // >0 when entity was just hit (white flash)
    Texture2D spriteSheet; // Character spritesheet (player only)
    int spriteRow;         // Row index in the spritesheet
} Entity;

// Forward declaration so entity functions can accept Game* without circular includes
typedef struct Game Game;

// Convert tile coordinates to pixel position (top-left corner)
Vector2 TileToScreen(int x, int y, int tileWidth, int tileHeight);

// Returns true if (x, y) is in bounds, not a wall, and not occupied
bool IsWalkable(const Game* game, int x, int y);

// Find a non-excluded entity at a tile. Currently only checks the player.
Entity* GetEntityAt(Game* game, int x, int y, Entity* exclude);

// Attempt to move an entity one tile in a direction.
// Returns true if the move or an attack occurred.
bool MoveEntity(Game* game, Entity* entity, Direction dir);

// Draw a single tile from the tileset at layer layerIndex at (x, y)
void DrawTile(const Game* game, int x, int y, int layerIndex);

#endif
