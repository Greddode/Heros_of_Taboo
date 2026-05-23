#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "tmx.h"
#include <stdbool.h>

#define MAX_HEALING 32
#define MAP_WIDTH 100
#define MAP_HEIGHT 100

// Direction an entity can move
typedef enum {
    DIR_NONE = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

// Shared entity type used for the player (monsters have their own Monster struct)
typedef struct {
    char name[32];
    int x, y;              // Current tile position
    int prevX, prevY;      // Previous tile position (for smooth interpolation)
    int hp, maxHp;
    int attack, defense;
    int level;
    int expValue;
    bool alive;
    bool isPlayer;         // True for the player entity, false for monsters using this struct
    Color color;
    bool facingRight;
    int animFrame;         // 0-3 for 4-frame animation cycle
    float hitFlashTimer;   // >0 when entity was just hit (draws white flash)
} Entity;

// Turn / state machine
typedef enum {
    STATE_PLAYER_TURN,
    STATE_ENEMY_TURN,
    STATE_GAME_OVER,
    STATE_WIN
} GameState;

// Top-level game state
typedef struct {
    MapData* map;           // Current map (TMX-loaded or procedurally generated)
    Texture2D tilesetTexture;

    Entity player;          // The hero

    int turnCount;
    GameState state;

    Camera2D camera;        // Camera centred on the player

    // Visibility grid: 0 = unseen, 1 = currently visible, 2 = explored
    unsigned char visibility[MAP_HEIGHT][MAP_WIDTH];

    // Blocking grid: 1 = wall or other obstruction, 0 = passable
    unsigned char blocking[MAP_HEIGHT][MAP_WIDTH];

    // Healing pickups placed on the map
    int healingCount;
    int healingTiles[MAX_HEALING][2];   // [count][x, y]
    bool healingCollected[MAX_HEALING];
} Game;

// Load a TMX map (or generate one procedurally if tmxFile is a directory).
// Returns false on failure.
bool InitGame(Game* game, const char* tmxFile);

// Free all assets and zero the Game struct.
void CleanupGame(Game* game);

// Poll keyboard and move the player if it is the player's turn.
void HandleInput(Game* game);

// Advance monster AI, animations, and timers. Must be called once per frame.
void UpdateGame(Game* game);

// Draw the tile layers, entities, and HUD.
void RenderGame(const Game* game);

// Returns true if (x, y) is in bounds, not a wall, and not occupied.
bool IsWalkable(const Game* game, int x, int y);

// Convert tile coordinates to pixel position (top-left corner).
Vector2 TileToScreen(int x, int y, int tileWidth, int tileHeight);

// Recalculate the shadowcast FOV from (px, py) with given sight radius.
// NOTE: Currently unused — InitGame sets all tiles to visible (no fog).
void ComputeFOV(Game* game, int px, int py, int radius);

// Attempt to move an entity one tile in a direction.
// Returns true if the move or an attack occurred.
bool MoveEntity(Game* game, Entity* entity, Direction dir);

// Find a non-excluded entity at a tile. Currently only checks the player.
Entity* GetEntityAt(Game* game, int x, int y, Entity* exclude);

// Draw a single tile from the tileset at layer layerIndex at (x, y).
void DrawTile(const Game* game, int x, int y, int layerIndex);

#endif // GAME_H