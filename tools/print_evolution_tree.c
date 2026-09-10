// 打印进化树(改完怪兽表后用来目视检查条件是否合理)
//
//   cc -std=c11 -Ifirmware-baseline/main tools/print_evolution_tree.c
//      firmware-baseline/main/vpet_species.c -o /tmp/vpet_tree
//   /tmp/vpet_tree
#include <stdio.h>

#include "vpet_species.h"

static void print_req(const vpet_evolution_t *evo)
{
    const vpet_species_t *target = vpet_species(evo->species);
    printf("%s%s(", target->name, evo->is_fallback ? "*" : "");
    printf("%u分", evo->req.min_age_min);
    if (evo->req.min_trainings) printf(" %u训", evo->req.min_trainings);
    if (evo->req.min_wins) printf(" %u胜", evo->req.min_wins);
    if (evo->req.min_battles) printf(" %u战", evo->req.min_battles);
    if (evo->req.min_weight_g) printf(" %ug", evo->req.min_weight_g);
    if (evo->req.max_mistakes < 99) printf(" 失误≤%u", evo->req.max_mistakes);
    if (evo->req.min_mistakes) printf(" 失误≥%u", evo->req.min_mistakes);
    printf(") ");
}

int main(void)
{
    for (vpet_stage_t stage = VPET_STAGE_EGG; stage < VPET_STAGE_COUNT; stage++) {
        printf("[%s]\n", vpet_stage_name(stage));
        for (uint8_t i = 0; i < vpet_species_count(); i++) {
            const vpet_species_t *sp = vpet_species(i);
            if (sp == NULL || sp->stage != stage || sp->evo_count == 0) continue;
            printf("  %-7s -> ", sp->name);
            for (uint8_t e = 0; e < sp->evo_count; e++) {
                if (sp->evolutions[e].species == 0) continue;
                print_req(&sp->evolutions[e]);
            }
            printf("\n");
        }
    }
    printf("\n* = 兜底分支(其他条件都不满足时走这条)\n");
    printf("时间单位:宠物分钟(1 宠物日 = 1440);默认 1 真实分钟 = 10 宠物分钟\n");
    return 0;
}
