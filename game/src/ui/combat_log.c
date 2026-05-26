#include "combat_log.h"
#include "core/game.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

void CombatLog_Add(CombatLog* log, Color color, const char* fmt, ...) {
    if (!log) return;
    int idx = log->count % COMBAT_LOG_MAX;
    va_list args;
    va_start(args, fmt);
    vsnprintf(log->entries[idx], COMBAT_LOG_LEN, fmt, args);
    va_end(args);
    log->entries[idx][COMBAT_LOG_LEN - 1] = '\0';
    log->colors[idx] = color;
    log->count++;
}

// Word-wrap a single message into a list of display lines (max 4 per message).
// Returns number of wrapped lines written to outLines (up to outMax).
static int WordWrapLine(const char* msg, int fontSize, int maxWidth, char outLines[][COMBAT_LOG_LEN], int outMax) {
    char buf[COMBAT_LOG_LEN];
    strncpy(buf, msg, COMBAT_LOG_LEN - 1);
    buf[COMBAT_LOG_LEN - 1] = '\0';

    int lines = 0;
    char line[COMBAT_LOG_LEN] = "";
    const char* token = strtok(buf, " ");
    while (token && lines < outMax) {
        char test[COMBAT_LOG_LEN];
        if (line[0])
            snprintf(test, sizeof(test), "%s %s", line, token);
        else
            snprintf(test, sizeof(test), "%s", token);

        if (MeasureText(test, fontSize) > maxWidth && line[0]) {
            strncpy(outLines[lines], line, COMBAT_LOG_LEN - 1);
            outLines[lines][COMBAT_LOG_LEN - 1] = '\0';
            lines++;
            line[0] = '\0';
            continue;
        }

        strncpy(line, test, COMBAT_LOG_LEN - 1);
        token = strtok(NULL, " ");
    }
    if (line[0] && lines < outMax) {
        strncpy(outLines[lines], line, COMBAT_LOG_LEN - 1);
        outLines[lines][COMBAT_LOG_LEN - 1] = '\0';
        lines++;
    }
    return lines;
}

void CombatLog_Render(const CombatLog* log, int x, int y, int maxLines, int fontSize,
                      Texture2D bgTex, int sliceMargin) {
    if (!log) return;
    int total = log->count;
    int available = (total < COMBAT_LOG_MAX) ? total : COMBAT_LOG_MAX;
    int visible  = (available < maxLines) ? available : maxLines;
    if (visible <= 0) return;

    int lineH = fontSize + 3;
    int logW = 350;

    // Build word-wrapped display lines
    #define MAX_WRAP 64
    char wrapLines[MAX_WRAP][COMBAT_LOG_LEN];
    Color wrapColors[MAX_WRAP];
    int wrapCount = 0;

    int margin = (bgTex.id > 0) ? sliceMargin + 4 : 4;
    int textMaxW = logW - margin * 2;

    int firstVirtual = total - visible;
    for (int i = 0; i < visible && wrapCount < MAX_WRAP; i++) {
        int idx = (firstVirtual + i) % COMBAT_LOG_MAX;
        const char* msg = log->entries[idx];
        Color c = log->colors[idx].a > 0 ? log->colors[idx] : LIGHTGRAY;

        if (MeasureText(msg, fontSize) <= textMaxW) {
            strncpy(wrapLines[wrapCount], msg, COMBAT_LOG_LEN - 1);
            wrapLines[wrapCount][COMBAT_LOG_LEN - 1] = '\0';
            wrapColors[wrapCount] = c;
            wrapCount++;
        } else {
            char linesBuf[4][COMBAT_LOG_LEN];
            int n = WordWrapLine(msg, fontSize, textMaxW, linesBuf, 4);
            for (int j = 0; j < n && wrapCount < MAX_WRAP; j++) {
                strncpy(wrapLines[wrapCount], linesBuf[j], COMBAT_LOG_LEN - 1);
                wrapLines[wrapCount][COMBAT_LOG_LEN - 1] = '\0';
                wrapColors[wrapCount] = c;
                wrapCount++;
            }
        }
    }

    // Background sized to fit wrapped lines
    int logH = wrapCount * lineH + 4;
    int bgY = y - logH;

    if (bgTex.id > 0)
        Draw9Slice(bgTex, (Rectangle){ (float)(x - 4), (float)bgY, (float)logW, (float)logH },
                   sliceMargin, sliceMargin, sliceMargin, sliceMargin);
    else
        DrawRectangle(x - 4, bgY, logW, logH, (Color){ 0, 0, 0, 180 });

    // Draw wrapped text, first line gets the original indent
    for (int i = 0; i < wrapCount; i++)
        DrawText(wrapLines[i], x, bgY + 2 + i * lineH, fontSize, wrapColors[i]);
}
