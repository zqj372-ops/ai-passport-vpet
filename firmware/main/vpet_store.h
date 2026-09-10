// vpet_store.h —— 存档:NVS 双槽 + CRC,掉电不丢档
#pragma once

#include <stdbool.h>

#include "vpet_model.h"

bool vpet_store_init(void);
bool vpet_store_load(vpet_model_t *model);
void vpet_store_save_async(const vpet_model_t *model);
