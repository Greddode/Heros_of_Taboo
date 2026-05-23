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
        TraceLog(LOG_WARNING, "No TMX map found. Generating procedural map...");
        gameLoaded = InitGame(&game, "resources/");
    }

    if (!gameLoaded) {
        TraceLog(LOG_ERROR, "Could not initialize game.");
        while (!WindowShouldClose())
        {
            BeginDrawing();
            ClearBackground(BLACK);
            DrawText("Failed to initialize game!", 200, 200, 20, RED);
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
        if (IsKeyPressed(KEY_R)) {
            CleanupGame(&game);
            const char* mapFile = "resources/map.tmx";
            if (!FileExists(mapFile)) mapFile = "../resources/map.tmx";
            if (!FileExists(mapFile)) mapFile = "map.tmx";
            if (!FileExists(mapFile)) mapFile = "resources/";
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