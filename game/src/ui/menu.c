#include "menu.h"
#include "text_data.h"
#include "core/audio.h"
#include "raylib.h"
#include <stdio.h>

#define MENU_OPTION_COUNT 5
#define GAME_MENU_OPTION_COUNT 3

static const char* s_options[MENU_OPTION_COUNT] = {
    "Play",
    "Settings",
    "Controls",
    "Credits",
    "Exit"
};

static const char* s_gameMenuOptions[GAME_MENU_OPTION_COUNT] = {
    "Return to Main Menu",
    "Settings",
    "Exit Game"
};

static int s_selection = 0;
static int s_gameMenuSel = 0;
static int s_settingsSel = 0;



MenuAction Menu_Update(void) {
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        s_selection--;
        if (s_selection < 0) s_selection = MENU_OPTION_COUNT - 1;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        s_selection++;
        if (s_selection >= MENU_OPTION_COUNT) s_selection = 0;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        switch (s_selection) {
            case 0: return MENU_PLAY;
            case 1: return MENU_SETTINGS;
            case 2: return MENU_CONTROLS;
            case 3: return MENU_CREDITS;
            case 4: return MENU_EXIT;
        }
    }
    return MENU_NONE;
}

void Menu_Render(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    ClearBackground((Color){ 15, 15, 25, 255 });

    const char* title = "Heros of Taboo";
    int titleW = MeasureText(title, 60);
    DrawText(title, (sw - titleW) / 2, sh / 4, 60, (Color){ 200, 180, 50, 255 });

    const char* subtitle = "A Roguelike Adventure";
    int subW = MeasureText(subtitle, 20);
    DrawText(subtitle, (sw - subW) / 2, sh / 4 + 70, 20, (Color){ 150, 150, 180, 255 });

    int optionY = sh / 2;
    for (int i = 0; i < MENU_OPTION_COUNT; i++) {
        Color c = (i == s_selection) ? (Color){ 255, 255, 100, 255 } : LIGHTGRAY;
        int textW = MeasureText(s_options[i], 30);
        DrawText(s_options[i], (sw - textW) / 2, optionY + i * 45, 30, c);
        if (i == s_selection) {
            DrawText("> ", (sw - textW) / 2 - 30, optionY + i * 45, 30, (Color){ 255, 255, 100, 255 });
        }
    }

    const char* hint = "Use Arrow Keys / WASD to navigate, Enter to select";
    int hintW = MeasureText(hint, 14);
    DrawText(hint, (sw - hintW) / 2, sh - 30, 14, (Color){ 80, 80, 100, 255 });
}

MenuAction Menu_CreditsUpdate(void) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
        return MENU_PLAY;
    }
    return MENU_NONE;
}

void Menu_Reset(void) {
    s_selection = 0;
    s_gameMenuSel = 0;
    s_settingsSel = 0;
}

void Menu_ResetSettings(void) {
    s_settingsSel = 0;
}

MenuAction GameMenu_Update(void) {
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        s_gameMenuSel--;
        if (s_gameMenuSel < 0) s_gameMenuSel = GAME_MENU_OPTION_COUNT - 1;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        s_gameMenuSel++;
        if (s_gameMenuSel >= GAME_MENU_OPTION_COUNT) s_gameMenuSel = 0;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        switch (s_gameMenuSel) {
            case 0: return MENU_PLAY;
            case 1: return MENU_SETTINGS;
            case 2: return MENU_EXIT;
        }
    }
    return MENU_NONE;
}

void GameMenu_Render(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 160 });

    int panelW = 360;
    int panelH = GAME_MENU_OPTION_COUNT * 50 + 80;
    int px = (sw - panelW) / 2;
    int py = (sh - panelH) / 2;
    DrawRectangle(px, py, panelW, panelH, (Color){ 20, 20, 35, 230 });
    DrawRectangleLines(px, py, panelW, panelH, (Color){ 100, 100, 140, 255 });

    const char* title = "Paused";
    int titleW = MeasureText(title, 36);
    DrawText(title, (sw - titleW) / 2, py + 20, 36, (Color){ 200, 180, 50, 255 });

    for (int i = 0; i < GAME_MENU_OPTION_COUNT; i++) {
        Color c = (i == s_gameMenuSel) ? (Color){ 255, 255, 100, 255 } : LIGHTGRAY;
        int textW = MeasureText(s_gameMenuOptions[i], 22);
        DrawText(s_gameMenuOptions[i], (sw - textW) / 2, py + 70 + i * 45, 22, c);
        if (i == s_gameMenuSel) {
            DrawText("> ", (sw - textW) / 2 - 25, py + 70 + i * 45, 22, (Color){ 255, 255, 100, 255 });
        }
    }
}

MenuAction Menu_ControlsUpdate(void) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
        return MENU_PLAY;
    }
    return MENU_NONE;
}

void Menu_ControlsRender(void) {
    RenderTextScreen(s_controls_data, sizeof(s_controls_data),
                     "ESC / BACKSPACE - Back to Menu");
}

void Menu_CreditsRender(void) {
    RenderTextScreen(s_credits_data, sizeof(s_credits_data),
                     "ESC / BACKSPACE - Back to Menu");
}

static void DrawVolumeBarItem(int cx, int cy, const char* label, float vol, bool selected, int fontSize, int barW) {
    int labelW = MeasureText(label, fontSize);

    Color labelColor = selected ? (Color){ 255, 255, 100, 255 } : LIGHTGRAY;
    int labelOffsetX = cx - labelW / 2;
    if (selected) {
        int arrowX = labelOffsetX - fontSize - 4;
        DrawText("> ", arrowX, cy - fontSize - 8, fontSize, (Color){ 255, 255, 100, 255 });
    }
    DrawText(label, labelOffsetX, cy - fontSize - 8, fontSize, labelColor);

    int volPercent = (int)(vol * 100.0f);
    int barH = fontSize;
    int barX = cx - barW / 2;
    int barY = cy;
    DrawRectangle(barX, barY, barW, barH, (Color){ 50, 50, 70, 255 });
    DrawRectangleLines(barX, barY, barW, barH, (Color){ 100, 100, 130, 255 });

    int fillW = (int)(barW * vol);
    if (fillW > 0) {
        DrawRectangle(barX, barY, fillW, barH, (Color){ 100, 200, 100, 255 });
    }

    char pctText[8];
    snprintf(pctText, sizeof(pctText), "%d%%", volPercent);
    int pctFont = fontSize - 4;
    int pctW = MeasureText(pctText, pctFont);
    DrawText(pctText, cx - pctW / 2, barY + barH + 4, pctFont, LIGHTGRAY);
}

static void Settings_UpdateSel(bool inc) {
    if (inc) {
        s_settingsSel++;
        if (s_settingsSel > 1) s_settingsSel = 1;
    } else {
        s_settingsSel--;
        if (s_settingsSel < 0) s_settingsSel = 0;
    }
}

static void Settings_AdjustVol(int dir) {
    float step = 0.05f;
    if (s_settingsSel == 0) {
        float vol = GetMusicVolume() + step * dir;
        if (vol < 0.0f) vol = 0.0f;
        if (vol > 1.0f) vol = 1.0f;
        SetAudioVolume(vol);
    } else {
        float vol = GetSFXVolume() + step * dir;
        if (vol < 0.0f) vol = 0.0f;
        if (vol > 1.0f) vol = 1.0f;
        SetSFXVolume(vol);
    }
}

MenuAction Menu_SettingsUpdate(void) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
        return MENU_PLAY;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) Settings_UpdateSel(false);
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) Settings_UpdateSel(true);
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) Settings_AdjustVol(-1);
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) Settings_AdjustVol(1);
    return MENU_NONE;
}

void Menu_SettingsRender(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    ClearBackground((Color){ 15, 15, 25, 255 });

    const char* title = "Settings";
    int titleW = MeasureText(title, 50);
    DrawText(title, (sw - titleW) / 2, sh / 5, 50, (Color){ 200, 180, 50, 255 });

    int cx = sw / 2;
    int cy = sh / 2 - 30;

    DrawVolumeBarItem(cx, cy, "Music Volume", GetMusicVolume(), s_settingsSel == 0, 22, 260);
    DrawVolumeBarItem(cx, cy + 70, "SFX Volume", GetSFXVolume(), s_settingsSel == 1, 22, 260);

    const char* hint = "UP / DOWN - select     LEFT / RIGHT or A / D - adjust     ESC - Back";
    int hintW = MeasureText(hint, 14);
    DrawText(hint, (sw - hintW) / 2, sh - 30, 14, (Color){ 80, 80, 100, 255 });
}

void Menu_SettingsUpdateGame(void) {
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) Settings_UpdateSel(false);
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) Settings_UpdateSel(true);
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) Settings_AdjustVol(-1);
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) Settings_AdjustVol(1);
}

void Menu_SettingsRenderGame(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 160 });

    int panelW = 360;
    int panelH = 185;
    int px = (sw - panelW) / 2;
    int py = (sh - panelH) / 2;
    DrawRectangle(px, py, panelW, panelH, (Color){ 20, 20, 35, 230 });
    DrawRectangleLines(px, py, panelW, panelH, (Color){ 100, 100, 140, 255 });

    const char* title = "Settings";
    int titleW = MeasureText(title, 20);
    DrawText(title, (sw - titleW) / 2, py + 8, 20, (Color){ 200, 180, 50, 255 });

    int cx = sw / 2;
    DrawVolumeBarItem(cx, py + 68, "Music Volume", GetMusicVolume(), s_settingsSel == 0, 13, 170);
    DrawVolumeBarItem(cx, py + 118, "SFX Volume", GetSFXVolume(), s_settingsSel == 1, 13, 170);

    const char* hint = "UP/DOWN select    L/R adjust    ESC - Back";
    int hintW = MeasureText(hint, 10);
    DrawText(hint, (sw - hintW) / 2, py + panelH - 14, 10, (Color){ 80, 80, 100, 255 });
}
