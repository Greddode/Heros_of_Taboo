#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

typedef int AudioMusicContext;
typedef int AudioSoundCategory;

#define AUDIO_INVALID (-1)

void InitAudioSystem(void);
void UpdateMusicSystem(void);
void ShutdownAudioSystem(void);

AudioMusicContext  Audio_RegisterMusicContext(const char* name, const char* dirPath);
AudioSoundCategory Audio_RegisterSoundCategory(const char* name, const char* dirPath);

void  Audio_SetMusicContext(AudioMusicContext ctx);
AudioMusicContext Audio_GetMusicContext(void);

void  Audio_PlaySound(AudioSoundCategory cat);

float Audio_GetMusicVolume(void);
void  Audio_SetMusicVolume(float vol);
float Audio_GetSFXVolume(void);
void  Audio_SetSFXVolume(float vol);

#endif
