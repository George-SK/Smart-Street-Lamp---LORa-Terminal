#ifndef _MMP510_H_
#define _MMP510_H_

#include "stdint.h"

#define MAX_BUF_LEN	50

typedef enum
{
	INIT_STATUS,
	PARK_ON,
	PARK_OFF,
	PARK_CHANGE_ON,
	PARK_CHANGE_OFF,
}ParkStatus;

typedef struct sMMP510
{
	ParkStatus parkStatus;
	uint8_t MMP510RxBuffer[MAX_BUF_LEN];
	uint8_t RxLen;
	uint8_t NeedUplink;
	uint8_t CmdFeedback;
}MMP510_t;

extern MMP510_t MMP510Info;

void Mmp510Init(void);
void MMP510MDataHandler(void);
int SendCmdToMMP510M(const uint8_t * cmdBuf,uint16_t bufLen);
void McuBusySet(uint8_t status);
uint8_t GetParkStatus(void);

#endif
