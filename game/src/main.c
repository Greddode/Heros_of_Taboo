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

    // Seed the RNG with current time for varied runs
    SetRandomSeed((unsigned int)time(NULL));

    Game game;
    bool gameLoaded = false;

    // --- Map loading: try TMX files first, then fall back to procedural generation ---
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

    // Fallback: no TMX found, generate a dungeon procedurally
    if (!gameLoaded) {
        TraceLog(LOG_WARNING, "No TMX map found. Generating procedural map...");
        gameLoaded = InitGame(&game, "resources/");
    }

    // If everything failed, show an error screen and exit
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

    // --- Main game loop: input → update → render ---
    while (!WindowShouldClose())
    {
        // Press R to restart from scratch
        if (IsKeyPressed(KEY_R)) {
            CleanupGame(&game);
            const char* mapFile = "resources/map.tmx";
            if (!FileExists(mapFile)) mapFile = "../resources/map.tmx";
            if (!FileExists(mapFile)) mapFile = "map.tmx";
            if (!FileExists(mapFile)) mapFile = "resources/";
            gameLoaded = InitGame(&game, mapFile);
            if (!gameLoaded) break;
        }

        // Read player input and move the player (only during player turn)
        HandleInput(&game);

        // Run monster AI and advance animation timers
        UpdateGame(&game);

        // Centre the camera on the player's current tile
        game.camera.target = (Vector2){
            game.player.ent.x * game.map->tileWidth + game.map->tileWidth / 2,
            game.player.ent.y * game.map->tileHeight + game.map->tileHeight / 2
        };

        // Clear screen and draw everything
        BeginDrawing();
            ClearBackground(BLACK);
            RenderGame(&game);
        EndDrawing();
    }

    // --- Clean shutdown ---
    CleanupGame(&game);
    CloseWindow();

    return 0;
}