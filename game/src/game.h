#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "tmx.h"
#include <stdbool.h>

#define MAX_ENEMIES 64
#define MAX_ENTITIES 65 // player + enemies
#define MAX_HEALING 32
#define MAP_WIDTH 100
#define MAP_HEIGHT 100

// Directions for movement
typedef enum {
    DIR_NONE = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

// Entity types
typedef enum {
    ENTITY_PLAYER,
    ENTITY_GOBLIN,
    ENTITY_SKELETON,
    ENTITY_ORC,
    ENTITY_OGRE
} EntityType;

// An entity in the game (player or enemy)
typedef struct {
    char name[32];
    EntityType type;
    int x, y;           // Tile position
    int prevX, prevY;   // Previous position for smooth movement
    int hp;
    int maxHp;
    int attack;
    int defense;
    int level;
    int expValue;
    bool alive;
    bool isPlayer;
    Color color;        // For rendering without sprites
    int tileGID;        // Optional tile GID for rendering
    
    // AI state
    int moveTimer;      // Countdown for AI movement
    bool hasMovedThisTurn;
    
    // Animation
    int animFrame;      // 0-3 for 4-frame animation cycle
    bool facingRight;   // true = facing right, false = facing left
} Entity;

// Game state
typedef enum {
    STATE_PLAYER_TURN,
    STATE_ENEMY_TURN,
    STATE_GAME_OVER,
    STATE_WIN
} GameState;

typedef struct {
    MapData* map;
    Texture2D tilesetTexture;
    
    Entity player;
    Entity enemies[MAX_ENEMIES];
    int enemyCount;
    
    int turnCount;
    GameState state;
    
    Camera2D camera;
    
    // Visibility map (simple field-of-view)
    // 0 = not visible, 1 = visible, 2 = explored
    unsigned char visibility[MAP_HEIGHT][MAP_WIDTH];
    
    // Blocking map: 1 = wall/blocked, 0 = passable
    unsigned char blocking[MAP_HEIGHT][MAP_WIDTH];
    // Entity sprite sheets
    Texture2D entitySprite;  // 64x16 sprite sheet (4 frames x 16px) for entity animations
    
    // Animation
    float animTimer;         // Time accumulator for continuous animations
    
    // Healing items
    int healingCount;
    int healingTiles[MAX_HEALING][2];  // x,y positions
    bool healingCollected[MAX_HEALING];
} Game;

// Initialize game from a TMX map
bool InitGame(Game* game, const char* tmxFile);

// Cleanup game
void CleanupGame(Game* game);

// Handle input for player turn
void HandleInput(Game* game);

// Update game logic (enemy turns, animations, etc.)
void UpdateGame(Game* game);

// Render the game
void RenderGame(const Game* game);

// Check if a tile is walkable
bool IsWalkable(const Game* game, int x, int y);

// Get the tile pixel position (top-left corner)
Vector2 TileToScreen(int x, int y, int tileWidth, int tileHeight);

// Get the tile coordinates from screen position
Vector2 ScreenToTile(float screenX, float screenY, int tileWidth, int tileHeight);

// Simple line-of-sight check (Bresenham)
bool HasLineOfSight(const Game* game, int x0, int y0, int x1, int y1, int maxDist);

// Compute field of view
void ComputeFOV(Game* game, int px, int py, int radius);

// Move an entity in a direction, returns true if moved
bool MoveEntity(Game* game, Entity* entity, Direction dir);

// Get entity at tile position (excluding the specified entity)
Entity* GetEntityAt(Game* game, int x, int y, Entity* exclude);

// Enemy AI
void ProcessEnemyAI(Game* game, Entity* enemy);

// Draw a single tile
void DrawTile(const Game* game, int x, int y, int layerIndex);

#endif // GAME_H