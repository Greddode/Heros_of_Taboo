#ifndef MENU_H
#define MENU_H

typedef enum {
    MENU_NONE,
    MENU_PLAY,
    MENU_SETTINGS,
    MENU_CONTROLS,
    MENU_STORY,
    MENU_CREDITS,
    MENU_EXIT
} MenuAction;

// Update the menu state and return the selected action
MenuAction Menu_Update(void);

// Draw the main menu
void Menu_Render(void);

// Update the credits screen, returns MENU_NONE while active, MENU_PLAY to go back
MenuAction Menu_CreditsUpdate(void);

// Draw the credits screen
void Menu_CreditsRender(void);

// Update the settings screen, returns MENU_PLAY to go back
MenuAction Menu_SettingsUpdate(void);

// Draw the settings screen
void Menu_SettingsRender(void);

// Update the in-game settings overlay (ESC handled by caller, volume keys only)
void Menu_SettingsUpdateGame(void);

// Draw the settings overlay panel (semi-transparent, like the pause menu)
void Menu_SettingsRenderGame(void);

// Update the story screen, returns MENU_PLAY to go back
MenuAction Menu_StoryUpdate(void);

// Draw the story screen
void Menu_StoryRender(void);

// Update the controls screen, returns MENU_PLAY to go back
MenuAction Menu_ControlsUpdate(void);

// Draw the controls screen
void Menu_ControlsRender(void);

// Update the in-game pause menu, returns selected action
MenuAction GameMenu_Update(void);

// Draw the in-game pause menu overlay (call after rendering the game)
void GameMenu_Render(void);

// Reset menu state (call when entering main menu)
void Menu_Reset(void);

// Reset story screen (free loaded text)
void Menu_ResetStory(void);

// Reset settings selection to "Music Volume"
void Menu_ResetSettings(void);

#endif
