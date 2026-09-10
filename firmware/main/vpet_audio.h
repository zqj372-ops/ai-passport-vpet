// vpet_audio.h —— 音效:16 kHz 单声道合成音,放在独立任务里,不阻塞 UI
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    VPET_SOUND_SELECT = 0,
    VPET_SOUND_FEED,
    VPET_SOUND_TRAIN,
    VPET_SOUND_HIT,
    VPET_SOUND_WIN,
    VPET_SOUND_LOSE,
    VPET_SOUND_EVOLVE,
    VPET_SOUND_DEATH,
    VPET_SOUND_ERROR,
    VPET_SOUND_ALERT,
} vpet_sound_t;

bool vpet_audio_start(uint8_t level);
void vpet_audio_play(vpet_sound_t sound);
void vpet_audio_set_level(uint8_t level);   // 0=静音 1=低 2=中 3=高
