#ifndef _TEST_H_
#define _TEST_H_

#include <string.h>
#include <math.h>
#include "board.h"

#include "multicast.h"
#include "internal-eeprom.h"
#include "LoRaMac.h"

#define IS_CAP_LETTER(c)    (((c) >= 'A') && ((c) <= 'F'))
#define IS_LC_LETTER(c)     (((c) >= 'a') && ((c) <= 'f'))
#define IS_09(c)            (((c) >= '0') && ((c) <= '9'))
#define ISVALIDHEX(c)       (IS_CAP_LETTER(c) || IS_LC_LETTER(c) || IS_09(c))
#define ISVALIDDEC(c)       IS_09(c)
#define CONVERTDEC(c)       (c - '0')

#define CONVERTHEX_ALPHA(c) (IS_CAP_LETTER(c) ? ((c) - 'A'+10) : ((c) - 'a'+10))
#define CONVERTHEX(c)       (IS_09(c) ? ((c) - '0') : CONVERTHEX_ALPHA(c))

typedef enum 
{
  NetJoin_ON = 0, 
  NetJoin_OFF,
} NetJoinStatus;

extern bool DownlinkAck_flag;
extern char strVersion[50];

int EmptyFunction(const char * format, ... );
void ProductionTestMode(uint8_t *RxBuffer, uint8_t Length);
void GetTxCycleTimeLagFromEEPROM(void);
void EraseTxCycleTime(void);
void RelayTestCount(void);
void DownlinkReceiveEvent(void);
uint32_t CalculateRandomVal(uint8_t i);

#endif
