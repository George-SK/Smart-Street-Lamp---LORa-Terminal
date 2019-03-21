/**
  ******************************************************************************
  * @file    IAP_Main/Src/flash_if.c 
  * @author  MCD Application Team
  * @brief   This file provides all the memory related operation functions.
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
#include "flash_if.h"
#include "common.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Unlocks Flash for write access
  * @param  None
  * @retval None
  */
void FLASH_If_Init(void)
{
  /* Unlock the Program memory */
  HAL_FLASH_Unlock();

  /* Clear all FLASH flags */
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP|FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | 
	                       FLASH_FLAG_SIZERR | FLASH_FLAG_OPTVERR);   
	
  /* Unlock the Program memory */
  HAL_FLASH_Lock();
}

/**
  * @brief  This function does an erase of all user flash area
  * @param  start: start of user flash area
  * @retval FLASHIF_OK : user flash area successfully erased
  *         FLASHIF_ERASEKO : error occurred
  */
uint32_t FLASH_If_Erase(uint32_t start, uint32_t end)
{
  FLASH_EraseInitTypeDef desc;
  uint32_t result = FLASHIF_OK;

  HAL_FLASH_Unlock();

  desc.PageAddress = start;
  desc.TypeErase = FLASH_TYPEERASE_PAGES;

/* NOTE: Following implementation expects the IAP code address to be < Application address */  
	if (start < end)
	{
		desc.NbPages = (end - start) / FLASH_PAGE_SIZE;
    if (HAL_FLASHEx_Erase(&desc, &result) != HAL_OK)
    {
      result = FLASHIF_ERASEKO;
    }
	}
  
  HAL_FLASH_Lock();

  return result;
}	

/* Public functions ---------------------------------------------------------*/
/**
  * @brief  This function writes a data buffer in flash (data are 32-bit aligned).
  * @note   After writing data buffer, the flash content is checked.
  * @param  destination: start address for target location
  * @param  p_source: pointer on buffer with data to write
  * @param  length: length of data buffer (unit is 32-bit word)
  * @retval uint32_t 0: Data successfully written to Flash memory
  *         1: Error occurred while writing data in Flash memory
  *         2: Written Data in flash memory is different from expected one
  */
uint32_t FLASH_If_Write(uint32_t destination, uint32_t *p_source, uint32_t length)
{
  uint32_t status = FLASHIF_OK;
  uint32_t *p_actual = p_source; /* Temporary pointer to data that will be written in a half-page space */
  uint32_t i = 0;

  HAL_FLASH_Unlock();
	
	#if 0
	while (p_actual < (uint32_t*)(p_source + length))
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, destination, *p_actual);
		destination += 4;
		p_actual++;
	}	
	#else
  while (p_actual < (uint32_t*)(p_source + length))
  {    
    /* Write the buffer to the memory */
    if (HAL_FLASHEx_HalfPageProgram( destination, p_actual ) == HAL_OK) /* No error occurred while writing data in Flash memory */
    {
      /* Check if flash content matches memBuffer */
      for (i = 0; i < WORDS_IN_HALF_PAGE; i++)
      {
        if ((*(uint32_t*)(destination + 4 * i)) != p_actual[i])
        {
          /* flash content doesn't match memBuffer */
          status = FLASHIF_WRITINGCTRL_ERROR;
          break;
        }
      }

      /* Increment the memory pointers */
      destination += FLASH_HALF_PAGE_SIZE;
      p_actual += WORDS_IN_HALF_PAGE;
    }
	  else
	  {
		  status = FLASHIF_WRITING_ERROR;
	  }

    if ( status != FLASHIF_OK )
    {
      break;
    }
  }
	#endif

  HAL_FLASH_Lock();

  return status;
}	

/**
  * @brief  FLASH backup function for IAP upgrade
  * @param  Source address, target address, file length
  * @retval uint32_t FLASHIF_OK if change is applied.
  */
//FLASH copy app code(16~102k) ==> (106~192k)
uint32_t FLASH_IF_COPY(uint32_t *srcaddr, uint32_t destaddr, uint32_t file_size)
{
	uint32_t status = FLASHIF_OK;
	uint32_t SIZE_1K = 1024;
	uint32_t p_packet[256];
	
	while ((file_size) && (status == FLASHIF_OK ))
  {
			for (int i = 0; i < SIZE_1K/4; i++)
			{
				p_packet[i] = *(__IO uint32_t*)srcaddr++;
				//printf("%08X ", p_packet[i]);
			}
			
			if (FLASH_If_Write(destaddr, p_packet, SIZE_1K/4) == FLASHIF_OK)                   
      {
				HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin); //Indicator light 
				destaddr += SIZE_1K;
			}
			else
			{
				 printf("Write flash error!!!\r\n");
				 status = FLASHIF_WRITING_ERROR;
				 break;
			}
			file_size -= SIZE_1K;
	}
	
	return status;
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
