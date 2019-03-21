#ifndef _FOTA_H_
#define _FOTA_H_

#include <math.h>
#include "board.h"
#include "multicast.h"
#include "internal-eeprom.h"
#include "LoRaMac.h"

#define FOTA_SOH   						  ((uint8_t)0x01)
#define FOTA_STX   						  ((uint8_t)0x02)
#define FOTA_END							  ((uint8_t)0x03)	
#define FOTA_ACK							  ((uint8_t)0x04)
#define FOTA_GET							  ((uint8_t)0x05)
#define FOTA_REP							  ((uint8_t)0x06)		
#define FOTA_RST							  ((uint8_t)0x07)	

#define FOTA_DEFAULT_REIS_PACK  5
#define FOTA_MAX_LOSS_PACK_NUM  500
#define FOTA_TEMP_BUFFER_SIZE   FOTA_MAX_LOSS_PACK_NUM/2*3+10
#define FOTA_REPORT_STATE_TIME  120000 //2min
#define FOTA_RECEIVE_TIMEOUT    600000 //10min

#define FOTA_DOWNLOAD_FLAG	    ((uint32_t)0x86427531)

#define FLASH_BOOT_RUNING_START ((uint32_t)0x08000000) //offset 0K
#define FLASH_BOOT_RUNING_END   ((uint32_t)0x08004000) //offset 16K
#define FLASH_BOOT_BACKUP_START	((uint32_t)0x0801A800) //offset 106K
#define FLASH_BOOT_BACKUP_END		((uint32_t)0x0801E800) //offset 122K
#define USER_BOOT_SIZE          ((uint32_t)0x00004000) //16K

#define FLASH_APP_RUNING_START	((uint32_t)0x08004000) //offset 16K
#define FLASH_APP_RUNING_END		((uint32_t)0x08019800) //offset 102K
#define FLASH_FOTA_BACKUP_START ((uint32_t)0x0801A800) //offset 106K
#define FLASH_FOTA_BACKUP_END   ((uint32_t)0x08030000) //offset 192K
#define FLASH_FOTA_SIZE    			((uint32_t)0x00015800) //86K

typedef enum
{
	FOTA_SUCCESS = 1,
	FOTA_FAILED,
	FOTA_LOSS,
	FOTA_TOO_LOSS,
	FOTA_TIMEOUT,
	FOTA_NO_START,
	FOTA_OUTDO_SIZE,
	FOTA_NO_APPEUI,
	FOTA_NO_VERSION,
	FOTA_PACKET_ERROR,
	FOTA_IDLE
}FOTAStatus;

typedef struct sFOTA
{
	bool FotaFlag;
	bool GetBootVerFlag;
	bool GetAppVerFlag;
	bool UpdateBootFlag;
	FOTAStatus FotaState;
	uint32_t FotaSize;
	uint16_t PackTotal;
	uint8_t PackLen; 
	uint8_t MaxReissuePack;
	uint16_t FrameNum;
	uint16_t TempNum;
	uint16_t LossPackTotal;
	uint16_t FrameSum;
	uint8_t DataLen;
	uint32_t DestAddr;
	uint8_t VersionName[8];
	uint8_t PackPayload[48];
	uint16_t LosePackBuf[FOTA_MAX_LOSS_PACK_NUM];
}FOTA_t;

extern FOTA_t FirmUpgrade;
extern TimerEvent_t SendFOTAStateTimer;
extern TimerEvent_t ReceivePackTimeoutTimer;

void SendFOTAStateEvent(void);
void ReceivePackTimeoutEvent(void);
void FOTA_PacketProcess(uint8_t *pdata, uint8_t size);
void FOTA_GetVersionNum(uint8_t *pdata, uint8_t size);
void FOTA_SystemBootloader(uint8_t *pdata, uint8_t size);
void FOTA_AddTempMulticast(uint8_t *pdata, uint8_t size);

#endif
