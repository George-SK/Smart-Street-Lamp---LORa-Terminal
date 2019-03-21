/**
  ******************************************************************************
  * @file    IAP_Main/Src/menu.c 
  * @author  MCD Application Team
  * @brief   This file provides the software which contains the main menu routine.
  *          The main menu gives the options of:
  *             - downloading a new binary file, 
  *             - uploading internal flash memory,
  *             - executing the binary file already loaded 
  *             - configuring the write protection of the Flash sectors where the 
  *               user loads his binary file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
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

/** @addtogroup STM32L0xx_IAP
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "common.h"
#include "flash_if.h"
#include "menu.h"
#include "ymodem.h"
#include "usart.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
COM_StatusTypeDef SerialDownload(void);
void SerialUpload_App(void);
void SerialUpload_Firmware(void);

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Download a file via serial port
  * @param  None
  * @retval None
  */
COM_StatusTypeDef SerialDownload(void)
{
  uint32_t size = 0;
  COM_StatusTypeDef result;

  result = Ymodem_Receive( &size );
  if (result == COM_OK)
  {
	  Serial_PutString((uint8_t *)"+DOWNLOAD:SUCCESS\r\n");
  }
  else if (result == COM_LIMIT)
  {
	  Serial_PutString((uint8_t *)"+DOWNLOAD:LIMIT\r\n");
  }
  else if (result == COM_DATA)
  {
		Serial_PutString((uint8_t *)"+DOWNLOAD:FAILED\r\n");
  }
  else if (result == COM_ABORT)
  {
    Serial_PutString((uint8_t *)"+DOWNLOAD:ABORTED\r\n");
  }
  else
  {
    Serial_PutString((uint8_t *)"+DOWNLOAD:EMPTY\r\n");
  }
	
	return result;
}

/**
  * @brief  Upload a app file via serial port.
  * @param  None
  * @retval None
  */
void SerialUpload(void)
{
  uint8_t status = 0;

  HAL_UART_Receive(&huart1, &status, 1, RX_TIMEOUT);
  if (status == CRC16)
  {
    /* Transmit the flash image through ymodem protocol */
    status = Ymodem_Transmit((uint8_t*)FIRMWARE_ADDRESS, (const uint8_t*)"L1_LoRaMacFirmware.bin", USER_FLASH_SIZE);

    if (status != 0)
    {
				Serial_PutString((uint8_t *)"+UPLOAD:FAILED\r\n");
    }
    else
    {
        Serial_PutString((uint8_t *)"+UPLOAD:SUCCESS\r\n");
    }
  }
}

/**
  * @brief  Display the Main Menu on HyperTerminal
  * @param  None
  * @retval None
  */
void Main_Menu(void)
{
#if 0
		if ((*(__IO uint32_t *)IAP_FLASH_MARKADDESS) == RUN_DOWNLOAD_FLAG)
		{
				FLASH_If_Erase(FLASH_APP_BACKUP_START, FLASH_APP_BACKUP_END);
				if (FLASH_IF_COPY((uint32_t*)APPLICATION_ADDRESS, FLASH_APP_BACKUP_START, USER_APP_SIZE) != FLASHIF_OK)
				{
						Serial_PutString((uint8_t *)"BACKUP FAILED\r\n");
						Iap_Load_App();
				}
			
			  SetFalshRun_Flag(UPDATA_APP_FAILED_FLAG);
				Serial_PutString((uint8_t *)"+DOWNLOAD:OK\r\n");
				
				if (SerialDownload() == COM_OK)
				{
						SetFalshRun_Flag(RUN_APP_FLAG); 
				}	
				else 
				{
						FLASH_If_Erase(APPLICATION_ADDRESS, FLASH_APP_RUNUING_END);
					  FLASH_IF_COPY((uint32_t*)FLASH_APP_BACKUP_START, APPLICATION_ADDRESS, USER_APP_SIZE);
				}
				HAL_Delay(100); 
				HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
				Iap_Load_App();
		}
#else
		if ((*(__IO uint32_t *)IAP_FLASH_MARKADDESS) == RUN_DOWNLOAD_FLAG)
		{
				/* Download user application in the Flash */
				Serial_PutString((uint8_t *)"+DOWNLOAD:OK\r\n");
				if (SerialDownload() == COM_OK)
				{
						SetFalshRun_Flag(RUN_APP_FLAG); 
						HAL_Delay(100); 
						Iap_Load_App();
				}	
		}
#endif
		else if ((*(__IO uint32_t *)IAP_FLASH_MARKADDESS) == RUN_UPLOAD_FLAG)
		{
				/* Upload user firmware from the Flash */
				SetFalshRun_Flag(RUN_APP_FLAG); 
				Serial_PutString((uint8_t *)"+UPLOAD:OK\r\n");
				SerialUpload();
				HAL_Delay(100); 
				Iap_Load_App();
		}
		else
		{
				Serial_PutString((uint8_t *)"Invalid data\r\n");
		}
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
