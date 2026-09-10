#include "vpet_audio.h"

#include "bsp_audio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define VPET_AUDIO_HZ 16000U
#define CHUNK_SAMPLES 256U

typedef struct {
    uint16_t freq;
    uint16_t ms;
    uint8_t wave;   // 0=方波 1=三角波 2=噪声
} tone_t;

typedef struct {
    uint8_t count;
    tone_t tones[4];
} sound_def_t;

static const sound_def_t SOUNDS[] = {
    [VPET_SOUND_SELECT] = { 1, { { 1760, 25, 0 } } },
    [VPET_SOUND_FEED]   = { 2, { { 880, 60, 1 }, { 1320, 70, 1 } } },
    [VPET_SOUND_TRAIN]  = { 2, { { 660, 45, 0 }, { 990, 55, 0 } } },
    [VPET_SOUND_HIT]    = { 1, { { 220, 45, 2 } } },
    [VPET_SOUND_WIN]    = { 3, { { 784, 90, 1 }, { 1046, 90, 1 }, { 1318, 140, 1 } } },
    [VPET_SOUND_LOSE]   = { 3, { { 660, 110, 1 }, { 523, 110, 1 }, { 392, 180, 1 } } },
    [VPET_SOUND_EVOLVE] = { 4, { { 659, 80, 1 }, { 880, 80, 1 }, { 1108, 80, 1 }, { 1318, 200, 1 } } },
    [VPET_SOUND_DEATH]  = { 3, { { 392, 200, 1 }, { 311, 220, 1 }, { 233, 320, 1 } } },
    [VPET_SOUND_ERROR]  = { 2, { { 233, 90, 0 }, { 196, 120, 0 } } },
    [VPET_SOUND_ALERT]  = { 2, { { 1318, 60, 0 }, { 1568, 60, 0 } } },
};

static QueueHandle_t s_queue;
static volatile uint8_t s_level;
static uint32_t s_noise = 0xC001D00DU;

static int16_t wave_sample(uint32_t phase, uint8_t wave, int16_t amplitude)
{
    uint16_t point = (uint16_t)(phase >> 16);
    if (wave == 0) {
        return point < 32768U ? amplitude : (int16_t)-amplitude;
    }
    if (wave == 1) {
        int32_t triangle = point < 32768U ? (int32_t)point * 2 - 32768
                                          : 98303 - (int32_t)point * 2;
        return (int16_t)(triangle * amplitude / 32768);
    }
    s_noise ^= s_noise << 13;
    s_noise ^= s_noise >> 17;
    s_noise ^= s_noise << 5;
    return (int16_t)(((int32_t)(s_noise & 0xFFFFU) - 32768) * amplitude / 32768);
}

static void play_tone(const tone_t *tone)
{
    if (tone->freq == 0 || tone->ms == 0 || s_level == 0) return;
    uint32_t total = VPET_AUDIO_HZ * tone->ms / 1000U;
    uint32_t step = (uint32_t)(((uint64_t)tone->freq << 32) / VPET_AUDIO_HZ);
    uint32_t phase = 0;
    int16_t samples[CHUNK_SAMPLES];
    int16_t amplitude = (int16_t)(2500 * s_level);
    if (amplitude > 9000) amplitude = 9000;

    uint32_t rendered = 0;
    while (rendered < total) {
        size_t count = total - rendered < CHUNK_SAMPLES ? total - rendered : CHUNK_SAMPLES;
        for (size_t i = 0; i < count; i++) {
            samples[i] = wave_sample(phase, tone->wave, amplitude);
            phase += step;
        }
        if (bsp_audio_write(samples, count * sizeof(int16_t)) != ESP_OK) break;
        rendered += count;
    }
    // 每个音之间留一点静音,避免连成一片
    int16_t silence[64] = { 0 };
    bsp_audio_write(silence, sizeof(silence));
}

static void audio_task(void *context)
{
    (void)context;
    uint8_t sound;
    while (xQueueReceive(s_queue, &sound, portMAX_DELAY) == pdTRUE) {
        if (sound >= sizeof(SOUNDS) / sizeof(SOUNDS[0])) continue;
        const sound_def_t *def = &SOUNDS[sound];
        for (uint8_t i = 0; i < def->count; i++) {
            play_tone(&def->tones[i]);
        }
    }
}

bool vpet_audio_start(uint8_t level)
{
    s_level = level > 3 ? 3 : level;
    if (bsp_audio_init() != ESP_OK) return false;
    if (bsp_audio_set_format(VPET_AUDIO_HZ, 16, 1) != ESP_OK) return false;
    bsp_audio_set_volume(85);
    s_queue = xQueueCreate(8, sizeof(uint8_t));
    if (s_queue == NULL) return false;
    return xTaskCreate(audio_task, "vpet_audio", 4096, NULL, 4, NULL) == pdPASS;
}

void vpet_audio_play(vpet_sound_t sound)
{
    if (s_queue == NULL || s_level == 0) return;
    uint8_t value = (uint8_t)sound;
    xQueueSend(s_queue, &value, 0);
}

void vpet_audio_set_level(uint8_t level)
{
    s_level = level > 3 ? 3 : level;
}
