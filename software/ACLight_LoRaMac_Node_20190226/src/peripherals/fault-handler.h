#ifndef _FAULT_HANDLER_H_
#define _FAULT_HANDLER_H_

#include "stdint.h"
#include "stdbool.h"

typedef struct {
	
	uint8_t SaveToFlashFlag;
	uint8_t AttributeId;
	uint8_t fault;
	uint8_t PreFault;
	int8_t LedPcbTemp;
	uint16_t ACVoltageLow;
	uint16_t ACVoltageHigh;
	uint16_t ACCurrentLow;
	uint16_t ACCurrentHigh;
	uint16_t ACPowerLow;
	uint16_t ACPowerHigh;
	float ACFactorLow;
	float ACFactorHigh;
	float DCVoltageLow;
	float DCVoltageHigh;
	uint16_t DCCurrentLow;
	uint16_t DCCurrentHigh;
	int16_t DriverTempLow;
	int16_t DriverTempHigh;
	int16_t LEDTempLow;
	int16_t LEDTempHigh;
} Fault_s;

typedef enum
{
	CTRL_MODULE_FAIL_PRO = 2,
	AC_VOLTAGE_IN_PRO,
	RELAY_BROKEN_PRO,
	DRIVER_TOO_COLD_PRO,
	LED_DRIVER_BROKEN_PRO,
	DC_VOLTAGE_OUT_PRO,
	AC_CURRENT_IN_PRO,	
	DC_CURRENT_OUT_PRO,
	DRIVER_TEMP_PRO,
	LED_PCB_BROKEN_PRO,
	LED_TEMP_PRO,
	POWER_FACTOR_PRO,
	SENSOR_FAIL_PRO,
	
}FaultTypes;


typedef enum
{
	NOT_HANDLE = 0,
	IS_HANDLING,
	HAS_HANDLED,
	
}FaultHandleState;

typedef enum
{
	NOT_REPORT = 0,
	HAS_REPORTED
	
}FaultReportState;

typedef struct {
	
	uint8_t appear;
	uint8_t HandleState;
	uint8_t ReportState;
	
}FaultHandle_s;

extern Fault_s LightFault;

extern FaultHandle_s DCVoltageHandle;

void FaultHandlerInit(void);
void LightFaultsFsm(void);
bool SetFaultThreshold(uint8_t *data, uint8_t len);
void ClearFaultParamInFlash(void);

#endif
