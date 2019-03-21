#ifndef _INTERNAL_EEPROM_H_
#define _INTERNAL_EEPROM_H_

#include "stdbool.h"
#include "stdint.h"

//Only for muticast!!!
#define DATA_EEPROM_START_ADDR      ((uint32_t)0x08080000)
#define DATA_EEPROM_END_ADDR        ((uint32_t)0x080807FF)

#define RN820X_FLASH_START				  ((uint32_t)0x08080800)
#define RN820X_FLASH_END					  ((uint32_t)0x08080900)

#define	PROFILE_FLASH_START				  ((uint32_t)0x08080904)
#define	PROFILE_FLASH_END					  ((uint32_t)0x08080A00)

#define TX_TIME_FLASH_START    		  ((uint32_t)0x08080A04)
#define TX_TIME_FLASH_END    	      ((uint32_t)0x08080A08)

#define DEVICE_CONTRAL_MODE_START   ((uint32_t)0x08080A0C)
#define DEVICE_CONTRAL_MODE_END     ((uint32_t)0x08080A10)

#define KWH_FLASH_STATR						  ((uint32_t)0x08080A20)
#define KWH_FLASH_END						    ((uint32_t)0x08080AA0)

#define FAULT_PARAM_FLASH_START		  ((uint32_t)0x08080AA4)
#define FAULT_PARAM_FLASH_STOP		  ((uint32_t)0x08080B08)

#define BOOT_FIRMWARE_VERSION_START ((uint32_t)0x08080B10)
#define BOOT_FIRMWARE_VERSION_END	  ((uint32_t)0x08080B14)

#define APP_FIRMWARE_VERSION_START  ((uint32_t)0x08080B18)
#define APP_FIRMWARE_VERSION_END		((uint32_t)0x08080B1C)

#define LIGHT_DIMMING_MODE_START    ((uint32_t)0x08080B20) 
#define LIGHT_DIMMING_MODE_END      ((uint32_t)0x08080B24) 

bool EraseMcuEEPROM(void);
bool WriteMcuEEPROM(uint32_t *data, uint32_t size);
void ReadMcuEEPROM(uint32_t *data, uint32_t *size);
bool EraseMcuEEPROMByWords(uint32_t StartAddr, uint32_t size);
bool WriteWordsToMcuEEPROM(uint32_t StartAddr, uint32_t *data, uint32_t size);
void ReadWordsFromMcuEEPROM(uint32_t StartAddr, uint32_t *data, uint32_t size);
bool WriteBytesToMcuEEPROM(uint32_t StartAddr, uint8_t *data, uint32_t size);
void ReadBytesFromMcuEEPROM(uint32_t StartAddr, uint8_t *data, uint32_t size);
void InternalEEPROMTest(void);

#endif
