// vpet_fonts.h —— 界面字体
//
// vpet_font_16 是用 lv_font_conv 从 Noto Sans CJK SC(OFL 许可)抽出的子集:
// 只包含本项目源码里真正用到的字形(ASCII + 界面/怪兽/招式用到的汉字),
// 因此体积很小,可以直接嵌进固件。
// 重新生成:tools/build_vpet_font.sh(见文件头注释)
#pragma once

#include "lvgl.h"

LV_FONT_DECLARE(vpet_font_16);

#define VPET_FONT (&vpet_font_16)
