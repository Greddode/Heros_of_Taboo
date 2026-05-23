#ifndef COMBAT_LOG_H
#define COMBAT_LOG_H

#include "raylib.h"

#define COMBAT_LOG_MAX 48   // Number of stored messages
#define COMBAT_LOG_LEN 128  // Max length of each message

// Ring buffer for combat events displayed on-screen
typedef struct {
    char entries[COMBAT_LOG_MAX][COMBAT_LOG_LEN];
    int count;    // Total messages added (for indexing into the ring)
} CombatLog;

// Append a formatted message to the combat log
void CombatLog_Add(CombatLog* log, const char* fmt, ...);

// Draw the last maxLines entries from the log, oldest at top, newest at bottom
void CombatLog_Render(const CombatLog* log, int x, int y, int maxLines, int fontSize);

#endif
