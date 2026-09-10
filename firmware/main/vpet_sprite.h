// vpet_sprite.h —— 原创像素风怪兽绘制
//
// 这里刻意把"美术"和"逻辑"分开:想换成自己的素材时,只要替换本文件的绘制实现
// (或改成读 PNG/数组),进化、养成、对战逻辑都不用动。
#pragma once

#include "lvgl.h"

lv_obj_t *vpet_sprite_create(lv_obj_t *parent, uint8_t species, int x, int y, int size);
void vpet_sprite_set_species(lv_obj_t *root, uint8_t species, int size);
