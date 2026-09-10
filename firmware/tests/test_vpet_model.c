// 主机端单元测试:养成模型 + 对战引擎(不依赖 ESP-IDF,直接 cc 编译)
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "vpet_battle.h"
#include "vpet_model.h"
#include "vpet_species.h"

static void test_hatch_and_stage(void)
{
    vpet_model_t model;
    vpet_model_init(&model, 12345);
    assert(model.species == 0);
    assert(model.egg == 1);

    uint32_t events = vpet_model_tick(&model, VPET_EGG_HATCH_MIN + 1);
    assert((events & VPET_EVENT_HATCH) != 0);
    assert(model.egg == 0);
    const vpet_species_t *sp = vpet_model_species(&model);
    assert(sp != NULL);
    assert(sp->stage == VPET_STAGE_BABY1);
    printf("hatch ok: %s\n", sp->name);
}

static void test_feeding_and_hearts(void)
{
    vpet_model_t model;
    vpet_model_init(&model, 777);
    vpet_model_tick(&model, VPET_EGG_HATCH_MIN + 1);

    // 掉一颗心需要 60 分钟;先跑 60 分钟
    vpet_model_tick(&model, 60);
    assert(model.hunger == VPET_HEARTS_MAX - 1);
    assert(vpet_model_species(&model)->stage == VPET_STAGE_BABY1);

    assert(vpet_model_feed_food(&model) == VPET_RESULT_DONE);
    assert(model.hunger == VPET_HEARTS_MAX);
    assert(vpet_model_feed_food(&model) == VPET_RESULT_FULL);

    model.strength = 1;
    assert(vpet_model_feed_protein(&model) == VPET_RESULT_DONE);
    assert(model.strength == 2);
}

static void test_care_mistake_and_death(void)
{
    vpet_model_t model;
    vpet_model_init(&model, 99);
    vpet_model_tick(&model, VPET_EGG_HATCH_MIN + 1);
    // 故意让它饿着:300 分钟里会掉光 4 颗心,之后每 30 分钟记一次失误
    uint32_t events = vpet_model_tick(&model, 600);
    assert((events & VPET_EVENT_HUNGRY) != 0);
    assert(model.care_mistakes > 0);

    // 继续放着不管直到死亡
    events = vpet_model_tick(&model, 3000);
    assert(model.alive == 0);
    assert((events & VPET_EVENT_DEATH) != 0);
    printf("death ok: mistakes=%u age=%u min\n", model.care_mistakes, model.age_min);
}

static void test_poop_and_sickness(void)
{
    vpet_model_t model;
    vpet_model_init(&model, 4242);
    vpet_model_tick(&model, VPET_EGG_HATCH_MIN + 1);
    // 保持吃饱,只看便便逻辑
    for (int i = 0; i < 600; i++) {
        model.hunger = VPET_HEARTS_MAX;
        model.strength = VPET_HEARTS_MAX;
        vpet_model_tick(&model, 1);
    }
    assert(model.poop >= 1);
    uint8_t cleaned = vpet_model_clean(&model);
    assert(cleaned >= 1);
    assert(model.poop == 0);
}

static void test_training(void)
{
    vpet_model_t model;
    vpet_model_init(&model, 5);
    vpet_model_tick(&model, VPET_EGG_HATCH_MIN + 1);
    uint16_t before = model.weight_g;
    for (int i = 0; i < 10; i++) {
        model.strength = 2;
        vpet_model_train(&model, 4);
    }
    assert(model.trainings == 10);
    assert(model.weight_g <= before);
    printf("training ok: weight=%u g, trainings=%u\n", model.weight_g, model.trainings);
}

static void test_evolution_chain(void)
{
    vpet_model_t model;
    vpet_model_init(&model, 2024);
    vpet_model_tick(&model, VPET_EGG_HATCH_MIN + 1);
    // 好好养 900 分钟,应该至少进化到幼年期II
    uint8_t last_stage = vpet_model_species(&model)->stage;
    uint32_t evolves = 0;
    for (int i = 0; i < 900; i++) {
        model.hunger = VPET_HEARTS_MAX;
        model.strength = VPET_HEARTS_MAX;
        uint32_t events = vpet_model_tick(&model, 1);
        if ((events & VPET_EVENT_EVOLVE) != 0) {
            evolves++;
            last_stage = vpet_model_species(&model)->stage;
            printf("evolve #%u -> %s (%s)\n", evolves,
                   vpet_model_species(&model)->name,
                   vpet_stage_name(last_stage));
        }
        if ((events & VPET_EVENT_DEATH) != 0) break;
    }
    assert(evolves >= 1);
    assert(last_stage >= VPET_STAGE_BABY2);
}

static void test_battle_determinism(void)
{
    vpet_model_t model;
    vpet_model_init(&model, 31337);
    vpet_model_tick(&model, VPET_EGG_HATCH_MIN + 120);
    model.species = 5;   // 火花龙
    model.age_min = 200;

    vpet_fighter_t left, right;
    vpet_fighter_from_model(&model, 3, &left);
    vpet_fighter_cpu(&model, 555, &right);

    vpet_battle_t a, b;
    vpet_battle_start(&a, &left, &right, 12345);
    vpet_battle_start(&b, &left, &right, 12345);
    vpet_battle_run(&a);
    vpet_battle_run(&b);
    assert(a.finished && b.finished);
    assert(a.winner == b.winner);
    assert(a.hp_left == b.hp_left && a.hp_right == b.hp_right);
    assert(a.log_count == b.log_count);
    printf("battle ok: winner=%u hp %u:%u rounds=%u\n", a.winner, a.hp_left, a.hp_right,
           a.round);

    // 不同种子应当能打出不同过程(概率上)
    vpet_battle_t c;
    vpet_battle_start(&c, &left, &right, 999);
    vpet_battle_run(&c);
    assert(c.finished);
}

static void test_species_table(void)
{
    assert(vpet_species_count() == VPET_SPECIES_COUNT);
    for (uint8_t i = 0; i < vpet_species_count(); i++) {
        const vpet_species_t *sp = vpet_species(i);
        assert(sp != NULL);
        assert(sp->name != NULL && sp->key != NULL);
        for (uint8_t e = 0; e < sp->evo_count; e++) {
            assert(sp->evolutions[e].species < VPET_SPECIES_COUNT);
        }
    }
    // 每一阶段都要有可选目标,保证进化树完整
    uint8_t stage_targets[VPET_STAGE_COUNT] = { 0 };
    for (uint8_t i = 0; i < vpet_species_count(); i++) {
        const vpet_species_t *sp = vpet_species(i);
        for (uint8_t e = 0; e < sp->evo_count; e++) {
            stage_targets[vpet_species(sp->evolutions[e].species)->stage]++;
        }
    }
    for (int s = 1; s < VPET_STAGE_COUNT; s++) {
        assert(stage_targets[s] > 0);
    }
    printf("species table ok: %u species\n", vpet_species_count());
}

// 进化树的结构检查:只能往上进化、所有分支都可达、失败分支确实存在
static void test_evolution_graph(void)
{
    for (uint8_t i = 0; i < vpet_species_count(); i++) {
        const vpet_species_t *sp = vpet_species(i);
        for (uint8_t e = 0; e < sp->evo_count; e++) {
            const vpet_evolution_t *evo = &sp->evolutions[e];
            if (evo->species == 0) {
                assert(evo->is_fallback == 0);   // 未使用的槽位
                continue;
            }
            const vpet_species_t *target = vpet_species(evo->species);
            assert(target != NULL);
            assert(target->stage > sp->stage);   // 不允许同级或倒退
        }
    }

    // 从蛋出发做一次广度优先:31 种都必须能养出来
    bool seen[VPET_SPECIES_COUNT] = { false };
    uint8_t queue[VPET_SPECIES_COUNT];
    uint8_t head = 0, tail = 0;
    seen[0] = true;
    queue[tail++] = 0;
    while (head < tail) {
        const vpet_species_t *sp = vpet_species(queue[head++]);
        for (uint8_t e = 0; e < sp->evo_count; e++) {
            uint8_t next = sp->evolutions[e].species;
            if (next == 0 || seen[next]) continue;
            seen[next] = true;
            queue[tail++] = next;
        }
    }
    for (uint8_t i = 0; i < vpet_species_count(); i++) {
        if (!seen[i]) printf("unreachable species: %s\n", vpet_species(i)->key);
        assert(seen[i]);
    }
    printf("evolution graph ok: %u species all reachable from egg\n", tail);
}

// 养歪了要能走到"失败分支"(暗影仔 -> 泥浆兽 -> 傀儡兽)
static void test_failure_branch(void)
{
    vpet_model_t model;
    vpet_model_init(&model, 7);
    vpet_model_tick(&model, VPET_EGG_HATCH_MIN + 1);

    model.species = 21;        // 根须兽
    model.stage_min = 360;
    model.care_mistakes = 6;
    model.trainings = 0;
    model.wins = 0;
    model.weight_g = 8;
    uint32_t events = vpet_model_tick(&model, 1);
    assert((events & VPET_EVENT_EVOLVE) != 0);
    assert(model.species == 24);      // 暗影仔(照顾失误换来的)

    model.stage_min = 1440;
    model.wins = 0;
    events = vpet_model_tick(&model, 1);
    assert((events & VPET_EVENT_EVOLVE) != 0);
    assert(model.species == 27);      // 泥浆兽

    model.stage_min = 2880;
    events = vpet_model_tick(&model, 1);
    assert((events & VPET_EVENT_EVOLVE) != 0);
    assert(model.species == 29);      // 傀儡兽(死胡同)
    assert(vpet_species(29)->evo_count == 0);
    printf("failure branch ok: shade -> sludge -> puppet\n");
}
// 双机对战的关键性质:两台设备各自把自己放左边,算出来的必须是同一场战斗
static void test_battle_symmetry(void)
{
    vpet_model_t model;
    vpet_model_init(&model, 4242);
    vpet_model_tick(&model, VPET_EGG_HATCH_MIN + 200);
    model.species = 9;    // 烈焰龙
    model.strength = 3;

    for (uint32_t seed = 1; seed <= 200; seed++) {
        vpet_fighter_t a, b;
        vpet_fighter_from_model(&model, (uint8_t)(seed % 5), &a);
        vpet_fighter_cpu(&model, seed * 7U, &b);

        uint32_t shared = vpet_battle_shared_seed(&a, &b, seed);
        vpet_battle_t x, y;
        vpet_battle_start(&x, &a, &b, shared);
        vpet_battle_start(&y, &b, &a, shared);
        vpet_battle_run(&x);
        vpet_battle_run(&y);

        assert(x.finished && y.finished);
        assert(x.hp_left == y.hp_right);    // A 的剩余血量
        assert(x.hp_right == y.hp_left);    // B 的剩余血量
        assert(x.log_count == y.log_count);
        if (x.winner == 0) {
            assert(y.winner == 0);
        } else {
            assert((x.winner == 1 && y.winner == 2) || (x.winner == 2 && y.winner == 1));
        }
    }
    printf("battle symmetry ok (200 seeds)\n");
}

int main(void)
{
    test_species_table();
    test_evolution_graph();
    test_failure_branch();
    test_hatch_and_stage();
    test_feeding_and_hearts();
    test_care_mistake_and_death();
    test_poop_and_sickness();
    test_training();
    test_evolution_chain();
    test_battle_determinism();
    test_battle_symmetry();
    printf("ALL VPET TESTS PASSED\n");
    return 0;
}
