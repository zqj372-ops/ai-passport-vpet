#include "vpet_sprite.h"

#include "vpet_species.h"

static lv_obj_t *blk(lv_obj_t *parent, int x, int y, int w, int h, int radius,
                     uint32_t color)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    return obj;
}

static void eyes(lv_obj_t *root, int u, int x_left, int x_right, int y, int size)
{
    blk(root, x_left, y, size, size, size / 2, 0x1B1B2A);
    blk(root, x_right, y, size, size, size / 2, 0x1B1B2A);
}

static void draw_creature(lv_obj_t *root, uint8_t species, int size)
{
    const vpet_species_t *sp = vpet_species(species);
    uint32_t body = sp ? sp->body_color : 0x9BE07A;
    uint32_t accent = sp ? sp->accent_color : 0x4F9E3A;
    int u = size / 10;
    if (u < 3) u = 3;
    uint8_t shape = sp ? sp->shape : 0;

    switch (shape) {
    case 0:   // 蛋
        blk(root, u * 2, u, u * 6, u * 8, u * 3, body);
        blk(root, u * 3, u * 3, u, u, u / 2, accent);
        blk(root, u * 6, u * 5, u, u, u / 2, accent);
        blk(root, u * 4, u * 7, u, u, u / 2, accent);
        break;

    case 1:   // 圆团子
        blk(root, u, u * 2, u * 8, u * 6, u * 3, body);
        eyes(root, u, u * 3, u * 6, u * 4, u);
        blk(root, u * 4, u * 6, u * 2, u / 2, u / 4, 0x1B1B2A);
        blk(root, u * 2, u * 8, u, u, u / 2, accent);
        blk(root, u * 7, u * 8, u, u, u / 2, accent);
        break;

    case 2:   // 小火苗
        blk(root, u * 2, u * 3, u * 6, u * 6, u * 2, body);
        blk(root, u * 4, u, u * 2, u * 3, u, accent);
        blk(root, u * 5, 0, u * 2, u * 2, u, 0xFFE066);
        eyes(root, u, u * 3, u * 6, u * 4, u);
        break;

    case 3:   // 圆点带小角
        blk(root, u * 2, u * 3, u * 6, u * 6, u * 3, body);
        blk(root, u, u, u, u * 2, u / 2, accent);
        blk(root, u * 8, u, u, u * 2, u / 2, accent);
        eyes(root, u, u * 3, u * 6, u * 5, u);
        break;

    case 4:   // 齿轮
        blk(root, u * 2, u * 2, u * 6, u * 6, u, body);
        for (int i = 1; i < 9; i += 2) {
            blk(root, u * i, u, u, u * 2, 0, accent);
            blk(root, u * i, u * 7, u, u * 2, 0, accent);
        }
        eyes(root, u, u * 3, u * 6, u * 4, u);
        break;

    case 5:   // 小龙
        blk(root, u * 2, u * 3, u * 6, u * 5, u * 2, body);
        blk(root, u * 6, u * 2, u * 3, u * 2, u / 2, body);       // 头部
        blk(root, u * 7, u, u, u * 2, 0, accent);                  // 角
        blk(root, u, u * 6, u * 2, u, u / 2, accent);              // 尾
        eyes(root, u, u * 6, u * 8, u * 3, u);
        blk(root, u * 2, u * 8, u * 2, u, u / 2, accent);
        blk(root, u * 6, u * 8, u * 2, u, u / 2, accent);
        break;

    case 6:   // 水泡
        blk(root, u * 2, u * 3, u * 6, u * 6, u * 3, body);
        blk(root, u, u, u * 2, u * 2, u, accent);
        blk(root, u * 7, u * 2, u * 2, u * 2, u, accent);
        eyes(root, u, u * 3, u * 6, u * 5, u);
        break;

    case 7:   // 叶芽
        blk(root, u * 2, u * 3, u * 6, u * 6, u * 2, body);
        blk(root, u * 4, 0, u * 2, u * 3, u / 2, accent);
        blk(root, u * 6, u, u * 3, u * 2, u, 0x8FE07A);
        eyes(root, u, u * 3, u * 6, u * 5, u);
        break;

    case 8:   // 尖刺
        blk(root, u * 2, u * 3, u * 6, u * 5, u, body);
        for (int i = 2; i < 8; i += 2) {
            blk(root, u * i, u, u, u * 3, 0, accent);
        }
        eyes(root, u, u * 3, u * 6, u * 4, u);
        blk(root, u * 2, u * 8, u * 2, u, 0, accent);
        blk(root, u * 6, u * 8, u * 2, u, 0, accent);
        break;

    case 9:   // 带翼
        blk(root, 0, u * 3, u * 2, u * 4, u / 2, accent);
        blk(root, u * 8, u * 3, u * 2, u * 4, u / 2, accent);
        blk(root, u * 2, u * 2, u * 6, u * 6, u * 2, body);
        eyes(root, u, u * 3, u * 6, u * 4, u);
        blk(root, u * 4, u * 8, u * 2, u, u / 2, 0xFFD34F);
        break;

    case 10:  // 硬壳
        blk(root, u, u * 3, u * 8, u * 6, u, body);
        for (int i = 2; i < 9; i += 3) {
            blk(root, u * i, u * 4, u, u * 4, 0, accent);
        }
        eyes(root, u, u * 2, u * 7, u * 6, u);
        break;

    case 11:  // 独角
        blk(root, u, u * 4, u * 8, u * 5, u * 2, body);
        blk(root, u * 4, 0, u * 2, u * 4, u / 2, accent);
        eyes(root, u, u * 2, u * 7, u * 6, u);
        break;

    case 12:  // 冠冕
        blk(root, u, u * 3, u * 8, u * 6, u * 2, body);
        blk(root, u * 3, 0, u * 4, u * 3, u / 4, accent);
        blk(root, 0, u * 2, u * 2, u * 4, u / 2, 0xFFE066);
        blk(root, u * 8, u * 2, u * 2, u * 4, u / 2, 0xFFE066);
        eyes(root, u, u * 3, u * 6, u * 4, u);
        break;

    case 13:  // 幽影
        blk(root, u * 2, u * 2, u * 6, u * 6, u * 2, body);
        blk(root, u * 3, u * 8, u, u * 2, 0, body);
        blk(root, u * 6, u * 8, u, u * 2, 0, body);
        blk(root, u * 3, u * 4, u * 2, u, 0, 0xFF4D6D);
        blk(root, u * 6, u * 4, u * 2, u, 0, 0xFF4D6D);
        break;

    case 14:  // 泥浆:软塌塌的一坨,带气泡
        blk(root, u, u * 4, u * 8, u * 5, u * 2, body);
        blk(root, u * 2, u * 2, u * 3, u * 3, u, accent);
        blk(root, u * 6, u * 3, u * 2, u * 2, u, accent);
        blk(root, u * 3, u * 9, u * 4, u, u / 2, accent);
        eyes(root, u, u * 3, u * 6, u * 5, u);
        break;

    case 15:  // 傀儡:方头方脑 + 提线
        blk(root, u * 2, u * 2, u * 6, u * 7, 0, body);
        blk(root, u * 3, 0, u, u * 2, 0, 0xE8E0D0);
        blk(root, u * 6, 0, u, u * 2, 0, 0xE8E0D0);
        blk(root, u * 3, u * 4, u * 2, u, 0, 0xFFE066);
        blk(root, u * 6, u * 4, u * 2, u, 0, 0xFFE066);
        blk(root, u * 3, u * 7, u * 4, u / 2, 0, accent);
        break;

    case 16:  // 晶体:菱形棱柱
        blk(root, u * 4, 0, u * 2, u * 3, 0, accent);
        blk(root, u * 2, u * 2, u * 6, u * 5, u / 2, body);
        blk(root, u, u * 5, u * 2, u * 3, 0, accent);
        blk(root, u * 7, u * 5, u * 2, u * 3, 0, accent);
        blk(root, u * 3, u * 9, u * 4, u, 0, body);
        eyes(root, u, u * 3, u * 6, u * 4, u);
        break;

    case 17:  // 战鸟:张开双翼 + 尖喙
        blk(root, 0, u * 2, u * 3, u * 2, 0, accent);
        blk(root, u * 7, u * 2, u * 3, u * 2, 0, accent);
        blk(root, u * 3, u * 3, u * 4, u * 5, u, body);
        blk(root, u * 7, u, u * 2, u * 2, 0, 0xFFE066);
        blk(root, u * 9, u * 3, u, u, 0, 0xFFB23E);
        eyes(root, u, u * 4, u * 6, u * 4, u);
        blk(root, u * 4, u * 8, u, u * 2, 0, accent);
        blk(root, u * 6, u * 8, u, u * 2, 0, accent);
        break;

    case 18:  // 树灵:树冠 + 枝干
        blk(root, u * 2, 0, u * 6, u * 4, u, body);
        blk(root, u * 4, u * 4, u * 2, u * 5, 0, accent);
        blk(root, u, u * 5, u * 2, u, 0, accent);
        blk(root, u * 7, u * 5, u * 2, u, 0, accent);
        blk(root, u * 3, u * 2, u * 4, u * 2, u / 2, 0x8FE07A);
        blk(root, u * 4, u * 5, u, u, u / 2, 0x1B1B2A);
        blk(root, u * 6, u * 5, u, u, u / 2, 0x1B1B2A);
        break;

    case 19:  // 岩石:多面体 + 裂纹
        blk(root, u, u * 3, u * 8, u * 6, u / 2, body);
        blk(root, u * 2, u * 2, u * 3, u * 2, 0, accent);
        blk(root, u * 6, u * 2, u * 3, u * 2, 0, accent);
        blk(root, u * 4, u * 5, u, u * 4, 0, accent);
        blk(root, u * 6, u * 6, u * 2, u, 0, accent);
        eyes(root, u, u * 4, u * 6, u * 3, u);
        break;

    case 20:  // 暗影:尖耳 + 只露一双眼
        blk(root, u * 2, u * 3, u * 6, u * 6, u, body);
        blk(root, u, 0, u * 2, u * 3, 0, body);
        blk(root, u * 7, 0, u * 2, u * 3, 0, body);
        blk(root, u * 3, u * 5, u, u, 0, 0xFF4D6D);
        blk(root, u * 6, u * 5, u, u, 0, 0xFF4D6D);
        blk(root, u * 3, u * 8, u * 4, u, 0, accent);
        break;

    default:
        blk(root, u * 2, u * 3, u * 6, u * 6, u, body);
        eyes(root, u, u * 3, u * 6, u * 4, u);
        break;
    }
}

void vpet_sprite_set_species(lv_obj_t *root, uint8_t species, int size)
{
    if (root == NULL) return;
    lv_obj_clean(root);
    draw_creature(root, species, size);
}

lv_obj_t *vpet_sprite_create(lv_obj_t *parent, uint8_t species, int x, int y, int size)
{
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(root, x, y);
    lv_obj_set_size(root, size, size);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    draw_creature(root, species, size);
    return root;
}
