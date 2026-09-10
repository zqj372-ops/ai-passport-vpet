#include "vpet_model.h"

#include <string.h>

enum {
    HUNGER_PERIOD_AWAKE = 60,     // 每 60 宠物分钟掉 1 颗饥饿心
    HUNGER_PERIOD_ASLEEP = 120,
    STRENGTH_PERIOD_AWAKE = 90,
    STRENGTH_PERIOD_ASLEEP = 180,
    POOP_PERIOD = 100,            // 每 100 分钟产生一坨便便
    STARVE_MISTAKE_PERIOD = 30,   // 饥饿心归零后每 30 分钟记一次照顾失误
    WEAK_MISTAKE_PERIOD = 45,
    FULL_POOP_SICK_PERIOD = 60,   // 便便满格持续 60 分钟 -> 生病
    LIGHT_MISTAKE_PERIOD = 60,    // 睡觉时没关灯每 60 分钟记一次照顾失误
    SICK_DEATH_PERIOD = 240,      // 生病 240 分钟没好转 -> 死亡
    HEAL_PERIOD = 60,             // 满足条件 60 分钟自愈
};

static uint16_t weight_base_for(uint8_t species)
{
    const vpet_species_t *sp = vpet_species(species);
    return sp == NULL ? 5 : (uint16_t)(sp->hp * 2);
}

void vpet_model_init(vpet_model_t *model, uint32_t seed)
{
    memset(model, 0, sizeof(*model));
    model->version = VPET_MODEL_VERSION;
    model->species = 0;          // 蛋
    model->alive = 1;
    model->egg = 1;
    model->hunger = VPET_HEARTS_MAX;
    model->strength = VPET_HEARTS_MAX;
    model->weight_g = 5;
    model->seed = seed == 0 ? 0x12345678U : seed;
    model->clock_offset = VPET_START_MINUTE_OF_DAY;
    model->last_hour = 0xFF;
}

uint32_t vpet_model_rand(vpet_model_t *model)
{
    // xorshift32:确定性、可存档,便于双机对战复现同一场战斗
    uint32_t x = model->seed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    model->seed = x;
    return x;
}

const vpet_species_t *vpet_model_species(const vpet_model_t *model)
{
    return vpet_species(model->species);
}

uint16_t vpet_model_clock_minutes(const vpet_model_t *model)
{
    return (uint16_t)((model->age_min + model->clock_offset) % VPET_DAY_MINUTES);
}

static bool in_sleep_window(uint16_t clock)
{
    return clock >= VPET_SLEEP_START_MIN || clock < VPET_SLEEP_END_MIN;
}

uint8_t vpet_model_attack_multiplier_band(const vpet_model_t *model)
{
    // 力量心越多,伤害倍率越高(0..4 对应 3..7 成)
    uint8_t hearts = model->strength;
    if (hearts > VPET_HEARTS_MAX) hearts = VPET_HEARTS_MAX;
    return hearts;
}

bool vpet_model_is_final_stage(const vpet_model_t *model)
{
    const vpet_species_t *sp = vpet_model_species(model);
    return sp != NULL && sp->stage == VPET_STAGE_ULTIMATE;
}

uint16_t vpet_model_lifespan_days(const vpet_model_t *model)
{
    const vpet_species_t *sp = vpet_model_species(model);
    if (sp == NULL) return 1;
    switch (sp->stage) {
    case VPET_STAGE_BABY1:    return 2;
    case VPET_STAGE_BABY2:    return 2;
    case VPET_STAGE_CHILD:    return 3;
    case VPET_STAGE_ADULT:    return 5;
    case VPET_STAGE_PERFECT:  return 7;
    case VPET_STAGE_ULTIMATE: return 10;
    default:                  return 1;
    }
}

uint8_t vpet_model_evolution_progress(const vpet_model_t *model)
{
    uint16_t target = vpet_next_evolution_age(model->species);
    if (target == 0) return 100;
    if (model->stage_min >= target) return 100;
    return (uint8_t)((uint32_t)model->stage_min * 100U / target);
}

static void add_care_mistake(vpet_model_t *model, uint32_t *events)
{
    model->care_mistakes++;
    *events |= VPET_EVENT_CARE_MISTAKE;
    if (model->care_mistakes >= VPET_CARE_MISTAKE_LIMIT && model->alive) {
        vpet_model_kill(model);
        *events |= VPET_EVENT_DEATH;
    }
}

void vpet_model_kill(vpet_model_t *model)
{
    model->alive = 0;
    model->egg = 0;
    model->sleeping = 0;
}

uint32_t vpet_model_tick(vpet_model_t *model, uint16_t pet_minutes)
{
    uint32_t events = VPET_EVENT_NONE;
    if (model == NULL || pet_minutes == 0 || !model->alive) return events;

    for (uint16_t i = 0; i < pet_minutes; i++) {
        model->age_min++;
        model->stage_min++;
        uint16_t clock = vpet_model_clock_minutes(model);

        if (model->egg) {
            if (model->age_min >= VPET_EGG_HATCH_MIN) {
                model->egg = 0;
                model->species = (vpet_model_rand(model) & 1U) ? 1U : 2U;
                model->age_min = 0;
                model->stage_min = 0;
                model->weight_g = weight_base_for(model->species);
                model->hunger = VPET_HEARTS_MAX;
                model->strength = VPET_HEARTS_MAX;
                events |= VPET_EVENT_HATCH;
            }
            continue;
        }

        // 睡眠窗口切换
        bool should_sleep = in_sleep_window(clock);
        if (should_sleep && !model->sleeping) {
            model->sleeping = 1;
            events |= VPET_EVENT_SLEEP;
        } else if (!should_sleep && model->sleeping) {
            model->sleeping = 0;
            events |= VPET_EVENT_WAKE;
        }

        // 吃饭/力量心衰减
        model->hunger_timer++;
        uint16_t hunger_period = model->sleeping ? HUNGER_PERIOD_ASLEEP : HUNGER_PERIOD_AWAKE;
        if (model->hunger_timer >= hunger_period) {
            model->hunger_timer = 0;
            if (model->hunger > 0) {
                model->hunger--;
                events |= VPET_EVENT_HUNGER_DOWN;
                if (model->hunger == 0) events |= VPET_EVENT_HUNGRY;
            }
        }

        model->strength_timer++;
        uint16_t strength_period = model->sleeping ? STRENGTH_PERIOD_ASLEEP : STRENGTH_PERIOD_AWAKE;
        if (model->strength_timer >= strength_period) {
            model->strength_timer = 0;
            if (model->strength > 0) {
                model->strength--;
                events |= VPET_EVENT_STRENGTH_DOWN;
                if (model->strength == 0) events |= VPET_EVENT_WEAK;
            }
        }

        // 便便(睡觉时不产生)
        if (!model->sleeping) {
            model->poop_timer++;
            if (model->poop_timer >= POOP_PERIOD) {
                model->poop_timer = 0;
                if (model->poop < VPET_POOP_MAX) {
                    model->poop++;
                    events |= VPET_EVENT_POOP;
                }
            }
        }

        // 便便满格 -> 生病
        if (model->poop >= VPET_POOP_MAX) {
            model->poop_full_timer++;
            if (model->poop_full_timer >= FULL_POOP_SICK_PERIOD) {
                model->poop_full_timer = 0;
                if (!model->sick) {
                    model->sick = 1;
                    model->sick_timer = 0;
                    events |= VPET_EVENT_SICK;
                }
            }
        } else {
            model->poop_full_timer = 0;
        }

        // 生病:满足"干净 + 吃饱 + 有力量"持续一段时间后自愈,否则会死
        if (model->sick) {
            model->sick_timer++;
            bool good_care = model->poop == 0 && model->hunger >= 2 && model->strength >= 1;
            if (good_care) {
                model->heal_timer++;
                if (model->heal_timer >= HEAL_PERIOD) {
                    model->sick = 0;
                    model->heal_timer = 0;
                    model->sick_timer = 0;
                    events |= VPET_EVENT_RECOVERED;
                }
            } else {
                model->heal_timer = 0;
            }
            if (model->sick_timer >= SICK_DEATH_PERIOD) {
                vpet_model_kill(model);
                events |= VPET_EVENT_DEATH;
                return events;
            }
        } else {
            model->sick_timer = 0;
            model->heal_timer = 0;
        }

        // 饿/没力量不管 -> 照顾失误
        if (model->hunger == 0) {
            model->starve_timer++;
            if (model->starve_timer >= STARVE_MISTAKE_PERIOD) {
                model->starve_timer = 0;
                add_care_mistake(model, &events);
                if (!model->alive) return events;
            }
        } else {
            model->starve_timer = 0;
        }

        if (model->strength == 0) {
            model->weak_timer++;
            if (model->weak_timer >= WEAK_MISTAKE_PERIOD) {
                model->weak_timer = 0;
                add_care_mistake(model, &events);
                if (!model->alive) return events;
            }
        } else {
            model->weak_timer = 0;
        }

        // 睡觉不关灯 -> 照顾失误
        if (model->sleeping && !model->lights_off) {
            model->light_timer++;
            if (model->light_timer >= LIGHT_MISTAKE_PERIOD) {
                model->light_timer = 0;
                add_care_mistake(model, &events);
                if (!model->alive) return events;
            }
        } else {
            model->light_timer = 0;
        }

        // 自然寿命
        uint32_t lifespan_min = (uint32_t)vpet_model_lifespan_days(model) * VPET_DAY_MINUTES;
        if (model->age_min >= lifespan_min) {
            vpet_model_kill(model);
            events |= VPET_EVENT_DEATH;
            return events;
        }

        // 进化判定
        uint8_t next = vpet_pick_evolution(model->species, model->stage_min,
                                           model->trainings, (uint8_t)model->wins,
                                           (uint8_t)model->battles, model->care_mistakes,
                                           model->weight_g,
                                           (uint32_t (*)(void *))vpet_model_rand, model);
        if (next != 0xFF) {
            model->species = next;
            model->stage_min = 0;
            model->hunger_timer = 0;
            model->strength_timer = 0;
            model->poop_timer = 0;
            events |= VPET_EVENT_EVOLVE;
        }
    }
    return events;
}

vpet_result_t vpet_model_feed_food(vpet_model_t *model)
{
    if (!model->alive || model->egg) return VPET_RESULT_BLOCKED;
    if (model->hunger >= VPET_HEARTS_MAX) return VPET_RESULT_FULL;
    model->hunger++;
    model->weight_g = (uint16_t)(model->weight_g + 1);
    model->starve_timer = 0;
    return VPET_RESULT_DONE;
}

vpet_result_t vpet_model_feed_protein(vpet_model_t *model)
{
    if (!model->alive || model->egg) return VPET_RESULT_BLOCKED;
    if (model->strength >= VPET_HEARTS_MAX) return VPET_RESULT_FULL;
    model->strength++;
    model->weight_g = (uint16_t)(model->weight_g + 2);
    model->weak_timer = 0;
    return VPET_RESULT_DONE;
}

uint8_t vpet_model_clean(vpet_model_t *model)
{
    if (!model->alive || model->egg) return 0;
    uint8_t cleaned = model->poop;
    model->poop = 0;
    model->poop_timer = 0;
    model->poop_full_timer = 0;
    return cleaned;
}

vpet_result_t vpet_model_toggle_light(vpet_model_t *model)
{
    if (!model->alive || model->egg) return VPET_RESULT_BLOCKED;
    model->lights_off = model->lights_off ? 0 : 1;
    if (model->lights_off) model->light_timer = 0;
    return VPET_RESULT_DONE;
}

vpet_result_t vpet_model_train(vpet_model_t *model, uint8_t score)
{
    if (!model->alive || model->egg) return VPET_RESULT_BLOCKED;
    if (model->sleeping) return VPET_RESULT_BLOCKED;
    if (model->strength == 0) return VPET_RESULT_NO_STRENGTH;
    model->trainings++;
    uint16_t base = weight_base_for(model->species);
    if (model->weight_g > base + 2) {
        model->weight_g = (uint16_t)(model->weight_g - 2);
    } else {
        model->weight_g = base;
    }
    if (score >= 2 && model->strength < VPET_HEARTS_MAX) {
        model->strength++;
    }
    return score >= 2 ? VPET_RESULT_DONE : VPET_RESULT_OK;
}

vpet_result_t vpet_model_battle_start(vpet_model_t *model)
{
    if (!model->alive || model->egg) return VPET_RESULT_BLOCKED;
    if (model->sleeping) return VPET_RESULT_BLOCKED;
    if (model->strength == 0) return VPET_RESULT_NO_STRENGTH;
    model->strength--;
    model->battles++;
    return VPET_RESULT_DONE;
}

void vpet_model_battle_finish(vpet_model_t *model, bool won)
{
    if (won) {
        model->wins++;
    } else if (model->weight_g > weight_base_for(model->species)) {
        model->weight_g = (uint16_t)(model->weight_g - 1);
    }
}
