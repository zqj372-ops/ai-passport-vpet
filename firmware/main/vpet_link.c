#include "vpet_link.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

static const char *TAG = "vpet_link";
static const char *ADV_NAME = "VPET";

static SemaphoreHandle_t s_lock;
static vpet_link_status_t s_status;
static vpet_link_card_t s_card;
static uint8_t s_addr_type;
static bool s_initialized;
static uint32_t s_start_ms;
static uint8_t s_mfg[2 + sizeof(vpet_link_card_t)];

static int gap_event(struct ble_gap_event *event, void *arg);

static void lock(void)
{
    if (s_lock != NULL) xSemaphoreTake(s_lock, portMAX_DELAY);
}

static void unlock(void)
{
    if (s_lock != NULL) xSemaphoreGive(s_lock);
}

// ---------------------------------------------------------------- 广播 / 扫描

static int start_advertising(void)
{
    struct ble_hs_adv_fields fields = { 0 };
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (const uint8_t *)ADV_NAME;
    fields.name_len = strlen(ADV_NAME);
    fields.name_is_complete = 1;
    fields.mfg_data = s_mfg;
    fields.mfg_data_len = sizeof(s_mfg);

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGW(TAG, "adv fields failed: %d", rc);
        return rc;
    }
    struct ble_gap_adv_params params = { 0 };
    params.conn_mode = BLE_GAP_CONN_MODE_NON;   // 只广播,不建立连接
    params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(s_addr_type, NULL, BLE_HS_FOREVER, &params, gap_event, NULL);
    if (rc != 0) ESP_LOGW(TAG, "adv start failed: %d", rc);
    return rc;
}

static int start_scanning(void)
{
    struct ble_gap_disc_params params = { 0 };
    params.passive = 1;
    params.filter_duplicates = 0;
    params.itvl = 0;      // 用控制器默认值
    params.window = 0;
    int rc = ble_gap_disc(s_addr_type, BLE_HS_FOREVER, &params, gap_event, NULL);
    if (rc != 0) ESP_LOGW(TAG, "scan start failed: %d", rc);
    return rc;
}

static bool parse_card(const uint8_t *data, uint8_t length, vpet_link_card_t *out)
{
    uint8_t index = 0;
    while (index + 1 < length) {
        uint8_t field_len = data[index];
        if (field_len == 0) break;
        if ((int)index + 1 + field_len > length) break;
        uint8_t type = data[index + 1];
        const uint8_t *payload = &data[index + 2];
        uint8_t payload_len = (uint8_t)(field_len - 1);
        if (type == 0xFF && payload_len >= 2 + sizeof(vpet_link_card_t)) {
            uint16_t company = (uint16_t)(payload[0] | (payload[1] << 8));
            if (company == VPET_LINK_COMPANY_ID) {
                vpet_link_card_t card;
                memcpy(&card, payload + 2, sizeof(card));
                if (card.magic[0] == 'V' && card.magic[1] == 'P' &&
                    card.version == VPET_LINK_CARD_VERSION &&
                    card.session != s_card.session) {
                    *out = card;
                    return true;
                }
            }
        }
        index = (uint8_t)(index + field_len + 1);
    }
    return false;
}

static void on_peer_found(const vpet_link_card_t *peer)
{
    ble_gap_disc_cancel();
    // 注意:这里【不能】停广播。对方可能还没扫到我们,停了它就会超时找不到人。
    // 广播一直保持到应用层离开对战界面时由 vpet_link_stop() 关闭。
    lock();
    s_status.peer = *peer;
    s_status.state = VPET_LINK_FOUND;
    unlock();
    ESP_LOGI(TAG, "peer found: species=%u hp=%u", peer->species, peer->hp);
}

static int gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_DISC: {
        if (s_status.state != VPET_LINK_SEARCHING) break;
        vpet_link_card_t peer;
        if (parse_card(event->disc.data, event->disc.length_data, &peer)) {
            on_peer_found(&peer);
        }
        break;
    }
    case BLE_GAP_EVENT_ADV_COMPLETE:
        if (s_status.state == VPET_LINK_SEARCHING) start_advertising();
        break;
    default:
        break;
    }
    return 0;
}

static void on_reset(int reason)
{
    lock();
    s_status.state = VPET_LINK_ERROR;
    s_status.last_error = reason;
    unlock();
    ESP_LOGW(TAG, "host reset: %d", reason);
}

static void on_sync(void)
{
    if (ble_hs_util_ensure_addr(0) != 0) return;
    if (ble_hs_id_infer_auto(0, &s_addr_type) != 0) return;
    start_advertising();
    start_scanning();
}

static void host_task(void *arg)
{
    (void)arg;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static esp_err_t ensure_host(void)
{
    if (s_initialized) return ESP_OK;
    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateMutex();
        if (s_lock == NULL) return ESP_ERR_NO_MEM;
    }
    esp_err_t err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init failed: %d", err);
        return err;
    }
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;
    nimble_port_freertos_init(host_task);
    s_initialized = true;
    return ESP_OK;
}

// ---------------------------------------------------------------- 对外接口

void vpet_link_card_from_model(const vpet_model_t *model, uint8_t charge_band,
                               uint16_t session, vpet_link_card_t *out)
{
    vpet_fighter_t fighter;
    memset(out, 0, sizeof(*out));
    out->magic[0] = 'V';
    out->magic[1] = 'P';
    out->version = VPET_LINK_CARD_VERSION;
    out->session = session;

    vpet_fighter_from_model(model, charge_band, &fighter);
    out->species = fighter.species;
    const vpet_species_t *sp = vpet_species(fighter.species);
    out->stage = sp ? (uint8_t)sp->stage : 0;
    out->hp = fighter.hp;
    out->max_hp = fighter.max_hp;
    out->power = fighter.power;
    out->guard = fighter.guard;
    out->attack_count = fighter.attack_count;
    for (uint8_t i = 0; i < VPET_MAX_ATTACKS; i++) {
        out->attack_power[i] = fighter.attack_power[i];
    }
    out->charge_band = fighter.charge_band;
    out->wins = (uint8_t)(model->wins > 255 ? 255 : model->wins);
}

void vpet_link_card_to_fighter(const vpet_link_card_t *card, vpet_fighter_t *out)
{
    memset(out, 0, sizeof(*out));
    out->species = card->species;
    out->hp = card->hp;
    out->max_hp = card->max_hp;
    out->power = card->power;
    out->guard = card->guard;
    out->attack_count = card->attack_count == 0 ? 1 : card->attack_count;
    for (uint8_t i = 0; i < VPET_MAX_ATTACKS; i++) {
        out->attack_power[i] = card->attack_power[i];
    }
    if (out->attack_count > VPET_MAX_ATTACKS) out->attack_count = VPET_MAX_ATTACKS;
    out->charge_band = card->charge_band > 4 ? 4 : card->charge_band;
}

esp_err_t vpet_link_start(const vpet_link_card_t *mine, uint16_t timeout_ms)
{
    if (mine == NULL) return ESP_ERR_INVALID_ARG;
    esp_err_t err = ensure_host();
    if (err != ESP_OK) return err;

    lock();
    s_card = *mine;
    memset(&s_status, 0, sizeof(s_status));
    s_status.state = VPET_LINK_SEARCHING;
    s_status.timeout_ms = timeout_ms;
    s_status.elapsed_ms = 0;
    unlock();

    s_mfg[0] = (uint8_t)(VPET_LINK_COMPANY_ID & 0xFF);
    s_mfg[1] = (uint8_t)(VPET_LINK_COMPANY_ID >> 8);
    memcpy(s_mfg + 2, mine, sizeof(*mine));

    // 主机已经在跑的情况下(第二次进联机),重新起广播和扫描即可
    if (ble_gap_adv_active()) ble_gap_adv_stop();
    ble_gap_disc_cancel();
    start_advertising();
    start_scanning();
    s_start_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    ESP_LOGI(TAG, "link search started (%u ms)", timeout_ms);
    return ESP_OK;
}

void vpet_link_stop(void)
{
    if (!s_initialized) return;
    ble_gap_disc_cancel();
    if (ble_gap_adv_active()) ble_gap_adv_stop();
    lock();
    if (s_status.state == VPET_LINK_SEARCHING) s_status.state = VPET_LINK_IDLE;
    unlock();
}

void vpet_link_poll(vpet_link_status_t *out)
{
    if (out == NULL) return;
    lock();
    if (s_status.state == VPET_LINK_SEARCHING && s_status.timeout_ms > 0) {
        uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        uint32_t elapsed = now_ms - s_start_ms;
        s_status.elapsed_ms = (uint16_t)(elapsed > 0xFFFFU ? 0xFFFFU : elapsed);
        if (elapsed >= s_status.timeout_ms) {
            s_status.state = VPET_LINK_TIMEOUT;
            unlock();
            vpet_link_stop();
            lock();
        }
    }
    *out = s_status;
    unlock();
}
