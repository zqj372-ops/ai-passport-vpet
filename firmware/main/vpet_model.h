// vpet_model.h —— 数码暴龙机(风格)固件的养成模型(纯 C,不依赖 ESP-IDF,可在电脑上跑单元测试)
//
// 机制要点(对齐经典暴龙机):
//   * 饥饿心 / 力量心各 4 格,归零后持续放着不管会累积"照顾失误";
//   * 吃饭、喂蛋白粉增加体重,训练降低体重并提升力量;
//   * 便便最多 4 坨,不清扫会生病,生病久不处理会死亡;
//   * 夜晚会睡觉,睡觉时不关灯会累积照顾失误;
//   * 年龄、训练次数、战绩、照顾失误、体重共同决定进化方向;
//   * 照顾失误过多或自然寿命到期会死亡,死亡后按确定键可以重新获得一颗蛋。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "vpet_species.h"

#define VPET_MODEL_VERSION 1U
#define VPET_HEARTS_MAX 4U
#define VPET_POOP_MAX 4U
#define VPET_DAY_MINUTES 1440U
#define VPET_CARE_MISTAKE_LIMIT 30U

// 睡眠时段(宠物体内分钟):21:00 ~ 09:00
#define VPET_SLEEP_START_MIN (21U * 60U)
#define VPET_SLEEP_END_MIN   (9U * 60U)

// 蛋孵化所需时间(宠物体内分钟)
#define VPET_EGG_HATCH_MIN 7U

// 出生时刻(一天中的第几分钟):默认上午 10 点,避免刚孵化就落在睡眠时段里
#define VPET_START_MINUTE_OF_DAY (10U * 60U)

typedef enum {
    VPET_EVENT_NONE        = 0,
    VPET_EVENT_HATCH       = 1U << 0,
    VPET_EVENT_POOP        = 1U << 1,
    VPET_EVENT_HUNGRY      = 1U << 2,   // 有饥饿心归零
    VPET_EVENT_WEAK        = 1U << 3,   // 有力量心归零
    VPET_EVENT_SICK        = 1U << 4,
    VPET_EVENT_RECOVERED   = 1U << 5,
    VPET_EVENT_SLEEP       = 1U << 6,
    VPET_EVENT_WAKE        = 1U << 7,
    VPET_EVENT_CARE_MISTAKE= 1U << 8,
    VPET_EVENT_EVOLVE      = 1U << 9,
    VPET_EVENT_DEATH       = 1U << 10,
    VPET_EVENT_HUNGER_DOWN = 1U << 11,
    VPET_EVENT_STRENGTH_DOWN = 1U << 12,
} vpet_event_t;

typedef struct {
    uint32_t version;
    uint8_t  species;
    uint8_t  alive;          // 0 = 墓碑
    uint8_t  egg;            // 1 = 还没孵化
    uint8_t  hunger;         // 剩余饥饿心 0..4(0 = 饿了)
    uint8_t  strength;       // 剩余力量心 0..4
    uint8_t  poop;
    uint8_t  sick;
    uint8_t  sleeping;
    uint8_t  lights_off;
    uint8_t  stage_min_age;  // 保留
    uint16_t weight_g;
    uint32_t age_min;        // 孵化后的总年龄(分钟),用于寿命与体内时钟
    uint16_t stage_min;      // 当前阶段已度过的时间(分钟),用于进化条件
    uint16_t care_mistakes;
    uint16_t trainings;
    uint16_t battles;
    uint16_t wins;
    uint16_t hunger_timer;
    uint16_t strength_timer;
    uint16_t poop_timer;
    uint16_t starve_timer;
    uint16_t weak_timer;
    uint16_t sick_timer;
    uint16_t poop_full_timer;
    uint16_t light_timer;
    uint16_t heal_timer;
    uint16_t clock_offset;   // 出生时刻,决定"体内时钟"的相位
    uint8_t  last_hour;
    uint8_t  reserved0;
    uint32_t seed;
    uint32_t real_seconds;   // 累计真实秒数(用于 UI 显示与调试)
} vpet_model_t;

// 初始化一颗蛋
void vpet_model_init(vpet_model_t *model, uint32_t seed);

// 推进 pet_minutes 分钟,返回 vpet_event_t 位掩码
uint32_t vpet_model_tick(vpet_model_t *model, uint16_t pet_minutes);

// 动作返回值
typedef enum {
    VPET_RESULT_OK = 0,
    VPET_RESULT_BLOCKED,     // 睡觉/死亡/蛋状态下不允许
    VPET_RESULT_FULL,        // 已经满了
    VPET_RESULT_EMPTY,       // 没有可清理的东西
    VPET_RESULT_NO_STRENGTH, // 力量不足以对战/训练
    VPET_RESULT_DONE,
} vpet_result_t;

vpet_result_t vpet_model_feed_food(vpet_model_t *model);
vpet_result_t vpet_model_feed_protein(vpet_model_t *model);
uint8_t vpet_model_clean(vpet_model_t *model);              // 返回清理掉的便便数
vpet_result_t vpet_model_toggle_light(vpet_model_t *model);
vpet_result_t vpet_model_train(vpet_model_t *model, uint8_t score);  // score 0..4
vpet_result_t vpet_model_battle_start(vpet_model_t *model);  // 消耗力量心
void vpet_model_battle_finish(vpet_model_t *model, bool won);
void vpet_model_kill(vpet_model_t *model);
uint32_t vpet_model_rand(vpet_model_t *model);

// 查询
const vpet_species_t *vpet_model_species(const vpet_model_t *model);
uint16_t vpet_model_clock_minutes(const vpet_model_t *model);   // 0..1439
uint8_t  vpet_model_attack_multiplier_band(const vpet_model_t *model); // 0..4,影响战斗伤害
bool     vpet_model_is_final_stage(const vpet_model_t *model);
uint16_t vpet_model_lifespan_days(const vpet_model_t *model);
uint8_t  vpet_model_evolution_progress(const vpet_model_t *model); // 0..100,用于 UI 进度条
