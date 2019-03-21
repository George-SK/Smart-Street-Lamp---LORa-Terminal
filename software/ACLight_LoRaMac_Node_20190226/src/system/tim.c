/**
  ******************************************************************************
  * File Name          : TIM.c
  * Description        : This file provides code for the configuration
  *                      of the TIM instances.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "tim.h"
#include "dali.h"

TIM3_APP_s tim3_app_s;

/* USER CODE BEGIN 0 */
void (*HAL_TIM_DaliTx)(void);
char (*HAL_TIM_DaliRx)(void);
void (*HAL_TIM_DaliRxTimeOut)(void);
void TIM3_interrupt(bool state, bool orientation);



/* USER CODE END 0 */

TIM_HandleTypeDef htim3;

/* TIM3 init function */
void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;
	
	//HAL_TIM_Base_DeInit(&htim3);

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 31;		// 1us for a counter
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = DALI_TIMER_INTERRUPT_INTERVAL - 1;			// 52us * 8 is 416us
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    assert_param( FAIL );
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    assert_param( FAIL );
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    assert_param( FAIL );
  }

}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspInit 0 */

  /* USER CODE END TIM3_MspInit 0 */
    /* TIM3 clock enable */
    __HAL_RCC_TIM3_CLK_ENABLE();

    /* TIM3 interrupt Init */
    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
  /* USER CODE BEGIN TIM3_MspInit 1 */

  /* USER CODE END TIM3_MspInit 1 */
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspDeInit 0 */

  /* USER CODE END TIM3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM3_CLK_DISABLE();

    /* TIM3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(TIM3_IRQn);
  /* USER CODE BEGIN TIM3_MspDeInit 1 */

  /* USER CODE END TIM3_MspDeInit 1 */
  }
} 

void TIM3_interrupt(bool state, bool orientation)
{
	if (state == 1) {
		HAL_TIM_Base_Start_IT(&htim3);
		if (!orientation) {
			tim3_app_s.tx_counter = 0;
		} else {
			tim3_app_s.rx_counter = 1;
		}
	} else {
		HAL_TIM_Base_Stop_IT(&htim3);
	}
}

/* USER CODE BEGIN 1 */
void TIM3_IRQHandler(void)
{
	char ret = 0;
	/* TIM Update event */
  if(__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_UPDATE) != RESET)
  {
    if(__HAL_TIM_GET_IT_SOURCE(&htim3, TIM_IT_UPDATE) !=RESET)
    {
      __HAL_TIM_CLEAR_IT(&htim3, TIM_IT_UPDATE);
//      HAL_TIM_PeriodElapsedCallback(&htim3);
		if (++tim3_app_s.tx_counter >= DALI_PULSE_WIDTH/DALI_TIMER_INTERRUPT_INTERVAL) {
			tim3_app_s.tx_counter = 0;
			HAL_TIM_DaliTx();
		}

		ret = HAL_TIM_DaliRx();

		if (ret == 1) {									//receive start
//			tim3_app_s.rx_counter = 1;
		} else if (ret == 2) {							//receive a bit
			if (tim3_app_s.rx_counter <= DALI_PULSE_WIDTH/DALI_TIMER_INTERRUPT_INTERVAL - 2) {
				HAL_TIM_DaliRxTimeOut();
				tim3_app_s.rx_counter = 0;
			} else {
				tim3_app_s.rx_counter = 1;
			}
		} else if (ret == 3) {
			tim3_app_s.rx_counter = 0;
		}

		if (tim3_app_s.rx_counter) {
			if (++tim3_app_s.rx_counter > DALI_PULSE_WIDTH/DALI_TIMER_INTERRUPT_INTERVAL + 2) {
				tim3_app_s.rx_counter = 0;
				HAL_TIM_DaliRxTimeOut();
			}
		}
		
    }
  }
}
/* USER CODE END 1 */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
