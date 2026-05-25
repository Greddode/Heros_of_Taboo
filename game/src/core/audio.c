#include "audio.h"
#include "raylib.h"
#include <stddef.h>
#include <stdlib.h>

#define MAX_MENU_TRACKS    8
#define MAX_GAME_TRACKS   16
#define MAX_HIT_SOUNDS     16
#define MAX_PICKUP_SOUNDS  16

#define MENU_MUSIC_DIR  "resources/audio/music/main_menu"
#define GAME_MUSIC_DIR  "resources/audio/music/game"

typedef enum {
    TRACK_NONE,
    TRACK_MENU,
    TRACK_GAME
} ActiveTrack;

static Music s_menuTracks[MAX_MENU_TRACKS];
static int s_menuTrackCount = 0;

static Music s_gameTracks[MAX_GAME_TRACKS];
static int s_gameTrackCount = 0;
static int s_currentGameTrack = -1;
static float s_lastGameTimePlayed = 0.0f;

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

static void LoadHitSounds(void) {
    LoadSoundsFromDir("resources/audio/sounds/hit", s_hitSounds, &s_hitSoundCount, MAX_HIT_SOUNDS);
}

static void LoadPickupSounds(void) {
    LoadSoundsFromDir("resources/audio/sounds/pickup", s_pickupSounds, &s_pickupSoundCount, MAX_PICKUP_SOUNDS);
}

void InitAudioSystem(void) {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;

    LoadMusicFromDir(MENU_MUSIC_DIR, s_menuTracks, &s_menuTrackCount, MAX_MENU_TRACKS);
    LoadMusicFromDir(GAME_MUSIC_DIR, s_gameTracks, &s_gameTrackCount, MAX_GAME_TRACKS);

    LoadHitSounds();
    LoadPickupSounds();

    s_audioReady = true;
}

static int PickRandomGameTrack(void) {
    if (s_gameTrackCount <= 0) return -1;
    if (s_gameTrackCount == 1) return 0;
    int next;
    do {
        next = GetRandomValue(0, s_gameTrackCount - 1);
    } while (next == s_currentGameTrack);
    return next;
}

static void PlayMenuTrack(void) {
    if (s_menuTrackCount <= 0) return;
    s_menuTracks[0].looping = true;
    PlayMusicStream(s_menuTracks[0]);
    SetMusicVolume(s_menuTracks[0], s_musicVolume);
}

static void PlayGameTrack(int idx) {
    if (idx < 0 || idx >= s_gameTrackCount) return;
    StopMusicStream(s_gameTracks[idx]);
    PlayMusicStream(s_gameTracks[idx]);
    SetMusicVolume(s_gameTracks[idx], s_musicVolume);
}

void UpdateMusicSystem(bool inGame) {
    if (!s_audioReady) return;

    ActiveTrack desired = inGame ? TRACK_GAME : TRACK_MENU;

    if (desired != s_activeTrack) {
        switch (s_activeTrack) {
            case TRACK_MENU: StopMusicStream(s_menuTracks[0]); break;
            case TRACK_GAME:
                if (s_currentGameTrack >= 0)
                    StopMusicStream(s_gameTracks[s_currentGameTrack]);
                break;
            default: break;
        }

        switch (desired) {
            case TRACK_MENU:
                PlayMenuTrack();
                break;
            case TRACK_GAME:
                s_currentGameTrack = PickRandomGameTrack();
                s_lastGameTimePlayed = 0.0f;
                PlayGameTrack(s_currentGameTrack);
                break;
            default: break;
        }

        s_activeTrack = desired;
    }

    switch (s_activeTrack) {
        case TRACK_MENU:
            UpdateMusicStream(s_menuTracks[0]);
            break;
        case TRACK_GAME:
            if (s_currentGameTrack >= 0) {
                float played = GetMusicTimePlayed(s_gameTracks[s_currentGameTrack]);
                float len = GetMusicTimeLength(s_gameTracks[s_currentGameTrack]);
                if (played < s_lastGameTimePlayed && len > 0.5f) {
                    StopMusicStream(s_gameTracks[s_currentGameTrack]);
                    s_currentGameTrack = PickRandomGameTrack();
                    PlayGameTrack(s_currentGameTrack);
                } else {
                    UpdateMusicStream(s_gameTracks[s_currentGameTrack]);
                }
                s_lastGameTimePlayed = played;
            }
            break;
        default: break;
    }
}

float GetMusicVolume(void) {
    return s_musicVolume;
}

void SetAudioVolume(float vol) {
    s_musicVolume = (vol < 0.0f) ? 0.0f : (vol > 1.0f) ? 1.0f : vol;
    if (!s_audioReady) return;
    if (s_activeTrack == TRACK_MENU && s_menuTrackCount > 0)
        SetMusicVolume(s_menuTracks[0], s_musicVolume);
    else if (s_activeTrack == TRACK_GAME && s_currentGameTrack >= 0)
        SetMusicVolume(s_gameTracks[s_currentGameTrack], s_musicVolume);
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
    for (int i = 0; i < s_menuTrackCount; i++)
        StopMusicStream(s_menuTracks[i]);
    for (int i = 0; i < s_gameTrackCount; i++)
        StopMusicStream(s_gameTracks[i]);
    for (int i = 0; i < s_hitSoundCount; i++) UnloadSound(s_hitSounds[i]);
    s_hitSoundCount = 0;
    for (int i = 0; i < s_pickupSoundCount; i++) UnloadSound(s_pickupSounds[i]);
    s_pickupSoundCount = 0;
    for (int i = 0; i < s_menuTrackCount; i++) UnloadMusicStream(s_menuTracks[i]);
    s_menuTrackCount = 0;
    for (int i = 0; i < s_gameTrackCount; i++) UnloadMusicStream(s_gameTracks[i]);
    s_gameTrackCount = 0;
    CloseAudioDevice();
    s_audioReady = false;
}
