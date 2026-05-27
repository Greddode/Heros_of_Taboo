#include "menu.h"
#include "text_data.h"
#include "core/audio.h"
#include "../core/game.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MENU_OPTION_COUNT 6
#define GAME_MENU_OPTION_COUNT 3

static const char* s_options[MENU_OPTION_COUNT] = {
    "Play",
    "Settings",
    "Controls",
    "Story",
    "Credits",
    "Exit"
};

static const char* s_gameMenuOptions[GAME_MENU_OPTION_COUNT] = {
    "Return to Main Menu",
    "Settings",
    "Exit Game"
};

static int s_selection = 0;
static int s_menuScroll = 0;
static int s_gameMenuSel = 0;
static int s_settingsSel = 0;
static int s_settingsScroll = 0;
static float s_guiScale = 1.0f; // GUI scale multiplier (1.0 = default)
static int s_controlsScroll = 0;
static int s_creditsScroll = 0;



MenuAction Menu_Update(void) {
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        if (s_selection > 0) {
            s_selection--;
            if (s_selection < s_menuScroll) {
                s_menuScroll = s_selection;
            }
        }
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        if (s_selection < MENU_OPTION_COUNT - 1) {
            s_selection++;
            float scale = GetUIScale();
            int sh = GetScreenHeight();
            int optionSpacing = (int)(45 * scale);
            int optionY = sh / 2;
            int viewH = sh - optionY - (int)(40 * scale);
            int maxVisible = viewH / optionSpacing;
            if (maxVisible < 1) maxVisible = 1;
            if (s_selection >= s_menuScroll + maxVisible) {
                s_menuScroll = s_selection - maxVisible + 1;
            }
        }
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        switch (s_selection) {
            case 0: return MENU_PLAY;
            case 1: return MENU_SETTINGS;
            case 2: return MENU_CONTROLS;
            case 3: return MENU_STORY;
            case 4: return MENU_CREDITS;
            case 5: return MENU_EXIT;
        }
    }
    return MENU_NONE;
}

void Menu_Render(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float scale = GetUIScale();

    ClearBackground((Color){ 15, 15, 25, 255 });

    int titleSize = (int)(60 * scale);
    const char* title = "Heroes of Taboo";
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (sw - titleW) / 2, sh / 4, titleSize, (Color){ 200, 180, 50, 255 });

    int subSize = (int)(20 * scale);
    const char* subtitle = "A Roguelike Adventure";
    int subW = MeasureText(subtitle, subSize);
    DrawText(subtitle, (sw - subW) / 2, sh / 4 + (int)(70 * scale), subSize, (Color){ 150, 150, 180, 255 });

    int optionSize = (int)(30 * scale);
    int optionSpacing = (int)(45 * scale);
    int optionY = sh / 2;
    int viewH = sh - optionY - (int)(40 * scale);
    int maxVisible = viewH / optionSpacing;
    if (maxVisible < 1) maxVisible = 1;

    BeginScissorMode(0, optionY, sw, viewH);
    for (int i = s_menuScroll; i < MENU_OPTION_COUNT; i++) {
        int y = optionY + (i - s_menuScroll) * optionSpacing;
        if (y > optionY + viewH) break;
        Color c = (i == s_selection) ? (Color){ 255, 255, 100, 255 } : LIGHTGRAY;
        int textW = MeasureText(s_options[i], optionSize);
        DrawText(s_options[i], (sw - textW) / 2, y, optionSize, c);
        if (i == s_selection) {
            DrawText("> ", (sw - textW) / 2 - (int)(30 * scale), y, optionSize, (Color){ 255, 255, 100, 255 });
        }
    }
    EndScissorMode();

    int hintSize = (int)(14 * scale);
    const char* hint = "Use Arrow Keys / WASD to navigate, Enter to select";
    int hintW = MeasureText(hint, hintSize);
    DrawText(hint, (sw - hintW) / 2, sh - (int)(30 * scale), hintSize, (Color){ 80, 80, 100, 255 });
}

MenuAction Menu_CreditsUpdate(void) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
        return MENU_PLAY;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        s_creditsScroll -= (int)(22 * s_guiScale);
        if (s_creditsScroll < 0) s_creditsScroll = 0;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        s_creditsScroll += (int)(22 * s_guiScale);
    }
    return MENU_NONE;
}

void Menu_Reset(void) {
    s_selection = 0;
    s_menuScroll = 0;
    s_gameMenuSel = 0;
    s_settingsSel = 0;
    s_settingsScroll = 0;
    s_controlsScroll = 0;
    s_creditsScroll = 0;
}

void Menu_ResetSettings(void) {
    s_settingsSel = 0;
}

MenuAction GameMenu_Update(void) {
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        s_gameMenuSel--;
        if (s_gameMenuSel < 0) s_gameMenuSel = 0;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        s_gameMenuSel++;
        if (s_gameMenuSel >= GAME_MENU_OPTION_COUNT) s_gameMenuSel = GAME_MENU_OPTION_COUNT - 1;
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
    float scale = GetUIScale();

    DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 160 });

    int panelW = (int)(360 * scale);
    int panelH = (int)((GAME_MENU_OPTION_COUNT * 50 + 80) * scale);
    int px = (sw - panelW) / 2;
    int py = (sh - panelH) / 2;
    DrawRectangle(px, py, panelW, panelH, (Color){ 20, 20, 35, 230 });
    DrawRectangleLines(px, py, panelW, panelH, (Color){ 100, 100, 140, 255 });

    int titleSize = (int)(36 * scale);
    const char* title = "Paused";
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (sw - titleW) / 2, py + (int)(20 * scale), titleSize, (Color){ 200, 180, 50, 255 });

    int optionSize = (int)(22 * scale);
    int optionSpacing = (int)(45 * scale);
    for (int i = 0; i < GAME_MENU_OPTION_COUNT; i++) {
        Color c = (i == s_gameMenuSel) ? (Color){ 255, 255, 100, 255 } : LIGHTGRAY;
        int textW = MeasureText(s_gameMenuOptions[i], optionSize);
        DrawText(s_gameMenuOptions[i], (sw - textW) / 2, py + (int)(70 * scale) + i * optionSpacing, optionSize, c);
        if (i == s_gameMenuSel) {
            DrawText("> ", (sw - textW) / 2 - (int)(25 * scale), py + (int)(70 * scale) + i * optionSpacing, optionSize, (Color){ 255, 255, 100, 255 });
        }
    }
}

static int s_storyScroll = 0;
static int s_storySel = 0;
static int s_storyLineCount = 0;
static char** s_storyLines = NULL;
static char* s_storyBuf = NULL;
static char** s_visualLines = NULL;
static int s_visualLineCount = 0;
static int s_visualCapacity = 0;

static void Story_LoadFile(void) {
    if (s_storyBuf) return;

    int len = sizeof(s_story_data);
    s_storyBuf = (char*)RL_MALLOC(len + 1);
    if (!s_storyBuf) return;
    for (int i = 0; i < len; i++) {
        s_storyBuf[i] = (char)(s_story_data[i] ^ 0xAB);
    }
    s_storyBuf[len] = '\0';

    int writeIdx = 0;
    for (int readIdx = 0; readIdx < len; readIdx++) {
        if (s_storyBuf[readIdx] != '\r') {
            s_storyBuf[writeIdx++] = s_storyBuf[readIdx];
        }
    }
    s_storyBuf[writeIdx] = '\0';
    int finalLen = writeIdx;

    // Replace UTF-8 em dash (U+2014 = 0xE2 0x80 0x94) with "--" (the default font doesn't have it)
    int di = 0;
    for (int si = 0; si < finalLen; si++) {
        unsigned char c = (unsigned char)s_storyBuf[si];
        if (c == 0xE2 && si + 2 < finalLen && (unsigned char)s_storyBuf[si+1] == 0x80 && (unsigned char)s_storyBuf[si+2] == 0x94) {
            s_storyBuf[di++] = '-';
            s_storyBuf[di++] = '-';
            si += 2;
        } else {
            s_storyBuf[di++] = c;
        }
    }
    s_storyBuf[di] = '\0';

    s_storyLineCount = 1;
    for (char* p = s_storyBuf; *p; p++) {
        if (*p == '\n') s_storyLineCount++;
    }

    s_storyLines = (char**)RL_MALLOC(s_storyLineCount * sizeof(char*));
    if (!s_storyLines) { RL_FREE(s_storyBuf); s_storyBuf = NULL; return; }

    s_storyLines[0] = s_storyBuf;
    int lineIdx = 1;
    for (char* p = s_storyBuf; *p; p++) {
        if (*p == '\n') {
            *p = '\0';
            s_storyLines[lineIdx++] = p + 1;
        }
    }
}

static void Story_FreeVisualLines(void) {
    for (int i = 0; i < s_visualLineCount; i++) {
        if (s_visualLines[i]) RL_FREE(s_visualLines[i]);
    }
    if (s_visualLines) RL_FREE(s_visualLines);
    s_visualLines = NULL;
    s_visualLineCount = 0;
    s_visualCapacity = 0;
}

static void Story_BuildVisualLines(int sw) {
    Story_FreeVisualLines();
    if (!s_storyLines) return;

    float scale = GetUIScale();
    int fontSize = (int)(18 * scale);
    int availW = sw - (int)(60 * scale);
    if (availW < (int)(80 * scale)) availW = (int)(80 * scale);

    for (int i = 0; i < s_storyLineCount; i++) {
        const char* src = s_storyLines[i];
        int srcLen = strlen(src);

        char* tmp = (char*)RL_MALLOC(srcLen + 1);
        if (!tmp) return;
        strcpy(tmp, src);

        char* words[256];
        int wordCount = 0;
        char* tok = strtok(tmp, " ");
        while (tok && wordCount < 256) {
            words[wordCount++] = tok;
            tok = strtok(NULL, " ");
        }

        if (wordCount == 0) {
            if (s_visualLineCount >= s_visualCapacity) {
                s_visualCapacity = s_visualCapacity ? s_visualCapacity * 2 : 64;
                s_visualLines = (char**)RL_REALLOC(s_visualLines, s_visualCapacity * sizeof(char*));
                if (!s_visualLines) { s_visualCapacity = 0; RL_FREE(tmp); return; }
            }
            s_visualLines[s_visualLineCount] = (char*)RL_MALLOC(1);
            if (s_visualLines[s_visualLineCount]) s_visualLines[s_visualLineCount][0] = '\0';
            s_visualLineCount++;
            RL_FREE(tmp);
            continue;
        }

        char lineBuf[1024];
        int linePx = 0;
        lineBuf[0] = '\0';

        for (int w = 0; w < wordCount; w++) {
            int wordW = MeasureText(words[w], fontSize);
            int spaceW = MeasureText(" ", fontSize);
            int needSep = linePx > 0 ? spaceW : 0;

            if (linePx + needSep + wordW > availW && linePx > 0) {
                if (s_visualLineCount >= s_visualCapacity) {
                    s_visualCapacity = s_visualCapacity ? s_visualCapacity * 2 : 64;
                    s_visualLines = (char**)RL_REALLOC(s_visualLines, s_visualCapacity * sizeof(char*));
                    if (!s_visualLines) { s_visualCapacity = 0; RL_FREE(tmp); return; }
                }
                s_visualLines[s_visualLineCount] = (char*)RL_MALLOC(strlen(lineBuf) + 1);
                if (s_visualLines[s_visualLineCount]) strcpy(s_visualLines[s_visualLineCount], lineBuf);
                s_visualLineCount++;
                lineBuf[0] = '\0';
                linePx = 0;
                needSep = 0;
            }

            if (needSep) strcat(lineBuf, " ");
            strcat(lineBuf, words[w]);
            linePx += needSep + wordW;
        }

        if (s_visualLineCount >= s_visualCapacity) {
            s_visualCapacity = s_visualCapacity ? s_visualCapacity * 2 : 64;
            s_visualLines = (char**)RL_REALLOC(s_visualLines, s_visualCapacity * sizeof(char*));
            if (!s_visualLines) { s_visualCapacity = 0; RL_FREE(tmp); return; }
        }
        s_visualLines[s_visualLineCount] = (char*)RL_MALLOC(strlen(lineBuf) + 1);
        if (s_visualLines[s_visualLineCount]) strcpy(s_visualLines[s_visualLineCount], lineBuf);
        s_visualLineCount++;

        RL_FREE(tmp);
    }
}

MenuAction Menu_StoryUpdate(void) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
        return MENU_PLAY;
    }

    int scrollDelta = 0;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) scrollDelta = -1;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) scrollDelta = 1;

    if (scrollDelta != 0) {
        s_storySel += scrollDelta;
        if (s_storySel < 0) s_storySel = 0;
        if (s_visualLineCount > 0 && s_storySel >= s_visualLineCount) s_storySel = s_visualLineCount - 1;

        // Ensure selected line is visible
        float scale = GetUIScale();
        int sh = GetScreenHeight();
        int titleSize = (int)(50 * scale);
        int lineH = (int)(22 * scale);
        int y = sh / 12 + titleSize + (int)(20 * scale);
        int viewH = sh - y - (int)(60 * scale);
        int maxVisible = viewH / lineH;
        if (maxVisible < 1) maxVisible = 1;

        if (s_storySel < s_storyScroll) {
            s_storyScroll = s_storySel;
        } else if (s_storySel >= s_storyScroll + maxVisible) {
            s_storyScroll = s_storySel - maxVisible + 1;
        }
    }
    return MENU_NONE;
}

void Menu_StoryRender(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float scale = GetUIScale();

    ClearBackground((Color){ 15, 15, 25, 255 });

    int titleSize = (int)(50 * scale);
    const char* title = "Story";
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (sw - titleW) / 2, sh / 12, titleSize, (Color){ 200, 180, 50, 255 });

    Story_LoadFile();

    Story_BuildVisualLines(sw);

    if (s_visualLineCount > 0) {
        if (s_storyScroll >= s_visualLineCount) s_storyScroll = s_visualLineCount - 1;
        if (s_storySel >= s_visualLineCount) s_storySel = s_visualLineCount - 1;

        int lineH = (int)(22 * scale);
        int y = sh / 12 + titleSize + (int)(20 * scale);
        int viewH = sh - y - (int)(60 * scale);
        int maxLines = viewH / lineH;
        if (maxLines < 1) maxLines = 1;

        int end = s_storyScroll + maxLines;
        if (end > s_visualLineCount) end = s_visualLineCount;

        BeginScissorMode(0, y, sw, viewH);
        for (int i = s_storyScroll; i < end; i++) {
            bool isSel = (i == s_storySel);
            Color c = isSel ? (Color){ 255, 255, 100, 255 } : LIGHTGRAY;
            int textW = MeasureText(s_visualLines[i], (int)(18 * scale));
            int lx = (sw - textW) / 2;
            if (isSel) {
                DrawText("> ", lx - (int)(22 * scale), y, (int)(18 * scale), (Color){ 255, 255, 100, 255 });
                DrawRectangle(lx - (int)(4 * scale), y - (int)(2 * scale), textW + (int)(8 * scale), lineH, (Color){ 40, 40, 60, 200 });
            }
            DrawText(s_visualLines[i], lx, y, (int)(18 * scale), c);
            y += lineH;
        }
        EndScissorMode();

        if (s_visualLineCount > maxLines) {
            int totalScroll = s_visualLineCount - maxLines;
            const char* scrollHint = (s_storyScroll == 0)
                ? "UP/DOWN - Scroll     ESC - Back"
                : (s_storyScroll >= totalScroll)
                    ? "At end - ESC - Back"
                    : "UP/DOWN - Scroll     ESC - Back";
            int hintSize = (int)(16 * scale);
            int hintW = MeasureText(scrollHint, hintSize);
            DrawText(scrollHint, (sw - hintW) / 2, sh - (int)(40 * scale), hintSize, (Color){ 100, 100, 130, 255 });
        }
    } else {
        const char* err = "Could not load story.txt";
        int errSize = (int)(20 * scale);
        int errW = MeasureText(err, errSize);
        DrawText(err, (sw - errW) / 2, sh / 2, errSize, RED);
        const char* hint = "ESC - Back";
        int hintSize = (int)(16 * scale);
        int hintW = MeasureText(hint, hintSize);
        DrawText(hint, (sw - hintW) / 2, sh - (int)(40 * scale), hintSize, (Color){ 100, 100, 130, 255 });
    }
}

void Menu_ResetStory(void) {
    Story_FreeVisualLines();
    if (s_storyBuf) {
        RL_FREE(s_storyBuf);
        s_storyBuf = NULL;
    }
    if (s_storyLines) {
        RL_FREE(s_storyLines);
        s_storyLines = NULL;
    }
    s_storyScroll = 0;
    s_storySel = 0;
    s_storyLineCount = 0;
}

MenuAction Menu_ControlsUpdate(void) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
        return MENU_PLAY;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        s_controlsScroll -= (int)(22 * s_guiScale);
        if (s_controlsScroll < 0) s_controlsScroll = 0;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        s_controlsScroll += (int)(22 * s_guiScale);
    }
    return MENU_NONE;
}

void Menu_ControlsRender(void) {
    RenderTextScreen(s_controls_data, sizeof(s_controls_data),
                     "ESC / BACKSPACE - Back to Menu", s_controlsScroll);
}

void Menu_CreditsRender(void) {
    RenderTextScreen(s_credits_data, sizeof(s_credits_data),
                     "ESC / BACKSPACE - Back to Menu", s_creditsScroll);
}

static void DrawVolumeBarItem(int cx, int cy, const char* label, float vol, bool selected, int fontSize, int barW, bool isGuiScale) {
    int labelW = MeasureText(label, fontSize);

    Color labelColor = selected ? (Color){ 255, 255, 100, 255 } : LIGHTGRAY;
    int labelOffsetX = cx - labelW / 2;
    if (selected) {
        int arrowX = labelOffsetX - fontSize - 4;
        DrawText("> ", arrowX, cy - fontSize - 8, fontSize, (Color){ 255, 255, 100, 255 });
    }
    DrawText(label, labelOffsetX, cy - fontSize - 8, fontSize, labelColor);

    int barH = fontSize;
    int barX = cx - barW / 2;
    int barY = cy;
    DrawRectangle(barX, barY, barW, barH, (Color){ 50, 50, 70, 255 });
    DrawRectangleLines(barX, barY, barW, barH, (Color){ 100, 100, 130, 255 });

    if (isGuiScale) {
        // For GUI scale: vol is the actual scale (1.0-4.0)
        // Convert to slider position: 1.0 -> 0.25, 2.0 -> 0.5, 3.0 -> 0.75, 4.0 -> 1.0
        float sliderPos = (vol - 1.0f) / 3.0f; // Maps 1.0-4.0 to 0.0-1.0
        sliderPos = sliderPos * 0.75f + 0.25f; // Maps 0.0-1.0 to 0.25-1.0
        
        int fillW = (int)(barW * sliderPos);
        if (fillW > 0) {
            DrawRectangle(barX, barY, fillW, barH, (Color){ 100, 200, 100, 255 });
        }
        
        // Display as 1.00x, 1.25x, 2.00x, 3.00x, 4.00x etc. instead of percentage
        char scaleText[16];
        snprintf(scaleText, sizeof(scaleText), "%.2fx", vol);
        int textFont = fontSize;
        int textW = MeasureText(scaleText, textFont);
        DrawText(scaleText, cx - textW / 2, barY + barH + 4, textFont, LIGHTGRAY);
    } else {
        // Regular percentage display for volume
        int volPercent = (int)(vol * 100.0f);
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
}

static void Settings_UpdateSel(bool inc) {
    if (inc) {
        s_settingsSel++;
        if (s_settingsSel > 2) s_settingsSel = 2;
        // Scroll down if selection past visible area
        float scale = GetUIScale();
        int sh = GetScreenHeight();
        int cy = sh / 2 - (int)(30 * scale);
        int itemSpacing = (int)(70 * scale);
        int viewH = sh - cy - (int)(40 * scale);
        int maxVisible = viewH / itemSpacing;
        if (maxVisible < 1) maxVisible = 1;
        if (s_settingsSel >= s_settingsScroll + maxVisible) {
            s_settingsScroll = s_settingsSel - maxVisible + 1;
        }
    } else {
        s_settingsSel--;
        if (s_settingsSel < 0) s_settingsSel = 0;
        if (s_settingsSel < s_settingsScroll) {
            s_settingsScroll = s_settingsSel;
        }
    }
}

static void Settings_AdjustVol(int dir) {
    if (s_settingsSel == 0) {
        float vol = GetMusicVolume() + 0.05f * dir;
        if (vol < 0.0f) vol = 0.0f;
        if (vol > 1.0f) vol = 1.0f;
        SetAudioVolume(vol);
    } else if (s_settingsSel == 1) {
        float vol = GetSFXVolume() + 0.05f * dir;
        if (vol < 0.0f) vol = 0.0f;
        if (vol > 1.0f) vol = 1.0f;
        SetSFXVolume(vol);
    } else if (s_settingsSel == 2) {
        float scale = GetGuiScale() + 0.25f * dir; // Change by 0.25 increments
        if (scale < 1.0f) scale = 1.0f;
        if (scale > 4.0f) scale = 4.0f;
        SetGuiScale(scale);
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
    float scale = GetUIScale();

    ClearBackground((Color){ 15, 15, 25, 255 });

    int titleSize = (int)(50 * scale);
    const char* title = "Settings";
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (sw - titleW) / 2, sh / 5, titleSize, (Color){ 200, 180, 50, 255 });

    int cx = sw / 2;
    int cy = sh / 2 - (int)(30 * scale);
    int itemSpacing = (int)(70 * scale);
    int itemSize = (int)(22 * scale);
    int barW = (int)(260 * scale);

    int viewY = cy;
    int viewH = sh - cy - (int)(40 * scale);
    int maxVisible = viewH / itemSpacing;
    if (maxVisible < 1) maxVisible = 1;

    BeginScissorMode(0, viewY, sw, viewH);
    for (int i = 0; i < 3; i++) {
        int idx = i + s_settingsScroll;
        if (idx >= 3) break;
        int itemY = cy + i * itemSpacing;
        bool isSelected = (idx == s_settingsSel);
        if (idx == 0) {
            DrawVolumeBarItem(cx, itemY, "Music Volume", GetMusicVolume(), isSelected, itemSize, barW, false);
        } else if (idx == 1) {
            DrawVolumeBarItem(cx, itemY, "SFX Volume", GetSFXVolume(), isSelected, itemSize, barW, false);
        } else if (idx == 2) {
            DrawVolumeBarItem(cx, itemY, "GUI Scale", GetGuiScale(), isSelected, itemSize, barW, true);
        }
    }
    EndScissorMode();

    int hintSize = (int)(14 * scale);
    const char* hint = "UP / DOWN - select     LEFT / RIGHT or A / D - adjust     ESC - Back";
    int hintW = MeasureText(hint, hintSize);
    DrawText(hint, (sw - hintW) / 2, sh - (int)(30 * scale), hintSize, (Color){ 80, 80, 100, 255 });
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
    float scale = GetUIScale();

    DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 160 });

    int panelW = (int)(360 * scale);
    int panelH = (int)(235 * scale);
    int px = (sw - panelW) / 2;
    int py = (sh - panelH) / 2;
    DrawRectangle(px, py, panelW, panelH, (Color){ 20, 20, 35, 230 });
    DrawRectangleLines(px, py, panelW, panelH, (Color){ 100, 100, 140, 255 });

    int titleSize = (int)(20 * scale);
    const char* title = "Settings";
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (sw - titleW) / 2, py + (int)(8 * scale), titleSize, (Color){ 200, 180, 50, 255 });

    int cx = sw / 2;
    int itemSize = (int)(13 * scale);
    int itemSpacing = (int)(50 * scale);
    int barW = (int)(170 * scale);

    if (s_settingsSel == 0) {
        DrawVolumeBarItem(cx, py + (int)(68 * scale), "Music Volume", GetMusicVolume(), true, itemSize, barW, false);
        DrawVolumeBarItem(cx, py + (int)(68 * scale) + itemSpacing, "SFX Volume", GetSFXVolume(), false, itemSize, barW, false);
        DrawVolumeBarItem(cx, py + (int)(68 * scale) + itemSpacing * 2, "GUI Scale", GetGuiScale(), false, itemSize, barW, true);
    } else if (s_settingsSel == 1) {
        DrawVolumeBarItem(cx, py + (int)(68 * scale), "Music Volume", GetMusicVolume(), false, itemSize, barW, false);
        DrawVolumeBarItem(cx, py + (int)(68 * scale) + itemSpacing, "SFX Volume", GetSFXVolume(), true, itemSize, barW, false);
        DrawVolumeBarItem(cx, py + (int)(68 * scale) + itemSpacing * 2, "GUI Scale", GetGuiScale(), false, itemSize, barW, true);
    } else if (s_settingsSel == 2) {
        DrawVolumeBarItem(cx, py + (int)(68 * scale), "Music Volume", GetMusicVolume(), false, itemSize, barW, false);
        DrawVolumeBarItem(cx, py + (int)(68 * scale) + itemSpacing, "SFX Volume", GetSFXVolume(), false, itemSize, barW, false);
        DrawVolumeBarItem(cx, py + (int)(68 * scale) + itemSpacing * 2, "GUI Scale", GetGuiScale(), true, itemSize, barW, true);
    }

    int hintSize = (int)(10 * scale);
    const char* hint = "UP/DOWN select    L/R adjust    ESC - Back";
    int hintW = MeasureText(hint, hintSize);
    DrawText(hint, (sw - hintW) / 2, py + panelH - (int)(14 * scale), hintSize, (Color){ 80, 80, 100, 255 });
}
