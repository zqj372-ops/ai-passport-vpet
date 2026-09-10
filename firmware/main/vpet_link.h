// vpet_link.h —— 双机对战通信(BLE 广播 + 扫描,无需连接)
//
// 思路:两台设备各自把"战斗卡"(种类/体力/攻防/招式/蓄力档位/战绩)塞进 BLE 广播,
// 同时被动扫描对方的广播。谁先看到对方不算"先手"——战斗引擎是对称的:
// 只要双方拿到同一组战斗卡,各自在自己屏幕上都会算出完全一样的战斗过程和胜负。
// 好处:不用建立连接、不用区分主从、断连也不会卡死,ESP32-C3 的这点内存完全够用。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "vpet_battle.h"
#include "vpet_model.h"

// 厂商自定义数据用的公司 ID(0xFFFF = 保留给测试,避免冒用真实公司)
#define VPET_LINK_COMPANY_ID 0xFFFFU
#define VPET_LINK_CARD_VERSION 1U

typedef struct __attribute__((packed)) {
    uint8_t magic[2];        // 'V','P'
    uint8_t version;
    uint8_t species;
    uint8_t stage;
    uint8_t hp;
    uint8_t max_hp;
    uint8_t power;
    uint8_t guard;
    uint8_t attack_count;
    uint8_t attack_power[VPET_MAX_ATTACKS];
    uint8_t charge_band;
    uint8_t wins;
    uint16_t session;        // 本次联机随机号,防止匹配到上一次的残留广播
} vpet_link_card_t;

typedef enum {
    VPET_LINK_IDLE = 0,
    VPET_LINK_SEARCHING,
    VPET_LINK_FOUND,
    VPET_LINK_TIMEOUT,
    VPET_LINK_ERROR,
} vpet_link_state_t;

typedef struct {
    vpet_link_state_t state;
    vpet_link_card_t peer;
    int last_error;
    uint16_t elapsed_ms;
    uint16_t timeout_ms;
} vpet_link_status_t;

void vpet_link_card_from_model(const vpet_model_t *model, uint8_t charge_band,
                               uint16_t session, vpet_link_card_t *out);
void vpet_link_card_to_fighter(const vpet_link_card_t *card, vpet_fighter_t *out);

// 开始搜索对手(内部会按需初始化 NimBLE)
esp_err_t vpet_link_start(const vpet_link_card_t *mine, uint16_t timeout_ms);
void vpet_link_stop(void);

// 在 LVGL 任务里轮询状态(不跨任务碰界面)
void vpet_link_poll(vpet_link_status_t *out);
