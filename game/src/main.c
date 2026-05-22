#include "raylib.h"
#include "game.h"
#include <string.h>
#include <time.h>

int main(void)
{
    const int screenWidth = 1024;
    const int screenHeight = 768;

    InitWindow(screenWidth, screenHeight, "Heros of Taboo - Roguelike");
    SetTargetFPS(60);
    SetExitKey(KEY_ESCAPE);
    
    // Initialize random seed
    SetRandomSeed((unsigned int)time(NULL));
    
    Game game;
    bool gameLoaded = false;
    
    // Try to load the map - first check for a TMX file
    const char* mapFiles[] = {
        "resources/map.tmx",
        "resources/map.TMX",
        "../resources/map.tmx",
        "./map.tmx",
        "map.tmx"
    };
    
    int numMaps = sizeof(mapFiles) / sizeof(mapFiles[0]);
    for (int i = 0; i < numMaps; i++) {
        if (FileExists(mapFiles[i])) {
            TraceLog(LOG_INFO, "Found map: %s", mapFiles[i]);
            gameLoaded = InitGame(&game, mapFiles[i]);
            if (gameLoaded) break;
        }
    }
    
    if (!gameLoaded) {
        TraceLog(LOG_WARNING, "No TMX map found. Creating a procedural test map.");
        // We'll create a hardcoded fallback since we can't easily create
        // a TMX file from code. This serves as a built-in test.
        TraceLog(LOG_WARNING, "Place a map.tmx in the resources/ folder to use Tiled maps.");
        
        // For now, just show instructions
        while (!WindowShouldClose())
        {
            BeginDrawing();
            ClearBackground(BLACK);
            
            DrawText("TURN-BASED ROGUELIKE DEMO", 200, 150, 30, YELLOW);
            DrawText("No map.tmx found!", 300, 200, 20, RED);
            DrawText("Create a Tiled map with:", 200, 250, 20, WHITE);
            DrawText("  - Orthogonal orientation", 200, 280, 20, WHITE);
            DrawText("  - CSV layer format", 200, 310, 20, WHITE);
            DrawText("  - Layer 0 = walls (any tile)", 200, 340, 20, WHITE);
            DrawText("  - Object layer with 'player' or 'Player' type", 200, 370, 20, WHITE);
            DrawText("  - Object layer with 'goblin'/'skeleton'/'orc' types for enemies", 200, 400, 20, WHITE);
            DrawText("", 200, 430, 20, WHITE);
            DrawText("Save as: resources/map.tmx", 200, 460, 20, GREEN);
            DrawText("Press ESC to exit", 200, 500, 20, GRAY);
            
            EndDrawing();
        }
        
        CloseWindow();
        return 0;
    }

    // Main game loop
    while (!WindowShouldClose())
    {
        // Handle restart
        if (IsKeyPressed(KEY_R) && (game.state == STATE_GAME_OVER || game.state == STATE_WIN)) {
            CleanupGame(&game);
            // Reload the same map
            const char* mapFile = "resources/map.tmx";
            if (!FileExists(mapFile)) mapFile = "../resources/map.tmx";
            if (!FileExists(mapFile)) mapFile = "map.tmx";
            gameLoaded = InitGame(&game, mapFile);
            if (!gameLoaded) break;
        }
        
        // Handle input during player turn
        HandleInput(&game);
        
        // Update game (process enemy turns)
        UpdateGame(&game);
        
        // Update camera to follow player
        game.camera.target = (Vector2){
            game.player.x * game.map->tileWidth + game.map->tileWidth / 2,
            game.player.y * game.map->tileHeight + game.map->tileHeight / 2
        };
        
        // Draw
        BeginDrawing();
            ClearBackground(BLACK);
            RenderGame(&game);
        EndDrawing();
    }

    CleanupGame(&game);
    CloseWindow();

    return 0;
}