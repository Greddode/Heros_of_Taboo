#include "game_audio.h"
#include "audio.h"

static AudioMusicContext  s_ctxMenu = AUDIO_INVALID;
static AudioMusicContext  s_ctxGame = AUDIO_INVALID;

static AudioSoundCategory s_sfxHit     = AUDIO_INVALID;
static AudioSoundCategory s_sfxPickup  = AUDIO_INVALID;
static AudioSoundCategory s_sfxRanged  = AUDIO_INVALID;
static AudioSoundCategory s_sfxMagic   = AUDIO_INVALID;
static AudioSoundCategory s_sfxLevelUp = AUDIO_INVALID;

void GameAudio_Init(void) {
    s_ctxMenu = Audio_RegisterMusicContext("menu", "resources/audio/music/main_menu");
    s_ctxGame = Audio_RegisterMusicContext("game", "resources/audio/music/game");

    s_sfxHit     = Audio_RegisterSoundCategory("hit",     "resources/audio/sounds/hit");
    s_sfxPickup  = Audio_RegisterSoundCategory("pickup",  "resources/audio/sounds/pickup");
    s_sfxRanged  = Audio_RegisterSoundCategory("ranged",  "resources/audio/sounds/ranged_attack");
    s_sfxMagic   = Audio_RegisterSoundCategory("magic",   "resources/audio/sounds/magic_attack");
    s_sfxLevelUp = Audio_RegisterSoundCategory("levelup", "resources/audio/sounds/levelup");

    Audio_SetMusicContext(s_ctxMenu);
}

void GameAudio_SetContext(bool inGame) {
    AudioMusicContext desired = inGame ? s_ctxGame : s_ctxMenu;
    if (desired != Audio_GetMusicContext())
        Audio_SetMusicContext(desired);
}

void GameAudio_PlayHitSound(void)        { Audio_PlaySound(s_sfxHit); }
void GameAudio_PlayPickupSound(void)     { Audio_PlaySound(s_sfxPickup); }
void GameAudio_PlayRangedAttackSound(void) { Audio_PlaySound(s_sfxRanged); }
void GameAudio_PlayMagicAttackSound(void)  { Audio_PlaySound(s_sfxMagic); }
void GameAudio_PlayLevelUpSound(void)    { Audio_PlaySound(s_sfxLevelUp); }
