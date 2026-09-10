#include "vpet_app.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "bsp_display.h"
#include "vpet_audio.h"
#include "vpet_battle.h"
#include "vpet_link.h"
#include "vpet_species.h"
#include "vpet_store.h"
#include "vpet_view.h"

static const char *TAG = "vpet";

typedef enum {
    ST_MAIN = 0,
    ST_PAGE,
    ST_CHARGE,
    ST_BATTLE,
    ST_LINK,
    ST_EVOLVE,
} app_state_t;

typedef enum {
    CHARGE_TRAIN = 0,
    CHARGE_BATTLE,
    CHARGE_LINK,
} charge_purpose_t;

static vpet_model_t *s_model;
static app_state_t s_state;
static charge_purpose_t s_charge_purpose;
static uint8_t s_selected;
static uint8_t s_sound_level = 2;
static bool s_battery_ok;

static vpet_main_view_t s_main;
static vpet_page_view_t s_page;
static vpet_charge_view_t s_charge;
static vpet_battle_view_t s_battle;
static vpet_battle_t s_battle_state;
static vpet_evo_view_t s_evo;

static lv_timer_t *s_sec_timer;
static lv_timer_t *s_charge_timer;
static lv_timer_t *s_battle_timer;
static lv_timer_t *s_link_timer;
static lv_timer_t *s_evo_timer;

static uint32_t s_saved_age_min;
static uint32_t s_pet_ms_accum;
static uint64_t s_last_us;
static int8_t s_marker_dir = 1;
static uint8_t s_marker;
static uint8_t s_charge_round;
static uint8_t s_charge_score;
static uint8_t s_battle_log_shown;
static uint32_t s_alert_cooldown;
static lv_obj_t *s_current_screen;
static uint16_t s_link_session;
static vpet_link_card_t s_my_card;
static uint8_t s_link_last_second;
static uint32_t s_evo_start_ms;
static bool s_evo_finished;

// LVGL 的 lv_screen_load() 只是切换活动屏幕,旧屏幕要自己删,否则反复进出菜单会漏内存
static void keep_screen(lv_obj_t *screen)
{
    if (s_current_screen != NULL && s_current_screen != screen) {
        lv_obj_delete(s_current_screen);
    }
    s_current_screen = screen;
}

// ---------------------------------------------------------------- 主界面

static void show_main(void)
{
    s_state = ST_MAIN;
    vpet_view_main_create(&s_main, s_model);
    keep_screen(s_main.screen);
    vpet_view_main_refresh(&s_main, s_model, s_selected, s_sound_level);
}

static void refresh_main(void)
{
    if (s_state != ST_MAIN) return;
    vpet_view_main_refresh(&s_main, s_model, s_selected, s_sound_level);
}

// ---------------------------------------------------------------- 状态页

static void show_status(void)
{
    char info[320];
    const vpet_species_t *sp = vpet_model_species(s_model);
    uint32_t days = s_model->age_min / VPET_DAY_MINUTES;
    uint32_t hours = (s_model->age_min % VPET_DAY_MINUTES) / 60U;
    uint8_t progress = vpet_model_evolution_progress(s_model);

    snprintf(info, sizeof(info),
             "阶段 : %s\n"
             "年龄 : %u天%u小时\n"
             "体重 : %u g\n"
             "失误 : %u / %u\n"
             "训练 : %u\n"
             "对战 : %u  胜 %u\n"
             "进化 : %u%%",
             sp ? vpet_stage_name(sp->stage) : "-", (unsigned)days, (unsigned)hours,
             s_model->weight_g, s_model->care_mistakes, VPET_CARE_MISTAKE_LIMIT,
             s_model->trainings, s_model->battles, s_model->wins, progress);

    vpet_view_page_create(&s_page, "状态", s_model, s_model->species);
    vpet_view_page_set(&s_page, "状态", info, s_model->species, progress);
    keep_screen(s_page.screen);
    s_state = ST_PAGE;
}

static void show_message(const char *title, const char *line1, const char *line2,
                         uint32_t bg, vpet_sound_t sound)
{
    lv_obj_t *scr = vpet_view_message_screen(title, line1, line2, bg);
    keep_screen(scr);
    s_state = ST_PAGE;
    vpet_audio_play(sound);
}

// ---------------------------------------------------------------- 训练/蓄力小游戏

static void charge_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_state != ST_CHARGE) return;
    int next = (int)s_marker + s_marker_dir * 3;
    if (next <= 0) {
        next = 0;
        s_marker_dir = 1;
    } else if (next >= 100) {
        next = 100;
        s_marker_dir = -1;
    }
    s_marker = (uint8_t)next;
    vpet_view_charge_set_marker(&s_charge, s_marker);
}

static void start_charge(charge_purpose_t purpose)
{
    s_charge_purpose = purpose;
    s_charge_round = 1;
    s_charge_score = 0;
    s_marker = 0;
    s_marker_dir = 1;
    const char *title = purpose == CHARGE_TRAIN ? "训练"
                      : purpose == CHARGE_LINK ? "联机蓄力" : "力量蓄力";
    vpet_view_charge_create(&s_charge, title, s_model);
    keep_screen(s_charge.screen);
    vpet_view_charge_set_round(&s_charge, 1, 3);
    vpet_view_charge_set_marker(&s_charge, s_marker);
    vpet_view_charge_set_hint(&s_charge, "指针到绿区按确定");
    s_state = ST_CHARGE;
    if (s_charge_timer == NULL) {
        s_charge_timer = lv_timer_create(charge_timer_cb, 40, NULL);
    } else {
        lv_timer_resume(s_charge_timer);
    }
}

static void finish_charge(void)
{
    if (s_charge_timer != NULL) {
        lv_timer_pause(s_charge_timer);
    }
    uint8_t band = s_charge_score > 4 ? 4 : s_charge_score;

    if (s_charge_purpose == CHARGE_TRAIN) {
        vpet_result_t result = vpet_model_train(s_model, s_charge_score);
        char line[64];
        snprintf(line, sizeof(line), "命中 %u/3", s_charge_score);
        show_message("训练结束", line,
                     result == VPET_RESULT_NO_STRENGTH ? "力量不足 先喂蛋白" : "干得好!",
                     0xDCE9DC, result == VPET_RESULT_NO_STRENGTH ? VPET_SOUND_ERROR
                                                                  : VPET_SOUND_TRAIN);
        vpet_store_save_async(s_model);
        return;
    }

    if (s_charge_purpose == CHARGE_LINK) {
        // 联机对战:把战斗卡塞进广播,开始搜索对手
        vpet_link_card_from_model(s_model, band, s_link_session, &s_my_card);
        esp_err_t err = vpet_link_start(&s_my_card, 20000);
        if (err != ESP_OK) {
            show_message("联机出错", "蓝牙初始化失败", "请重试", 0xF3E0D0,
                         VPET_SOUND_ERROR);
            return;
        }
        vpet_view_page_create(&s_page, "联机对战", s_model, s_model->species);
        keep_screen(s_page.screen);
        vpet_view_page_set(&s_page, "联机对战",
                           "正在寻找对手…\n请把两台设备靠近", 0xFF, 0);
        s_link_last_second = 0xFF;
        s_state = ST_LINK;
        if (s_link_timer != NULL) lv_timer_resume(s_link_timer);
        return;
    }

    // CPU 对战
    vpet_fighter_t left, right;
    vpet_fighter_from_model(s_model, band, &left);
    vpet_fighter_cpu(s_model, vpet_model_rand(s_model), &right);
    vpet_battle_start(&s_battle_state, &left, &right, vpet_model_rand(s_model));
    vpet_view_battle_create(&s_battle, &s_battle_state);
    keep_screen(s_battle.screen);
    vpet_view_battle_refresh(&s_battle, &s_battle_state, "对战开始!", NULL, 100, 100);
    s_battle_log_shown = 0;
    s_state = ST_BATTLE;
    if (s_battle_timer != NULL) {
        lv_timer_resume(s_battle_timer);
    }
}

// ---------------------------------------------------------------- 联机对战

static void start_link_battle(const vpet_link_card_t *peer)
{
    vpet_fighter_t mine, theirs;
    vpet_fighter_from_model(s_model, s_my_card.charge_band, &mine);
    vpet_link_card_to_fighter(peer, &theirs);

    // 种子必须可交换:两台设备各自算出的种子要一样(XOR 满足交换律)
    uint32_t extra = (uint32_t)s_my_card.session ^ (uint32_t)peer->session;
    uint32_t seed = vpet_battle_shared_seed(&mine, &theirs, extra);

    vpet_battle_start(&s_battle_state, &mine, &theirs, seed);
    vpet_view_battle_create(&s_battle, &s_battle_state);
    keep_screen(s_battle.screen);
    const vpet_species_t *peer_sp = vpet_species(peer->species);
    char head[64];
    snprintf(head, sizeof(head), "对手 %s", peer_sp ? peer_sp->name : "?");
    vpet_view_battle_refresh(&s_battle, &s_battle_state, head, NULL, 100, 100);
    s_battle_log_shown = 0;
    s_state = ST_BATTLE;
    if (s_battle_timer != NULL) lv_timer_resume(s_battle_timer);
    ESP_LOGI(TAG, "link battle start: peer=%s seed=%08x", peer_sp ? peer_sp->name : "?",
             (unsigned)seed);
}

static void link_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_state != ST_LINK) return;

    vpet_link_status_t status;
    vpet_link_poll(&status);

    if (status.state == VPET_LINK_SEARCHING) {
        uint8_t seconds = (uint8_t)(status.elapsed_ms / 1000U);
        if (seconds != s_link_last_second) {
            s_link_last_second = seconds;
            uint8_t progress = status.timeout_ms
                                   ? (uint8_t)((uint32_t)status.elapsed_ms * 100U /
                                               status.timeout_ms)
                                   : 0;
            char info[128];
            snprintf(info, sizeof(info),
                     "正在寻找对手…\n请把两台设备靠近\n\n%u秒 / 20秒",
                     (unsigned)seconds);
            vpet_view_page_set(&s_page, "联机对战", info, 0xFF, progress);
        }
    } else if (status.state == VPET_LINK_FOUND) {
        if (s_link_timer != NULL) lv_timer_pause(s_link_timer);
        vpet_audio_play(VPET_SOUND_ALERT);
        start_link_battle(&status.peer);
    } else if (status.state == VPET_LINK_TIMEOUT || status.state == VPET_LINK_ERROR) {
        if (s_link_timer != NULL) lv_timer_pause(s_link_timer);
        show_message("没找到对手", "另一台设备也要", "打开联机对战",
                     0xF3E0D0, VPET_SOUND_ERROR);
    }
}

// ---------------------------------------------------------------- 对战

static uint8_t hp_percent(uint8_t hp, uint8_t max)
{
    if (max == 0) return 0;
    return (uint8_t)((uint32_t)hp * 100U / max);
}

static void battle_log_text(char *buffer, size_t length)
{
    buffer[0] = '\0';
    size_t used = 0;
    uint8_t start = s_battle_log_shown > 3 ? (uint8_t)(s_battle_log_shown - 3) : 0;
    for (uint8_t i = start; i < s_battle_log_shown && i < s_battle_state.log_count; i++) {
        const vpet_battle_log_t *entry = &s_battle_state.log[i];
        const char *who = entry->attacker == 1 ? "你 " : "对手 ";
        const char *attack = vpet_battle_attack_name(&s_battle_state, entry->attacker,
                                                     entry->attack);
        int written = snprintf(buffer + used, length - used, "%s%s -%u\n", who, attack,
                               entry->damage);
        if (written <= 0 || (size_t)written >= length - used) break;
        used += (size_t)written;
        if (used >= length) break;
    }
    if (used == 0) {
        snprintf(buffer, length, "准备…");
    }
}

static void battle_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_state != ST_BATTLE) return;
    if (s_battle_state.finished) return;

    vpet_battle_step(&s_battle_state);
    s_battle_log_shown = s_battle_state.log_count;

    char log[200];
    battle_log_text(log, sizeof(log));
    const char *result = NULL;
    if (s_battle_state.finished) {
        if (s_battle_state.winner == 1) {
            result = "你赢了!";
        } else if (s_battle_state.winner == 2) {
            result = "你输了…";
        } else {
            result = "平局";
        }
        vpet_model_battle_finish(s_model, s_battle_state.winner == 1);
        vpet_audio_play(s_battle_state.winner == 1 ? VPET_SOUND_WIN : VPET_SOUND_LOSE);
        vpet_store_save_async(s_model);
        lv_timer_pause(s_battle_timer);
    } else {
        vpet_audio_play(VPET_SOUND_HIT);
    }
    vpet_view_battle_refresh(&s_battle, &s_battle_state, log, result,
                             hp_percent(s_battle_state.hp_left, s_battle_state.left.max_hp),
                             hp_percent(s_battle_state.hp_right, s_battle_state.right.max_hp));
}

static bool consume_battle_power(void)
{
    if (vpet_model_battle_start(s_model) != VPET_RESULT_DONE) {
        show_message("不能对战", "力量不足", "先喂蛋白粉",
                     0xF3E0D0, VPET_SOUND_ERROR);
        return false;
    }
    return true;
}

static void start_battle(void)
{
    if (!consume_battle_power()) return;
    start_charge(CHARGE_BATTLE);
}

static void start_link(void)
{
    if (!consume_battle_power()) return;
    s_link_session = (uint16_t)(vpet_model_rand(s_model) & 0xFFFFU);
    start_charge(CHARGE_LINK);
}

// ---------------------------------------------------------------- 进化动画

static void evolution_timer_cb(lv_timer_t *timer);

static void finish_evolution_animation(void)
{
    const vpet_species_t *sp = vpet_model_species(s_model);
    vpet_view_evolution_frame(&s_evo, VPET_EVO_DURATION_MS, true,
                              sp ? sp->name : "?", sp ? vpet_stage_name(sp->stage) : "");
    s_evo_finished = true;
    if (s_evo_timer != NULL) lv_timer_pause(s_evo_timer);
}

static void start_evolution_animation(uint8_t old_species, uint8_t new_species)
{
    s_evo_start_ms = lv_tick_get();
    s_evo_finished = false;
    vpet_view_evolution_create(&s_evo, old_species, new_species);
    keep_screen(s_evo.screen);
    s_state = ST_EVOLVE;
    vpet_audio_play(VPET_SOUND_EVOLVE);
    if (s_evo_timer == NULL) {
        s_evo_timer = lv_timer_create(evolution_timer_cb, 60, NULL);
    } else {
        lv_timer_resume(s_evo_timer);
    }
}

static void evolution_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_state != ST_EVOLVE || s_evo_finished) return;
    uint32_t elapsed = lv_tick_get() - s_evo_start_ms;
    if (elapsed >= VPET_EVO_DURATION_MS) {
        finish_evolution_animation();
        return;
    }
    vpet_view_evolution_frame(&s_evo, (uint16_t)elapsed, false, NULL, NULL);
}

// ---------------------------------------------------------------- 动作

static void do_menu_action(void)
{
    if (!s_model->alive) {
        vpet_model_init(s_model, vpet_model_rand(s_model));
        vpet_store_save_async(s_model);
        vpet_audio_play(VPET_SOUND_EVOLVE);
        show_main();
        return;
    }

    switch ((vpet_menu_t)s_selected) {
    case VPET_MENU_FOOD:
        vpet_audio_play(vpet_model_feed_food(s_model) == VPET_RESULT_DONE ? VPET_SOUND_FEED
                                                                          : VPET_SOUND_ERROR);
        break;
    case VPET_MENU_PROTEIN:
        vpet_audio_play(vpet_model_feed_protein(s_model) == VPET_RESULT_DONE
                            ? VPET_SOUND_FEED : VPET_SOUND_ERROR);
        break;
    case VPET_MENU_TRAIN:
        if (!s_model->sleeping && s_model->strength > 0) {
            start_charge(CHARGE_TRAIN);
        } else {
            vpet_audio_play(VPET_SOUND_ERROR);
        }
        break;
    case VPET_MENU_CLEAN: {
        uint8_t cleaned = vpet_model_clean(s_model);
        vpet_audio_play(cleaned > 0 ? VPET_SOUND_TRAIN : VPET_SOUND_ERROR);
        break;
    }
    case VPET_MENU_LIGHT:
        vpet_model_toggle_light(s_model);
        vpet_audio_play(VPET_SOUND_SELECT);
        break;
    case VPET_MENU_STATUS:
        show_status();
        break;
    case VPET_MENU_BATTLE:
        start_battle();
        break;
    case VPET_MENU_LINK:
        start_link();
        break;
    case VPET_MENU_SOUND:
        s_sound_level = (uint8_t)((s_sound_level + 1) & 3U);
        vpet_audio_set_level(s_sound_level);
        vpet_audio_play(VPET_SOUND_SELECT);
        break;
    default:
        break;
    }
    refresh_main();
    vpet_store_save_async(s_model);
}

// ---------------------------------------------------------------- 时间推进与事件

static void handle_events(uint32_t events, uint8_t previous_species)
{
    if (events & VPET_EVENT_EVOLVE) {
        start_evolution_animation(previous_species, s_model->species);
        vpet_store_save_async(s_model);
        return;
    }
    if (events & VPET_EVENT_DEATH) {
        show_message("再见了", "你的怪兽离开了", "按确定键获得新的蛋",
                     0xE8E0E0, VPET_SOUND_DEATH);
        vpet_store_save_async(s_model);
        return;
    }
    if (events & VPET_EVENT_HATCH) {
        vpet_audio_play(VPET_SOUND_EVOLVE);
        show_main();
        vpet_store_save_async(s_model);
        return;
    }
    if (s_alert_cooldown > 0) s_alert_cooldown--;
    if (events & VPET_EVENT_SICK) {
        vpet_audio_play(VPET_SOUND_ALERT);
        s_alert_cooldown = 30;
    } else if ((events & (VPET_EVENT_HUNGRY | VPET_EVENT_WEAK)) && s_alert_cooldown == 0) {
        vpet_audio_play(VPET_SOUND_ALERT);
        s_alert_cooldown = 30;
    } else if (events & VPET_EVENT_CARE_MISTAKE) {
        vpet_audio_play(VPET_SOUND_ERROR);
        s_alert_cooldown = 15;
    } else if (events & VPET_EVENT_POOP) {
        vpet_audio_play(VPET_SOUND_SELECT);
    }
}

static void second_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    uint64_t now = esp_timer_get_time();
    uint64_t elapsed_us = now - s_last_us;
    s_last_us = now;

    // 真实毫秒 -> 宠物毫秒 -> 宠物分钟
    uint64_t pet_ms = elapsed_us / 1000U * VPET_TIME_SCALE;
    s_pet_ms_accum += (uint32_t)pet_ms;
    uint16_t minutes = (uint16_t)(s_pet_ms_accum / 60000U);
    if (minutes == 0) return;
    s_pet_ms_accum -= (uint32_t)minutes * 60000U;
    s_model->real_seconds += (uint32_t)(elapsed_us / 1000000U);

    uint8_t previous_species = s_model->species;
    uint32_t events = vpet_model_tick(s_model, minutes);
    if (events != VPET_EVENT_NONE) {
        handle_events(events, previous_species);
    }
    if (s_state == ST_MAIN) refresh_main();

    // 每 30 宠物分钟存一次档
    if (s_model->age_min / 30U != s_saved_age_min / 30U) {
        s_saved_age_min = s_model->age_min;
        vpet_store_save_async(s_model);
    }
}

// ---------------------------------------------------------------- 按键

static void handle_main_key(bsp_btn_t button, bsp_btn_ev_t event)
{
    if (event == BSP_BTN_CLICK) {
        if (button == BSP_BTN_UP) {
            s_selected = (uint8_t)((s_selected + VPET_MENU_COUNT - 1) % VPET_MENU_COUNT);
            vpet_audio_play(VPET_SOUND_SELECT);
        } else if (button == BSP_BTN_DOWN) {
            s_selected = (uint8_t)((s_selected + 1) % VPET_MENU_COUNT);
            vpet_audio_play(VPET_SOUND_SELECT);
        } else {
            do_menu_action();
        }
        refresh_main();
    }
}

static void handle_charge_key(bsp_btn_t button, bsp_btn_ev_t event)
{
    if (event == BSP_BTN_LONG) {
        if (s_charge_timer != NULL) lv_timer_pause(s_charge_timer);
        s_alert_cooldown = 0;
        show_main();
        return;
    }
    if (event != BSP_BTN_CLICK || button != BSP_BTN_OK) return;

    bool hit = s_marker >= 35 && s_marker <= 65;
    s_charge_round++;
    if (hit) {
        s_charge_score++;
        vpet_audio_play(VPET_SOUND_TRAIN);
    } else {
        vpet_audio_play(VPET_SOUND_ERROR);
    }
    char text[48];
    snprintf(text, sizeof(text), "命中 %u    第 %u/3 轮", s_charge_score,
             (unsigned)(s_charge_round > 3 ? 3 : s_charge_round));
    vpet_view_charge_set_score(&s_charge, text);

    if (s_charge_round > 3) {
        finish_charge();
    } else {
        vpet_view_charge_set_round(&s_charge, s_charge_round, 3);
    }
}

static void handle_battle_key(bsp_btn_t button, bsp_btn_ev_t event)
{
    if (event == BSP_BTN_LONG) {
        if (s_battle_timer != NULL) lv_timer_pause(s_battle_timer);
        vpet_link_stop();     // 联机对战结束后收起广播(没联机时是空操作)
        s_alert_cooldown = 0;
        show_main();
        return;
    }
    if (event == BSP_BTN_CLICK && button == BSP_BTN_OK && !s_battle_state.finished) {
        // 快进一轮
        battle_timer_cb(s_battle_timer);
    }
}

void vpet_app_button(bsp_btn_t button, bsp_btn_ev_t event)
{
    if (!bsp_lvgl_lock(300)) return;
    switch (s_state) {
    case ST_MAIN:
        handle_main_key(button, event);
        break;
    case ST_CHARGE:
        handle_charge_key(button, event);
        break;
    case ST_BATTLE:
        handle_battle_key(button, event);
        break;
    case ST_LINK:
        if (event == BSP_BTN_LONG || (event == BSP_BTN_CLICK && button == BSP_BTN_OK)) {
            vpet_link_stop();
            if (s_link_timer != NULL) lv_timer_pause(s_link_timer);
            show_main();
        }
        break;
    case ST_EVOLVE:
        if (!s_evo_finished) {
            finish_evolution_animation();     // 按一下直接看结果,不用等动画播完
        } else if (event == BSP_BTN_CLICK || event == BSP_BTN_LONG) {
            show_main();
        }
        break;
    case ST_PAGE:
        if (event == BSP_BTN_LONG ||
            (event == BSP_BTN_CLICK && button == BSP_BTN_OK && !s_model->alive)) {
            if (!s_model->alive && event == BSP_BTN_CLICK) {
                vpet_model_init(s_model, vpet_model_rand(s_model));
                vpet_store_save_async(s_model);
            }
            show_main();
        } else if (event == BSP_BTN_CLICK) {
            show_main();
        }
        break;
    default:
        break;
    }
    bsp_lvgl_unlock();
}

// ---------------------------------------------------------------- 启动

bool vpet_app_start(vpet_model_t *model, bool battery_ok)
{
    s_model = model;
    s_battery_ok = battery_ok;
    s_selected = 0;
    s_last_us = esp_timer_get_time();
    s_saved_age_min = model->age_min;

    show_main();
    s_sec_timer = lv_timer_create(second_timer_cb, 1000, NULL);
    s_battle_timer = lv_timer_create(battle_timer_cb, 550, NULL);
    if (s_battle_timer != NULL) lv_timer_pause(s_battle_timer);
    s_link_timer = lv_timer_create(link_timer_cb, 200, NULL);
    if (s_link_timer != NULL) lv_timer_pause(s_link_timer);
    ESP_LOGI(TAG, "vpet app started (battery=%d, scale=%ux)", battery_ok, VPET_TIME_SCALE);
    return s_sec_timer != NULL;
}
