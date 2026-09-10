#include "vpet_store.h"

#include <stddef.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#define VPET_STORE_MAGIC 0x56504554U   // "VPET"

typedef struct {
    uint32_t magic;
    uint32_t sequence;
    uint32_t model_size;
    vpet_model_t model;
    uint32_t crc;
} vpet_record_t;

static const char *TAG = "vpet_store";
static const char *NAMESPACE = "vpet";
static QueueHandle_t s_queue;
static uint32_t s_sequence;

static uint32_t crc32(const void *data, size_t length)
{
    const uint8_t *bytes = data;
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < length; i++) {
        crc ^= bytes[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (0xEDB88320U & (uint32_t)-(int32_t)(crc & 1U));
        }
    }
    return ~crc;
}

static bool record_valid(const vpet_record_t *record, size_t length)
{
    if (length != sizeof(*record) || record->magic != VPET_STORE_MAGIC) return false;
    if (record->model_size != sizeof(record->model)) return false;
    if (record->model.version != VPET_MODEL_VERSION) return false;
    return record->crc == crc32(record, offsetof(vpet_record_t, crc));
}

static bool read_slot(nvs_handle_t handle, const char *key, vpet_record_t *record,
                      uint32_t *sequence)
{
    size_t length = sizeof(*record);
    if (nvs_get_blob(handle, key, record, &length) != ESP_OK) return false;
    if (!record_valid(record, length)) return false;
    if (sequence != NULL) *sequence = record->sequence;
    return true;
}

bool vpet_store_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs init retry: %s", esp_err_to_name(err));
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs init failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool vpet_store_load(vpet_model_t *model)
{
    nvs_handle_t handle;
    if (nvs_open(NAMESPACE, NVS_READONLY, &handle) != ESP_OK) return false;

    vpet_record_t record;
    uint32_t seq_a = 0, seq_b = 0;
    bool ok_a = read_slot(handle, "slot0", &record, &seq_a);
    vpet_model_t model_a = record.model;
    bool ok_b = read_slot(handle, "slot1", &record, &seq_b);
    vpet_model_t model_b = record.model;
    nvs_close(handle);

    if (!ok_a && !ok_b) return false;
    if (ok_a && (!ok_b || seq_a >= seq_b)) {
        *model = model_a;
        s_sequence = seq_a;
    } else {
        *model = model_b;
        s_sequence = seq_b;
    }
    return true;
}

static bool write_slot(const vpet_model_t *model, uint32_t sequence)
{
    nvs_handle_t handle;
    if (nvs_open(NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return false;

    vpet_record_t record = {
        .magic = VPET_STORE_MAGIC,
        .sequence = sequence,
        .model_size = sizeof(vpet_model_t),
        .model = *model,
    };
    record.crc = crc32(&record, offsetof(vpet_record_t, crc));

    const char *key = (sequence & 1U) ? "slot1" : "slot0";
    esp_err_t err = nvs_set_blob(handle, key, &record, sizeof(record));
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "save failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

static void save_task(void *context)
{
    (void)context;
    vpet_model_t model;
    while (xQueueReceive(s_queue, &model, portMAX_DELAY) == pdTRUE) {
        // 收到多份请求时只写最后一份,避免频繁写 flash
        vpet_model_t next;
        while (xQueueReceive(s_queue, &next, 0) == pdTRUE) {
            model = next;
        }
        write_slot(&model, ++s_sequence);
    }
}

void vpet_store_save_async(const vpet_model_t *model)
{
    if (s_queue == NULL) {
        s_queue = xQueueCreate(4, sizeof(vpet_model_t));
        if (s_queue == NULL) return;
        xTaskCreate(save_task, "vpet_save", 4096, NULL, 3, NULL);
    }
    xQueueSend(s_queue, model, 0);
}
