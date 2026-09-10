#include "vpet_species.h"

#include <stddef.h>

// 绘制样式 id(与 vpet_sprite.c 中的分支一一对应)
enum {
    SHAPE_EGG = 0,
    SHAPE_BLOB,      // 圆团子
    SHAPE_SPARK,     // 火苗/电光
    SHAPE_DOT,       // 圆点带小角
    SHAPE_GEAR,      // 齿轮
    SHAPE_DRAGON,    // 小龙
    SHAPE_BUBBLE,    // 水泡
    SHAPE_SPROUT,    // 叶芽
    SHAPE_SPIKE,     // 尖刺
    SHAPE_WING,      // 带翼
    SHAPE_SHELL,     // 硬壳
    SHAPE_HORN,      // 独角
    SHAPE_CROWN,     // 冠冕
    SHAPE_GHOST,     // 幽影
    SHAPE_SLUDGE,    // 泥浆
    SHAPE_PUPPET,    // 傀儡
    SHAPE_CRYSTAL,   // 晶体
    SHAPE_BIRD,      // 战鸟
    SHAPE_TREE,      // 树灵
    SHAPE_STONE,     // 岩石
    SHAPE_SHADE,     // 暗影
};

// 进化条件速查(经典暴龙机的思路):
//   req = { 最短年龄, 最少训练, 最少胜场, 最少对战, 失误上限, 失误下限, 体重下限 }
//   * 条件全满足才算候选;多个候选时随机挑一个;
//   * 都不满足 → 走兜底分支(is_fallback=1),所以每只都至少有一条稳定出路;
//   * 强力线(完全体/究极体)失误上限卡得很紧;
//   * "养歪线"用失误【下限】表达:泥浆兽(≥8)、暗影仔(≥4)、傀儡兽(≥12)。
static const vpet_species_t SPECIES[VPET_SPECIES_COUNT] = {
    // ---- 0 蛋 --------------------------------------------------------------
    { "蛋", "egg", VPET_STAGE_EGG, 1, 0, 0, { "—", "—", "—" }, { 0, 0, 0 },
      0xF3F0E4, 0xE8C56A, SHAPE_EGG, 4,
      { { 1, 1, { 7, 0, 0, 0, 99, 0, 0 } },
        { 2, 0, { 7, 0, 0, 0, 99, 0, 0 } },
        { 18, 0, { 7, 0, 0, 0, 99, 0, 0 } },
        { 19, 0, { 7, 0, 0, 0, 99, 0, 0 } } } },

    // ---- 幼年期I -----------------------------------------------------------
    { "点点兽", "dot", VPET_STAGE_BABY1, 3, 1, 1, { "撞击", "泡泡", "—" }, { 1, 1, 0 },
      0x8FD3FF, 0x4A9BFF, SHAPE_DOT, 3,
      { { 3, 1, { 120, 0, 0, 0, 99, 0, 0 } },
        { 4, 0, { 120, 1, 0, 0, 3, 0, 8 } },
        { 20, 0, { 120, 0, 0, 0, 1, 0, 0 } } } },

    { "火苗兽", "flick", VPET_STAGE_BABY1, 3, 1, 1, { "撞击", "火花", "—" }, { 1, 1, 0 },
      0xFFC46B, 0xFF7A3C, SHAPE_SPARK, 3,
      { { 3, 1, { 120, 0, 0, 0, 99, 0, 0 } },
        { 4, 0, { 120, 1, 0, 0, 3, 0, 8 } },
        { 20, 0, { 120, 0, 0, 0, 1, 0, 0 } } } },

    // ---- 幼年期II ----------------------------------------------------------
    { "团子兽", "puff", VPET_STAGE_BABY2, 5, 2, 2, { "头撞", "泡泡", "突进" }, { 1, 2, 2 },
      0xA8E6A1, 0x62C46A, SHAPE_BLOB, 3,
      { { 5, 0, { 360, 2, 0, 1, 2, 0, 12 } },
        { 7, 0, { 360, 4, 0, 1, 1, 0, 10 } },
        { 6, 1, { 480, 0, 0, 0, 99, 0, 0 } } } },

    { "齿轮仔", "cog", VPET_STAGE_BABY2, 5, 2, 3, { "齿轮斩", "头撞", "突进" }, { 2, 1, 2 },
      0xC9CDD6, 0x8A93A6, SHAPE_GEAR, 3,
      { { 8, 0, { 360, 3, 0, 1, 2, 0, 14 } },
        { 22, 0, { 360, 1, 0, 1, 3, 0, 0 } },
        { 5, 1, { 480, 0, 0, 0, 99, 0, 0 } } } },

    // ---- 成长期 ------------------------------------------------------------
    { "火花龙", "spark", VPET_STAGE_CHILD, 8, 4, 3, { "火焰吐息", "利爪", "突进" }, { 3, 2, 2 },
      0xFF9A4D, 0xFFD34F, SHAPE_DRAGON, 4,
      { { 9, 0, { 1080, 8, 2, 4, 3, 0, 18 } },
        { 11, 0, { 1080, 6, 3, 5, 3, 0, 0 } },
        { 27, 0, { 1080, 0, 0, 0, 99, 8, 0 } },
        { 12, 1, { 1440, 0, 0, 0, 99, 0, 0 } } } },

    { "水泡兽", "bubble", VPET_STAGE_CHILD, 9, 3, 4, { "水泡弹", "漩涡", "头撞" }, { 3, 2, 1 },
      0x7FD1F0, 0x3D7BD6, SHAPE_BUBBLE, 2,
      { { 10, 0, { 1080, 6, 1, 3, 3, 0, 16 } },
        { 12, 1, { 1440, 0, 0, 0, 99, 0, 0 } } } },

    { "叶芽兽", "sprout", VPET_STAGE_CHILD, 8, 3, 4, { "藤鞭", "飞叶", "吸取" }, { 3, 2, 2 },
      0x9BE07A, 0x4F9E3A, SHAPE_SPROUT, 3,
      { { 25, 0, { 1080, 10, 1, 3, 2, 0, 17 } },
        { 28, 0, { 1080, 6, 2, 3, 2, 0, 0 } },
        { 12, 1, { 1440, 0, 0, 0, 99, 0, 0 } } } },

    { "铁钉兽", "spike", VPET_STAGE_CHILD, 9, 4, 3, { "尖刺冲撞", "金属爪", "突进" }, { 3, 3, 1 },
      0xB9BEC9, 0x6E7686, SHAPE_SPIKE, 2,
      { { 12, 0, { 1080, 12, 2, 4, 3, 0, 20 } },
        { 9, 1, { 1440, 0, 0, 0, 99, 0, 0 } } } },

    // ---- 成熟期 ------------------------------------------------------------
    { "烈焰龙", "blaze", VPET_STAGE_ADULT, 14, 6, 5, { "烈焰爆风", "龙爪", "火球" }, { 4, 3, 3 },
      0xFF7A3C, 0xFFC46B, SHAPE_DRAGON, 2,
      { { 13, 0, { 2160, 18, 5, 8, 6, 0, 26 } },
        { 16, 1, { 2880, 0, 0, 0, 99, 0, 0 } } } },

    { "冰甲兽", "frost", VPET_STAGE_ADULT, 15, 5, 6, { "冰晶突刺", "霜冻吐息", "重压" }, { 4, 3, 3 },
      0xA8D8FF, 0x5A8FD8, SHAPE_SHELL, 2,
      { { 14, 0, { 2160, 16, 4, 8, 2, 0, 30 } },
        { 13, 1, { 2880, 0, 0, 0, 99, 0, 0 } } } },

    { "雷翼兽", "bolt", VPET_STAGE_ADULT, 13, 7, 4, { "雷击", "电光翼", "俯冲" }, { 5, 4, 3 },
      0xFFE066, 0x8E7BFF, SHAPE_WING, 3,
      { { 15, 0, { 2160, 20, 8, 9, 5, 0, 24 } },
        { 30, 0, { 2160, 20, 15, 16, 4, 0, 0 } },
        { 16, 1, { 2880, 0, 0, 0, 99, 0, 0 } } } },

    { "岩壳兽", "rock", VPET_STAGE_ADULT, 16, 5, 7, { "岩壁粉碎", "落石", "体重压制" }, { 4, 4, 3 },
      0xC2A87A, 0x8A6B45, SHAPE_HORN, 2,
      { { 14, 0, { 2160, 22, 4, 8, 6, 0, 34 } },
        { 15, 1, { 2880, 0, 0, 0, 99, 0, 0 } } } },

    // ---- 完全体 ------------------------------------------------------------
    { "太阳龙", "solar", VPET_STAGE_PERFECT, 19, 8, 7, { "日光裁决", "圣焰", "光刃" }, { 5, 4, 4 },
      0xFFD34F, 0xFFF3B0, SHAPE_CROWN, 1,
      { { 16, 0, { 3600, 25, 10, 14, 5, 0, 0 } } } },

    { "深渊兽", "abyss", VPET_STAGE_PERFECT, 20, 8, 6, { "深渊吞噬", "暗影爪", "诅咒" }, { 5, 4, 4 },
      0x8E7BFF, 0x3B2F6B, SHAPE_GHOST, 1,
      { { 17, 0, { 3600, 18, 12, 16, 6, 0, 0 } } } },

    { "暴风兽", "tempest", VPET_STAGE_PERFECT, 18, 9, 6, { "飓风斩", "音爆", "风刃" }, { 5, 4, 4 },
      0x9FE8D8, 0x4FA3A0, SHAPE_WING, 3,
      { { 17, 0, { 3600, 20, 10, 14, 5, 0, 0 } },
        { 30, 0, { 3600, 22, 15, 18, 5, 0, 0 } },
        { 16, 1, { 3600, 0, 0, 0, 99, 0, 0 } } } },

    // ---- 究极体 ------------------------------------------------------------
    { "创世龙", "genesis", VPET_STAGE_ULTIMATE, 24, 10, 9, { "创世之光", "终末之刃", "天罚" }, { 5, 5, 5 },
      0xFFF3B0, 0xFFB23E, SHAPE_CROWN, 0, { { 0, 0, { 0, 0, 0, 0, 0, 0, 0 } } } },

    { "终焉兽", "omega", VPET_STAGE_ULTIMATE, 25, 11, 8, { "终焉咆哮", "虚空爪", "灭世弹" }, { 5, 5, 5 },
      0x6B4F8E, 0xD14FA0, SHAPE_GHOST, 0, { { 0, 0, { 0, 0, 0, 0, 0, 0, 0 } } } },

    // ---- 新增:幼年期 -------------------------------------------------------
    { "水滴兽", "droplet", VPET_STAGE_BABY1, 3, 1, 1, { "撞击", "水花", "—" }, { 1, 1, 0 },
      0x9BE3F0, 0x4FB6D8, SHAPE_BUBBLE, 3,
      { { 20, 1, { 120, 0, 0, 0, 99, 0, 0 } },
        { 21, 0, { 120, 2, 0, 0, 2, 0, 8 } },
        { 3, 0, { 120, 0, 0, 0, 1, 0, 0 } } } },

    { "叶籽兽", "seed", VPET_STAGE_BABY1, 3, 1, 1, { "撞击", "叶刃", "—" }, { 1, 1, 0 },
      0xC6E8A0, 0x7DBF52, SHAPE_SPROUT, 3,
      { { 21, 1, { 120, 0, 0, 0, 99, 0, 0 } },
        { 20, 0, { 120, 2, 0, 0, 2, 0, 0 } },
        { 3, 0, { 120, 0, 0, 0, 1, 0, 0 } } } },

    { "泡泡仔", "bubblet", VPET_STAGE_BABY2, 5, 2, 2, { "泡泡", "头撞", "突进" }, { 2, 1, 2 },
      0xBEEBF7, 0x62B8E0, SHAPE_BUBBLE, 3,
      { { 6, 0, { 360, 2, 0, 1, 3, 0, 12 } },
        { 22, 0, { 360, 2, 1, 1, 3, 0, 0 } },
        { 7, 1, { 480, 0, 0, 0, 99, 0, 0 } } } },

    { "根须兽", "root", VPET_STAGE_BABY2, 5, 2, 3, { "根鞭", "头撞", "吸取" }, { 2, 1, 2 },
      0xBFCE8A, 0x7A8F4A, SHAPE_TREE, 4,
      { { 7, 0, { 360, 4, 0, 1, 2, 0, 10 } },
        { 23, 0, { 360, 3, 0, 1, 2, 0, 16 } },
        { 24, 0, { 360, 0, 0, 0, 99, 4, 0 } },
        { 6, 1, { 480, 0, 0, 0, 99, 0, 0 } } } },

    // ---- 新增:成长期 -------------------------------------------------------
    { "电光仔", "volt", VPET_STAGE_CHILD, 8, 4, 3, { "电击", "闪光", "突进" }, { 3, 2, 2 },
      0xFFE066, 0xFFB23E, SHAPE_SPARK, 3,
      { { 11, 0, { 1080, 8, 4, 5, 3, 0, 0 } },
        { 28, 0, { 1080, 8, 2, 3, 3, 0, 0 } },
        { 25, 1, { 1440, 0, 0, 0, 99, 0, 0 } } } },

    { "岩砾兽", "pebble", VPET_STAGE_CHILD, 10, 3, 5, { "落石", "头撞", "滚压" }, { 3, 2, 2 },
      0xA9A08C, 0x6F6552, SHAPE_STONE, 2,
      { { 12, 0, { 1080, 10, 2, 4, 3, 0, 22 } },
        { 10, 1, { 1440, 0, 0, 0, 99, 0, 0 } } } },

    { "暗影仔", "shade", VPET_STAGE_CHILD, 8, 4, 3, { "暗影爪", "偷袭", "突进" }, { 3, 2, 2 },
      0x6E628C, 0x3A2F55, SHAPE_SHADE, 2,
      { { 26, 0, { 1080, 6, 1, 3, 99, 0, 0 } },
        { 27, 1, { 1440, 0, 0, 0, 99, 0, 0 } } } },

    // ---- 新增:成熟期 -------------------------------------------------------
    { "木灵兽", "dryad", VPET_STAGE_ADULT, 15, 6, 6, { "木灵鞭", "飞叶风暴", "吸取" }, { 4, 3, 3 },
      0x8FD06A, 0x4C7F35, SHAPE_TREE, 3,
      { { 13, 0, { 2160, 24, 5, 8, 4, 0, 26 } },
        { 29, 0, { 2160, 0, 0, 0, 99, 12, 0 } },
        { 15, 1, { 2880, 0, 0, 0, 99, 0, 0 } } } },

    { "幽影兽", "wraith", VPET_STAGE_ADULT, 13, 7, 4, { "暗影突袭", "恐惧凝视", "诅咒" }, { 5, 4, 3 },
      0x7A5FA8, 0x2E2440, SHAPE_SHADE, 2,
      { { 14, 0, { 2160, 14, 6, 8, 99, 0, 0 } },
        { 29, 1, { 2880, 0, 0, 0, 99, 0, 0 } } } },

    { "泥浆兽", "sludge", VPET_STAGE_ADULT, 12, 4, 5, { "泥浆弹", "恶臭", "重压" }, { 3, 2, 3 },
      0x9A8B5E, 0x6B5E3A, SHAPE_SLUDGE, 1,
      { { 29, 1, { 2880, 0, 0, 0, 99, 0, 0 } } } },

    { "晶石兽", "crystal", VPET_STAGE_ADULT, 16, 6, 8, { "水晶刃", "棱镜光", "重击" }, { 4, 4, 3 },
      0x9FE3E0, 0x4F8FA8, SHAPE_CRYSTAL, 2,
      { { 15, 0, { 2160, 14, 6, 9, 4, 0, 0 } },
        { 13, 1, { 2880, 0, 0, 0, 99, 0, 0 } } } },

    // ---- 新增:完全体 / 究极体 ---------------------------------------------
    { "傀儡兽", "puppet", VPET_STAGE_PERFECT, 16, 6, 6, { "傀儡丝", "诅咒", "重击" }, { 4, 4, 3 },
      0xB5573F, 0x5E2A22, SHAPE_PUPPET, 0, { { 0, 0, { 0, 0, 0, 0, 0, 0, 0 } } } },

    { "战神兽", "ares", VPET_STAGE_ULTIMATE, 25, 12, 9, { "战神斩", "雷霆突刺", "战吼" }, { 5, 5, 4 },
      0xFFC46B, 0xE43B2F, SHAPE_BIRD, 0, { { 0, 0, { 0, 0, 0, 0, 0, 0, 0 } } } },
};

const vpet_species_t *vpet_species(uint8_t id)
{
    if (id >= VPET_SPECIES_COUNT) return NULL;
    return &SPECIES[id];
}

uint8_t vpet_species_count(void)
{
    return VPET_SPECIES_COUNT;
}

const char *vpet_stage_name(vpet_stage_t stage)
{
    switch (stage) {
    case VPET_STAGE_EGG:      return "蛋";
    case VPET_STAGE_BABY1:    return "幼年期I";
    case VPET_STAGE_BABY2:    return "幼年期II";
    case VPET_STAGE_CHILD:    return "成长期";
    case VPET_STAGE_ADULT:    return "成熟期";
    case VPET_STAGE_PERFECT:  return "完全体";
    case VPET_STAGE_ULTIMATE: return "究极体";
    default:                  return "未知";
    }
}

static bool requirements_met(const vpet_evo_req_t *req, uint16_t age_min,
                             uint16_t trainings, uint8_t wins, uint8_t battles,
                             uint16_t care_mistakes, uint16_t weight_g)
{
    if (age_min < req->min_age_min) return false;
    if (trainings < req->min_trainings) return false;
    if (wins < req->min_wins) return false;
    if (battles < req->min_battles) return false;
    if (care_mistakes > req->max_mistakes) return false;
    if (care_mistakes < req->min_mistakes) return false;
    if (weight_g < req->min_weight_g) return false;
    return true;
}

uint8_t vpet_pick_evolution(uint8_t species_id, uint16_t age_min,
                            uint16_t trainings, uint8_t wins, uint8_t battles,
                            uint16_t care_mistakes, uint16_t weight_g,
                            uint32_t (*rand_fn)(void *ctx), void *rand_ctx)
{
    const vpet_species_t *sp = vpet_species(species_id);
    if (sp == NULL || sp->evo_count == 0) return 0xFF;

    // 先挑出所有满足条件的分支;在满足集合里按 rand 选一个(经典暴龙机也是带随机性的)。
    uint8_t candidates[VPET_MAX_EVOLUTIONS];
    uint8_t count = 0;
    for (uint8_t i = 0; i < sp->evo_count && i < VPET_MAX_EVOLUTIONS; i++) {
        const vpet_evolution_t *evo = &sp->evolutions[i];
        if (evo->is_fallback || evo->species == 0) continue;
        if (requirements_met(&evo->req, age_min, trainings, wins, battles,
                             care_mistakes, weight_g)) {
            candidates[count++] = i;
        }
    }
    if (count > 0) {
        uint8_t pick = 0;
        if (rand_fn != NULL && count > 1) {
            pick = (uint8_t)(rand_fn(rand_ctx) % count);
        }
        return sp->evolutions[candidates[pick]].species;
    }

    // 没有满足条件的分支:走兜底(但如果连最短年龄都没到,就还不能进化)
    for (uint8_t i = 0; i < sp->evo_count && i < VPET_MAX_EVOLUTIONS; i++) {
        const vpet_evolution_t *evo = &sp->evolutions[i];
        if (!evo->is_fallback || evo->species == 0) continue;
        if (age_min < evo->req.min_age_min) return 0xFF;
        return evo->species;
    }
    return 0xFF;
}

uint16_t vpet_next_evolution_age(uint8_t species_id)
{
    const vpet_species_t *sp = vpet_species(species_id);
    if (sp == NULL || sp->evo_count == 0) return 0;
    uint16_t best = 0xFFFFU;
    for (uint8_t i = 0; i < sp->evo_count && i < VPET_MAX_EVOLUTIONS; i++) {
        if (sp->evolutions[i].species == 0) continue;
        uint16_t age = sp->evolutions[i].req.min_age_min;
        if (age < best) best = age;
    }
    return best == 0xFFFFU ? 0 : best;
}
