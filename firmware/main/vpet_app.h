// vpet_app.h —— 应用入口:界面、计时、按键与事件处理
#pragma once

#include <stdbool.h>

#include "bsp_button.h"
#include "vpet_model.h"

// 时间倍率:1 真实分钟 = VPET_TIME_SCALE 宠物分钟(默认 10 倍,1 宠物日约 2.4 小时)
#define VPET_TIME_SCALE 10U

// 必须在持有 LVGL 锁的情况下调用
bool vpet_app_start(vpet_model_t *model, bool battery_ok);
void vpet_app_button(bsp_btn_t button, bsp_btn_ev_t event);
