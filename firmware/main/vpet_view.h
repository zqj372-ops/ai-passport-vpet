// vpet_view.h —— LVGL 界面(全部英文标签:设备只装了 Montserrat 字体,中文需要另嵌字库)
#pragma once

#include "lvgl.h"

#include "vpet_battle.h"
#include "vpet_model.h"

#define VPET_MENU_COUNT 9U

typedef enum {
    VPET_MENU_FOOD = 0,
    VPET_MENU_PROTEIN,
    VPET_MENU_TRAIN,
    VPET_MENU_CLEAN,
    VPET_MENU_LIGHT,
    VPET_MENU_STATUS,
    VPET_MENU_BATTLE,
    VPET_MENU_LINK,
    VPET_MENU_SOUND,
} vpet_menu_t;

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *sprite;
    lv_obj_t *title;
    lv_obj_t *clock;
    lv_obj_t *hunger_hearts[VPET_HEARTS_MAX];
    lv_obj_t *strength_hearts[VPET_HEARTS_MAX];
    lv_obj_t *status;
    lv_obj_t *menu_panels[VPET_MENU_COUNT];
    lv_obj_t *menu_labels[VPET_MENU_COUNT];
    lv_obj_t *hint;
    uint8_t species;
    uint8_t stage;
} vpet_main_view_t;

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *body;
    lv_obj_t *sprite;
    lv_obj_t *title;
    lv_obj_t *info;
    lv_obj_t *progress_bg;
    lv_obj_t *progress_fg;
    lv_obj_t *hint;
} vpet_page_view_t;

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *round_label;
    lv_obj_t *track;
    lv_obj_t *marker;
    lv_obj_t *target;
    lv_obj_t *hint;
    lv_obj_t *score_label;
    lv_obj_t *sprite;
} vpet_charge_view_t;

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *left_sprite;
    lv_obj_t *right_sprite;
    lv_obj_t *left_name;
    lv_obj_t *right_name;
    lv_obj_t *left_hp_bg;
    lv_obj_t *left_hp_fg;
    lv_obj_t *right_hp_bg;
    lv_obj_t *right_hp_fg;
    lv_obj_t *log;
    lv_obj_t *result;
    lv_obj_t *hint;
} vpet_battle_view_t;

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *old_sprite;
    lv_obj_t *new_sprite;
    lv_obj_t *flash;
    lv_obj_t *title;
    lv_obj_t *name;
    lv_obj_t *stage;
    lv_obj_t *hint;
} vpet_evo_view_t;

// 进化动画总时长(毫秒):前 600ms 闪烁变身,之后白光渐亮,到点出结果
#define VPET_EVO_DURATION_MS 1500U

void vpet_view_main_create(vpet_main_view_t *view, const vpet_model_t *model);
void vpet_view_main_refresh(vpet_main_view_t *view, const vpet_model_t *model,
                            uint8_t selected, uint8_t sound_level);

void vpet_view_page_create(vpet_page_view_t *view, const char *title,
                           const vpet_model_t *model, uint8_t species_override);
void vpet_view_page_set(vpet_page_view_t *view, const char *title, const char *info,
                        uint8_t species, uint8_t progress);

void vpet_view_charge_create(vpet_charge_view_t *view, const char *title,
                             const vpet_model_t *model);
void vpet_view_charge_set_round(vpet_charge_view_t *view, uint8_t round, uint8_t total);
void vpet_view_charge_set_marker(vpet_charge_view_t *view, uint8_t percent);
void vpet_view_charge_set_hint(vpet_charge_view_t *view, const char *text);
void vpet_view_charge_set_score(vpet_charge_view_t *view, const char *text);

void vpet_view_battle_create(vpet_battle_view_t *view, const vpet_battle_t *battle);
void vpet_view_battle_refresh(vpet_battle_view_t *view, const vpet_battle_t *battle,
                              const char *log_line, const char *result, uint8_t left_hp_pct,
                              uint8_t right_hp_pct);

// 进化动画:老形态和新形态叠在同一个位置,靠闪烁/显隐做"变身"效果
void vpet_view_evolution_create(vpet_evo_view_t *view, uint8_t old_species,
                                uint8_t new_species);
// elapsed_ms:动画已经播放的毫秒数;finished=true 时显示结果文字
void vpet_view_evolution_frame(vpet_evo_view_t *view, uint16_t elapsed_ms, bool finished,
                               const char *name, const char *stage_name);

lv_obj_t *vpet_view_message_screen(const char *title, const char *line1, const char *line2,
                                   uint32_t bg);
