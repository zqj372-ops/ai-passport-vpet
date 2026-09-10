// vpet_battle.h —— 对战引擎(纯 C,确定性)
//
// 同一组输入 + 同一个种子 -> 完全相同的战斗过程。
// 这样以后做双机对战时,只要把双方的"战斗卡"(种类/体力/攻击/防御/招式/蓄力档位)和种子
// 交换一次,两台设备就能各自算出同一场战斗,不需要实时同步每一帧。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "vpet_model.h"
#include "vpet_species.h"

#define VPET_BATTLE_LOG_MAX 32U
#define VPET_BATTLE_MAX_ROUNDS 24U

typedef struct {
    uint8_t species;
    uint8_t hp;
    uint8_t max_hp;
    uint8_t power;
    uint8_t guard;
    uint8_t attack_count;
    uint8_t attack_power[VPET_MAX_ATTACKS];
    uint8_t charge_band;      // 蓄力档位 0..4,由训练小游戏决定
} vpet_fighter_t;

typedef struct {
    uint8_t turn;
    uint8_t attacker;         // 1 = 左(己方)  2 = 右(对手)
    uint8_t attack;
    uint8_t damage;
} vpet_battle_log_t;

typedef struct {
    vpet_fighter_t left;
    vpet_fighter_t right;
    uint8_t hp_left;
    uint8_t hp_right;
    uint8_t round;
    uint8_t log_count;
    uint8_t winner;           // 0 = 未结束,1 = 左胜,2 = 右胜
    bool finished;
    uint32_t seed;
    uint32_t stream_left;    // 左方独立的随机流(只由种子和左方战斗卡决定)
    uint32_t stream_right;
    vpet_battle_log_t log[VPET_BATTLE_LOG_MAX];
} vpet_battle_t;

// 从养成模型生成己方战斗卡
void vpet_fighter_from_model(const vpet_model_t *model, uint8_t charge_band,
                             vpet_fighter_t *out);

// 生成 CPU 对手:按玩家当前阶段匹配强度,并带一点随机
void vpet_fighter_cpu(const vpet_model_t *model, uint32_t seed, vpet_fighter_t *out);

void vpet_battle_start(vpet_battle_t *battle, const vpet_fighter_t *left,
                       const vpet_fighter_t *right, uint32_t seed);

// 同一场战斗卡 + 同一个种子,无论双方各自把自己放左边还是右边,结果必须完全一致。
// vpet_battle_stream() 只依赖"种子 + 单张战斗卡",所以两台设备能独立算出同一场战斗。
uint32_t vpet_battle_stream(uint32_t seed, const vpet_fighter_t *fighter);
// 由双方战斗卡推导共享种子(可交换:换左右顺序结果不变)
uint32_t vpet_battle_shared_seed(const vpet_fighter_t *a, const vpet_fighter_t *b,
                                 uint32_t extra);

// 推进一步(一轮里双方各出手一次);返回 true 表示战斗还没结束
bool vpet_battle_step(vpet_battle_t *battle);

// 直接跑完整场(用于"快速对战"与单元测试)
void vpet_battle_run(vpet_battle_t *battle);

const char *vpet_battle_attack_name(const vpet_battle_t *battle, uint8_t attacker,
                                    uint8_t attack_index);
