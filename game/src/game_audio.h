#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

#include <stdbool.h>

void GameAudio_Init(void);
void GameAudio_SetContext(bool inGame);

void GameAudio_PlayHitSound(void);
void GameAudio_PlayPickupSound(void);
void GameAudio_PlayRangedAttackSound(void);
void GameAudio_PlayMagicAttackSound(void);
void GameAudio_PlayLevelUpSound(void);

#endif
