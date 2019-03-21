#include "profile.h"
#include "board.h"
#include "internal-eeprom.h"

#define PROFILE_MAX_SIZE	100

static TimeList_t Profile1TimeList, Profile2TimeList;
static TimerEvent_t Profile1Timer, Profile2Timer;
static SyncTime_t SyncTimeInfo;
static uint8_t Profile1Head[4] = {0xe1, 0xe2, 0xe3, 0xe4};
static uint8_t Profile2Head[4] = {0xd1, 0xd2, 0xd3, 0xd4};

/*!
 * Number of seconds in a minute
 */
static const uint8_t SecondsInMinute = 60;

/*!
 * Number of seconds in an hour
 */
static const uint16_t SecondsInHour = 3600;

/*!
 * Number of seconds in a day
 */
static const uint32_t SecondsInDay = 86400;

/*!
 * \brief RTC Handler
 */
extern RTC_HandleTypeDef RtcHandle;	//Defined in rtc-board.c

static void Profile1TimeListEvent( void );
static void Profile2TimeListEvent( void );
static void Profile1DealFuc(void);
static void Profile2DealFuc(void);
void SetFirstTimerValue(TimerEvent_t *timer, TimeList_t *TimeList, void (*DealFuc)(void));
void ScheduleNextTime(TimerEvent_t *timer, TimeList_t *TimeList, void (*DealFuc)(void));
static uint8_t ReadProfileFromeFlash(uint8_t *data, uint8_t *len);

extern void SetTxDutyCycle(uint32_t time);

void ProfileInit(void)
{
	//TimerInit( &SyncTimer, TimeSyncTimerEvent );	
	TimerInit( &Profile1Timer, Profile1TimeListEvent );
	TimerInit( &Profile2Timer, Profile2TimeListEvent );
	
#if !defined (DS1302_RTC_ENABLE)
	SyncTimeInfo.LastSyncServerTime = 0;	//00:00:00
	SyncTimeInfo.LastSyncRtcTime = RtcGetTimerValue();
#endif
	
	//SetFirstTimerValue(&SyncTimer, &SyncTimeList, NULL);
	//LoadProfileFromFlash();
}

void TimeSyncFunc(uint32_t timeValue)
{
	uint8_t hour = timeValue/3600;
	uint8_t minute = (timeValue-(hour*3600))/60;
	uint8_t second = timeValue%60;
	
	SyncTimeInfo.LastSyncServerTime = timeValue;
	SyncTimeInfo.LastSyncRtcTime = RtcGetTimerValue();
	
	//SetFirstTimerValue(&SyncTimer, &SyncTimeList, NULL);	//Synchronize time again.
	
	Uart1.PrintfLog("RTC time sync [%d:%d:%d]\r\n", hour, minute, second);
	
	StreetLightParam.TimeSync = 1;
	StreetLightParam.TimeSyncRecv = 1;
	
	
	/*if(CheckProfileTypeInFlash() != NO_PROFILE)	//Reload the profile.
	{
		if(LoadProfileFromFlash() == 0)	//No profile.
		{
			Uart1.PrintfLog(" No profile!\r\n");
		}
	}*/
	
}

uint32_t ConvertCalendarTimeToTimerTime(RTC_TimeTypeDef CalendarTime)
{
	uint32_t time;
	
	time = (CalendarTime.Hours*SecondsInHour) + (CalendarTime.Minutes*SecondsInMinute) + CalendarTime.Seconds;
	
	return time;
}

uint32_t GetCurrentTime(void)
{
	uint32_t crrentRtcTime, time;
	
	crrentRtcTime = RtcGetTimerValue();
	
	if(crrentRtcTime >= SyncTimeInfo.LastSyncRtcTime)	//Warning: the time lag twice sync must be less than 49 days and more than 1ms!!! 
	{
		time = ((crrentRtcTime - SyncTimeInfo.LastSyncRtcTime + (SyncTimeInfo.LastSyncServerTime*1000))/1000) % (SecondsInDay);
	}
	else
	{
		time = (((0xFFFFFFFF - SyncTimeInfo.LastSyncRtcTime) + crrentRtcTime + (SyncTimeInfo.LastSyncServerTime*1000))/1000) % (SecondsInDay);
	}
	
	return time;
}

void ScheduleNextTime(TimerEvent_t *timer, TimeList_t *TimeList, void (*DealFuc)(void))
{
	uint32_t time, CurrentTime, NextTime;
	uint8_t hour, minute, second;
	
	if(TimeList->size == 0)
	{
		return;
	}
	
	CurrentTime = GetCurrentTime();
	NextTime = TimeList->profileTime[(TimeList->index+1)%TimeList->size].time;	
	
	if(NextTime > CurrentTime)
	{
		time  = NextTime - CurrentTime;
	}
	else	//Next day.
	{
		time = SecondsInDay - CurrentTime + NextTime;
	}
	
	TimerSetValue( timer, time*1000 );
	TimerStart( timer );
	//Uart1.PrintfLog("Current time:%d, set next:%ds\r\n", CurrentTime, time);
	hour = CurrentTime/3600;
	minute = (CurrentTime-(hour*3600))/60;
	second = CurrentTime%60;
	
	Uart1.PrintfLog("[ScheduleNextTime]Time:%d:%d:%d, next schedule:%d\r\n", hour, minute, second, time);
	
	TimeList->index = (TimeList->index+1)%TimeList->size;
	//Uart1.PrintfLog("TimeList.index:%d\r\n", TimeList->index);
	
	if(DealFuc != NULL)
	{
		DealFuc();
	}
}

void SetFirstTimerValue(TimerEvent_t *timer, TimeList_t *TimeList, void (*DealFuc)(void))
{
	uint32_t time, CurrentTime, NextTime;
	uint8_t hour, minute, second;
	
	if(TimeList->size == 0)
		return;
	
	CurrentTime = GetCurrentTime();
	
	if(TimeList->size == 1 || CurrentTime < TimeList->profileTime[0].time || CurrentTime >= TimeList->profileTime[TimeList->size-1].time)
	{
		TimeList->index = 0;
		
		NextTime = TimeList->profileTime[0].time;		
	}
	else
	{
		for(uint8_t i = 0; i < TimeList->size-1; i++)
		{
			if((TimeList->profileTime[i].time <= CurrentTime) && (TimeList->profileTime[i+1].time > CurrentTime))
				TimeList->index = i;
		}
		
		NextTime = TimeList->profileTime[TimeList->index+1].time;
		TimeList->index = (TimeList->index+1)%TimeList->size;
	}
	
	if(NextTime > CurrentTime)
	{
		time  = NextTime - CurrentTime;
	}
	else	//Next day.
	{
		time = SecondsInDay - CurrentTime + NextTime;
	}
	
	TimerSetValue( timer, time*1000 );
	TimerStart( timer );
	//Uart1.PrintfLog("set time:%ds C:%d\r\n", time, CurrentTime);
	hour = CurrentTime/3600;
	minute = (CurrentTime-(hour*3600))/60;
	second = CurrentTime%60;
	
	Uart1.PrintfLog("[SetFirstTimerValue()]Time:%d:%d:%d, next schedule:%d\r\n", hour, minute, second, time);
	
	if(DealFuc != NULL)
	{
		DealFuc();
	}
}

static void Profile1DealFuc(void)
{
	//Dimming
	if(Profile1TimeList.index == 0)
	{
		StreetLightParam.pwmDuty = Profile1TimeList.profileTime[Profile1TimeList.size-1].pwm;
	}
	else
	{
		StreetLightParam.pwmDuty = Profile1TimeList.profileTime[Profile1TimeList.index-1].pwm;
	}
	
	if (StreetLightParam.JoinState == 1)
		SetTxDutyCycle(randr( 6000, StreetLightParam.TxCycleTimeLag[0] * 60000 )); //6s~2min
	
	SetBrightness(StreetLightParam.pwmDuty);
	Uart1.PrintfLog("Set duty:%d\r\n", StreetLightParam.pwmDuty);
}

static void Profile2DealFuc(void)
{
	//Dimming
	if(Profile2TimeList.index == 0)
	{
		StreetLightParam.pwmDuty = Profile2TimeList.profileTime[Profile2TimeList.size-1].pwm;
	}
	else
	{
		StreetLightParam.pwmDuty = Profile2TimeList.profileTime[Profile2TimeList.index-1].pwm;
	}
	
	if (StreetLightParam.JoinState == 1)
		SetTxDutyCycle(randr( 6000, StreetLightParam.TxCycleTimeLag[0] * 60000 )); //6s~2min
	
	SetBrightness(StreetLightParam.pwmDuty);
	Uart1.PrintfLog("Set duty:%d\r\n", StreetLightParam.pwmDuty);
}

static void Profile1TimeListEvent( void )
{
	ScheduleNextTime(&Profile1Timer, &Profile1TimeList, Profile1DealFuc);
}

static void Profile2TimeListEvent( void )
{
	ScheduleNextTime(&Profile2Timer, &Profile2TimeList, Profile2DealFuc);
}

void AddProfile1(uint8_t *data, uint8_t len, uint8_t saveFalg)
{
	uint8_t size = 0;
	
	if(len < 7)
		return;
	
	Profile1TimeList.id = (data[0]<<8) | data[1];
	
	if(((len-2)*8) % 5 == 0)
	{
		for(uint8_t i = 2; i < len; i=i+5)
		{
			Profile1TimeList.profileTime[size++].pwm = (((data[i]&0xf8)>>3)&0x1f)*5;
			Profile1TimeList.profileTime[size++].pwm = ((((data[i]&0x07)<<2)|(data[i+1]>>6))&0x1f)*5;
			Profile1TimeList.profileTime[size++].pwm = (data[i+1]*0x3e)>>1;
			Profile1TimeList.profileTime[size++].pwm = ((((data[i+1]&0x01)<<4)|((data[i+2]&0xf0)>>4))&0x1f)*5;
			Profile1TimeList.profileTime[size++].pwm = ((((data[i+2]&0x0f)<<1)|((data[i+3]&0x80)>>7))&0x1f)*5;
			Profile1TimeList.profileTime[size++].pwm = (((data[i+3]&0x7c)>>2)&0x1f)*5;
			Profile1TimeList.profileTime[size++].pwm = ((((data[i+3]&0x03)<<3)|((data[i+4]&0xe0)>>5))&0x1f)*5;
			Profile1TimeList.profileTime[size++].pwm = (data[i+4]&0x1f)*5;
		}
	}
	
	for(uint8_t i = 0; i < size; i++)
	{
		Profile1TimeList.profileTime[i].time = i*30*60;
	}
	
	Profile1TimeList.size = size;
	
	TimerStop(&Profile2Timer);	//If use profile1, stop profile2.
	SetFirstTimerValue(&Profile1Timer, &Profile1TimeList, Profile1DealFuc);
	
	if(saveFalg)
		SaveProfileToFlash(PROFILE1_TYPE, data, len);
	
	Uart1.PrintfLog("Add profile1\r\n");
}

void AddProfile2(uint8_t *data, uint8_t len, uint8_t saveFalg)
{
	uint8_t size = 0;
	
	if(len < 3)
		return;
	
	Profile2TimeList.id = data[0];
	
	if((len-1) % 2 == 0)
	{
		for(uint8_t i = 1; i < len; i=i+2)
		{
			Profile2TimeList.profileTime[size].time = (((data[i]<<8)|data[i+1])&0x07ff)*60;
			Profile2TimeList.profileTime[size].pwm = ((((data[i]<<8)|data[i+1])&0xf800)>>11)*5;
			Uart1.PrintfLog("TIME:%d PWM:%d\r\n", Profile2TimeList.profileTime[size].time, Profile2TimeList.profileTime[size].pwm);
			size++;
		}
	}
	
	Profile2TimeList.size = size;
	
	TimerStop(&Profile1Timer);	//If use profile2, stop profile1.
	SetFirstTimerValue(&Profile2Timer, &Profile2TimeList, Profile2DealFuc);
	
	if(saveFalg)
		SaveProfileToFlash(PROFILE2_TYPE, data, len);
	
	Uart1.PrintfLog("Add profile2\r\n");
}

void SaveProfileToFlash(uint8_t ProfileType, uint8_t *data, uint8_t len)
{
	uint8_t DataLenBuf[4] = {0, 0, 0, 0};
	
	if(ProfileType == PROFILE1_TYPE)		
		WriteBytesToMcuEEPROM(PROFILE_FLASH_START, Profile1Head, 4);	//Write profile head.
	else if(ProfileType == PROFILE2_TYPE)
		WriteBytesToMcuEEPROM(PROFILE_FLASH_START, Profile2Head, 4);
	
	DataLenBuf[0] = len;
	WriteBytesToMcuEEPROM(PROFILE_FLASH_START+4, DataLenBuf, 4);	//Write profile size.
	WriteBytesToMcuEEPROM(PROFILE_FLASH_START+8, data, len);	//Write profile data.
}

static uint8_t ReadProfileFromeFlash(uint8_t *data, uint8_t *len)
{
	uint8_t head[4], dataLenBuf[4], dataLen = 0;;
	
	ReadBytesFromMcuEEPROM(PROFILE_FLASH_START, head, 4);	//Read profile head.
	
	if(head[0]==Profile1Head[0] && head[1]==Profile1Head[1] && head[2]==Profile1Head[2] && head[3]==Profile1Head[3])
	{
		ReadBytesFromMcuEEPROM(PROFILE_FLASH_START+4, dataLenBuf, 4);	//Read profile size.
		dataLen = dataLenBuf[0];
		*len = dataLen;
		ReadBytesFromMcuEEPROM(PROFILE_FLASH_START+8, data, dataLen);	//Read profile data.
		return PROFILE1_TYPE;
	}
	else if(head[0]==Profile2Head[0] && head[1]==Profile2Head[1] && head[2]==Profile2Head[2] && head[3]==Profile2Head[3])
	{
		ReadBytesFromMcuEEPROM(PROFILE_FLASH_START+4, dataLenBuf, 4);	//Read profile size.
		dataLen = dataLenBuf[0];
		*len = dataLen;
		ReadBytesFromMcuEEPROM(PROFILE_FLASH_START+8, data, dataLen);	//Read profile data.
		return PROFILE2_TYPE;
	}
	else
		return NO_PROFILE;		
}

//George@20180716:get profile from flash
uint8_t GetProfileFlash(void)
{
		uint8_t data[PROFILE_MAX_SIZE] , len, profileType;
		profileType = ReadProfileFromeFlash(data, &len);
	  return profileType;
}

uint8_t LoadProfileFromFlash(void)
{
	uint8_t data[PROFILE_MAX_SIZE] ,len, profileType = NO_PROFILE;
	
	profileType = ReadProfileFromeFlash(data, &len);
	
	if(profileType == PROFILE1_TYPE)
	{
		AddProfile1(data, len, 0);
		Uart1.PrintfLog("Load profile1 from flash\r\n");
	}
	else if(profileType == PROFILE2_TYPE)
	{
		AddProfile2(data, len, 0);
		Uart1.PrintfLog("Load profile2 from flash\r\n");
	}
	else
	{
		Uart1.PrintfLog("no profile in flash\r\n");
	}
	
	return profileType;
}

uint8_t CheckProfileTypeInFlash(void)
{
	uint8_t head[4];
	
	ReadBytesFromMcuEEPROM(PROFILE_FLASH_START, head, 4);	//Read profile head.
	
	if(head[0]==Profile1Head[0] && head[1]==Profile1Head[1] && head[2]==Profile1Head[2] && head[3]==Profile1Head[3])
	{
		return PROFILE1_TYPE;
	}
	else if(head[0]==Profile2Head[0] && head[1]==Profile2Head[1] && head[2]==Profile2Head[2] && head[3]==Profile2Head[3])
	{
		return PROFILE2_TYPE;
	}
	else
		return NO_PROFILE;		
}

uint16_t GetProfileId(void)
{
	uint8_t ProfileType;
	uint16_t ProfileId;
	uint8_t data[2];
	
	ProfileType = CheckProfileTypeInFlash();
	
	if(ProfileType != NO_PROFILE)
	{
		ReadBytesFromMcuEEPROM(PROFILE_FLASH_START+8, data, 1);	//Read profile id.
		
		ProfileId = data[0];
	}
	else 
		ProfileId = 0;
	
	return ProfileId;
}

void RemoveProfile(void)
{
	EraseMcuEEPROMByWords(PROFILE_FLASH_START, (PROFILE_FLASH_END-PROFILE_FLASH_START)/4);
}

void ExitProfile(void)
{
	TimerStop(&Profile1Timer);
	TimerStop(&Profile2Timer);
}

