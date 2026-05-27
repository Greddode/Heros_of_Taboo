#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

void InitAudioSystem(void);
void UpdateMusicSystem(bool inGame);
void ShutdownAudioSystem(void);
float GetMusicVolume(void);
void SetAudioVolume(float vol);
float GetSFXVolume(void);
void SetSFXVolume(float vol);
void PlayHitSound(void);
void PlayPickupSound(void);
void PlayRangedAttackSound(void);
void PlayMagicAttackSound(void);
void PlayLevelUpSound(void);

#endif
