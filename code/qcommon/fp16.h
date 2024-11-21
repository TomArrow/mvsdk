
#ifndef FP16_H
#define FP16_H
#include "../game/q_shared.h"
#include "../api/mvapi.h"

//static uint32_t fp16_ieee_to_fp32_bits(uint32_t h);
static float fp16_ieee_to_fp32_value(short h);
static short fp16_ieee_from_fp32_value(float f);

#endif
