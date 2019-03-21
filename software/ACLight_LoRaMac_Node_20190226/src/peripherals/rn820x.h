#ifndef _RN802X_H_
#define _RN802X_H_

#include "stdint.h"
#include "stdbool.h"

typedef struct sRN820x
{
	uint16_t current;
	float voltage;
	float power;
	float powerFactor;
	float kWh;
	float kWhInFlash;

	uint8_t RxData[20];
	uint8_t RxLen;
	
}RN820x_t;

typedef struct CelibratePro
{
	uint8_t StartChar1;
	uint8_t Addr[6];
	uint8_t StartChar2;
	uint8_t CtrlCode;
	uint8_t DataLen;
	uint8_t	CheckSum;
	uint8_t EndChar;

	uint8_t FlagCode[4];
	uint8_t Password[8];
	
	float Ku;
	float Kia;
	float Kib;
	float Kpp;
	uint16_t Ec;
	
	void (*UartSend)(uint8_t *data, uint16_t len, uint32_t Timeout);
	void (*UartRecv)(uint8_t *data, uint8_t len);
	
}CelibratePro_t;

typedef enum
{
	DATA_ERR,
	DATA_INVALID,
	HOST_READ,
	HOST_WRITE,
	ACK_NORMAL,
	ACK_ABNORMAL
}CmdType;

extern RN820x_t RN820xInfo;

void Rn820xTest(void);
bool CheckData(uint8_t cmd, uint8_t *data, uint8_t len);
void Rn820xInit(void);
bool GetRn820xParam(RN820x_t *RN820xInfo);
void SaveRn820xParam(RN820x_t *RN820xInfo);
void GetRn820xParamFromFlash(RN820x_t *RN820xInfo);

CmdType ParseHostData(uint8_t *data, uint8_t DataLen);
uint8_t SlaveUpload(int type);
void CelibrateInit(void);
void CelibrateHandler(uint8_t *data, uint8_t len);
void GetCelibrateFactorFromFlash(void);
void Add33Hex(uint8_t *data, uint8_t len);
void Reduce33Hex(uint8_t *data, uint8_t len);
void ClearKwhInFlash(void);

#endif
