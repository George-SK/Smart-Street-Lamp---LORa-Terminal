#include "upgrad.h"
#include "test.h"
#include "macro-definition.h"
#include "Region.h"

extern RN820x_t RN820xInfo; 
extern TimerEvent_t NetLedStateTimer;

extern uint8_t DevEui[];
extern uint8_t AppEui[];
extern uint8_t AppKey[];

bool ATcommend_flag = false;
bool AutoTestMode_flag = false;
char RegionFB[15];
char strVersion[50];

//George@20180607:save data to flash 
void SetDataToFalsh(uint32_t address, uint32_t data)
{
  FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t PAGEError = 0;
	
	HAL_FLASH_Unlock();
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = IAP_FLASH_MARKADDESS;
	EraseInitStruct.NbPages     = 1;

	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
		Uart1.PrintfLog("FLASH Erase Fail...\r\n");

	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data) != HAL_OK)
		Uart1.PrintfLog("FLASH Write Fail...\r\n");

	HAL_FLASH_Lock();
}

//George@20180713:get data from flash
uint32_t GetWordDataFromFlash(uint32_t address)
{
	uint32_t data;
	data = *(__IO uint32_t *)address;
	return data;
}

//George@20080829:get software version
void GetSoftwareVersion(void)
{
	char LightContral[10];
	
#if   defined( REGION_CN470 ) 
	strcpy(RegionFB, "CN470");
#elif defined( REGION_CN779 ) 
	strcpy(RegionFB, "CN779");
#elif defined( REGION_EU433 ) 
	strcpy(RegionFB, "EU433");
#elif defined( REGION_EU868 )
	strcpy(RegionFB, "EU868");
#elif defined( REGION_IN865 ) 
	strcpy(RegionFB, "IN865");
#elif	defined( REGION_KR920 )
	strcpy(RegionFB, "KR920");
#elif defined( REGION_AS923 ) 
	strcpy(RegionFB, "AS923");
#elif defined( REGION_AU915 ) 
	strcpy(RegionFB, "AU915");
#elif defined( REGION_US915 ) 
	strcpy(RegionFB, "US915");
#elif defined( REGION_US915_HYBRID )
	strcpy(RegionFB, "US915_HYBRID");
#else 
	#error "No Region"
#endif
	
	if (LightDimmingMode == DAC_dimming_mode)
		strcpy(LightContral, "0~10V_");
	else if (LightDimmingMode == PWM_dimming_mode)
		strcpy(LightContral, "PWM_");
	else if (LightDimmingMode == DALI_dimming_mode)
		strcpy(LightContral, "DALI_");
	else
		strcpy(LightContral, "0~10_");
	
	strncpy(strVersion, APP_VERSION, (strlen(APP_VERSION)-6));
	strcat(strVersion+(strlen(APP_VERSION)-6), RegionFB);
	strcat(strVersion+(strlen(APP_VERSION)-6)+(strlen(RegionFB)), "_");
	strcat(strVersion+(strlen(APP_VERSION)-6)+(strlen(RegionFB))+1, LightContral);
	strcpy(strVersion+(strlen(APP_VERSION)-6)+(strlen(RegionFB))+(strlen(LightContral))+1, APP_VERSION+(strlen(APP_VERSION)-6));
}

//George@20180607:IAP
void UpgradeProgram(uint8_t *RxBuffer, uint8_t Length)
{	
	uint8_t DEVEUI[8];
	uint32_t DataLen, DataBuf[100];
	uint32_t TxReportTime;
	uint8_t profileType;
	
	if (memcmp(RxBuffer, "AT+", 3) == 0)
	{
			ATcommend_flag = true;
			if (memcmp(RxBuffer+3, "DOWNLOAD", Length-3) == 0 && Length == strlen("AT+DOWNLOAD")) 
			{
					SetDataToFalsh(IAP_FLASH_MARKADDESS, RUN_DOWNLOAD_FLAG);
					NVIC_SystemReset();
			}
			else if (memcmp(RxBuffer+3, "DOWNLOAD_BOOT", Length-3) == 0 && Length == strlen("AT+DOWNLOAD_BOOT")) 
			{

			}
			else if (memcmp(RxBuffer+3, "UPLOAD", Length-3) == 0 && Length == strlen("AT+UPLOAD"))
			{
					SetDataToFalsh(IAP_FLASH_MARKADDESS, RUN_UPLOAD_FLAG);
					NVIC_SystemReset();
			}
			else if (memcmp(RxBuffer+3, "GETID", Length-3) == 0 && Length == strlen("AT+GETID"))
			{
					BoardGetUniqueId(DEVEUI);
					for(uint8_t i = 0; i < 8; i++)
					{
						AppKey[i] = DEVEUI[i];
					}
				  Uart1.PrintfLog("+GETID:%02X", DEVEUI[0]);
					for (int i = 1; i < 8; i++) 
					{
							Uart1.PrintfLog("-%02X", DEVEUI[i]);
					}
					Uart1.PrintfLog(",");
					
					Uart1.PrintfLog("%02X", AppEui[0]); 
					for (int i = 1; i < 8 ; i++) 
          { 
							Uart1.PrintfLog("-%02X", AppEui[i]); 
					}
					Uart1.PrintfLog(",");
								
					for (int i = 0; i < 16; i++) 
					{ 
							Uart1.PrintfLog(" %02X", AppKey[i]); 
					}
					Uart1.PrintfLog(",");
					Uart1.PrintfLog("%s\r\n", RegionFB);
		}
		else if (memcmp(RxBuffer+3, "GETSV", Length-3) == 0 && Length == 8)
		{
				Uart1.PrintfLog("+GETSV:%s\r\n", strVersion);
		}
		else if (memcmp(RxBuffer+3, "PDTEST=", Length-4) == 0 && Length == 11)
		{
				if (RxBuffer[10] == '0')
				{
						Uart1.PrintfLog = printf;
						AutoTestMode_flag = false;
						Uart1.PrintfLog("+PDTEST:OFF\r\n");
				}
				else if (RxBuffer[10] == '1')
				{
						Uart1.PrintfLog("+PDTEST:ON\r\n");
						SetBrightness(100);
						Uart1.PrintfLog = EmptyFunction;
						AutoTestMode_flag = true;
						TimerStart( &NetLedStateTimer );
				}
				else
						Uart1.PrintfLog("AT PARAM ERROR\r\n");
		}
		else if (memcmp(RxBuffer+3, "CEPROM", Length-3) == 0 && Length == 9)
		{
			  /*clear multcast¡¢profile¡¢kwh¡¢txtime*/
				EraseMultcastAddress(); 
				RemoveProfile();
				EraseTxCycleTime();
				for(int i=0; i<3; i++)
				{
					ClearKwhInFlash();
					GetRn820xParam(&RN820xInfo);
					if (RN820xInfo.kWh <= 0.005)
						break;
				}
			
			  GetRn820xParam(&RN820xInfo);
				ReadMcuEEPROM(DataBuf, &DataLen);
				profileType = GetProfileFlash();
				ReadWordsFromMcuEEPROM(TX_TIME_FLASH_START, &TxReportTime, 1);
			
				if (DataLen == 0 && profileType == 0 && RN820xInfo.kWh <=0.005 && TxReportTime == 0)
						Uart1.PrintfLog("+CEPROM:OK\r\n");
				else
					  Uart1.PrintfLog("+CEPROM:FAIL\r\n");
		}
		else if (memcmp(RxBuffer+3, "RESET", Length-3) == 0 && Length == strlen("AT+RESET"))
		{	
				Uart1.PrintfLog("System reset\r\n\r\n");
				NVIC_SystemReset();
		}
		else if (memcmp(RxBuffer+3, "DIMMING=", Length-4) == 0 && Length == strlen("AT+DIMMING=1"))
		{
				uint8_t DimmingMode = 0;
			
				switch (RxBuffer[11])
				{
						case '0': DimmingMode = 0; break;
						case '1': DimmingMode = 1; break;
						case '2': DimmingMode = 2; break;
						default : Uart1.PrintfLog("AT PARAM ERROR\r\n"); break;
				}		
				
				SetDataToEEPROM(LIGHT_DIMMING_MODE_START, 0x00, DimmingMode);
				Uart1.PrintfLog("+DIMMING:OK\r\n\r\n");
				HAL_Delay(100);
				NVIC_SystemReset();
		}
		else if (memcmp(RxBuffer+3, "SEND=", Length-4) == 0 && Length == strlen("AT+SEND=0"))
		{
				frame_t frame;
				MibRequestConfirm_t mibReq;
				uint8_t DR = 0, DS = 51;
			
				switch(RxBuffer[8])
				{
						case '0': DR = 0; DS = 45; break;
						case '1': DR = 1; DS = 45; break;
						case '2': DR = 2; DS = 45; break;
						case '3': DR = 3; DS = 90; break;
						case '4': DR = 4; DS = 150; break;
						case '5': DR = 5; DS = 150; break;
						case '6': DR = 6; DS = 150; break;
						default : DR = 0; break;
				}
			
				mibReq.Type = MIB_CHANNELS_DATARATE;
				mibReq.Param.ChannelsDatarate = DR;;
				LoRaMacMibSetRequestConfirm( &mibReq );

				frame.port = FOTA_REPORT_DATA_PORT;
				frame.IsTxConfirmed = 0;
				frame.nbRetries = 3;
				frame.AppDataSize = DS+4;
				Uart1.PrintfLog("\r\n--------------------\r\n");
				for (int i=1; i<=frame.AppDataSize; i++)
				{
						frame.AppData[i] = i;
						Uart1.PrintfLog("%d ", frame.AppData[i]);
				}
				Uart1.PrintfLog("\r\n--------------------\r\n");
				InsertOneFrame(&frame);
		}
	}
}

void System_SoftReset(uint8_t *pdata, uint8_t size)
{
		if (size >= 9 && pdata[0] == 0x08)
		{
				if (memcmp(pdata+1, DevEui, 8) == 0)
				{
						Uart1.PrintfLog("System reset\r\n\r\n");
						NVIC_SystemReset();
				}
		}			
		else if (size >= 6 && pdata[0] == 0x09)
		{
				if (memcmp(pdata+1, "RESET", 5) == 0)
				{
						Uart1.PrintfLog("System reset\r\n\r\n");
						NVIC_SystemReset();
				}
		}
		else
		{
				Uart1.PrintfLog("System reset param error\r\n");
		}
}

void Check_MulticastAddr(uint8_t *pdata, uint8_t size)
{
		frame_t frame;
		uint8_t Tbuff[50];
		uint8_t Dlen;
	
		if (size >= 1 && pdata[0] == 0x01)
		{
				Dlen = GetMulticastInfo(Tbuff);
				
				frame.port = CHEAK_CUR_MULTICAST_PORT;
				frame.IsTxConfirmed = 1;
				frame.nbRetries = 3;
				frame.AppDataSize = Dlen;
				memcpy(frame.AppData, Tbuff, Dlen);
				InsertOneFrame(&frame);
			
#if 1
				for (int i=0; i<Dlen; i++)
				{
						if (i % 10 == 0)
							Uart1.PrintfLog("\r\n");
 						Uart1.PrintfLog("%02X ", frame.AppData[i]);
				}

				Uart1.PrintfLog("\r\n");
#endif
		}
}

//end
