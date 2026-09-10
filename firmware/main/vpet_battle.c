#include "vpet_battle.h"

#include <string.h>

static uint32_t mix32(uint32_t x)
{
    x ^= 0x9E3779B9U;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

static uint32_t battle_rand(uint32_t *state)
{
    *state = mix32(*state);
    return *state;
}

static uint32_t hash_fighter(const vpet_fighter_t *f)
{
    uint32_t hash = 2166136261U;
    const uint8_t bytes[] = {
        f->species, f->hp, f->max_hp, f->power, f->guard, f->attack_count,
        f->attack_power[0], f->attack_power[1], f->attack_power[2], f->charge_band,
    };
    for (size_t i = 0; i < sizeof(bytes); i++) {
        hash = (hash ^ bytes[i]) * 16777619U;
    }
    return hash;
}

uint32_t vpet_battle_stream(uint32_t seed, const vpet_fighter_t *fighter)
{
    return mix32(seed ^ hash_fighter(fighter));
}

uint32_t vpet_battle_shared_seed(const vpet_fighter_t *a, const vpet_fighter_t *b,
                                 uint32_t extra)
{
    // 异或是可交换运算:双方各自计算都会得到同一个种子
    return mix32(hash_fighter(a) ^ mix32(hash_fighter(b)) ^ mix32(extra ^ 0x5BF03635U));
}

static uint8_t clamp_u8(int value, int min, int max)
{
    if (value < min) return (uint8_t)min;
    if (value > max) return (uint8_t)max;
    return (uint8_t)value;
}

void vpet_fighter_from_model(const vpet_model_t *model, uint8_t charge_band,
                             vpet_fighter_t *out)
{
    const vpet_species_t *sp = vpet_model_species(model);
    memset(out, 0, sizeof(*out));
    if (sp == NULL) return;
    out->species = model->species;
    out->hp = (uint8_t)(sp->hp + model->strength);
    out->max_hp = out->hp;
    out->power = sp->power;
    out->guard = (uint8_t)(sp->guard + (model->weight_g > 20 ? 1 : 0));
    out->attack_count = 0;
    for (uint8_t i = 0; i < VPET_MAX_ATTACKS; i++) {
        if (sp->attacks[i] == NULL || sp->attack_power[i] == 0) continue;
        out->attack_power[out->attack_count] = sp->attack_power[i];
        out->attack_count++;
    }
    if (out->attack_count == 0) {
        out->attack_count = 1;
        out->attack_power[0] = 1;
    }
    out->charge_band = charge_band > 4 ? 4 : charge_band;
}

void vpet_fighter_cpu(const vpet_model_t *model, uint32_t seed, vpet_fighter_t *out)
{
    uint32_t state = seed == 0 ? 0x9E3779B9U : seed;
    const vpet_species_t *player = vpet_model_species(model);
    vpet_stage_t want = player == NULL ? VPET_STAGE_CHILD : player->stage;

    // 收集同阶段候选
    uint8_t pool[VPET_SPECIES_COUNT];
    uint8_t count = 0;
    for (uint8_t i = 1; i < vpet_species_count(); i++) {
        const vpet_species_t *sp = vpet_species(i);
        if (sp == NULL) continue;
        if (sp->stage == want && sp->stage != VPET_STAGE_EGG) {
            pool[count++] = i;
        }
    }
    if (count == 0) {
        // 兜底:按玩家强度造一个无名对手
        memset(out, 0, sizeof(*out));
        out->species = model->species;
        out->hp = (uint8_t)((player ? player->hp : 6) + 1);
        out->max_hp = out->hp;
        out->power = (uint8_t)((player ? player->power : 3) + 1);
        out->guard = player ? player->guard : 2;
        out->attack_count = 1;
        out->attack_power[0] = 2;
        out->charge_band = (uint8_t)(battle_rand(&state) % 5U);
        return;
    }
    uint8_t pick = pool[battle_rand(&state) % count];
    const vpet_species_t *sp = vpet_species(pick);
    memset(out, 0, sizeof(*out));
    out->species = pick;
    out->hp = sp->hp;
    out->max_hp = sp->hp;
    out->power = sp->power;
    out->guard = sp->guard;
    out->attack_count = 0;
    for (uint8_t i = 0; i < VPET_MAX_ATTACKS; i++) {
        if (sp->attacks[i] == NULL || sp->attack_power[i] == 0) continue;
        out->attack_power[out->attack_count++] = sp->attack_power[i];
    }
    if (out->attack_count == 0) {
        out->attack_count = 1;
        out->attack_power[0] = 1;
    }
    out->charge_band = (uint8_t)(battle_rand(&state) % 5U);
}

void vpet_battle_start(vpet_battle_t *battle, const vpet_fighter_t *left,
                       const vpet_fighter_t *right, uint32_t seed)
{
    memset(battle, 0, sizeof(*battle));
    battle->left = *left;
    battle->right = *right;
    battle->hp_left = left->hp;
    battle->hp_right = right->hp;
    battle->round = 0;
    battle->winner = 0;
    battle->finished = false;
    battle->seed = seed == 0 ? 0xA5A5A5A5U : seed;
    battle->stream_left = vpet_battle_stream(battle->seed, left);
    battle->stream_right = vpet_battle_stream(battle->seed, right);
    if (battle->hp_left == 0) {
        battle->finished = true;
        battle->winner = 2;
    } else if (battle->hp_right == 0) {
        battle->finished = true;
        battle->winner = 1;
    }
}

static uint8_t compute_damage(const vpet_fighter_t *attacker,
                              const vpet_fighter_t *defender,
                              uint8_t attack_index, uint8_t roll)
{
    uint8_t atk_power = attacker->attack_power[attack_index % attacker->attack_count];
    int raw = atk_power * 2 + attacker->power / 2 + (int)(roll % 3U) - defender->guard / 2;
    if (raw < 1) raw = 1;
    int multiplier = 60 + (int)attacker->charge_band * 10;   // 60% ~ 100%
    int damage = raw * multiplier / 100;
    if (damage < 1) damage = 1;
    return clamp_u8(damage, 1, 9);
}

bool vpet_battle_step(vpet_battle_t *battle)
{
    if (battle->finished) return false;

    battle->round++;

    // 每一方各自从"只属于自己的随机流"里取一次值:攻击招式 + 伤害浮动都用它。
    // 因为随机流只依赖(种子 + 自己的战斗卡),两台设备算出来的过程与结果完全一致,
    // 而且和"谁在左边"无关 —— 同时出手,不存在先手顺序问题。
    uint32_t left_roll = battle_rand(&battle->stream_left);
    uint32_t right_roll = battle_rand(&battle->stream_right);
    uint8_t left_attack = (uint8_t)(left_roll % battle->left.attack_count);
    uint8_t right_attack = (uint8_t)(right_roll % battle->right.attack_count);
    uint8_t left_damage = compute_damage(&battle->left, &battle->right, left_attack,
                                         (uint8_t)(left_roll >> 8));
    uint8_t right_damage = compute_damage(&battle->right, &battle->left, right_attack,
                                          (uint8_t)(right_roll >> 8));

    battle->hp_right = (uint8_t)(battle->hp_right > left_damage ? battle->hp_right - left_damage : 0);
    battle->hp_left = (uint8_t)(battle->hp_left > right_damage ? battle->hp_left - right_damage : 0);

    if (battle->log_count < VPET_BATTLE_LOG_MAX) {
        battle->log[battle->log_count++] = (vpet_battle_log_t){
            .turn = battle->round, .attacker = 1, .attack = left_attack, .damage = left_damage,
        };
    }
    if (battle->log_count < VPET_BATTLE_LOG_MAX) {
        battle->log[battle->log_count++] = (vpet_battle_log_t){
            .turn = battle->round, .attacker = 2, .attack = right_attack, .damage = right_damage,
        };
    }

    if (battle->hp_left == 0 || battle->hp_right == 0) {
        battle->finished = true;
        if (battle->hp_left == 0 && battle->hp_right == 0) {
            battle->winner = 0;              // 同归于尽算平局
        } else {
            battle->winner = battle->hp_left == 0 ? 2 : 1;
        }
    }

    if (!battle->finished && battle->round >= VPET_BATTLE_MAX_ROUNDS) {
        battle->finished = true;
        if (battle->hp_left == battle->hp_right) {
            battle->winner = 0;   // 平局
        } else {
            battle->winner = battle->hp_left > battle->hp_right ? 1 : 2;
        }
    }
    return !battle->finished;
}

void vpet_battle_run(vpet_battle_t *battle)
{
    while (vpet_battle_step(battle)) {
    }
}

const char *vpet_battle_attack_name(const vpet_battle_t *battle, uint8_t attacker,
                                    uint8_t attack_index)
{
    const vpet_species_t *sp = vpet_species(attacker == 1 ? battle->left.species
                                                          : battle->right.species);
    if (sp == NULL) return "?";
    for (uint8_t i = 0, seen = 0; i < VPET_MAX_ATTACKS; i++) {
        if (sp->attacks[i] == NULL || sp->attack_power[i] == 0) continue;
        if (seen == attack_index) return sp->attacks[i];
        seen++;
    }
    return "?";
}
