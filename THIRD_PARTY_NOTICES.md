# 第三方组件与许可证

本项目包含或派生自以下第三方作品，各自的许可证如下。

## 1. FoloToy AI Passport 固件框架（MIT）

`firmware/` 目录派生自 <https://github.com/FoloToy/ai-passport>，
Copyright © 2026 FoloToy，按 MIT 许可证发布（全文见仓库根目录 `LICENSE`）。

本项目在其基础上新增了游戏逻辑、界面、对战与字库子集；
上游的 `LICENSE`、`docs/`、`AGENTS.md` 等原始文件均予保留。

## 2. Noto Sans CJK SC（SIL Open Font License 1.1）

`firmware/main/vpet_font_16.c` 是用 `lv_font_conv` 从 Noto Sans CJK SC 抽取的**位图字体子集**
（只包含本项目界面用到的字形），并非再分发原始字体文件。

- 字体项目：<https://github.com/notofonts/noto-cjk>
- 版权：Copyright © 2014-2021 Adobe (<http://www.adobe.com/>)，保留字体名称 "Noto"
- 许可证：SIL Open Font License 1.1（全文见 [`licenses/OFL-1.1.txt`](licenses/OFL-1.1.txt)）

OFL 允许嵌入、修改与再分发（包括作为位图子集嵌入固件），并要求本许可证随附；
本项目未使用保留字体名称发布任何字体软件。

## 3. LVGL（MIT）

界面基于 LVGL 9.x（<https://github.com/lvgl/lvgl>，MIT），
由 ESP-IDF 组件管理器（`dependencies.lock`）在构建时拉取，不随本仓库分发。

## 4. Espressif ESP-IDF（Apache-2.0）

构建使用 ESP-IDF v5.5.3（<https://github.com/espressif/esp-idf>，Apache License 2.0），
同样在构建时拉取，不随本仓库分发。

## 5. 怪兽形象

31 种怪兽的像素形象由本项目用代码绘制（`firmware/main/vpet_sprite.c`），为原创设计，
不包含任何第三方角色美术资源。

数码暴龙机 / Digital Monster 等名称与角色版权归 Bandai / 东映动画等权利人所有；
本项目仅复刻玩法机制，不包含其美术、名称或代码。
