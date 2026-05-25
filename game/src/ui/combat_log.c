#include "combat_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// Append a formatted message to the ring buffer
void CombatLog_Add(CombatLog* log, const char* fmt, ...) {
    if (!log) return;
    int idx = log->count % COMBAT_LOG_MAX;
    va_list args;
    va_start(args, fmt);
    vsnprintf(log->entries[idx], COMBAT_LOG_LEN, fmt, args);
    va_end(args);
    log->entries[idx][COMBAT_LOG_LEN - 1] = '\0';
    log->count++;
}

// Draw the last maxLines entries anchored at bottom-right (y = bottom line)
void CombatLog_Render(const CombatLog* log, int x, int y, int maxLines, int fontSize) {
    if (!log) return;
    int total = log->count;
    int available = (total < COMBAT_LOG_MAX) ? total : COMBAT_LOG_MAX;
    int visible  = (available < maxLines) ? available : maxLines;
    if (visible <= 0) return;

    int lineH = fontSize + 3;
    int logW = 350;
    int logH = visible * lineH + 4;
    int bgY = y - logH;

    DrawRectangle(x - 4, bgY, logW, logH, (Color){ 0, 0, 0, 180 });

    int firstVirtual = total - visible;
    for (int i = 0; i < visible; i++) {
        int idx = (firstVirtual + i) % COMBAT_LOG_MAX;
        DrawText(log->entries[idx], x, bgY + 2 + i * lineH, fontSize, LIGHTGRAY);
    }
}
