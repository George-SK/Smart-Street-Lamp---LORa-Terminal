#include "internal-eeprom.h"
#include "board.h"
#if defined (STM32L073xx) || defined (STM32L072xx)
#include "stm32l0xx_hal_flash.h"
#include "stm32l0xx_hal_flash_ex.h"
#else
#include "stm32l1xx_hal_flash.h"
#include "stm32l1xx_hal_flash_ex.h"
#endif

/* Private typedef -----------------------------------------------------------*/
typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;

/* Private define ------------------------------------------------------------*/
#define DATA_EEPROM_PAGE_SIZE      0x8
#define DATA_32                    0x12345678
#define FAST_DATA_32               0x55667799

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO HAL_StatusTypeDef FLASHStatus = HAL_OK;
__IO TestStatus DataMemoryProgramStatus = PASSED;
uint32_t NbrOfPage = 0, j = 0, Address = 0;

bool EraseMcuEEPROMByWords(uint32_t StartAddr, uint32_t size)
{
	TestStatus FlashStatus = PASSED;
	uint32_t TmpAddr;
	
	HAL_FLASHEx_DATAEEPROM_Unlock();
	
	/* Clear all pending flags */      
  //__HAL_FLASH_GET_FLAG(FLASH_FLAG_EOP|FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | FLASH_FLAG_OPTVERR);
	
	TmpAddr = StartAddr;

  /* Erase the Data EEPROM Memory pages by Word (32-bit) */
  for(uint32_t i = 0; i < size; i++)
  {
#if defined (STM32L073xx) || defined (STM32L072xx)
		FLASHStatus = HAL_FLASHEx_DATAEEPROM_Erase(TmpAddr + (4 * i));
#else
		FLASHStatus = HAL_FLASHEx_DATAEEPROM_Erase(FLASH_TYPEERASEDATA_WORD, TmpAddr + (4 * i));
#endif
  }
   
  /* Check the correctness of written data */
  for(uint32_t i = 0; i < size; i++)
  {
    if(*(__IO uint32_t*)TmpAddr != 0x0)
    {
      FlashStatus = FAILED;
    }
		
    if(TmpAddr < 0x08080FFF)
				TmpAddr = TmpAddr + 4;
  }
	
	HAL_FLASHEx_DATAEEPROM_Lock();
	
	if(FlashStatus == FAILED)
		return false;
	
	return true;
}

bool EraseMcuEEPROM(void)
{
	TestStatus FlashStatus = PASSED;
	
	HAL_FLASHEx_DATAEEPROM_Unlock();
	
	/* Clear all pending flags */      
  //__HAL_FLASH_GET_FLAG(FLASH_FLAG_EOP|FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | FLASH_FLAG_OPTVERR);
	
	Address = DATA_EEPROM_START_ADDR;

  NbrOfPage = ((DATA_EEPROM_END_ADDR - Address) + 1 ) >> 2; 
  
  /* Erase the Data EEPROM Memory pages by Word (32-bit) */
  for(j = 0; j < NbrOfPage; j++)
  {
#if defined (STM32L073xx) || defined (STM32L072xx)
		FLASHStatus = HAL_FLASHEx_DATAEEPROM_Erase(Address + (4 * j));
#else
		FLASHStatus = HAL_FLASHEx_DATAEEPROM_Erase(FLASH_TYPEERASEDATA_WORD, Address + (4 * j));
#endif
  }
   
  /* Check the correctness of written data */
  while(Address < DATA_EEPROM_END_ADDR)
  {
    if(*(__IO uint32_t*)Address != 0x0)
    {
      FlashStatus = FAILED;
    }
    Address = Address + 4;
  }
	
	HAL_FLASHEx_DATAEEPROM_Lock();
	
	if(FlashStatus == FAILED)
		return false;
	
	return true;
}

bool WriteWordsToMcuEEPROM(uint32_t StartAddr, uint32_t *data, uint32_t size)
{
	uint32_t TmpAddr;
	
	TmpAddr = StartAddr;
	
	HAL_FLASHEx_DATAEEPROM_Unlock();
	
	/* Program the Data EEPROM Memory pages by Word (32-bit) */
	for(uint32_t i = 0; i < size; i++)
	{
#if defined (STM32L073xx) || defined (STM32L072xx)
		FLASHStatus = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, TmpAddr, data[i]);
#else
		FLASHStatus = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTWORD, TmpAddr, data[i]);
#endif

		if(FLASHStatus == HAL_OK)
		{
			if(TmpAddr < 0x08080FFF)
				TmpAddr = TmpAddr + 4;
			else
			{
				HAL_FLASHEx_DATAEEPROM_Lock();
				Uart1.PrintfLog("Overflow!\r\n");
				return false;
			}
		}
		else
		{
			HAL_FLASHEx_DATAEEPROM_Lock();
			Uart1.PrintfLog("Write failed:%d\r\n", FLASHStatus);
			return false;
		}
	}

	HAL_FLASHEx_DATAEEPROM_Lock();
	return true;
}

bool WriteMcuEEPROM(uint32_t *data, uint32_t size)
{
	EraseMcuEEPROM();
	
	Address = DATA_EEPROM_START_ADDR;  
  
	HAL_FLASHEx_DATAEEPROM_Unlock();
	
	/* Program the Data EEPROM Memory pages by Word (32-bit) */
	for(uint32_t i = 0; i < size; i++)
	{
#if defined (STM32L073xx) || defined (STM32L072xx)
		FLASHStatus = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, Address, data[i]);
#else
		FLASHStatus = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTWORD, Address, data[i]);
#endif

		if(FLASHStatus == HAL_OK)
		{
			if(Address < DATA_EEPROM_END_ADDR)
				Address = Address + 4;
			else
			{
				HAL_FLASHEx_DATAEEPROM_Lock();
				Uart1.PrintfLog("Overflow!\r\n");
				return false;
			}
		}
		else
		{
			HAL_FLASHEx_DATAEEPROM_Lock();
			Uart1.PrintfLog("Write failed!\r\n");
			return false;
		}
	}

	HAL_FLASHEx_DATAEEPROM_Lock();
	return true;
}

void ReadWordsFromMcuEEPROM(uint32_t StartAddr, uint32_t *data, uint32_t size)
{
	uint32_t i = 0, TmpAddr;
	
	TmpAddr = StartAddr;
	
	HAL_FLASHEx_DATAEEPROM_Unlock();
  
  /* Check the correctness of written data */
  while(i < size)
  {
    data[i++] = *(__IO uint32_t*)TmpAddr;
		
	  TmpAddr = TmpAddr + 4;
  }
	
	HAL_FLASHEx_DATAEEPROM_Lock();
}

void ReadMcuEEPROM(uint32_t *data, uint32_t *size)
{
	uint32_t i = 0;
	
	Address = DATA_EEPROM_START_ADDR;
	
	HAL_FLASHEx_DATAEEPROM_Unlock();
  
  /* Check the correctness of written data */
  while(Address < DATA_EEPROM_END_ADDR)
  {
    if(*(__IO uint32_t*)Address != 0x0)
    {
      data[i++] = *(__IO uint32_t*)Address;
    }
		else
			break;
		
	  Address = Address + 4;
  }
	
	*size = i;
	
	HAL_FLASHEx_DATAEEPROM_Lock();
}

bool WriteBytesToMcuEEPROM(uint32_t StartAddr, uint8_t *data, uint32_t size)
{
  uint32_t DataToSave[64];
  bool WriteStatus;
  
  if(size > 64*4)
    return false;
  
  memset(DataToSave, 0, 64*4);
  memcpy(DataToSave, data, size);
  
  WriteStatus = WriteWordsToMcuEEPROM(StartAddr, DataToSave, size/4 + (size%4!=0));
  
  return WriteStatus;
}

void ReadBytesFromMcuEEPROM(uint32_t StartAddr, uint8_t *data, uint32_t size)
{
  uint32_t DataToRead[64];
  
  if(size > 64*4)
    return;
  
  memset(DataToRead, 0, 64*4);  
  
  ReadWordsFromMcuEEPROM(StartAddr, DataToRead, size/4 + (size%4!=0));  //Modified by Steven in 2018/04/11.
  
  memcpy(data, DataToRead, size);
}

void InternalEEPROMTest(void)
{
	HAL_FLASHEx_DATAEEPROM_Unlock();
	
	/* Clear all pending flags */      
  //__HAL_FLASH_GET_FLAG(FLASH_FLAG_EOP|FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | FLASH_FLAG_OPTVERR);
	
	Address = DATA_EEPROM_START_ADDR;

  NbrOfPage = ((DATA_EEPROM_END_ADDR - Address) + 1 ) >> 2; 
  
  /* Erase the Data EEPROM Memory pages by Word (32-bit) */
  for(j = 0; j < NbrOfPage; j++)
  {
#if defined (STM32L073xx) || defined (STM32L072xx)
		FLASHStatus = HAL_FLASHEx_DATAEEPROM_Erase(Address + (4 * j));
#else    
		FLASHStatus = HAL_FLASHEx_DATAEEPROM_Erase(FLASH_TYPEERASEDATA_WORD, Address + (4 * j));
#endif
  }
   
  /* Check the correctness of written data */
  while(Address < DATA_EEPROM_END_ADDR)
  {
    if(*(__IO uint32_t*)Address != 0x0)
    {
      DataMemoryProgramStatus = FAILED;
    }
    Address = Address + 4;
  }
	
	if(DataMemoryProgramStatus != PASSED)
		Uart1.PrintfLog("Internal eeprom test failed!\r\n");
	else
		Uart1.PrintfLog("Internal eeprom test success!\r\n");
}

//George@20180713:Erase multcast address
void EraseMultcastAddress(void)
{
		EraseMcuEEPROMByWords(DATA_EEPROM_START_ADDR, (DATA_EEPROM_END_ADDR - DATA_EEPROM_START_ADDR)/4);
}

