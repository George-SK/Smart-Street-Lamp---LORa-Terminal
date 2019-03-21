#ifndef _OPT3001_H_
#define _OPT3001_H_

#include "stdbool.h"
#include "stdint.h"

float ReadOpt3001Value(void);
void EnableOpt3001(bool enable);
void Opt3001Init(void);
uint8_t SetHighLowLimit(float lux, uint8_t LowOrHigh);
uint16_t ReadOpt3001Reg(uint16_t reg);

#endif

