/*******************************************************************************
  * @file    GL5516.c
  * @author  
  * @version 
  * @date    
  * @brief   
  *****************************************************************************/	
#include "gl5516.h"

void GL5516_init(void)
{
	GpioInit( &Adc.AdcInput, PHOTORESISTOR_PIN, PIN_ANALOGIC, PIN_OPEN_DRAIN, PIN_NO_PULL, 0 ); //
}

uint16_t GL5516GetVoltage(void)
{
	return AdcReadChannel( &Adc, PHOTORESISTOR_CHANNEL );
}

float GL5516VoltageConvertToLumen(void)
{
	uint16_t AdcVal = 0;
	float voltage = 0, lumen = 0;

	AdcVal = GL5516GetVoltage();
	voltage = AdcVal/4095.0*3.3;

	Uart1.PrintfLog("ADC_Val %d voltage %.02f\r\n", AdcVal, voltage);
	
	return lumen;
}
