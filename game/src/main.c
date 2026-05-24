#include "raylib.h"
#include "core/game.h"
#include "core/audio.h"
#include "ui/menu.h"
#include <string.h>
#include <time.h>

typedef enum {
    SCENE_MENU,
    SCENE_SETTINGS,
    SCENE_CONTROLS,
    SCENE_CREDITS,
    SCENE_GAME,
    SCENE_EXIT
} Scene;

int main(void)
{
    const int screenWidth = 1024;
    const int screenHeight = 768;

    InitWindow(screenWidth, screenHeight, "Heros of Taboo - Roguelike");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(screenWidth, screenHeight);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    SetRandomSeed((unsigned int)time(NULL));

    InitAudioSystem();

    Scene scene = SCENE_MENU;
    Game game;
    bool gameMenuOpen = false;
    bool settingsMenuOpen = false;
    bool prevEscDown = false;
    bool prevRDown = false;

    while (!WindowShouldClose() && scene != SCENE_EXIT)
    {
        UpdateMusicSystem(scene == SCENE_GAME);

        switch (scene)
        {
            // =================================================================
            case SCENE_MENU:
            {
                MenuAction action = Menu_Update();
                if (action == MENU_PLAY) {
                    const char* mapFiles[] = {
                        "resources/map.tmx", "resources/map.TMX",
                        "../resources/map.tmx", "./map.tmx", "map.tmx"
                    };
                    bool loaded = false;
                    for (int i = 0; i < 5; i++) {
                        if (FileExists(mapFiles[i])) {
                            loaded = InitGame(&game, mapFiles[i]);
                            if (loaded) break;
                        }
                    }
                    if (!loaded) loaded = InitGame(&game, "resources/");
                    if (loaded) {
                        gameMenuOpen = false;
                        settingsMenuOpen = false;
                        prevEscDown = false;
                        prevRDown = false;
                        scene = SCENE_GAME;
                    }
                } else if (action == MENU_SETTINGS) {
                    Menu_ResetSettings();
                    scene = SCENE_SETTINGS;
                } else if (action == MENU_CONTROLS) {
                    scene = SCENE_CONTROLS;
                } else if (action == MENU_CREDITS) {
                    scene = SCENE_CREDITS;
                } else if (action == MENU_EXIT) {
                    scene = SCENE_EXIT;
                }

                BeginDrawing();
                Menu_Render();
                EndDrawing();
                break;
            }

            // =================================================================
            case SCENE_SETTINGS:
            {
                BeginDrawing();
                Menu_SettingsRender();
                EndDrawing();
                if (Menu_SettingsUpdate() == MENU_PLAY) scene = SCENE_MENU;
                break;
            }

            // =================================================================
            case SCENE_CONTROLS:
            {
                BeginDrawing();
                Menu_ControlsRender();
                EndDrawing();
                if (Menu_ControlsUpdate() == MENU_PLAY) scene = SCENE_MENU;
                break;
            }

            // =================================================================
            case SCENE_CREDITS:
            {
                BeginDrawing();
                Menu_CreditsRender();
                EndDrawing();
                if (Menu_CreditsUpdate() == MENU_PLAY) scene = SCENE_MENU;
                break;
            }

            // =================================================================
            case SCENE_GAME:
            {
                Scene nextScene = SCENE_GAME;
                bool restartGame = false;

                // ESC rising-edge
                bool escDown = IsKeyDown(KEY_ESCAPE);
                bool escPressed = escDown && !prevEscDown;
                prevEscDown = escDown;

                if (escPressed) {
                    if (settingsMenuOpen) {
                        settingsMenuOpen = false;
                    } else if (gameMenuOpen) {
                        gameMenuOpen = false;
                    } else if (game.state == STATE_GAME_OVER || game.state == STATE_WIN) {
                        nextScene = SCENE_MENU;
                    } else {
                        gameMenuOpen = true;
                    }
                }

                if (settingsMenuOpen) {
                    Menu_SettingsUpdateGame();
                } else if (gameMenuOpen) {
                    MenuAction gmAction = GameMenu_Update();
                    if (gmAction == MENU_SETTINGS) { settingsMenuOpen = true; Menu_ResetSettings(); }
                    if (gmAction == MENU_PLAY) nextScene = SCENE_MENU;
                    if (gmAction == MENU_EXIT) nextScene = SCENE_EXIT;
                } else {
                    // R rising-edge
                    bool rDown = IsKeyDown(KEY_R);
                    if (rDown && !prevRDown) {
                        prevRDown = rDown;
                        restartGame = true;
                    } else {
                        prevRDown = rDown;
                    }

                    if (!restartGame) {
                        HandleInput(&game);
                        UpdateGame(&game);

                        float t = (game.animTimer <= 0.0f) ? 1.0f : 1.0f - (game.animTimer / MOVE_ANIM_DURATION);
                        float prevCX = (float)(game.player.ent.prevX * game.map->tileWidth);
                        float curCX  = (float)(game.player.ent.x * game.map->tileWidth);
                        float prevCY = (float)(game.player.ent.prevY * game.map->tileHeight);
                        float curCY  = (float)(game.player.ent.y * game.map->tileHeight);
                        game.camera.target = (Vector2){
                            prevCX * (1.0f - t) + curCX * t + game.map->tileWidth / 2,
                            prevCY * (1.0f - t) + curCY * t + game.map->tileHeight / 2
                        };
                    }
                }

                // Update camera offset every frame for window resize support
                game.camera.offset = (Vector2){ GetScreenWidth() / 2, GetScreenHeight() / 2 };

                // Always draw the frame before handling scene transitions
                BeginDrawing();
                ClearBackground(BLACK);
                RenderGame(&game);
                if (settingsMenuOpen) {
                    Menu_SettingsRenderGame();
                } else if (gameMenuOpen) {
                    GameMenu_Render();
                }
                EndDrawing();

                // Now handle deferred scene transitions (after EndDrawing)
                if (restartGame) {
                    CleanupGame(&game);
                    const char* mapFile = "resources/map.tmx";
                    if (!FileExists(mapFile)) mapFile = "../resources/map.tmx";
                    if (!FileExists(mapFile)) mapFile = "map.tmx";
                    if (!FileExists(mapFile)) mapFile = "resources/";
                    if (!InitGame(&game, mapFile)) gameMenuOpen = false;
                }
                if (nextScene != SCENE_GAME) {
                    CleanupGame(&game);
                    Menu_Reset();
                    scene = nextScene;
                }
                break;
            }

            default:
                break;
        }
    }

    CleanupGame(&game);
    ShutdownAudioSystem();
    CloseWindow();
    return 0;
}
