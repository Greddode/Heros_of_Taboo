#include "audio.h"
#include "raylib.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MUSIC_CONTEXTS   16
#define MAX_SOUND_CATEGORIES 32
#define MAX_TRACKS_PER_CTX   16
#define MAX_SOUNDS_PER_CAT   16

typedef struct {
    char   name[64];
    char   dirPath[256];
    Music  tracks[MAX_TRACKS_PER_CTX];
    int    trackCount;
    int    currentTrack;
    float  lastTimePlayed;
    bool   loaded;
} MusicContext;

typedef struct {
    char  name[64];
    char  dirPath[256];
    Sound sounds[MAX_SOUNDS_PER_CAT];
    int   soundCount;
    bool  loaded;
} SoundCategory;

static MusicContext   s_musicContexts[MAX_MUSIC_CONTEXTS];
static int            s_musicContextCount = 0;

static SoundCategory  s_soundCategories[MAX_SOUND_CATEGORIES];
static int            s_soundCategoryCount = 0;

static AudioMusicContext s_activeContext = AUDIO_INVALID;
static bool              s_audioReady   = false;
static float             s_musicVolume  = 0.5f;
static float             s_sfxVolume    = 0.5f;

static void LoadSoundsFromDir(const char *dirPath, Sound *sounds, int *count, int maxCount) {
    FilePathList files = LoadDirectoryFiles(dirPath);
    for (unsigned int i = 0; i < files.count && *count < maxCount; i++) {
        if (IsFileExtension(files.paths[i], ".wav")) {
            sounds[*count] = LoadSound(files.paths[i]);
            if (sounds[*count].stream.buffer != NULL) {
                (*count)++;
            }
        }
    }
    UnloadDirectoryFiles(files);
}

static void LoadMusicFromDir(const char *dirPath, Music *tracks, int *count, int maxCount) {
    FilePathList files = LoadDirectoryFiles(dirPath);
    for (unsigned int i = 0; i < files.count && *count < maxCount; i++) {
        if (IsFileExtension(files.paths[i], ".ogg")) {
            tracks[*count] = LoadMusicStream(files.paths[i]);
            if (tracks[*count].stream.buffer != NULL) {
                tracks[*count].looping = true;
                (*count)++;
            }
        }
    }
    UnloadDirectoryFiles(files);
}

static int PickNextTrack(MusicContext *ctx) {
    if (ctx->trackCount <= 0) return -1;
    if (ctx->trackCount == 1) return 0;
    int next;
    do {
        next = GetRandomValue(0, ctx->trackCount - 1);
    } while (next == ctx->currentTrack);
    return next;
}

void InitAudioSystem(void) {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    s_audioReady = true;
}

AudioMusicContext Audio_RegisterMusicContext(const char* name, const char* dirPath) {
    if (s_musicContextCount >= MAX_MUSIC_CONTEXTS) return AUDIO_INVALID;
    MusicContext* ctx = &s_musicContexts[s_musicContextCount];
    strncpy(ctx->name, name, sizeof(ctx->name) - 1);
    ctx->name[sizeof(ctx->name) - 1] = '\0';
    strncpy(ctx->dirPath, dirPath, sizeof(ctx->dirPath) - 1);
    ctx->dirPath[sizeof(ctx->dirPath) - 1] = '\0';
    ctx->trackCount     = 0;
    ctx->currentTrack   = -1;
    ctx->lastTimePlayed = 0.0f;
    ctx->loaded         = false;
    return s_musicContextCount++;
}

AudioSoundCategory Audio_RegisterSoundCategory(const char* name, const char* dirPath) {
    if (s_soundCategoryCount >= MAX_SOUND_CATEGORIES) return AUDIO_INVALID;
    SoundCategory* cat = &s_soundCategories[s_soundCategoryCount];
    strncpy(cat->name, name, sizeof(cat->name) - 1);
    cat->name[sizeof(cat->name) - 1] = '\0';
    strncpy(cat->dirPath, dirPath, sizeof(cat->dirPath) - 1);
    cat->dirPath[sizeof(cat->dirPath) - 1] = '\0';
    cat->soundCount = 0;
    cat->loaded     = false;
    return s_soundCategoryCount++;
}

void Audio_SetMusicContext(AudioMusicContext handle) {
    if (!s_audioReady) return;

    if (s_activeContext != AUDIO_INVALID) {
        MusicContext* cur = &s_musicContexts[s_activeContext];
        if (cur->currentTrack >= 0)
            StopMusicStream(cur->tracks[cur->currentTrack]);
    }

    s_activeContext = handle;
    if (handle == AUDIO_INVALID) return;

    MusicContext* ctx = &s_musicContexts[handle];

    if (!ctx->loaded) {
        LoadMusicFromDir(ctx->dirPath, ctx->tracks, &ctx->trackCount, MAX_TRACKS_PER_CTX);
        ctx->loaded = true;
    }

    if (ctx->trackCount == 0) return;
    ctx->currentTrack   = PickNextTrack(ctx);
    ctx->lastTimePlayed = 0.0f;
    PlayMusicStream(ctx->tracks[ctx->currentTrack]);
    SetMusicVolume(ctx->tracks[ctx->currentTrack], s_musicVolume);
}

AudioMusicContext Audio_GetMusicContext(void) {
    return s_activeContext;
}

void UpdateMusicSystem(void) {
    if (!s_audioReady || s_activeContext == AUDIO_INVALID) return;
    MusicContext* ctx = &s_musicContexts[s_activeContext];
    if (ctx->currentTrack < 0 || ctx->trackCount == 0) return;

    Music* m = &ctx->tracks[ctx->currentTrack];
    float played = GetMusicTimePlayed(*m);
    float len    = GetMusicTimeLength(*m);

    if (played < ctx->lastTimePlayed && len > 0.5f) {
        StopMusicStream(*m);
        ctx->currentTrack = PickNextTrack(ctx);
        m = &ctx->tracks[ctx->currentTrack];
        PlayMusicStream(*m);
        SetMusicVolume(*m, s_musicVolume);
    } else {
        UpdateMusicStream(*m);
    }
    ctx->lastTimePlayed = played;
}

void Audio_PlaySound(AudioSoundCategory handle) {
    if (!s_audioReady || handle < 0 || handle >= s_soundCategoryCount) return;
    SoundCategory* cat = &s_soundCategories[handle];

    if (!cat->loaded) {
        LoadSoundsFromDir(cat->dirPath, cat->sounds, &cat->soundCount, MAX_SOUNDS_PER_CAT);
        cat->loaded = true;
    }
    if (cat->soundCount == 0) return;

    int idx = GetRandomValue(0, cat->soundCount - 1);
    SetSoundVolume(cat->sounds[idx], s_sfxVolume);
    PlaySound(cat->sounds[idx]);
}

float Audio_GetMusicVolume(void) {
    return s_musicVolume;
}

void Audio_SetMusicVolume(float vol) {
    s_musicVolume = (vol < 0.0f) ? 0.0f : (vol > 1.0f) ? 1.0f : vol;
    if (!s_audioReady || s_activeContext == AUDIO_INVALID) return;
    MusicContext* ctx = &s_musicContexts[s_activeContext];
    if (ctx->currentTrack >= 0)
        SetMusicVolume(ctx->tracks[ctx->currentTrack], s_musicVolume);
}

float Audio_GetSFXVolume(void) {
    return s_sfxVolume;
}

void Audio_SetSFXVolume(float vol) {
    s_sfxVolume = (vol < 0.0f) ? 0.0f : (vol > 1.0f) ? 1.0f : vol;
}

void ShutdownAudioSystem(void) {
    if (!s_audioReady) return;

    for (int i = 0; i < s_musicContextCount; i++) {
        MusicContext* ctx = &s_musicContexts[i];
        if (!ctx->loaded) continue;
        for (int j = 0; j < ctx->trackCount; j++)
            StopMusicStream(ctx->tracks[j]);
        for (int j = 0; j < ctx->trackCount; j++)
            UnloadMusicStream(ctx->tracks[j]);
        ctx->trackCount     = 0;
        ctx->currentTrack   = -1;
        ctx->lastTimePlayed = 0.0f;
        ctx->loaded         = false;
    }

    for (int i = 0; i < s_soundCategoryCount; i++) {
        SoundCategory* cat = &s_soundCategories[i];
        if (!cat->loaded) continue;
        for (int j = 0; j < cat->soundCount; j++)
            UnloadSound(cat->sounds[j]);
        cat->soundCount = 0;
        cat->loaded     = false;
    }

    s_musicContextCount  = 0;
    s_soundCategoryCount = 0;
    s_activeContext      = AUDIO_INVALID;

    CloseAudioDevice();
    s_audioReady = false;
}
