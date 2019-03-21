#include "fault-handler.h"
#include "board.h"
#include "internal-eeprom.h"

#define FAULT_WAIT_TIME	15*60000	//15 minutes.

extern void SetTxDutyCycle(uint32_t time);

static uint8_t BrightnessBeforeFault = 0, BrightnessTmp = 0;

FaultHandle_s ACVoltageHandle = {0, 0, 0};
FaultHandle_s ACCurrentHandle = {0, 0, 0};

Fault_s LightFault = {
	.SaveToFlashFlag = 0,
	.AttributeId = 0,
	.fault = 0,
	.PreFault = 0,
	.LedPcbTemp = 35,
	.ACVoltageLow = 100,
	.ACVoltageHigh = 277,
	.ACCurrentLow = 0,
	.ACCurrentHigh = 5100,
	.ACPowerLow = 0,
	.ACPowerHigh = 3*260,
	.ACFactorLow = 0.5,
	.ACFactorHigh = 1,
	.DCVoltageLow = 0,
	.DCVoltageHigh = 91,
	.DCCurrentLow = 0,
	.DCCurrentHigh = 10000,
	.DriverTempLow = -40,
	.DriverTempHigh = 120,
	.LEDTempLow = -40,
	.LEDTempHigh = 120,
};

uint8_t FaultState = 0;

//For AC Voltage fault.
static TimerEvent_t ACVoltageTimer;
static uint8_t ACVoltageHandleCount = 0;

//For AC Current fault.
static TimerEvent_t ACCurrentTimer;

void ClearAllFaultHandle(void)
{
	memset1((uint8_t *)&ACVoltageHandle, 0, sizeof(FaultHandle_s));
	memset1((uint8_t *)&ACCurrentHandle, 0, sizeof(FaultHandle_s));
	
	TimerStop(&ACVoltageTimer);
	TimerStop(&ACCurrentTimer);
	
	FaultState = 0;
	ACVoltageHandleCount = 0;
}

void ClearFaultParamInFlash(void)
{
	EraseMcuEEPROMByWords(FAULT_PARAM_FLASH_START, (FAULT_PARAM_FLASH_STOP-FAULT_PARAM_FLASH_START)/4);
}

void SaveFaultParamThreshold(void)
{
	LightFault.SaveToFlashFlag = 0x88;
	WriteBytesToMcuEEPROM(FAULT_PARAM_FLASH_START, (uint8_t *)(&LightFault), sizeof(Fault_s));
}

void LoadFaultParamFromFlash(void)
{
	Fault_s FaultParamToRead;
	
	memset(&FaultParamToRead, 0, sizeof(Fault_s));
	
	ReadBytesFromMcuEEPROM(FAULT_PARAM_FLASH_START, (uint8_t *)(&FaultParamToRead), sizeof(Fault_s));
	
	if(FaultParamToRead.SaveToFlashFlag == 0x88)
	{
		LightFault.AttributeId = FaultParamToRead.AttributeId;
		LightFault.ACVoltageLow = FaultParamToRead.ACVoltageLow;
		LightFault.ACVoltageHigh = FaultParamToRead.ACVoltageHigh;
		LightFault.ACCurrentHigh = FaultParamToRead.ACCurrentHigh;
		LightFault.DCVoltageHigh = FaultParamToRead.DCVoltageHigh;
		LightFault.DCCurrentHigh = FaultParamToRead.DCCurrentHigh;
		LightFault.DriverTempLow = FaultParamToRead.DriverTempLow;
		LightFault.DriverTempHigh = FaultParamToRead.DriverTempHigh;
		LightFault.LEDTempHigh = FaultParamToRead.LEDTempHigh;
		
		Uart1.PrintfLog("Load Fault params from flash...\r\n");
		Uart1.PrintfLog("----------------------------\r\n");
		Uart1.PrintfLog("Fault Threshold Attribute Id:%d\r\n", LightFault.AttributeId);
		Uart1.PrintfLog("ACVoltageLow:%d\r\n", LightFault.ACVoltageLow);
		Uart1.PrintfLog("ACVoltageHigh:%d\r\n", LightFault.ACVoltageHigh);
		Uart1.PrintfLog("ACCurrentHigh:%d\r\n", LightFault.ACCurrentHigh);
		Uart1.PrintfLog("DCVoltageHigh:%f\r\n", LightFault.DCVoltageHigh);
		Uart1.PrintfLog("DCCurrentHigh:%d\r\n", LightFault.DCCurrentHigh);
		Uart1.PrintfLog("DriverTempLow:%d\r\n", LightFault.DriverTempLow);
		Uart1.PrintfLog("DriverTempHigh:%d\r\n", LightFault.DriverTempHigh);
		Uart1.PrintfLog("LEDTempHigh:%d\r\n", LightFault.LEDTempHigh);
		Uart1.PrintfLog("----------------------------\r\n");
	}
}

void ACVoltageTimerEvent(void)
{
	//GetRn820xParam(&RN820xInfo);
	//GetDcRn820xParam(&DCRN820xInfo);
	
	if(RN820xInfo.voltage > LightFault.ACVoltageHigh || RN820xInfo.voltage < LightFault.ACVoltageLow)
	{
		ACVoltageHandleCount++;
		
		if(ACVoltageHandleCount >= 2)
		{
			LightFault.fault = AC_VOLTAGE_IN_PRO;	//Set the fault flag;
			ACVoltageHandle.HandleState = HAS_HANDLED;
			Uart1.PrintfLog("Fault:AC Voltage\r\n");
			SetBrightness(0);	//Close the lamp;
			ACVoltageHandleCount = 0;			
			TimerStop(&ACVoltageTimer);
			FaultState = 0;
			return;
		}
		else
			TimerStart(&ACVoltageTimer);
	}
	else
	{	
		if(LightFault.fault == AC_VOLTAGE_IN_PRO)
			LightFault.fault = 0;	//Clear the fault flag;
		ACVoltageHandle.HandleState = NOT_HANDLE;
		SetBrightness(BrightnessBeforeFault);
		TimerStop(&ACVoltageTimer);
		FaultState = 0;
		ACVoltageHandleCount = 0;
	}
}

void ACCurrentTimerEvent(void)
{
	//GetRn820xParam(&RN820xInfo);
	//GetDcRn820xParam(&DCRN820xInfo);
	
	if(ACCurrentHandle.appear > 0)	//Check 3 times.
	{
		if(RN820xInfo.current > LightFault.ACCurrentHigh)
		{
			if(ACCurrentHandle.appear >= 3)
			{
				ACCurrentHandle.appear = 0;
				BrightnessBeforeFault = GetBrightness();
				BrightnessTmp = BrightnessBeforeFault/2;
				if(BrightnessTmp < 10 && BrightnessTmp > 0)
					BrightnessTmp = 10;
				SetBrightness(BrightnessTmp);
				TimerSetValue( &ACCurrentTimer, FAULT_WAIT_TIME );
				TimerStart(&ACCurrentTimer);
				Uart1.PrintfLog("Fault:AC Current\r\n");
				LightFault.fault = AC_CURRENT_IN_PRO;	//Report.
				return;
			}
			else
				Uart1.PrintfLog("AC Current appear:%d\r\n", ACCurrentHandle.appear);
			
			ACCurrentHandle.appear++;
			TimerStart(&ACCurrentTimer);
		}
		else
		{
			FaultState = 0;
			ACCurrentHandle.HandleState = NOT_HANDLE;
			ACCurrentHandle.appear = 0;
			TimerStop(&ACCurrentTimer);
			Uart1.PrintfLog("AC Current appear:%d\r\n", ACCurrentHandle.appear);
			return;
		}		
	}
	else
	{
		if(RN820xInfo.current > LightFault.ACCurrentHigh)	//The Fault still exist.
		{
			BrightnessTmp = GetBrightness();
			BrightnessTmp = BrightnessTmp/2;
			if(BrightnessTmp < 10 && BrightnessTmp > 0)
				BrightnessTmp = 10;
			SetBrightness(BrightnessTmp);
			ACCurrentHandle.HandleState = HAS_HANDLED;
		}
		else
		{
			if(LightFault.fault == AC_CURRENT_IN_PRO)
				LightFault.fault = 0;	//Clear the fault flag;
			ACCurrentHandle.HandleState = NOT_HANDLE;
		}
		
		TimerStop( &ACCurrentTimer );
		FaultState = 0;
	}	
}

void FaultHandlerInit(void)
{
	TimerInit( &ACVoltageTimer, ACVoltageTimerEvent );
	TimerSetValue( &ACVoltageTimer, 1000 );
	
	TimerInit( &ACCurrentTimer, ACCurrentTimerEvent );
	TimerSetValue( &ACCurrentTimer, 5000 );

	LoadFaultParamFromFlash();
}

bool SetFaultThreshold(uint8_t *data, uint8_t len)
{
	if(len != 9)
		return false;
	
	LightFault.AttributeId = data[0];
	LightFault.ACVoltageLow = data[1]*2;
	LightFault.ACVoltageHigh = data[2]*2;
	LightFault.ACCurrentHigh = data[3]*20;
	LightFault.DCVoltageHigh = data[4]*0.5;
	LightFault.DCCurrentHigh = data[5]*50;
	
	uint32_t TempTmp = 0;
	int16_t ValueTmp = 0;
	
	TempTmp = (data[6]<<16)|(data[7]<<8)|data[8];	
	
	ValueTmp = (TempTmp&(0x3f<<12))>>12;
	
	if(ValueTmp&0x20)
	{
		ValueTmp &= ~0x20;
		ValueTmp *= 5;
		ValueTmp = -ValueTmp;
	}
	else
		ValueTmp *= 5;
	
	LightFault.DriverTempLow = ValueTmp;	
	
	ValueTmp = (TempTmp&(0x3f<<6))>>6;
	
	if(ValueTmp&0x20)
	{
		ValueTmp &= ~0x20;
		ValueTmp *= 5;
		ValueTmp = -ValueTmp;
	}
	else
		ValueTmp *= 5;
	
	LightFault.DriverTempHigh = ValueTmp;
	
	ValueTmp = TempTmp&0x3f;
	
	if(ValueTmp&0x20)
	{
		ValueTmp &= ~0x20;
		ValueTmp *= 5;
		ValueTmp = -ValueTmp;
	}
	else
		ValueTmp *= 5;
	
	LightFault.LEDTempHigh = ValueTmp;
	
	Uart1.PrintfLog("----------------------------\r\n");
	Uart1.PrintfLog("Fault Threshold setting:\r\n");
	Uart1.PrintfLog("ACVoltageLow:%d\r\n", LightFault.ACVoltageLow);
	Uart1.PrintfLog("ACVoltageHigh:%d\r\n", LightFault.ACVoltageHigh);
	Uart1.PrintfLog("ACCurrentHigh:%d\r\n", LightFault.ACCurrentHigh);
	Uart1.PrintfLog("DCVoltageHigh:%f\r\n", LightFault.DCVoltageHigh);
	Uart1.PrintfLog("DCCurrentHigh:%d\r\n", LightFault.DCCurrentHigh);
	Uart1.PrintfLog("DriverTempLow:%d\r\n", LightFault.DriverTempLow);
	Uart1.PrintfLog("DriverTempHigh:%d\r\n", LightFault.DriverTempHigh);
	Uart1.PrintfLog("LEDTempHigh:%d\r\n", LightFault.LEDTempHigh);
	Uart1.PrintfLog("----------------------------\r\n");
	
	SaveFaultParamThreshold();
	
	return true;
}

//AC Voltage IN handler.
void ACVoltageFaultHandler(float voltage)
{
	if(voltage > LightFault.ACVoltageHigh || voltage < LightFault.ACVoltageLow)
	{		
		if(ACVoltageHandle.HandleState == NOT_HANDLE)
		{			
			ClearAllFaultHandle();
			BrightnessBeforeFault = GetBrightness();
			ACVoltageHandle.HandleState = IS_HANDLING;
			FaultState = 1;
			TimerStart(&ACVoltageTimer);						
		}		
	}
	else	//The voltage is normal.
	{
		if(ACVoltageHandle.HandleState == HAS_HANDLED)
		{
			FaultState = 0;
			ACVoltageHandle.HandleState = NOT_HANDLE;
			SetBrightness(BrightnessBeforeFault);	//Relieve the fault and recover the brightness.
			
			if(LightFault.fault == AC_VOLTAGE_IN_PRO)
				LightFault.fault = 0;
		}
	}
}

//AC Current IN handler.

void ACCurrentFaultHandler(uint16_t current)
{
	if(current > LightFault.ACCurrentHigh)
	{		
		if(ACCurrentHandle.HandleState == NOT_HANDLE)
		{
			ClearAllFaultHandle();
			FaultState = 1;
			ACCurrentHandle.appear = 1;
			ACCurrentHandle.HandleState = IS_HANDLING;
			TimerSetValue( &ACCurrentTimer, 1500 );	//Check 3 times.
			TimerStart(&ACCurrentTimer);
		}
	}
	else
	{
		if(ACCurrentHandle.HandleState == HAS_HANDLED)	//If become normal, clear the fault.
		{
			if(LightFault.fault == AC_CURRENT_IN_PRO)
				LightFault.fault = 0;
			ACCurrentHandle.HandleState = NOT_HANDLE;
		}
	}
}

void AllFaulsHandler(RN820x_t *RN280xParam)
{	
	ACVoltageFaultHandler(RN280xParam->voltage);
	
	if(ACVoltageHandle.HandleState == NOT_HANDLE)
		ACCurrentFaultHandler(RN280xParam->current );
}

void LightFaultsFsm(void)
{
	//GetRn820xParam(&RN820xInfo);
	AllFaulsHandler(&RN820xInfo);	

	if(LightFault.PreFault != LightFault.fault && StreetLightParam.JoinState)	//Report the fault message.
	{
		SetTxDutyCycle(20000);
		LightFault.PreFault = LightFault.fault;
	}
}
