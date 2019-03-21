#ifndef _RS485_H_
#define _RS485_H_

#include "stdint.h"

void Rs485Init(void);
void SetRxTxMode(uint8_t mode);
void RS485SendData(uint8_t *data, uint8_t len);

#endif
