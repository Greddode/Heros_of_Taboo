#include "resources.h"
#include <string.h>

#define MAX_CACHED_TEXTURES 64

typedef struct {
    char path[512];
    Texture2D tex;
    bool used;
} CachedTexture;

static CachedTexture s_cache[MAX_CACHED_TEXTURES];
static int s_cacheCount = 0;
static Texture2D s_nullTexture = { 0 };

Texture2D* Resources_LoadTexture(const char* path) {
    for (int i = 0; i < s_cacheCount; i++) {
        if (s_cache[i].used && strcmp(s_cache[i].path, path) == 0) {
            return &s_cache[i].tex;
        }
    }

    if (s_cacheCount >= MAX_CACHED_TEXTURES) {
        return &s_nullTexture;
    }

    Texture2D tex = LoadTexture(path);
    if (tex.id == 0) return &s_nullTexture;

    CachedTexture* entry = &s_cache[s_cacheCount++];
    strncpy(entry->path, path, sizeof(entry->path) - 1);
    entry->path[sizeof(entry->path) - 1] = '\0';
    entry->tex = tex;
    entry->used = true;
    return &entry->tex;
}

void Resources_UnloadAll(void) {
    for (int i = 0; i < s_cacheCount; i++) {
        if (s_cache[i].used) {
            UnloadTexture(s_cache[i].tex);
            s_cache[i].used = false;
        }
    }
    s_cacheCount = 0;
}
