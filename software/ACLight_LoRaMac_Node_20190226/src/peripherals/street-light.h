#ifndef _STREET_LIGHT_H_
#define _STREET_LIGHT_H_

#include "opt3001.h"

#define EUD_320S670DT	0	 //Light driver type.

#define AUTO_DIMMING_MODE		    0
#define MANUAL_DIMMING_MODE	    1
#define PROFILE_DIMMING_MODE	  2

#define ABNORMAL_POWER_LEVEL	  5
#define ABNORMAL_PWM_LEVEL	    5

typedef struct StreetLight_s {
	float lux;
	uint8_t pwmDuty;
	float currentDuty;
	uint8_t ctrlMode;
	uint8_t RecvFlag;
	uint8_t RecvData[10];
	uint8_t AbnormalState;
	uint8_t TimeSync;
	uint8_t TimeSyncRecv;
	uint8_t PreLuxLevel;
	float	PreLuxValue;
	float CurrentLuxBuf[3];
	
	float current;
	float voltage;
	float power;
	float kwh;
	uint8_t ManualCtrlFlag;
	uint8_t TxCycleCount;
	uint32_t TxCycleTime;
	uint8_t TxCycleTimeLag[4];
	uint8_t ADRJustTxCycleTime;
	uint8_t FailureCounter;
	uint8_t MsgTypeCount;
	uint8_t JoinState;
	uint8_t ReportFlag;
	uint16_t FeedDogCount;
}StreetLight_s;

typedef enum
{
		DAC_dimming_mode = 0,
		PWM_dimming_mode,
		DALI_dimming_mode
}DimmingMode;

extern DimmingMode LightDimmingMode;

extern StreetLight_s StreetLightParam;

void StreetLightInit(void);
void StreetLightFsm(void);
void PowerSimulation(uint8_t pwm, float *current, float *voltage, float *power);
void SetRelay(uint8_t state);
void SetNetStateLed(uint8_t state);
void SetBrightness(uint8_t duty);
void EnterDimmingMode(uint8_t mode, uint8_t NeedToUpload);
void DACInit(void);
void AutoDimming2(float lux);
uint8_t GetBrightness(void);
void SetDataToEEPROM(uint32_t addr, uint8_t arg1, uint8_t arg2);
bool GetDataFromEEPROM(uint32_t addr, uint8_t *arg1, uint8_t *arg2);
void SaveCtrModeAndBrightness(uint8_t *buffer);

#endif

