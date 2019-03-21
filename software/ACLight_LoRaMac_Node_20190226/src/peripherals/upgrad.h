#ifndef _UPGRAD_H_
#define _UPGRAD_H_

#include <string.h>
#include <math.h>
#include "board.h"
#include "internal-eeprom.h"

#define RUN_DOWNLOAD_FLAG	      ((uint32_t)0x12345678)
#define RUN_UPLOAD_FLAG		      ((uint32_t)0x87654321)
#define IAP_FLASH_MARKADDESS    ((uint32_t)0x08003F00) //offset 15K
#define RELAY_COUNT_FLASH       ((uint32_t)0x08003F08)

extern bool AutoTestMode_flag;
extern bool ATcommend_flag;
extern char strVersion[50];

void GetSoftwareVersion(void);
void UpgradeProgram(uint8_t *RxBuffer, uint8_t Length);
void SetDataToFalsh(uint32_t address, uint32_t data);
uint32_t GetWordDataFromFlash(uint32_t address);
void System_SoftReset(uint8_t *pdata, uint8_t size);
void Check_MulticastAddr(uint8_t *pdata, uint8_t size);

#endif
