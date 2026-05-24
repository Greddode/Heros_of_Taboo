#include "audio.h"
#include "raylib.h"
#include <stddef.h>

#define MENU_MUSIC_PATH  "resources/audio/music/Abnormal Circumstances.mp3"
#define GAME_MUSIC_PATH  "resources/audio/music/Element.mp3"
#define MAX_HIT_SOUNDS     16
#define MAX_PICKUP_SOUNDS  16

typedef enum {
    TRACK_NONE,
    TRACK_MENU,
    TRACK_GAME
} ActiveTrack;

static Music s_menuMusic;
static Music s_gameMusic;
static ActiveTrack s_activeTrack = TRACK_NONE;
static bool s_audioReady = false;
static float s_musicVolume = 0.5f;
static float s_sfxVolume = 0.5f;

static Sound s_hitSounds[MAX_HIT_SOUNDS];
static int s_hitSoundCount = 0;

static Sound s_pickupSounds[MAX_PICKUP_SOUNDS];
static int s_pickupSoundCount = 0;

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

static void LoadHitSounds(void) {
    LoadSoundsFromDir("resources/audio/sounds/hit", s_hitSounds, &s_hitSoundCount, MAX_HIT_SOUNDS);
}

static void LoadPickupSounds(void) {
    LoadSoundsFromDir("resources/audio/sounds/pickup", s_pickupSounds, &s_pickupSoundCount, MAX_PICKUP_SOUNDS);
}

void InitAudioSystem(void) {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;

    s_menuMusic = LoadMusicStream(MENU_MUSIC_PATH);
    s_gameMusic = LoadMusicStream(GAME_MUSIC_PATH);

    s_menuMusic.looping = true;
    s_gameMusic.looping = true;

    LoadHitSounds();
    LoadPickupSounds();

    s_audioReady = true;
}

void UpdateMusicSystem(bool inGame) {
    if (!s_audioReady) return;

    ActiveTrack desired = inGame ? TRACK_GAME : TRACK_MENU;

    if (desired != s_activeTrack) {
        // Stop current
        switch (s_activeTrack) {
            case TRACK_MENU: StopMusicStream(s_menuMusic); break;
            case TRACK_GAME: StopMusicStream(s_gameMusic); break;
            default: break;
        }

        // Start new
        switch (desired) {
            case TRACK_MENU:
                PlayMusicStream(s_menuMusic);
                SetMusicVolume(s_menuMusic, s_musicVolume);
                break;
            case TRACK_GAME:
                PlayMusicStream(s_gameMusic);
                SetMusicVolume(s_gameMusic, s_musicVolume);
                break;
            default:
                break;
        }

        s_activeTrack = desired;
    }

    // Stream the active track every frame
    switch (s_activeTrack) {
        case TRACK_MENU: UpdateMusicStream(s_menuMusic); break;
        case TRACK_GAME: UpdateMusicStream(s_gameMusic); break;
        default: break;
    }
}

float GetMusicVolume(void) {
    return s_musicVolume;
}

void SetAudioVolume(float vol) {
    s_musicVolume = (vol < 0.0f) ? 0.0f : (vol > 1.0f) ? 1.0f : vol;
    if (!s_audioReady) return;
    SetMusicVolume(s_menuMusic, s_musicVolume);
    SetMusicVolume(s_gameMusic, s_musicVolume);
}

float GetSFXVolume(void) {
    return s_sfxVolume;
}

void SetSFXVolume(float vol) {
    s_sfxVolume = (vol < 0.0f) ? 0.0f : (vol > 1.0f) ? 1.0f : vol;
}

void PlayHitSound(void) {
    if (!s_audioReady || s_hitSoundCount == 0) return;
    int idx = GetRandomValue(0, s_hitSoundCount - 1);
    SetSoundVolume(s_hitSounds[idx], s_sfxVolume);
    PlaySound(s_hitSounds[idx]);
}

void PlayPickupSound(void) {
    if (!s_audioReady || s_pickupSoundCount == 0) return;
    int idx = GetRandomValue(0, s_pickupSoundCount - 1);
    SetSoundVolume(s_pickupSounds[idx], s_sfxVolume);
    PlaySound(s_pickupSounds[idx]);
}

void ShutdownAudioSystem(void) {
    if (!s_audioReady) return;
    StopMusicStream(s_menuMusic);
    StopMusicStream(s_gameMusic);
    for (int i = 0; i < s_hitSoundCount; i++) UnloadSound(s_hitSounds[i]);
    s_hitSoundCount = 0;
    for (int i = 0; i < s_pickupSoundCount; i++) UnloadSound(s_pickupSounds[i]);
    s_pickupSoundCount = 0;
    UnloadMusicStream(s_menuMusic);
    UnloadMusicStream(s_gameMusic);
    CloseAudioDevice();
    s_audioReady = false;
}
