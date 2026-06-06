#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <stdarg.h>

// Debug log categories — bitmask enum
typedef enum {
    DEBUG_NONE       = 0,
    DEBUG_GENERATION = (1 << 0),
    DEBUG_SPAWNER    = (1 << 1),
    DEBUG_COMBAT     = (1 << 2),
    DEBUG_MOVEMENT   = (1 << 3),
    DEBUG_INVENTORY  = (1 << 4),
    DEBUG_AI         = (1 << 5),
    DEBUG_FLOOR      = (1 << 6),
    DEBUG_PLAYER     = (1 << 7),
    DEBUG_ALL        = (int)(0xFFFFFFFF)
} DebugCategory;

// Global filter — set via DebugLog_SetFilter
extern unsigned int g_debugCategories;

// Set the active debug categories (bitmask OR of DebugCategory values)
void DebugLog_SetFilter(unsigned int mask);

#ifdef DEBUG

void DebugLog_Write(unsigned int category, const char* fmt, ...);

#define DebugLog(category, fmt, ...) DebugLog_Write(category, fmt, ##__VA_ARGS__)

#else

#define DebugLog(category, fmt, ...) ((void)0)

#endif

#endif
