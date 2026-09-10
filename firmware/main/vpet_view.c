#include "vpet_view.h"

#include "vpet_species.h"
#include "vpet_fonts.h"
#include "vpet_sprite.h"

#define COL_BG      0xDCE9DC
#define COL_PANEL   0xF7F7EF
#define COL_INK     0x1B2A22
#define COL_ACCENT  0x2E7D4F
#define COL_HEART   0xE43B2F
#define COL_EMPTY   0xC9CFC4
#define COL_WARN    0xB0480F
#define COL_SELECT  0x1F5C3A
#define COL_EVO_BG  0x101A24
#define COL_EVO_HI  0xE8F6FF

#define VPET_EVO_SPARK_MS 600
#define VPET_EVO_BURST_MS VPET_EVO_DURATION_MS

static const char *MENU_LABELS[VPET_MENU_COUNT] = {
    "食物", "蛋白", "训练", "清扫", "灯光", "状态", "对战", "联机", "音效",
};

static lv_obj_t *mk(lv_obj_t *parent, int x, int y, int w, int h, int radius,
                    uint32_t color)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    return obj;
}

static lv_obj_t *mk_label(lv_obj_t *parent, const char *text, int x, int y,
                          const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

static lv_obj_t *screen_new(uint32_t bg)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_bg_color(scr, lv_color_hex(bg), 0);
    return scr;
}

void vpet_view_main_create(vpet_main_view_t *view, const vpet_model_t *model)
{
    view->screen = screen_new(COL_BG);
    const vpet_species_t *sp = vpet_model_species(model);

    view->title = mk_label(view->screen, sp ? sp->name : "?", 8, 6,
                           VPET_FONT, COL_INK);
    view->clock = mk_label(view->screen, "--:--", 168, 10, VPET_FONT,
                           COL_ACCENT);

    lv_obj_t *stage_bg = mk(view->screen, 8, 32, 224, 18, 4, COL_PANEL);
    view->status = mk_label(stage_bg, "", 4, 1, VPET_FONT, COL_INK);

    view->sprite = vpet_sprite_create(view->screen, model->species, 60, 56, 120);
    view->species = model->species;

    for (int i = 0; i < VPET_HEARTS_MAX; i++) {
        int x = 74 + i * 20;
        view->hunger_hearts[i] = mk(view->screen, x, 182, 14, 12, 3, COL_HEART);
        view->strength_hearts[i] = mk(view->screen, x, 202, 14, 12, 3, COL_HEART);
    }
    mk_label(view->screen, "饱食", 8, 180, VPET_FONT, COL_INK);
    mk_label(view->screen, "力量", 8, 200, VPET_FONT, COL_INK);

    for (uint8_t i = 0; i < VPET_MENU_COUNT; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = 3 + col * 78;
        int y = 212 + row * 28;
        view->menu_panels[i] = mk(view->screen, x, y, 74, 26, 6, COL_PANEL);
        view->menu_labels[i] = mk_label(view->menu_panels[i], MENU_LABELS[i], 0, 0,
                                        VPET_FONT, COL_INK);
        lv_obj_center(view->menu_labels[i]);
    }

    view->hint = mk_label(view->screen, "", 8, 298, VPET_FONT, COL_INK);
    lv_screen_load(view->screen);
}

static void set_heart(lv_obj_t *obj, bool filled)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(filled ? COL_HEART : COL_EMPTY), 0);
}

void vpet_view_main_refresh(vpet_main_view_t *view, const vpet_model_t *model,
                            uint8_t selected, uint8_t sound_level)
{
    const vpet_species_t *sp = vpet_model_species(model);
    uint16_t clock = vpet_model_clock_minutes(model);

    lv_label_set_text_fmt(view->clock, "%02u:%02u", clock / 60U, clock % 60U);

    if (sp != NULL) {
        if (model->species != view->species) {
            vpet_sprite_set_species(view->sprite, model->species, 120);
            view->species = model->species;
            lv_label_set_text(view->title, sp->name);
        }
        lv_label_set_text_fmt(view->status, "%s  %u天 %ug",
                              vpet_stage_name(sp->stage),
                              (unsigned)(model->age_min / VPET_DAY_MINUTES),
                              (unsigned)model->weight_g);
    }

    for (int i = 0; i < VPET_HEARTS_MAX; i++) {
        set_heart(view->hunger_hearts[i], (int)model->hunger > i);
        set_heart(view->strength_hearts[i], (int)model->strength > i);
    }

    if (!model->alive) {
        lv_label_set_text(view->hint, "按确定键");
    } else if (model->egg) {
        lv_label_set_text(view->hint, "孵化中…");
    } else if (model->sick) {
        lv_label_set_text(view->hint, "生病了 快清扫喂食");
    } else if (model->sleeping) {
        lv_label_set_text(view->hint, model->lights_off ? "睡觉中" : "睡觉中 请关灯");
    } else if (model->poop >= VPET_POOP_MAX) {
        lv_label_set_text(view->hint, "需要清扫");
    } else if (model->hunger == 0) {
        lv_label_set_text(view->hint, "肚子饿了");
    } else if (model->strength == 0) {
        lv_label_set_text(view->hint, "力量不足 喂蛋白");
    } else {
        lv_label_set_text(view->hint, "上下选择");
    }

    for (uint8_t i = 0; i < VPET_MENU_COUNT; i++) {
        bool sel = i == selected;
        bool enabled = model->alive && !model->egg && !model->sleeping;
        if (i == VPET_MENU_LIGHT) enabled = model->alive && !model->egg;
        if (i == VPET_MENU_STATUS) enabled = true;
        if (i == VPET_MENU_SOUND) enabled = true;
        lv_obj_set_style_bg_color(view->menu_panels[i],
                                  lv_color_hex(sel ? COL_SELECT : COL_PANEL), 0);
        lv_obj_set_style_text_color(view->menu_labels[i],
                                    lv_color_hex(sel ? 0xFFFFFF : (enabled ? COL_INK : 0x9AA79C)), 0);
    }

    lv_label_set_text_fmt(view->hint, "%s  音量%u", lv_label_get_text(view->hint),
                          (unsigned)sound_level);
}

void vpet_view_page_create(vpet_page_view_t *view, const char *title,
                           const vpet_model_t *model, uint8_t species_override)
{
    view->screen = screen_new(COL_BG);
    view->title = mk_label(view->screen, title, 8, 8, VPET_FONT, COL_INK);
    view->sprite = vpet_sprite_create(view->screen, species_override, 84, 40, 72);
    view->body = mk(view->screen, 8, 118, 224, 150, 6, COL_PANEL);
    view->info = mk_label(view->body, "", 6, 4, VPET_FONT, COL_INK);
    view->progress_bg = mk(view->screen, 8, 276, 224, 12, 4, COL_EMPTY);
    view->progress_fg = mk(view->screen, 8, 276, 0, 12, 4, COL_ACCENT);
    view->hint = mk_label(view->screen, "长按确定键返回", 8, 296, VPET_FONT,
                          COL_INK);
    (void)model;
    lv_screen_load(view->screen);
}

void vpet_view_page_set(vpet_page_view_t *view, const char *title, const char *info,
                        uint8_t species, uint8_t progress)
{
    if (title != NULL) lv_label_set_text(view->title, title);
    if (info != NULL) lv_label_set_text(view->info, info);
    // species = 0xFF 表示"只更新文字与进度条",不重建精灵(联机搜索时每 200ms 刷一次)
    if (species != 0xFF) vpet_sprite_set_species(view->sprite, species, 72);
    int width = (int)progress * 224 / 100;
    if (width < 0) width = 0;
    lv_obj_set_width(view->progress_fg, width);
}

void vpet_view_charge_create(vpet_charge_view_t *view, const char *title,
                             const vpet_model_t *model)
{
    view->screen = screen_new(COL_BG);
    mk_label(view->screen, title, 8, 8, VPET_FONT, COL_INK);
    view->round_label = mk_label(view->screen, "第 1/3 轮", 8, 36,
                                 VPET_FONT, COL_ACCENT);
    view->sprite = vpet_sprite_create(view->screen, model->species, 84, 56, 72);
    view->track = mk(view->screen, 20, 168, 200, 20, 6, COL_EMPTY);
    view->target = mk(view->screen, 20 + 70, 168, 60, 20, 6, 0xA8D8A8);
    view->marker = mk(view->screen, 22, 170, 10, 16, 3, COL_SELECT);
    view->hint = mk_label(view->screen, "指针到绿区按确定", 8, 200,
                          VPET_FONT, COL_INK);
    view->score_label = mk_label(view->screen, "", 8, 224, VPET_FONT, COL_INK);
    mk_label(view->screen, "长按确定键返回", 8, 296, VPET_FONT, COL_INK);
    lv_screen_load(view->screen);
}

void vpet_view_charge_set_round(vpet_charge_view_t *view, uint8_t round, uint8_t total)
{
    lv_label_set_text_fmt(view->round_label, "第 %u/%u 轮", (unsigned)round,
                          (unsigned)total);
}

void vpet_view_charge_set_marker(vpet_charge_view_t *view, uint8_t percent)
{
    if (percent > 94) percent = 94;
    lv_obj_set_x(view->marker, 22 + (int)percent * 196 / 100);
}

void vpet_view_charge_set_hint(vpet_charge_view_t *view, const char *text)
{
    lv_label_set_text(view->hint, text);
}

void vpet_view_charge_set_score(vpet_charge_view_t *view, const char *text)
{
    lv_label_set_text(view->score_label, text);
}

void vpet_view_battle_create(vpet_battle_view_t *view, const vpet_battle_t *battle)
{
    view->screen = screen_new(COL_BG);
    const vpet_species_t *left = vpet_species(battle->left.species);
    const vpet_species_t *right = vpet_species(battle->right.species);

    view->left_name = mk_label(view->screen, left ? left->name : "?", 6, 6,
                               VPET_FONT, COL_INK);
    view->right_name = mk_label(view->screen, right ? right->name : "?", 150, 6,
                                VPET_FONT, COL_INK);
    view->left_hp_bg = mk(view->screen, 6, 26, 100, 10, 3, COL_EMPTY);
    view->left_hp_fg = mk(view->screen, 6, 26, 100, 10, 3, COL_ACCENT);
    view->right_hp_bg = mk(view->screen, 134, 26, 100, 10, 3, COL_EMPTY);
    view->right_hp_fg = mk(view->screen, 134, 26, 100, 10, 3, COL_ACCENT);

    view->left_sprite = vpet_sprite_create(view->screen, battle->left.species, 16, 52, 88);
    view->right_sprite = vpet_sprite_create(view->screen, battle->right.species, 136, 52, 88);

    lv_obj_t *log_bg = mk(view->screen, 8, 154, 224, 60, 6, COL_PANEL);
    view->log = mk_label(log_bg, "", 6, 4, VPET_FONT, COL_INK);
    lv_obj_set_width(view->log, 210);
    lv_label_set_long_mode(view->log, LV_LABEL_LONG_WRAP);

    view->result = mk_label(view->screen, "", 8, 222, VPET_FONT, COL_SELECT);
    view->hint = mk_label(view->screen, "确定=快进  长按=退出", 8, 296,
                          VPET_FONT, COL_INK);
    lv_screen_load(view->screen);
}

void vpet_view_battle_refresh(vpet_battle_view_t *view, const vpet_battle_t *battle,
                              const char *log_line, const char *result, uint8_t left_hp_pct,
                              uint8_t right_hp_pct)
{
    lv_obj_set_width(view->left_hp_fg, (int)left_hp_pct * 100 / 100);
    lv_obj_set_width(view->right_hp_fg, (int)right_hp_pct * 100 / 100);
    if (log_line != NULL) lv_label_set_text(view->log, log_line);
    if (result != NULL) lv_label_set_text(view->result, result);
    (void)battle;
}

lv_obj_t *vpet_view_message_screen(const char *title, const char *line1, const char *line2,
                                   uint32_t bg)
{
    lv_obj_t *scr = screen_new(bg);
    mk_label(scr, title, 8, 30, VPET_FONT, COL_INK);
    if (line1 != NULL) mk_label(scr, line1, 8, 80, VPET_FONT, COL_INK);
    if (line2 != NULL) mk_label(scr, line2, 8, 110, VPET_FONT, COL_INK);
    mk_label(scr, "按确定键", 8, 160, VPET_FONT, COL_ACCENT);
    lv_screen_load(scr);
    return scr;
}

// ---------------------------------------------------------------- 进化动画

void vpet_view_evolution_create(vpet_evo_view_t *view, uint8_t old_species,
                                uint8_t new_species)
{
    view->screen = screen_new(COL_EVO_BG);
    view->title = mk_label(view->screen, "进化中…", 8, 8, VPET_FONT, COL_EVO_HI);
    view->old_sprite = vpet_sprite_create(view->screen, old_species, 60, 64, 120);
    view->new_sprite = vpet_sprite_create(view->screen, new_species, 60, 64, 120);
    lv_obj_add_flag(view->new_sprite, LV_OBJ_FLAG_HIDDEN);

    view->name = mk_label(view->screen, "", 8, 200, VPET_FONT, COL_EVO_HI);
    view->stage = mk_label(view->screen, "", 8, 226, VPET_FONT, COL_ACCENT);
    view->hint = mk_label(view->screen, "", 8, 262, VPET_FONT, COL_EVO_HI);
    lv_obj_add_flag(view->name, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view->stage, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view->hint, LV_OBJ_FLAG_HIDDEN);

    // 覆盖全屏的白光,放最后创建保证在最上层
    view->flash = mk(view->screen, 0, 0, 240, 320, 0, COL_EVO_HI);
    lv_obj_set_style_bg_opa(view->flash, LV_OPA_TRANSP, 0);
    lv_screen_load(view->screen);
}

void vpet_view_evolution_frame(vpet_evo_view_t *view, uint16_t elapsed_ms, bool finished,
                               const char *name, const char *stage_name)
{
    if (view == NULL || view->screen == NULL) return;

    if (finished) {
        lv_obj_remove_flag(view->new_sprite, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->old_sprite, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_opa(view->flash, LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(view->screen, lv_color_hex(COL_BG), 0);
        lv_obj_set_style_text_color(view->title, lv_color_hex(COL_INK), 0);
        lv_label_set_text(view->title, "进化完成!");
        if (name != NULL) lv_label_set_text(view->name, name);
        if (stage_name != NULL) lv_label_set_text(view->stage, stage_name);
        lv_obj_remove_flag(view->name, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(view->stage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(view->hint, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(view->hint, "按确定键继续");
        return;
    }

    if (elapsed_ms < VPET_EVO_SPARK_MS) {
        // 快速闪烁:老/新形态每 100ms 交换一次,白光同步脉冲
        bool show_new = ((elapsed_ms / 100) % 2) == 1;
        if (show_new) {
            lv_obj_remove_flag(view->new_sprite, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(view->old_sprite, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(view->new_sprite, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(view->old_sprite, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_set_style_bg_opa(view->flash, show_new ? LV_OPA_30 : LV_OPA_TRANSP, 0);
        return;
    }

    // 白光渐亮,新形态定住
    lv_obj_remove_flag(view->new_sprite, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view->old_sprite, LV_OBJ_FLAG_HIDDEN);
    uint16_t span = VPET_EVO_BURST_MS - VPET_EVO_SPARK_MS;
    uint16_t progress = elapsed_ms > VPET_EVO_SPARK_MS ? (uint16_t)(elapsed_ms - VPET_EVO_SPARK_MS)
                                                       : 0;
    if (progress > span) progress = span;
    lv_opa_t opacity = (lv_opa_t)(progress * (LV_OPA_COVER / 2) / span);   // 0 ~ 50%
    lv_obj_set_style_bg_opa(view->flash, opacity, 0);
}
