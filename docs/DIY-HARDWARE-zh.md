# FoloToy AI Passport DIY 复刻指南

面向 [FoloToy/ai-passport](https://github.com/FoloToy/ai-passport)（MIT）的硬件复刻与二次开发笔记。

本目录内容：

- `firmware-baseline/`：官方固件源码（`main` 分支，含 `components/bsp` 板级支持包）
- 本文件：路线选择、元件清单、接线表、编译烧录、现成玩法、对战方案

> 事实来源：官方仓库 `docs/hardware-design/specifications.zh_CN.md`、
> `docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.zh_CN.md`、`components/bsp/include/bsp_pins.h`。
> 引脚以 `bsp_pins.h` 为唯一事实来源，本文只是导读。

---

## 0. 先选路线

| 路线 | 成本 | 工作量 | 适合谁 |
| --- | --- | --- | --- |
| A. 买官方整机 + 刷自制固件 | ¥99 | 10 分钟 | 只想玩、只写固件 |
| B. 模块拼装（本指南主线） | 约 ¥100–180 | 1–2 天 | 想自己焊一台、要加官方没有的硬件 |
| C. 自绘 PCB + 打样 | ¥300 起 | 1–2 周 | 要接近量产形态、复刻多台 |

官方整机 ¥99，**纯 DIY 不比买整机便宜**，价值在于：硬件完全可控、能加主动 NFC / 更大电池 / 更大屏幕，以及可批量复刻。

---

## 1. 复刻目标：官方硬件规格

| 项目 | 规格 |
| --- | --- |
| MCU | ESP32-C3，8 MB Flash，**无 PSRAM** |
| 显示 | ST7789P3，240×320，SPI2 @ 40 MHz，RGB565，需反色（`BSP_LCD_INVERT_COLOR=1`） |
| 按键 | UP / DOWN / OK 三键，共用 GPIO0 的 ADC 电阻梯 |
| 音频 | ES8311 codec，I2S0 全双工（播放 + 麦克风录音），外接功放常通 |
| 电量 | CW2017 电量计（I2C 0x63），可选 |
| NFC | **被动 NTAG213 标签**，仅支持 NDEF 读写，MCU 侧没有接口 |
| 充电 | USB Type-C 5 V；烧录/日志走 C3 原生 USB Serial/JTAG（GPIO18/19） |
| 电源键 | 独立硬件电源键（长按 0.5 s 开机、约 2 s 关机），与三枚功能键分离 |
| 尺寸 | 60 × 95 × 8.5 mm，50 g |

---

## 2. 元件清单（模块拼装版）

### 2.1 必需件

| # | 器件 | 规格 / 搜索关键词 | 参考价 | 说明 |
| --- | --- | --- | --- | --- |
| 1 | ESP32-C3 开发板 | 「ESP32-C3 开发板」4 MB，USB-C，**GPIO0–10、18–21 全部引出** | ¥10–25 | 官方是 8 MB；4 MB 需改一行 flash 配置（见 §4.3），分区表本身放得下 |
| 2 | SPI 屏 | 「ST7789 240×320 SPI 2.0寸 / 2.4寸」，7 针（GND/VCC/SCL/SDA/RES/DC/CS/BLK） | ¥15–35 | 官方是 ST7789P3；通用 ST7789 屏可能要调初始化序列/反色 |
| 3 | 轻触开关 ×3 | 6×6 或贴片按键 | ¥1 | 接 GPIO0 电阻梯 |
| 4 | 电阻 | 10 kΩ ×1、0 Ω ×1、1 kΩ ×1、2.2 kΩ ×1、4.7 kΩ ×2 | ¥2 | 10 kΩ 是 ADC 上拉；4.7 kΩ 是 I2C 上拉 |
| 5 | 音频 codec 模块 | 「ES8311 音频模块」（带咪头/喇叭座） | ¥15–30 | 官方方案，可直接复用 BSP |
| 6 | 喇叭 | 8 Ω 2 W，2030 腔体 | ¥3 | 需功放（多数 ES8311 模块自带 NS4150） |
| 7 | 锂电池 | 3.7 V 500 mAh，带保护板 | ¥15–25 | 官方 520 mAh |
| 8 | 充电板 | 「TP4056 Type-C 充电板」 | ¥5 | 给锂电充电；也可选带电量输出的模块 |
| 9 | 洞洞板/转接板 + 排针排线 | — | ¥10 | 固定与连线 |

### 2.2 可选项

| 器件 | 作用 | 缺失后果 |
| --- | --- | --- |
| CW2017 电量计模块 | 精确电量/电压 | 电池页显示 `[FAIL]`，其他功能不受影响（BSP 中电池是软依赖） |
| NTAG213 贴纸 | 手机碰一碰读资料 | 无 NFC 功能（本来也只是被动标签） |
| 主动 NFC 模块（PN532 / RC522） | 自制「碰一碰」交互 | 需要自己写驱动，官方固件不含此能力 |
| 3D 打印外壳 | 卡片形态 | 裸板使用 |
| 独立电源键 + 软开关电路 | 硬开关机 | 可用拨动开关替代 |

### 2.3 替代方案（省钱 / 好买）

- 音频改用 **MAX98357A（I2S 功放）+ INMP441（I2S 麦克风）**：比 ES8311 便宜、好买，但需要改
  `components/bsp` 里的 codec 初始化（`esp_codec_dev` 自带这两颗的驱动）。
- 屏幕可先用任意 ST7789 240×320；花屏/负片时先改 `BSP_LCD_INVERT_COLOR`，再对供应商初始化序列。
- 想省事可先只做「屏幕 + 三键 + 喇叭」，音频录制、电量计后续再补。

**工具**：电烙铁、万用表、杜邦线、USB-C 数据线（必须支持数据，不是纯充电线）。

---

## 3. 接线表

固件按下面的映射写死（来自 `components/bsp/include/bsp_pins.h`），**照抄即可零改动跑官方固件**。

### 3.1 显示（SPI2，40 MHz，mode 0）

| 屏幕引脚 | ESP32-C3 |
| --- | --- |
| SCL/SCK | GPIO8 |
| SDA/MOSI | GPIO9 |
| CS | GPIO1 |
| DC | GPIO20 |
| RES/RST | 接 3.3 V（固件用软复位，`BSP_LCD_RST = -1`） |
| BLK/BL | GPIO21（LEDC PWM 调光，5 kHz / 10 bit） |
| VCC / GND | 3.3 V / GND |

### 3.2 三键电阻梯（GPIO0 / ADC1_CH0）

```
3.3V ── 10kΩ ──┬── GPIO0
                ├── 按键 UP   ── 0 Ω   ── GND
                ├── 按键 DOWN ── 1 kΩ  ── GND
                └── 按键 OK   ── 2.2 kΩ ── GND
```

电压窗口（`BSP_BTN_MV_TABLE`）：UP `0–150 mV`、DOWN `150–447 mV`、OK `447–1900 mV`、松开 `≈3300 mV`。
不要用芯片内部上拉（约 45 kΩ，三档会挤在一起）。

### 3.3 音频（I2S0 全双工 + I2C0 控制）

| 信号 | ESP32-C3 | 说明 |
| --- | --- | --- |
| MCLK | GPIO6 | codec 要求 |
| BCLK | GPIO5 | |
| WS | GPIO3 | MCU 为 I2S master |
| DOUT | GPIO2 | MCU → codec（播放） |
| DIN | GPIO4 | codec → MCU（录音） |
| I2C SDA | GPIO10 | ES8311(0x18) 与 CW2017(0x63) 共用 |
| I2C SCL | GPIO7 | 需要 4.7 kΩ 外部上拉 |
| PA 使能 | 不接 MCU（`BSP_I2S_PA_CTRL = -1`，功放常通） | |

### 3.4 其它

| 项 | 说明 |
| --- | --- |
| 烧录/日志 | C3 原生 USB（GPIO18/19），接 USB-C 即可 |
| 电池 ADC | CW2017 走 I2C，不占 ADC |
| 注意 | GPIO21 与 UART0 默认 TX 冲突，所以控制台必须用 USB Serial/JTAG（官方配置已如此） |

---

## 4. 编译与烧录

### 4.1 环境

ESP-IDF **v5.5.3**（其它版本会出现配置差异）：

```bash
# 已在本机装好：
source /home/autumn/esp/esp-idf-v5.5.3/export.sh
idf.py --version     # 必须显示 ESP-IDF v5.5.3
```

首次在别的机器上装：

```bash
git clone --branch v5.5.3 --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf-v5.5.3
~/esp/esp-idf-v5.5.3/install.sh esp32c3
source ~/esp/esp-idf-v5.5.3/export.sh
```

### 4.2 官方基线构建（8 MB 芯片）

```bash
cd firmware-baseline
./tools/validate.sh --firmware      # 产出 build/FoloToy-AI-Passport-full.bin
python -m esptool --chip esp32c3 -p /dev/ttyACM0 -b 460800 \
    write-flash 0x0 build/FoloToy-AI-Passport-full.bin
```

⚠️ `cardid`（0x356000，设备身份）不能覆盖；**永远不要对已用过的设备执行 `idf.py erase-flash`**。
整机设备优先用分段 `idf.py flash`，只有空白芯片才从 `0x0` 整片写。

### 4.3 4 MB 开发板的改动

分区表仍然放得下（`cardid` 结束于 0x35A000 < 0x400000），只需改 flash 大小：

```diff
# sdkconfig.defaults
-CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y
+CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
```

然后 `idf.py set-target esp32c3 && idf.py build flash monitor`。
注意 `tools/validate.sh --firmware` 会强制校验 8 MB 布局，4 MB 板请用普通 `idf.py` 流程。

### 4.4 已在本机验证的构建结果

2026-09-10 在 Ubuntu x86_64 + ESP-IDF v5.5.3（`esp32c3`）上实测：

| 固件 | 应用大小 | 合并镜像 | 校验结果 |
| --- | --- | --- | --- |
| `feature/vpet`（养成 + 进化动画 + 双机 BLE 对战 + 中文界面，31 种怪兽） | 1,069,440 B / 3 MB 分区（剩 66%） | 1,134,976 B | `Protected firmware layout: PASS`、`Firmware build: PASS` |
| `main` 基线（硬件菜单） | 1,513,760 B / 3 MB 分区（剩 52%） | 1,579,296 B | `Protected firmware layout: PASS`、`Firmware build: PASS` |
| `demo/tamagezi`（虚拟宠物） | 715,168 B / 1 MB 分区（剩 32%） | 780,704 B | 构建成功 |

产物（可直接刷写）：

- `artifacts/vpet-full.bin`：`sha256 a52699c678a776ad1384cc0dcad9032e7c5277639312f97c161b9cc0872bef06`
- `artifacts/baseline-full.bin`：`sha256 3d1333ebd0cda04d2b6b7ca9793e5cd5c4654f08732b3642f9745ac422db216f`
- `artifacts/tamagezi-full.bin`：`sha256 236476b60d30836a50af58e4f26d7a2af485f20b17a3a0706d19ad98e6398162`

> `demo/tamagezi` 分支是较早的快照，没有 `tools/validate.sh` 和自定义分区表，用的是默认 1 MB 应用分区 +
> `CONFIG_ESPTOOLPY_HEADER_FLASHSIZE_UPDATE=y`（按实际 flash 容量回写），所以它**天然兼容 4 MB 开发板**，
> 不需要 §4.3 的改动。

---

## 5. 现成的养成玩法：`demo/tamagezi`

官方仓库已经有一个**完整的离线虚拟宠物固件**（社区贡献，非常适合作为「数码暴龙机」的起点）：

```bash
cd firmware-baseline
git fetch --depth 1 origin demo/tamagezi
git checkout demo/tamagezi
idf.py set-target esp32c3 && idf.py build flash monitor
```

该分支包含：12 只可选宠物、养育与健康、活跃时长成长、训练/学习小游戏、4 种打工、金币商店、
档案与融合、双槽 CRC 保护存档、电量显示、16 kHz 合成音效。三键操作：UP/DOWN 移动，
短按 OK 选择，长按 OK 返回。

另有 `demo/white-rabbit-pet`、`demo/tetris-game` 等分支可参考。

---

## 6. 关于 NFC 与「对战」的现实说明

1. **官方 AI Passport 的 NFC 是被动 NTAG213**：只能被手机读写，ESP32-C3 拿不到它，不能用来通信。
2. **正版万代数码暴龙机也不是 NFC 对战**：初代到最新 COLOR 系列用的是机身金属触点（コネクタ）接触式通信
   （见 Bandai 官方说明书 Digital Monster COLOR 第 16 页）。
3. 想让两台自制设备对战，现实做法：
   - **BLE**（推荐）：ESP32-C3 自带 BLE 5 LE，官方 `demo/claude-buddy-port` 分支有加密 BLE 通信、
     协议解析、状态机的完整参考；
   - **Wi-Fi/ESP-NOW**：适合多台、局域网；
   - **主动 NFC**（PN532/RC522）：可以做成「碰一碰」握手，但需要自写驱动，官方固件无此能力。
4. NFC 更合适的用法是：**做成卡片**（像万代 SCSA D-Ark 那样，卡片内置 NFC 标签，机器刷卡片触发事件），
   而不是设备间对战通信。

---

## 7. 上电验收清单

| # | 检查 | 期望 |
| --- | --- | --- |
| 1 | 烧录后 USB 日志 | 出现 `app_main` 初始化日志，无 backtrace |
| 2 | 屏幕 | 菜单正常显示（若负片 → 改 `BSP_LCD_INVERT_COLOR`） |
| 3 | 三键 | Button 页实时显示三档电压，落在 §3.2 窗口内 |
| 4 | 音频 | Audio 页能播放、能录音回放 |
| 5 | 电池 | Battery 页显示 SOC/电压（未接 CW2017 显示 FAIL 属正常） |
| 6 | 无线 | Wi-Fi 扫描页能列出热点；BLE 页能广播 `FoloPassport` |
| 7 | 低功耗 | light/deep sleep 示例能唤醒（deep sleep 会重启应用，正常） |

---

## 8. 进度

- [x] 拉取官方固件源码与硬件文档
- [x] 引脚表 / 接线表整理
- [x] ESP-IDF v5.5.3 工具链安装（本机路径 `/home/autumn/esp/esp-idf-v5.5.3`）
- [x] 基线固件编译验证（`Firmware build: PASS`）
- [x] 虚拟宠物固件（`demo/tamagezi`）编译验证
- [x] 自研数码暴龙机固件（`feature/vpet`）：模型/战斗/存档 + 主机单元测试 + 固件门禁全通过
- [x] 双机 BLE 对战：广播战斗卡 + 对称确定性战斗引擎（200 组种子双机对称性测试通过）
- [x] 中文界面：`tools/build_vpet_font.sh` 从 Noto Sans CJK SC 抽取子集（241 个汉字 → 335 字形）
- [x] 进化动画：闪烁变身 + 白光爆闪 + 结果亮相（可按键跳过）
- [x] 进化树扩到 31 种：新增 13 种怪兽、7 个新绘制样式，含失败分支与究极体"战神兽"线
- [ ] 元件采购 + 焊接
- [ ] 上机验收
