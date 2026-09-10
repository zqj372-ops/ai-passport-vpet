// vpet_species.h —— 数码暴龙机(风格)固件:怪兽种类表与进化树定义
//
// 设计原则:
//   * 形象为原创数据(shape + 配色),不复制任何官方角色美术;
//     想换成自己的素材时,只需要替换 vpet_sprite_draw() 的实现和本表的颜色/形状字段。
//   * 机制沿用经典暴龙机:蛋 -> 幼年期I -> 幼年期II -> 成长期 -> 成熟期 -> 完全体 -> 究极体,
//     进化条件由年龄、训练次数、对战战绩、照顾失误和体重共同决定。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define VPET_SPECIES_COUNT 31U
#define VPET_MAX_EVOLUTIONS 4U
#define VPET_MAX_ATTACKS 3U

typedef enum {
    VPET_STAGE_EGG = 0,
    VPET_STAGE_BABY1,
    VPET_STAGE_BABY2,
    VPET_STAGE_CHILD,
    VPET_STAGE_ADULT,
    VPET_STAGE_PERFECT,
    VPET_STAGE_ULTIMATE,
    VPET_STAGE_COUNT,
} vpet_stage_t;

// 进化条件:全部满足才走这条分支;作为兜底分支时把 req 字段留 0。
typedef struct {
    uint16_t min_age_min;    // 达到该年龄(宠物体内分钟)才可能进化
    uint16_t min_trainings;  // 最少训练次数
    uint8_t  min_wins;       // 最少胜场
    uint8_t  min_battles;    // 最少对战次数
    uint8_t  max_mistakes;   // 照顾失误上限(养得越好越容易走强力线)
    uint8_t  min_mistakes;   // 照顾失误下限(养歪了才会走的失败线)
    uint16_t min_weight_g;   // 体重下限(部分分支要求"养得胖")
} vpet_evo_req_t;

typedef struct {
    uint8_t species;         // 目标种类 id
    uint8_t is_fallback;     // 1=前几条都不满足时走这条
    vpet_evo_req_t req;
} vpet_evolution_t;

typedef struct {
    const char *name;        // 中文名(原创)
    const char *key;         // 英文标识,用于存档与调试
    vpet_stage_t stage;
    uint8_t hp;              // 体力上限
    uint8_t power;           // 攻击
    uint8_t guard;           // 防御
    const char *attacks[VPET_MAX_ATTACKS];
    uint8_t attack_power[VPET_MAX_ATTACKS];   // 1..5
    uint32_t body_color;     // 主色(0xRRGGBB)
    uint32_t accent_color;   // 点缀色
    uint8_t shape;           // 绘制样式 id,见 vpet_sprite.h
    uint8_t evo_count;
    vpet_evolution_t evolutions[VPET_MAX_EVOLUTIONS];
} vpet_species_t;

const vpet_species_t *vpet_species(uint8_t id);
uint8_t vpet_species_count(void);
const char *vpet_stage_name(vpet_stage_t stage);

// 按进化条件挑选下一个种类;返回 0xFF 表示还不能进化。
uint8_t vpet_pick_evolution(uint8_t species_id, uint16_t age_min,
                            uint16_t trainings, uint8_t wins, uint8_t battles,
                            uint16_t care_mistakes, uint16_t weight_g,
                            uint32_t (*rand_fn)(void *ctx), void *rand_ctx);

// 该种类进化到下一形态所需的最短年龄(用于 UI 提示);无进化返回 0。
uint16_t vpet_next_evolution_age(uint8_t species_id);
