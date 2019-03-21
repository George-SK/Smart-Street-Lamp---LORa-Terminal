#ifndef _PROFILE_H_
#define _PROFILE_H_

#include "stdint.h"

#define NO_PROFILE			0
#define PROFILE1_TYPE		1
#define PROFILE2_TYPE		2

typedef struct ProfileTime{
	uint32_t time;
	uint8_t pwm;	
}ProfileTime_t;

typedef struct TimeList{
	ProfileTime_t profileTime[50];
	uint8_t index;
	uint8_t size;
	uint16_t id;
}TimeList_t;

typedef struct SyncTime_s{
	uint32_t LastSyncServerTime;
	uint32_t LastSyncRtcTime;
}SyncTime_t;

void ProfileInit(void);
void TimeSyncFunc(uint32_t timeValue);
void AddProfile1(uint8_t *data, uint8_t len, uint8_t saveFalg);
void AddProfile2(uint8_t *data, uint8_t len, uint8_t saveFalg);
uint8_t LoadProfileFromFlash(void);
uint8_t CheckProfileTypeInFlash(void);
uint32_t GetCurrentTime(void);
uint16_t GetProfileId(void);
void RemoveProfile(void);
void ExitProfile(void);
void SaveProfileToFlash(uint8_t ProfileType, uint8_t *data, uint8_t len);

uint8_t GetProfileFlash(void);  //George@20180716:get profile from flash

#endif
