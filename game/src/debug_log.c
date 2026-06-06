#include "debug_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <windows.h>

#define TIMESTAMP_LEN 64

static FILE* s_logFile = NULL;
static int s_initialized = 0;
static char s_baseDir[MAX_PATH];

unsigned int g_debugCategories = DEBUG_ALL;

void DebugLog_SetFilter(unsigned int mask) {
    g_debugCategories = mask;
}

static void GetTimestamp(char* buf, int len) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    if (t) {
        strftime(buf, len, "%Y-%m-%d_%H-%M-%S", t);
    } else {
        snprintf(buf, len, "unknown");
    }
}

static void EnsureLogDirs(void) {
    if (s_initialized) return;
    s_initialized = 1;

    char exePath[MAX_PATH];
    DWORD plen = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (plen == 0 || plen >= MAX_PATH) return;

    char* sep = strrchr(exePath, '\\');
    if (!sep) return;
    *(sep + 1) = '\0';
    snprintf(s_baseDir, MAX_PATH, "%slogs", exePath);

    char latestDir[MAX_PATH], oldDir[MAX_PATH];
    snprintf(latestDir, MAX_PATH, "%s\\latest", s_baseDir);
    snprintf(oldDir,    MAX_PATH, "%s\\old",    s_baseDir);
    CreateDirectoryA(s_baseDir, NULL);
    CreateDirectoryA(latestDir, NULL);
    CreateDirectoryA(oldDir,    NULL);

    // Archive existing log from latest/ to old/ with timestamp
    char oldPath[MAX_PATH], archivedPath[MAX_PATH];
    snprintf(oldPath, MAX_PATH, "%s\\debug.log", latestDir);
    FILE* oldLog = fopen(oldPath, "r");
    if (oldLog) {
        fclose(oldLog);
        char ts[TIMESTAMP_LEN];
        GetTimestamp(ts, TIMESTAMP_LEN);
        snprintf(archivedPath, MAX_PATH, "%s\\debug_%s.log", oldDir, ts);
        rename(oldPath, archivedPath);
    }

    // Open fresh combined log
    char logPath[MAX_PATH];
    snprintf(logPath, MAX_PATH, "%s\\debug.log", latestDir);
    s_logFile = fopen(logPath, "w");
    if (s_logFile) {
        setbuf(s_logFile, NULL);
        char ts[TIMESTAMP_LEN];
        GetTimestamp(ts, TIMESTAMP_LEN);
        fprintf(s_logFile, "--- Heroes of Taboo Debug Log ---\n");
        fprintf(s_logFile, "--- Started: %s ---\n", ts);
        fprintf(s_logFile, "--- Categories: 0x%08X ---\n\n", g_debugCategories);
    }
}

void DebugLog_Write(unsigned int category, const char* fmt, ...) {
    if (!(g_debugCategories & category)) return;
    EnsureLogDirs();
    if (!s_logFile) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(s_logFile, fmt, args);
    fputc('\n', s_logFile);
    va_end(args);
}
