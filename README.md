# AI Passport V-Pet —— 数码暴龙机风格开源固件

给 **FoloToy AI Passport**（¥99 的开放式可穿戴 AI 硬件）写的一套开源养成游戏固件：
完整的经典暴龙机机制 + 中文界面 + 进化动画 + **双机 BLE 对战**。

> 这不是官方固件。本项目基于 FoloToy 官方开源仓库 [FoloToy/ai-passport](https://github.com/FoloToy/ai-passport)（MIT）二次开发，
> 复用其 BSP（显示/按键/音频/电量）与构建体系，游戏逻辑、界面、对战与字库为新增实现。

## 特性

| 模块 | 内容 |
| --- | --- |
| 养成 | 饥饿心 / 力量心各 4 格、体重、便便、生病、睡眠与关灯、照顾失误计数 |
| 进化 | **31 种原创怪兽**、7 个阶段（蛋 → 幼年期I/II → 成长期 → 成熟期 → 完全体 → 究极体），多分支 + 兜底分支 + "养歪"失败线 |
| 进化动画 | 老/新形态闪烁变身 → 白光渐亮 → 结果亮相，可按确定键跳过 |
| 训练 | 3 轮"力量蓄力"小游戏（指针停在绿区得分），影响力量与体重 |
| 对战 | 打 CPU：先蓄力再自动对战，记录胜率 |
| 联机对战 | 两台设备 **BLE 广播 + 扫描**交换战斗卡，各自独立算出**完全一致**的战斗 |
| 存档 | NVS 双槽 + CRC，掉电不丢 |
| 音效 | 进食/训练/命中/胜负/进化/死亡等 16 kHz 合成音 |
| 界面 | 全中文，内嵌 Noto Sans CJK SC 子集字库（274 字形，约 36 KB） |

怪兽形象为**原创像素风**（数据驱动绘制），不包含任何第三方角色美术。

<details>
<summary>English overview</summary>

An unofficial open-source virtual-pet firmware for the **FoloToy AI Passport** (a ¥99 hackable
ESP32-C3 wearable), in the spirit of the classic Digimon virtual pets:

- 31 original monsters across 7 evolution stages, multi-branch evolution tree with
  "neglect" failure lines, driven by age / training / battle record / care mistakes / weight
- Evolution animation, power-charge training mini-game, CPU battles
- **Two-device battles over BLE**: both units broadcast a 17-byte battle card and passively scan;
  the battle engine is symmetric and deterministic, so both screens compute the identical fight
  without pairing or connecting
- Full Chinese UI with an embedded Noto Sans CJK SC subset font; NVS save with CRC; synthesized sound

Flash `artifacts/vpet-full.bin` with the official web flasher
(<https://ai-passport.folotoy.cn/tools/web-flasher/>). Built with ESP-IDF v5.5.3; CI in this repo
runs the host tests and a full firmware build.

Monster art is original pixel drawing — no third-party character assets are included.

</details>

## 快速开始（刷机，3 分钟）

需要：一台 AI Passport、一根能传数据的 USB-C 线、Chrome 或 Edge。

1. 打开官方浏览器刷机工具：<https://ai-passport.folotoy.cn/tools/web-flasher/>
2. 用数据线连接设备并开机 → 点「连接设备」→ 选择名称含 `USB JTAG/serial debug unit` 的串口
3. 选择本地固件：[`artifacts/vpet-full.bin`](artifacts/vpet-full.bin)
4. 点「开始写入」，等校验通过、设备自动重启

⚠️ 刷机会覆盖设备上现有的全部内容（和安装官方小游戏一样）；**不要勾选「清除设备数据」**。
想恢复出厂固件：扫设备背面二维码 → 在专属页面用 Chrome/Edge 走官方恢复流程。

## 操作

三枚按键：`上` / `下` / `确定`。

- **上/下**：切换菜单项
- **短按确定**：执行
- **长按确定**：返回主界面

主界面菜单（3×3）：`食物` `蛋白` `训练` `清扫` `灯光` `状态` `对战` `联机` `音效`。

双机对战：两台都进 `联机` → 各玩一次蓄力小游戏 → 20 秒内自动搜索（两台靠近 1 米内）→ 找到即开打。
两边屏幕上的战斗过程和胜负完全一致。

## 玩法细节

### 养成循环

- 每 60 宠物分钟掉 1 颗饥饿心、90 分钟掉 1 颗力量心（睡觉时更慢）
- 吃饭 +1g、喂蛋白 +2g，训练 -2g 并提升力量
- 便便最多 4 坨，满格不清扫会生病，病久了会死
- 21:00–09:00 睡觉，**睡觉不关灯会记照顾失误**
- 照顾失误满 30 次死亡；死亡后按确定键重新领蛋
- 时间倍率：1 真实分钟 = 10 宠物分钟（1 宠物日 ≈ 2.4 小时），改 `firmware/main/vpet_app.h` 里的 `VPET_TIME_SCALE`

### 进化树（节选）

```
蛋 ─┬─ 点点兽 ─┬─ 团子兽 ─┬─ 火花龙 ─┬─ 烈焰龙 → 太阳龙 → 创世龙
    ├─ 火苗兽  │          │          ├─ 雷翼兽 → 暴风兽 → 终焉兽 / 战神兽
    ├─ 水滴兽  │          │          └─ 泥浆兽 → 傀儡兽        (失误≥8 的失败线)
    └─ 叶籽兽  ├─ 齿轮仔 ─┼─ 铁钉兽 → 岩壳兽 → 深渊兽 → 终焉兽
               │          └─ 电光仔 → 晶石兽 → 暴风兽
               ├─ 泡泡仔 ─┬─ 水泡兽 → 冰甲兽 → 深渊兽
               │          └─ 电光仔
               └─ 根须兽 ─┬─ 叶芽兽 → 木灵兽 ─┬─ 太阳龙
                          │                   └─ 傀儡兽         (失误≥12 的失败线)
                          ├─ 岩砾兽 → 岩壳兽
                          └─ 暗影仔 → 幽影兽 → 深渊兽           (失误≥4 的失败线)
```

完整进化条件（含训练/胜场/体重/失误上下限）可以打印出来看：

```bash
cc -std=c11 -Ifirmware/main tools/print_evolution_tree.c firmware/main/vpet_species.c \
   -o /tmp/vpet_tree && /tmp/vpet_tree
```

### 双机对战原理

不做 BLE 连接，而是两边各自把 17 字节的"战斗卡"塞进广播、同时被动扫描：

- 战斗引擎是**对称确定性**的：每一方的随机流只由「共享种子 + 自己的战斗卡」推导
- 共享种子用异或推导（可交换），所以两台设备算出的过程与胜负**完全一致**，且与"谁在左边"无关
- 好处：不用配对、不用区分主从、断连不会卡死，也不吃 ESP32-C3 那点内存

## 从源码构建

需要 ESP-IDF **v5.5.3**（ESP32-C3）：

```bash
git clone --branch v5.5.3 --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf-v5.5.3
~/esp/esp-idf-v5.5.3/install.sh esp32c3
source ~/esp/esp-idf-v5.5.3/export.sh

cd firmware
./tools/validate.sh --firmware     # 产出 build/FoloToy-AI-Passport-full.bin
./tools/validate.sh --static       # 仓库检查 + 主机单元测试
```

开发分支在 `firmware/`，核心文件：

| 文件 | 作用 |
| --- | --- |
| `main/vpet_species.c` | 31 种怪兽 + 进化树数据 |
| `main/vpet_model.c` | 养成模型（计时、喂食、生病、死亡、进化判定） |
| `main/vpet_battle.c` | 对称确定性对战引擎 |
| `main/vpet_link.c` | BLE 广播/扫描与战斗卡编解码 |
| `main/vpet_sprite.c` | 原创像素怪兽绘制（想换素材只改这一个文件） |
| `main/vpet_view.c` / `vpet_app.c` | LVGL 界面与状态机 |
| `main/vpet_font_16.c` | 内嵌中文字库（生成物） |
| `tests/test_vpet_model.c` | 主机单元测试（进化树结构、失败线、对战对称性） |

重新生成中文字库（改了文案或怪兽名之后）：

```bash
tools/build_vpet_font.sh    # 需要 node + python3(+fontTools);会自动校验字形是否齐全
```

## 仓库结构

```
artifacts/vpet-full.bin   可直接刷机的固件（含中文界面、进化动画、BLE 对战）
docs/QUICKSTART-zh.md     整机路线:购买 → 开箱 → 刷机 → 恢复出厂
docs/DIY-HARDWARE-zh.md   自己焊一台:元件清单、接线表、编译烧录
firmware/                 固件源码（ESP-IDF 工程,基于官方 BSP）
tools/                    字库生成脚本、进化树打印工具
licenses/                 第三方许可证全文
```

## 兼容性说明

- 只支持 **FoloToy AI Passport**（ESP32-C3 / 8 MB Flash / ST7789P3 240×320 / ES8311）
- 刷写镜像从 `0x0` 开始，长度 1.1 MB，**不覆盖** `0x356000` 起的设备身份分区（cardid）
- 官方固件可以随时通过设备二维码页面恢复

## 许可证与致谢

- 本项目：MIT（见 [LICENSE](LICENSE)）
- 上游固件框架：FoloToy AI Passport（MIT，Copyright © 2026 FoloToy）
- 中文字库：Noto Sans CJK SC（SIL Open Font License 1.1，见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)）
- 怪兽形象：本项目原创像素绘制，不含任何第三方角色美术

数码暴龙机 / Digital Monster、FoloToy、AI Passport 等名称与商标归各自权利人所有；
本项目是非官方爱好者作品，仅复刻**玩法机制**（养成、进化、对战），与厂商无关联。
