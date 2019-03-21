#include "board.h"
#include "street-light.h"
#include "test.h"
#include "macro-definition.h"
#include "ds1302.h"

/* Definition for DACx clock resources */
#define DACx                            DAC
#define DACx_CHANNEL_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

#define DACx_CLK_ENABLE()               __HAL_RCC_DAC_CLK_ENABLE()
#define DACx_FORCE_RESET()              __HAL_RCC_DAC_FORCE_RESET()
#define DACx_RELEASE_RESET()            __HAL_RCC_DAC_RELEASE_RESET()

/* Definition for DACx Channel Pin */
#define DACx_CHANNEL_PIN                GPIO_PIN_4
#define DACx_CHANNEL_GPIO_PORT          GPIOA

/* Definition for DACx's Channel */
#define DACx_CHANNEL                    DAC_CHANNEL_1//DAC_CHANNEL_2

DAC_HandleTypeDef    DacHandle;
static DAC_ChannelConfTypeDef sDacConfig;

#if defined (STM32L073xx)	/* PC6----TIM3_CH1 */

#define PWM_IO	PC_6
#define RELAY_IO	PC_8
#define NET_STATE_LED		PC_5

#define TIM_CHANNEL_x		TIM_CHANNEL_1

/* Definition for TIMx clock resources */
#define TIMx                           TIM3
#define TIMx_CLK_ENABLE()              __HAL_RCC_TIM3_CLK_ENABLE()

/* Definition for TIMx Channel Pins */
#define TIMx_CHANNEL_GPIO_PORT()       __HAL_RCC_GPIOC_CLK_ENABLE()

#define TIMx_GPIO_PORT_CHANNELx        GPIOC
#define TIMx_GPIO_PIN_CHANNELx         GPIO_PIN_6
#define TIMx_GPIO_AF_CHANNELx          GPIO_AF2_TIM3

#elif defined (STM32L072xx)

#define PWM_IO	PC_6
#define RELAY_IO	PA_1
#define NET_STATE_LED		PC_13

#define TIM_CHANNEL_x		TIM_CHANNEL_1

/* Definition for TIMx clock resources */
#define TIMx                           TIM3
#define TIMx_CLK_ENABLE()              __HAL_RCC_TIM3_CLK_ENABLE()

/* Definition for TIMx Channel Pins */
#define TIMx_CHANNEL_GPIO_PORT()       __HAL_RCC_GPIOC_CLK_ENABLE()

#define TIMx_GPIO_PORT_CHANNELx        GPIOC
#define TIMx_GPIO_PIN_CHANNELx         GPIO_PIN_6
#define TIMx_GPIO_AF_CHANNELx          GPIO_AF2_TIM3

#else

#define PWM_IO	PB_10
#define RELAY_IO	PA_0
#define NET_STATE_LED		PA_15

#define TIM_CHANNEL_x		TIM_CHANNEL_3

#define TIMx_GPIO_AF_CHANNELx          GPIO_AF1_TIM2
/* Definition for TIMx clock resources */
#define TIMx                           TIM2
#define TIMx_CLK_ENABLE()              __HAL_RCC_TIM2_CLK_ENABLE()

/* Definition for TIMx Channel Pins */
#define TIMx_CHANNEL_GPIO_PORT()       __HAL_RCC_GPIOB_CLK_ENABLE()

#define TIMx_GPIO_PORT_CHANNELx        GPIOB
#define TIMx_GPIO_PIN_CHANNELx         GPIO_PIN_10
#define TIMx_GPIO_AF_CHANNELx          GPIO_AF1_TIM2

#endif

/* Private typedef -----------------------------------------------------------*/
#define  PERIOD_VALUE       (uint32_t)(8000 - 1)  /* Period Value  */
#define  PULSE3_VALUE       (uint32_t)(PERIOD_VALUE*(100-10)/100)        /* Capture Compare 3 Value  */

#define OPEN_VALUE	17
#define CLOSE_VALUE	60//25
#define LOW_LIMIT_VALUE	20
#define HIGH_LIMIT_VALUE	100
#define INIT_VALUE	100
#define MIN_LIGHT_UP_VALUE	15
#define MIN_LIGHT_DOWN_VALUE	10
#define ADJUST_DIFF_VALUE	0.8
#define ADJUST_TIMER_PERIOD	500

#define OPEN_LIGHT_LUX_VALUE	15
#define CLOSE_LIGHT_LUX_VALUE	100
#define LIGHT_SENSOR_SAMPLE_TIME	10000	//unit: ms
#define MIN_MAX_LUX_GAP	5
#define LIGHT_LUX_DIFF	10

#if (EUD_320S670DT)
#define MAX_OUTPUT_CURRENT	6667
#else
#define MAX_OUTPUT_CURRENT	2795
#endif

uint8_t InvalidData = 0;
uint8_t ValidData = 0;
DimmingMode LightDimmingMode = DAC_dimming_mode; //George@20181114:light dimming mode

/* Private function ---------------------------------------------------------*/
void SetTxDutyCycle(uint32_t time);	//Implemented in file "main.c".
static void OnAutoDimmingTimerEvent(void);

/* Private variables ---------------------------------------------------------*/
static TimerEvent_t PwmAdjustTimer;

/* Timer handler declaration */
TIM_HandleTypeDef    TimHandle;

/* Timer Output Compare Configuration Structure declaration */
TIM_OC_InitTypeDef sConfig;

/* Counter Prescaler value */
uint32_t uhPrescalerValue = 0;

Gpio_t StreetLightPwmIo;

StreetLight_s StreetLightParam;

static TimerEvent_t AutoDimmingTimer;

struct AutoDimmingLevel_s {
	const uint8_t level;
	const float LuxThreshold;
	const uint8_t pwm;
};

static const struct AutoDimmingLevel_s AutoDimmingLevel[] = 
{
	{
		.level = 0,
		.LuxThreshold = 20,
		.pwm = 100,
	},
	{
		.level = 1,
		.LuxThreshold = 50,
		.pwm = 50,
	},
	{
		.level = 2,
		.LuxThreshold = 100,
		.pwm = 0,
	},
};

void OnAutoDimmingTimerEvent(void)
{
	static uint8_t LuxIndex = 0;
	
	if(LuxIndex >= 3)
		LuxIndex = 0;

#ifdef GL5516_PHOTOREISTANCE_ENABLE
	StreetLightParam.CurrentLuxBuf[LuxIndex++] = GL5516VoltageConvertToLumen();
#else
	StreetLightParam.CurrentLuxBuf[LuxIndex++] = ReadOpt3001Value();
#endif
	
	TimerSetValue( &AutoDimmingTimer, LIGHT_SENSOR_SAMPLE_TIME );
	TimerStart( &AutoDimmingTimer );
}

void SetPwmDutyCycle(uint8_t duty)
{
	uint32_t pulse;
	
	if (duty != 0 || duty >= 100)
		duty = 100;
	else 
		SetRelay(0);

	StreetLightParam.pwmDuty = duty;
	
	if (LightDimmingMode == DAC_dimming_mode)
	{
		if (HAL_DAC_SetValue(&DacHandle, DACx_CHANNEL, DAC_ALIGN_12B_R, duty/100.0*4095) != HAL_OK)
		{
				/* Setting value Error */
		}
		
		if (HAL_DAC_Start(&DacHandle, DACx_CHANNEL) != HAL_OK)
		{
				/* Start Error */
		}
	}
	else if (LightDimmingMode == PWM_dimming_mode)
	{
		pulse = PERIOD_VALUE*(100-duty)/100;
			/* Set the Capture Compare Register value */
#if defined (STM32L073xx)	/* PC6----TIM3_CH1 */
		TIMx->CCR1 = pulse;
#else
		TIMx->CCR3 = pulse;
#endif
	}
	else if (LightDimmingMode == DALI_dimming_mode)
	{
			RecvRolaPowerCMD(duty); 
	}
	else
	{
		if (HAL_DAC_SetValue(&DacHandle, DACx_CHANNEL, DAC_ALIGN_12B_R, duty/100.0*4095) != HAL_OK)
		{
				/* Setting value Error */
		}
		
		if (HAL_DAC_Start(&DacHandle, DACx_CHANNEL) != HAL_OK)
		{
				/* Start Error */
		}
	}
}

void SetBrightness(uint8_t duty)
{
	if (LightDimmingMode == DALI_dimming_mode)
	{
			RecvRolaPowerCMD(duty); //shengyu@20180806:add dali dimming
	}
	else
	{
			if(duty > 100)
				duty = 100;
			/*+++++++++++++++++++++++++++++++++*/
			if(duty > 0)
				GpioWrite( &Led3, 0 );
			else
				GpioWrite( &Led3, 1 );
			/*+++++++++++++++++++++++++++++++++*/
			StreetLightParam.pwmDuty = duty;
			TimerStart(&PwmAdjustTimer);
	}
}

void PwmAdjustTimerEvent(void)
{	
	if (LightDimmingMode == DAC_dimming_mode)
	{
			if(StreetLightParam.currentDuty < StreetLightParam.pwmDuty-ADJUST_DIFF_VALUE)
			{			
				if(StreetLightParam.currentDuty < MIN_LIGHT_UP_VALUE)
				{
					StreetLightParam.currentDuty = MIN_LIGHT_UP_VALUE;
					SetRelay(1);	//Open the Relay. Added by Steven in 2017/12/28.
				}			
				else if(StreetLightParam.currentDuty < 30)
				{
					StreetLightParam.currentDuty += 1;
					//TimerSetValue( &PwmAdjustTimer, 100 );
				}
				else if(StreetLightParam.currentDuty < 60)
				{
					StreetLightParam.currentDuty += 1;
					//TimerSetValue( &PwmAdjustTimer, 100 );
				}
				else
				{
					StreetLightParam.currentDuty += 1;
					//TimerSetValue( &PwmAdjustTimer, 100 );
				}
			}
			else if(StreetLightParam.currentDuty > StreetLightParam.pwmDuty+ADJUST_DIFF_VALUE)
			{			
				if(StreetLightParam.currentDuty < MIN_LIGHT_DOWN_VALUE)
				{
					StreetLightParam.currentDuty = 0;
					SetRelay(0);	//Close the Relay.	Added by Steven in 2017/12/28.
					/* Set the Capture Compare Register value */
					/*##-3- Set DAC Channel1 DHR register ######################################*/
					if (HAL_DAC_SetValue(&DacHandle, DACx_CHANNEL, DAC_ALIGN_12B_R, 0) != HAL_OK)
					{
						/* Setting value Error */
					}

					/*##-4- Enable DAC Channel1 ################################################*/
					if (HAL_DAC_Start(&DacHandle, DACx_CHANNEL) != HAL_OK)
					{
						/* Start Error */
					}
					
					TimerStop(&PwmAdjustTimer);
					return;
				}			
				else if(StreetLightParam.currentDuty < 30)
				{
					StreetLightParam.currentDuty -= 1;
					//TimerSetValue( &PwmAdjustTimer, 100 );
				}
				else if(StreetLightParam.currentDuty < 60)
				{
					StreetLightParam.currentDuty -= 1;
					//TimerSetValue( &PwmAdjustTimer, 100 );
				}
				else
				{
					StreetLightParam.currentDuty -= 1;
					//TimerSetValue( &PwmAdjustTimer, 100 );
				}
			}
			else
			{
				/*##-3- Set DAC Channel1 DHR register ######################################*/
				if (HAL_DAC_SetValue(&DacHandle, DACx_CHANNEL, DAC_ALIGN_12B_R, StreetLightParam.currentDuty/100.0*4095) != HAL_OK)
				{
					/* Setting value Error */
				}

				/*##-4- Enable DAC Channel1 ################################################*/
				if (HAL_DAC_Start(&DacHandle, DACx_CHANNEL) != HAL_OK)
				{
					/* Start Error */
				}
				TimerStop(&PwmAdjustTimer);
				return;
			}
		
		/*##-3- Set DAC Channel1 DHR register ######################################*/
		if (HAL_DAC_SetValue(&DacHandle, DACx_CHANNEL, DAC_ALIGN_12B_R, StreetLightParam.currentDuty/100.0*4095) != HAL_OK)
		{
			/* Setting value Error */
		}

		/*##-4- Enable DAC Channel1 ################################################*/
		if (HAL_DAC_Start(&DacHandle, DACx_CHANNEL) != HAL_OK)
		{
			/* Start Error */
		}
		
		TimerSetValue( &PwmAdjustTimer, 30 );
	}
	else if (LightDimmingMode == PWM_dimming_mode)
	{
		uint32_t pulse;
		
		if(StreetLightParam.currentDuty < StreetLightParam.pwmDuty-ADJUST_DIFF_VALUE)
		{
			if(StreetLightParam.currentDuty < MIN_LIGHT_UP_VALUE)
			{
				StreetLightParam.currentDuty = MIN_LIGHT_UP_VALUE;
				SetRelay(1);	//Open the Relay. Added by Steven in 2017/12/28.
			}			
			else if(StreetLightParam.currentDuty < 30)
			{
				StreetLightParam.currentDuty += 1;
				TimerSetValue( &PwmAdjustTimer, 30 );
			}
			else if(StreetLightParam.currentDuty < 60)
			{
				StreetLightParam.currentDuty += 1;
				TimerSetValue( &PwmAdjustTimer, 30 );
			}
			else
			{
				StreetLightParam.currentDuty += 2;
				TimerSetValue( &PwmAdjustTimer, 100 );
			}
			//StreetLightParam.currentDuty++;
		}
		else if(StreetLightParam.currentDuty > StreetLightParam.pwmDuty+ADJUST_DIFF_VALUE)
		{
			if(StreetLightParam.currentDuty < MIN_LIGHT_DOWN_VALUE)
			{
				StreetLightParam.currentDuty = 0;
				SetRelay(0);	//Close the Relay.	Added by Steven in 2017/12/28.
				return;
			}			
			else if(StreetLightParam.currentDuty < 30)
			{
				StreetLightParam.currentDuty -= 1;
				TimerSetValue( &PwmAdjustTimer, 30 );
			}
			else if(StreetLightParam.currentDuty < 60)
			{
				StreetLightParam.currentDuty -= 1;
				TimerSetValue( &PwmAdjustTimer, 30 );
			}
			else
			{
				StreetLightParam.currentDuty -= 2;
				TimerSetValue( &PwmAdjustTimer, 100 );
			}
			//StreetLightParam.currentDuty--;
		}
		else
		{
			TimerStop(&PwmAdjustTimer);
			return;
		}
		
		pulse = PERIOD_VALUE*(float)(100-StreetLightParam.currentDuty)/100;
		/* Set the Capture Compare Register value */
	#if defined (STM32L073xx)	/* PC6----TIM3_CH1 */
		TIMx->CCR1 = pulse;
	#else
		TIMx->CCR3 = pulse;
	#endif
	}
	else
	{
			//This should not be executed
	}
		
	TimerStart(&PwmAdjustTimer);
}

void AutoDimming(float lux)
{
	uint8_t level = 0;	
	uint8_t i = 0, len = 0, pwm = 0;
	
	len = sizeof(AutoDimmingLevel)/sizeof(struct AutoDimmingLevel_s);
	
	for(i = 0; i < len; i++)
	{
		if(i == 0)
		{
			if(lux < AutoDimmingLevel[i].LuxThreshold)
			{
				break;
			}
		}
		else
		{
			if(lux < AutoDimmingLevel[i].LuxThreshold && lux >= AutoDimmingLevel[i-1].LuxThreshold)
			{
				break;
			}
		}
	}
	
	level = i;
		
	if(i == len)
	{
		pwm = 0;
	}
	else
	{
		pwm = AutoDimmingLevel[i].pwm;
	}

	if(level != StreetLightParam.PreLuxLevel)
	{
		SetBrightness(pwm);
	}
	
	StreetLightParam.PreLuxLevel = level;
}

void swap(float *a,float *b)  
{  
	float temp; 
	
	temp = *a;  
	*a = *b;  
	*b = temp;  
}  

void BubbleSort(float a[],uint8_t len)    
{  
	uint8_t i,j;
	
	for(i = 0; i < len; i++)  
	{  
		for(j = i+1; j < len; j++)  
		{  
			if(a[i] > a[j])  
			swap(&a[i], &a[j]);  
		}  
	}  
}  

void GetLuxValue(float *LuxValue)
{
	float LuxBuf[3];
	
	for(uint8_t i = 0; i < 3; i++)
	{
		LuxBuf[i] = StreetLightParam.CurrentLuxBuf[i];
	}	
	
	BubbleSort(LuxBuf, 3);
	
	*LuxValue = LuxBuf[1];
}

void AutoDimming2(float lux)
{
	if(lux < 8)
	{
		if(StreetLightParam.PreLuxLevel != 1)
		{
			if(StreetLightParam.JoinState == 1)
				SetTxDutyCycle(randr( 6000, StreetLightParam.TxCycleTimeLag[0] * 60000 )); //6s~2min
			
			SetBrightness(100);
			StreetLightParam.PreLuxLevel = 1;
		}		
	}
	else if(lux > 30)
	{
		if(StreetLightParam.PreLuxLevel != 0)
		{
			if(StreetLightParam.JoinState == 1)
				SetTxDutyCycle(randr( 6000, StreetLightParam.TxCycleTimeLag[0] * 60000 )); //6s~2min
			
			SetBrightness(0);
			StreetLightParam.PreLuxLevel = 0;
		}		
	}
}

float GetAbs(float value)
{
	if(value < 0)
		value = 0 - value;
	
	return value;
}

void StreetLightFsm(void)
{
	//uint8_t pwmDuty;
	
	/*lux = ReadOpt3001Value();
	StreetLightParam.lux = lux;*/
	GetLuxValue(&StreetLightParam.lux);
	
	if(StreetLightParam.ctrlMode == AUTO_DIMMING_MODE)
	{		
		
		//if(GetAbs(StreetLightParam.lux-StreetLightParam.PreLuxValue) >= LIGHT_LUX_DIFF)
		//{
		//	AutoDimming2(StreetLightParam.lux);
		//	StreetLightParam.PreLuxValue = StreetLightParam.lux;
		//}
		//AutoDimming(lux);
		AutoDimming2(StreetLightParam.lux);
	}
	
	if(StreetLightParam.TimeSyncRecv)	//Time sync handling.
	{
		if(StreetLightParam.ctrlMode == PROFILE_DIMMING_MODE)
		{
			if(LoadProfileFromFlash() == NO_PROFILE)	//If no profile, enter auto dimming mode.
			{
				Uart1.PrintfLog("Profile mode failed!\r\n");
			}
			else
			{
				Uart1.PrintfLog("Reload the profile\r\n");
			}
		}
		
		StreetLightParam.TimeSyncRecv = 0;
	}
	
	if(StreetLightParam.RecvFlag)	//Dimming mode handling.
	{
		if(StreetLightParam.RecvData[0]&0x80)
		{
			EnterDimmingMode(MANUAL_DIMMING_MODE, 1);			
		}
		else
		{
			if(StreetLightParam.RecvData[0] == 0)
			{
				EnterDimmingMode(AUTO_DIMMING_MODE, 1);
				Uart1.PrintfLog("Enter auto dimming mode\r\n");
			}
			else	//PROFILE_DIMMING_MODE
			{
				if(StreetLightParam.ctrlMode != PROFILE_DIMMING_MODE)
				{
					if(StreetLightParam.TimeSync)	//Load profile if time has been synchronized.
					{
						if(LoadProfileFromFlash() == NO_PROFILE)	//If no profile, enter auto dimming mode.
						{
							Uart1.PrintfLog("Profile mode failed!\r\n");
						}
						else
						{
							EnterDimmingMode(PROFILE_DIMMING_MODE, 1);
							Uart1.PrintfLog("Enter profile dimming mode\r\n");
						}
					}
				}
			}
		}			
		
		StreetLightParam.RecvFlag = 0;
	}
}

void StreetLightInit(void)
{
	SetNetStateLed(0);
	SetRelay(1);	//Open Relay.
	
	//GpioInit( &StreetLightPwmIo, PWM_IO, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );
	
	/* Compute the prescaler value to have TIM2 counter clock equal to 16000000 Hz */
  uhPrescalerValue = (uint32_t)(SystemCoreClock / 16000000) - 1;

  /*##-1- Configure the TIM peripheral #######################################*/
  /* -----------------------------------------------------------------------
  TIM2 Configuration: generate 4 PWM signals with 4 different duty cycles.

    In this example TIM2 input clock (TIM2CLK) is set to APB1 clock (PCLK1),
    since APB1 prescaler is equal to 1.
      TIM2CLK = PCLK1
      PCLK1 = HCLK
      => TIM2CLK = HCLK = SystemCoreClock

    To get TIM2 counter clock at 16 MHz, the prescaler is computed as follows:
       Prescaler = (TIM2CLK / TIM2 counter clock) - 1
       Prescaler = ((SystemCoreClock) /16 MHz) - 1

    To get TIM2 output clock at 24 KHz, the period (ARR)) is computed as follows:
       ARR = (TIM2 counter clock / TIM2 output clock) - 1
           = 665

    TIM2 Channel1 duty cycle = (TIM2_CCR1/ TIM2_ARR + 1)* 100 = 50%
    TIM2 Channel2 duty cycle = (TIM2_CCR2/ TIM2_ARR + 1)* 100 = 37.5%
    TIM2 Channel3 duty cycle = (TIM2_CCR3/ TIM2_ARR + 1)* 100 = 25%
    TIM2 Channel4 duty cycle = (TIM2_CCR4/ TIM2_ARR + 1)* 100 = 12.5%

    Note:
     SystemCoreClock variable holds HCLK frequency and is defined in system_stm32l1xx.c file.
     Each time the core clock (HCLK) changes, user had to update SystemCoreClock
     variable value. Otherwise, any configuration based on this variable will be incorrect.
     This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetSysClockFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
  ----------------------------------------------------------------------- */

  /* Initialize TIMx peripheral as follows:
       + Prescaler = (SystemCoreClock / 16000000) - 1
       + Period = (16000 - 1)
       + ClockDivision = 0
       + Counter direction = Up
  */

	if (GetDataFromEEPROM(LIGHT_DIMMING_MODE_START, &InvalidData, &ValidData) == true)
	{
			switch(ValidData)
			{	
					case 0: LightDimmingMode = DAC_dimming_mode; break;
					case 1: LightDimmingMode = PWM_dimming_mode; break;
					case 2: LightDimmingMode = DALI_dimming_mode; break;
					default : LightDimmingMode = DAC_dimming_mode; break;
			}
	}
	else
	{
			LightDimmingMode = DAC_dimming_mode;
	}
	
	if (LightDimmingMode == DAC_dimming_mode)
	{
			DACInit();
	}
	else if (LightDimmingMode == PWM_dimming_mode)
	{
			TimHandle.Instance = TIMx;

			TimHandle.Init.Prescaler         = uhPrescalerValue;
			TimHandle.Init.Period            = PERIOD_VALUE;
			TimHandle.Init.ClockDivision     = 0;
			TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
			if (HAL_TIM_PWM_Init(&TimHandle) != HAL_OK)
			{
				/* Initialization Error */
				Uart1.PrintfLog("Initialization error\r\n");
			}

			/*##-2- Configure the PWM channels #########################################*/
			/* Common configuration for all channels */
			sConfig.OCMode       = TIM_OCMODE_PWM1;
			sConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
			sConfig.OCFastMode   = TIM_OCFAST_DISABLE;
#if !defined (STM32L073xx) && !defined (STM32L072xx)
			sConfig.OCIdleState  = TIM_OCIDLESTATE_RESET;
#endif
		 
			/* Set the pulse value for channel 3 */
			sConfig.Pulse = PULSE3_VALUE;
			if (HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, TIM_CHANNEL_x) != HAL_OK)
			{
				/* Configuration Error */
				Uart1.PrintfLog("Initialization error\r\n");
			}

			/*##-3- Start PWM signals generation #######################################*/ 
			/* Start channel 3 */
			if (HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_x) != HAL_OK)
			{
				/* PWM generation Error */
				Uart1.PrintfLog("PWM Generation error\r\n");
			}
	}
	else if (LightDimmingMode == DALI_dimming_mode)
	{
			MX_TIM3_Init();
			dali_init();
	}	
	else
	{
			DACInit();
	}
	
	uint8_t ContralMode = 0, ModeBrightness = 100;

#if defined (DS1302_RTC_ENABLE)  //George@20181030:get contral mode
	
	DS1302_TimeSync DS1302_Get;
	uint32_t CurTimeVal;
	
	DS1302Initialize(); 
	DS1302_Get = DS1302GetTimeSync();
	CurTimeVal = DS1302_Get.hour*3600+DS1302_Get.min*60+DS1302_Get.sec;
	TimeSyncFunc(CurTimeVal);
	
	if (GetDataFromEEPROM(DEVICE_CONTRAL_MODE_START, &ContralMode, &ModeBrightness) == true)
	{
			Uart1.PrintfLog("ContralMode=%02X ModeBrightness=%d\r\n", ContralMode, ModeBrightness);
			StreetLightParam.currentDuty = ModeBrightness;
		
			if (ContralMode == PROFILE_DIMMING_MODE || (ContralMode & 0x80 == 1)) 
			{
					if(LoadProfileFromFlash() != NO_PROFILE)	
					{
							StreetLightParam.ctrlMode = PROFILE_DIMMING_MODE;
							Uart1.PrintfLog("Profile dimming mode\r\n");
					}
					else
					{
							StreetLightParam.ctrlMode =AUTO_DIMMING_MODE;
							if (ContralMode & 0x80)
							{
									SetPwmDutyCycle(ModeBrightness);
									SetDataToEEPROM(DEVICE_CONTRAL_MODE_START, PROFILE_DIMMING_MODE, ModeBrightness);
							}
							Uart1.PrintfLog("Auto dimming mode\r\n");
					}
			}
			else if (ContralMode == MANUAL_DIMMING_MODE)
			{
					StreetLightParam.ctrlMode = MANUAL_DIMMING_MODE;
					if (ContralMode & 0x80)
					{
							SetPwmDutyCycle(ModeBrightness);
							SetDataToEEPROM(DEVICE_CONTRAL_MODE_START, MANUAL_DIMMING_MODE, ModeBrightness);
					}
					Uart1.PrintfLog("Manual dimming mode\r\n");
			}
			else
			{
					StreetLightParam.ctrlMode = AUTO_DIMMING_MODE;
					if (ContralMode & 0x80)
					{
							SetPwmDutyCycle(ModeBrightness);
							SetDataToEEPROM(DEVICE_CONTRAL_MODE_START, AUTO_DIMMING_MODE, ModeBrightness);
					}
					Uart1.PrintfLog("Auto dimming mode\r\n");
			}
	}
	else
	{
			Uart1.PrintfLog("Get eeprom data error\r\n");
	}
#else
	StreetLightParam.ctrlMode = MANUAL_DIMMING_MODE;
	GetDataFromEEPROM(DEVICE_CONTRAL_MODE_START, &ContralMode, &ModeBrightness);

	if (ContralMode & 0x80) //Abnormal reset
	{
			SetPwmDutyCycle(ModeBrightness);
			StreetLightParam.currentDuty = ModeBrightness;
			SetDataToEEPROM(DEVICE_CONTRAL_MODE_START, MANUAL_DIMMING_MODE, ModeBrightness);
			Uart1.PrintfLog("Manual dimming mode Brightness=%d%\r\n", (uint8_t)StreetLightParam.currentDuty);
	}
	else
	{
			SetPwmDutyCycle(INIT_VALUE);
			StreetLightParam.currentDuty = INIT_VALUE;
			Uart1.PrintfLog("Manual dimming mode Brightness=%d%\r\n", (uint8_t)StreetLightParam.currentDuty);
	}
#endif
	
	EnableOpt3001(1);
	StreetLightParam.PreLuxLevel = 0xff;
	StreetLightParam.TxCycleTime = 1200000;	//20 minutes.
	
	TimerInit( &PwmAdjustTimer, PwmAdjustTimerEvent );
	TimerSetValue( &PwmAdjustTimer, ADJUST_TIMER_PERIOD );
	
	TimerInit( &AutoDimmingTimer, OnAutoDimmingTimerEvent );
	TimerSetValue( &AutoDimmingTimer, 100 );
	TimerStart( &AutoDimmingTimer );
	
	StreetLightParam.PreLuxValue = 0xffffff;	
	StreetLightParam.MsgTypeCount = 0;
	
	GetTxCycleTimeLagFromEEPROM(); //George@20180712:get tx cycle time lag from flash
}

/**
  * @brief TIM MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  * @param htim: TIM handle pointer
  * @retval None
  */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
  GPIO_InitTypeDef   GPIO_InitStruct;
  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* TIMx Peripheral clock enable */
  TIMx_CLK_ENABLE();

  /* Enable GPIO Channels Clock */
  TIMx_CHANNEL_GPIO_PORT();

  /* Configure PB.10 (TIM2_Channel3) in output, push-pull, alternate function mode
  */
  /* Common configuration for all channels */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  GPIO_InitStruct.Alternate = TIMx_GPIO_AF_CHANNELx;
  GPIO_InitStruct.Pin = TIMx_GPIO_PIN_CHANNELx;
  HAL_GPIO_Init(TIMx_GPIO_PORT_CHANNELx, &GPIO_InitStruct);
}

void PowerSimulation(uint8_t pwm, float *current, float *voltage, float *power)
{
	//Calculate current.
	if(pwm < MIN_LIGHT_UP_VALUE)
		*current = 0;
	else if(pwm < 90)
	{
		*current = (10+((pwm-10)*9.0/8))/100*MAX_OUTPUT_CURRENT;
	}
	else
	{
		*current = MAX_OUTPUT_CURRENT;
	}
	
#if (EUD_320S670DT)
	//Calculate voltage.
	if(pwm < MIN_LIGHT_UP_VALUE)
		*voltage = 0;
	else if(pwm <= 50)
	{
		*voltage = (45.3+((pwm-MIN_LIGHT_UP_VALUE)*0.2/5));
	}
	else
	{
		*voltage = (46.9+((pwm-50)*0.1/5));
	}
#else
	//Calculate voltage.
	if(pwm < MIN_LIGHT_UP_VALUE)
		*voltage = 0;
	else if(pwm <= 50)
	{
		*voltage = (31.4+((pwm-MIN_LIGHT_UP_VALUE)*0.2/5));
	}
	else
	{
		*voltage = (33+((pwm-50)*0.1/5));
	}
#endif
	
	*power = (*current)*(*voltage)/1000;
}

void SetRelay(uint8_t state)
{
	Gpio_t RelayIo;
	
	if(state)
		GpioInit( &RelayIo, RELAY_IO, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );	//New relay.
	else
		GpioInit( &RelayIo, RELAY_IO, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_DOWN, 1 );
}

void SetNetStateLed(uint8_t state)
{
	Gpio_t NetStateLedIo;
	
	if(state)
		GpioInit( &NetStateLedIo, NET_STATE_LED, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
	else
		GpioInit( &NetStateLedIo, NET_STATE_LED, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_DOWN, 0 );
}

//Added by Steven for test in 2018/06/13.
void EnterDimmingMode(uint8_t mode, uint8_t NeedToUpload)
{	
	switch(mode)
	{
		case MANUAL_DIMMING_MODE:
			
			if(StreetLightParam.ctrlMode == PROFILE_DIMMING_MODE)	//Warning!!! It may be a potential BUG!
			{
				ExitProfile();
			}
			
			StreetLightParam.ctrlMode = MANUAL_DIMMING_MODE;
			StreetLightParam.TxCycleCount = 1;	//TX dutycycle:3s~2 minutes
		
			if(NeedToUpload)
				SetTxDutyCycle(randr( 6000, StreetLightParam.TxCycleTimeLag[0] * 60000 )); //6s~2min
			
			StreetLightParam.pwmDuty = StreetLightParam.RecvData[0]&0x7f;
			SetBrightness(StreetLightParam.pwmDuty);
			Uart1.PrintfLog("Set duty:%d\r\n", StreetLightParam.pwmDuty);
			break;
		case AUTO_DIMMING_MODE:
					
			if(StreetLightParam.ctrlMode == PROFILE_DIMMING_MODE)	//Warning!!! It may be a potential BUG!
			{
				ExitProfile();
			}				
		
			StreetLightParam.ctrlMode = AUTO_DIMMING_MODE;
			StreetLightParam.PreLuxLevel = 0xff;
			StreetLightParam.TxCycleCount = 1;			
			
			if(NeedToUpload)
				SetTxDutyCycle(randr( 6000, StreetLightParam.TxCycleTimeLag[0] * 60000 )); //6s~2min
			break;
		case PROFILE_DIMMING_MODE:
			StreetLightParam.ctrlMode = PROFILE_DIMMING_MODE;
			StreetLightParam.TxCycleCount = 1;	//TX dutycycle:20 minutes
			
			if(NeedToUpload)
				SetTxDutyCycle(randr( 6000, StreetLightParam.TxCycleTimeLag[0] * 60000 )); //6s~2min
			break;
		default:
			break;
	}
}

/**
  * @brief DAC MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  * @param hdac: DAC handle pointer
  * @retval None
  */
void HAL_DAC_MspInit(DAC_HandleTypeDef *hdac)
{
  GPIO_InitTypeDef          GPIO_InitStruct;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO clock ****************************************/
  DACx_CHANNEL_GPIO_CLK_ENABLE();
  /* DAC Periph clock enable */
  DACx_CLK_ENABLE();

  /*##-2- Configure peripheral GPIO ##########################################*/
  /* DAC Channel1 GPIO pin configuration */
  GPIO_InitStruct.Pin = DACx_CHANNEL_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DACx_CHANNEL_GPIO_PORT, &GPIO_InitStruct);
}

/**
  * @brief  DeInitializes the DAC MSP.
  * @param  hdac: pointer to a DAC_HandleTypeDef structure that contains
  *         the configuration information for the specified DAC.
  * @retval None
  */
void HAL_DAC_MspDeInit(DAC_HandleTypeDef *hdac)
{
  /* Enable DAC reset state */
  DACx_FORCE_RESET();

  /* Release DAC from reset state */
  DACx_RELEASE_RESET();
}

void DACInit(void)
{
	/*##-1- Configure the DAC peripheral #######################################*/
  DacHandle.Instance = DACx;

  if (HAL_DAC_Init(&DacHandle) != HAL_OK)
  {
    /* Initialization Error */
  }

  /*##-2- Configure DAC channel1 #############################################*/
  sDacConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sDacConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;

  if (HAL_DAC_ConfigChannel(&DacHandle, &sDacConfig, DACx_CHANNEL) != HAL_OK)
  {
    /* Channel configuration Error */
  }

  /*##-3- Set DAC Channel1 DHR register ######################################*/
  if (HAL_DAC_SetValue(&DacHandle, DACx_CHANNEL, DAC_ALIGN_12B_R, 4095) != HAL_OK)
  {
    /* Setting value Error */
  }

  /*##-4- Enable DAC Channel1 ################################################*/
  if (HAL_DAC_Start(&DacHandle, DACx_CHANNEL) != HAL_OK)
  {
    /* Start Error */
  }
}

uint8_t GetBrightness(void)
{
	return StreetLightParam.pwmDuty;
}

//George@20181101:set contral mode to eeprom
void SetDataToEEPROM(uint32_t addr, uint8_t arg1, uint8_t arg2)
{
		uint8_t SaveBuffer[4], len = 0;

		SaveBuffer[len++] = 0x88;
		SaveBuffer[len++] = 0x66;
		SaveBuffer[len++] = arg1;
		SaveBuffer[len++] = arg2;

		WriteBytesToMcuEEPROM(addr, SaveBuffer, len);
}

//George@20181101:get contral mode from eeprom
bool GetDataFromEEPROM(uint32_t addr, uint8_t *arg1, uint8_t *arg2)
{
		bool resultFlag = false;
		uint8_t GetBuffer[4];

		ReadBytesFromMcuEEPROM(addr, GetBuffer, 4);
 
		if (GetBuffer[0] == 0x88 && GetBuffer[1] == 0x66)
		{
				*arg1 = GetBuffer[2];
				*arg2 = GetBuffer[3];
				resultFlag = true;
		}		

		return resultFlag;
}

//George@20181101:save contral mode and brightness
void SaveCtrModeAndBrightness(uint8_t *buffer)
{
		uint8_t CtrMode;
		uint8_t ContralMode, ModeBrightness;
		
		if (buffer[0]&0x80)
		{
				CtrMode = MANUAL_DIMMING_MODE;
		}
		else
		{
				if ((buffer[0]&0x4f) == 0x40)
				{
						CtrMode = PROFILE_DIMMING_MODE;
				}
				else
				{
						CtrMode = AUTO_DIMMING_MODE;
				}
		}	
		
		if (GetDataFromEEPROM(DEVICE_CONTRAL_MODE_START, &ContralMode, &ModeBrightness) == true)
		{
				if (CtrMode == ContralMode && (buffer[0]&0x7f) == ModeBrightness)
					return;
		}
		
		Uart1.PrintfLog("Light set contral mode=%d brightness=%d\r\n", CtrMode, buffer[0]&0x7f);
		SetDataToEEPROM(DEVICE_CONTRAL_MODE_START, CtrMode, buffer[0]&0x7f);
}
