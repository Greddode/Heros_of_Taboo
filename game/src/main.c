#include "raylib.h"
#include "game.h"
#include "audio.h"
#include "game_audio.h"
#include "ui/menu.h"
#include "ui/map_ui.h"
#include "systems/input_system.h"
#include <string.h>
#include <time.h>

typedef enum {
    SCENE_MENU,
    SCENE_SETTINGS,
    SCENE_CONTROLS,
    SCENE_STORY,
    SCENE_CREDITS,
    SCENE_GAME,
    SCENE_EXIT
} Scene;

int main(void)
{
    const int screenWidth = 1024;
    const int screenHeight = 768;

    InitWindow(screenWidth, screenHeight, "Heroes of Taboo - Roguelike");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(screenWidth, screenHeight);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    SetRandomSeed((unsigned int)time(NULL));

    InitAudioSystem();
    GameAudio_Init();

    Scene scene = SCENE_MENU;
    GameWorld game;
    InventoryUIState invUI;
    InventoryUI_Init(&invUI);
    bool gameMenuOpen = false;
    bool settingsMenuOpen = false;
    bool prevEscDown = false;

    while (!WindowShouldClose() && scene != SCENE_EXIT)
    {
        GameAudio_SetContext(scene == SCENE_GAME);
        UpdateMusicSystem();

        switch (scene)
        {
            // =================================================================
            case SCENE_MENU:
            {
                MenuAction action = Menu_Update();
                if (action == MENU_PLAY) {
                    BeginDrawing();
                    ClearBackground(BLACK);
                    const char* loadText = "DROPPING...";
                    int fontSize = 40;
                    int tw = MeasureText(loadText, fontSize);
                    DrawText(loadText, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() / 2 - fontSize / 2, fontSize, WHITE);
                    EndDrawing();

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
                        scene = SCENE_GAME;
                    }
                } else if (action == MENU_SETTINGS) {
                    Menu_ResetSettings();
                    scene = SCENE_SETTINGS;
                } else if (action == MENU_CONTROLS) {
                    scene = SCENE_CONTROLS;
                } else if (action == MENU_STORY) {
                    Menu_ResetStory();
                    scene = SCENE_STORY;
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
            case SCENE_STORY:
            {
                BeginDrawing();
                Menu_StoryRender();
                EndDrawing();
                if (Menu_StoryUpdate() == MENU_PLAY) { Menu_ResetStory(); scene = SCENE_MENU; }
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

                bool escDown = IsKeyDown(KEY_ESCAPE);
                bool escPressed = escDown && !prevEscDown;
                prevEscDown = escDown;

                bool wasMap = (game.state == STATE_MAP);
                if (wasMap)
                    MapUI_Update(&game);

                if (escPressed && !wasMap) {
                    if (settingsMenuOpen) {
                        settingsMenuOpen = false;
                    } else if (gameMenuOpen) {
                        gameMenuOpen = false;
                    } else if (game.state == STATE_INVENTORY) {
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
                    bool shiftHeld = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
                    if (shiftHeld && IsKeyPressed(KEY_R)) {
                        restartGame = true;
                    }

                    if (!restartGame) {
                    InputSystem_Inventory(&game, &invUI);
                    InputSystem_PlayerTurn(&game, &invUI);
                    UpdateGame(&game);

                        float d = (game.animDuration > 0.0f) ? game.animDuration : MOVE_ANIM_DURATION;
                        float t = (game.animTimer <= 0.0f) ? 1.0f : 1.0f - (game.animTimer / d);
                        CPosition* pp = World_GetPosition(&game.ecs, game.playerEntity);
                        if (pp) {
                            float prevCX = (float)(pp->prevX * game.map->tileWidth);
                            float curCX  = (float)(pp->x * game.map->tileWidth);
                            float prevCY = (float)(pp->prevY * game.map->tileHeight);
                            float curCY  = (float)(pp->y * game.map->tileHeight);
                            game.camera.target = (Vector2){
                                prevCX * (1.0f - t) + curCX * t + game.map->tileWidth / 2,
                                prevCY * (1.0f - t) + curCY * t + game.map->tileHeight / 2
                            };
                        }
                    }
                }

                game.camera.offset = (Vector2){ GetScreenWidth() / 2, GetScreenHeight() / 2 };

                BeginDrawing();
                ClearBackground(BLACK);
                RenderGame(&game, &invUI);
                if (game.state == STATE_MAP) MapUI_Render(&game);
                if (settingsMenuOpen) {
                    Menu_SettingsRenderGame();
                } else if (gameMenuOpen) {
                    GameMenu_Render();
                }
                EndDrawing();

                if (restartGame) {
                    BeginDrawing();
                    ClearBackground(BLACK);
                    const char* loadText = "LOADING...";
                    int fontSize = 40;
                    int tw = MeasureText(loadText, fontSize);
                    DrawText(loadText, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() / 2 - fontSize / 2, fontSize, WHITE);
                    EndDrawing();

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
