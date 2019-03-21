#include "fota.h"
#include "upgrad.h"
#include "macro-definition.h"
#include "Region.h"

#if defined (FOTA_ENABLE)

extern void SetTxDutyCycle(uint32_t time);
extern void OffLineTimerReset(void);
extern TimerEvent_t TxNextPacketTimer;
extern TimerEvent_t DeviceOfflineTimer;
extern uint8_t AppEui[];

static bool ClassAMode_flag = false;
static uint32_t RX2_FQ;
static uint8_t RX2_DR;

TimerEvent_t SendFOTAStateTimer;
TimerEvent_t ReceivePackTimeoutTimer;
TimerEvent_t SwitchWorkModeTimer;

//George@20181011:FOTA struct init
FOTA_t FirmUpgrade = {
	.FotaFlag = false,
	.GetBootVerFlag = false,
	.GetAppVerFlag = false,
	.UpdateBootFlag = false,
	.FotaState = FOTA_IDLE,
	.FotaSize = 0,
	.PackTotal = 0,
	.PackLen = 0,
	.MaxReissuePack = 0,
	.FrameNum = 0, 
	.TempNum = 0,
	.LossPackTotal = 0,
	.FrameSum = 0,
	.DataLen = 0,
	.DestAddr = 0,
};

static bool WriteFlashByHalfPage(uint32_t destination, uint32_t *p_source, uint32_t length);
static bool WriteFlashByWord(uint32_t destaddr, uint32_t *p_source, uint32_t length);
static void ReadFlashByWord(uint32_t destaddr, uint32_t *p_source, uint32_t length);
static bool FLASH_If_Erase(uint32_t start, uint32_t end);

//George@20181109:flash backup
static bool FLASH_IF_COPY(uint32_t *srcaddr, uint32_t destaddr, uint32_t file_size)
{
	bool status = true;
	uint16_t SIZE_1K = 1024;
	uint32_t p_packet[256];
	
	while ((file_size) && (status == true ))
  {
			for (int i = 0; i < SIZE_1K/4; i++)
			{
					p_packet[i] = *(__IO uint32_t*)srcaddr++;
					//Uart1.PrintfLog("%08X ", p_packet[i]);
			}
			//Uart1.PrintfLog("\r\n");
			
			if (WriteFlashByWord(destaddr, p_packet, SIZE_1K/4) == true)                   
      {
					HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_5); //Indicator light 
					destaddr += SIZE_1K;
			}
			else
			{
				  Uart1.PrintfLog("Write flash error!!!\r\n");
				  status = false;
				  break;
			}
			file_size -= SIZE_1K;
	}
	return status;
}

//George@20181108:bootloader firmware copy
static void UpdataBootFirmware(void)
{
		FLASH_If_Erase(FLASH_BOOT_RUNING_START, FLASH_BOOT_RUNING_END);
		while (FLASH_IF_COPY((uint32_t*)FLASH_BOOT_BACKUP_START, FLASH_BOOT_RUNING_START, USER_BOOT_SIZE) != true)
		{
				Uart1.PrintfLog("FLASH copy failed wait next copy!!!\r\n");
				HAL_Delay(100);
		}
}

//George@20181018:The array element looks for the delete function
static void ArrayDeleteProcess(uint16_t *inBuf, uint16_t *Num, uint16_t Delete)
{
		uint16_t FrameIndex;
		uint16_t i = 0, j = 0;
	
		for (i=0; i<*Num; i++)
		{
				if (Delete == inBuf[i])
				{
						FrameIndex = i;
						break;
				}
		}

		for (j=FrameIndex; j<*Num-1; j++)
		{
				inBuf[j] = inBuf[j+1];
		}
		
		*Num -= 1;
}

//George@20181016:data process
static uint16_t DataProcess(uint16_t *inBuf, uint8_t *outBuf, uint16_t Num)
{
		uint16_t i = 0, j = 0;
	
		while(j < Num)
		{
				outBuf[i+0] = (inBuf[j] & 0x0ff0)>>4;
				outBuf[i+1] = (inBuf[j] & 0x000f)<<4 | (inBuf[j+1] & 0x0f00)>>8;
				outBuf[i+2] = (inBuf[j+1] & 0x00ff);
			
				i += 3;
				j += 2;
		}
		
		return i;
}

//George@20181011:report timer
static void SetandStartReportTimer(uint32_t tim)
{
		TimerSetValue(&SendFOTAStateTimer, randr(20, tim));	//20ms~XXms.
		TimerStart(&SendFOTAStateTimer);
}

//George@20181116:switch work mode event
static void SwitchWorkModeEvent(void)
{
		MibRequestConfirm_t mibReq;
	
		mibReq.Type = MIB_DEVICE_CLASS;
		mibReq.Param.Class = CLASS_C;
		LoRaMacMibSetRequestConfirm( &mibReq );
		Uart1.PrintfLog("ClassA switch to ClassC successfully\r\n");
	
		TimerInit(&ReceivePackTimeoutTimer, ReceivePackTimeoutEvent); 
		TimerSetValue(&ReceivePackTimeoutTimer, FOTA_RECEIVE_TIMEOUT); 
		TimerStart(&ReceivePackTimeoutTimer);
}

//George@20181011:receive pack timeout event
void ReceivePackTimeoutEvent(void)
{
		FirmUpgrade.FotaState = FOTA_TIMEOUT;
		SetandStartReportTimer(FOTA_REPORT_STATE_TIME);	
		Uart1.PrintfLog("Receive pack timeout exit FOTA!!!\r\n");
}

//George@20181011:send lose pack event
void SendFOTAStateEvent(void)
{
		uint8_t tempBuf[FOTA_TEMP_BUFFER_SIZE];
		frame_t frame;
		MibRequestConfirm_t mibReq;
		uint8_t version[8];
		uint16_t count = 0;
		uint8_t payloadSize;
	
		if (FirmUpgrade.GetBootVerFlag == true || FirmUpgrade.GetAppVerFlag == true)
		{
				if (FirmUpgrade.GetBootVerFlag == true)
				{
						FirmUpgrade.GetBootVerFlag = false;
						ReadBytesFromMcuEEPROM(BOOT_FIRMWARE_VERSION_START, version, 6);
				}
				else
				{
						FirmUpgrade.GetAppVerFlag = false;
						ReadBytesFromMcuEEPROM(APP_FIRMWARE_VERSION_START, version, 6);
				}

				frame.port = FOTA_GET_VERSION_PORT;
				frame.IsTxConfirmed = 1;
				frame.nbRetries = 8;
				frame.AppDataSize = 15;
				frame.AppData[0] = FOTA_REP;
				memcpy(frame.AppData+1, AppEui, 8);
				memcpy(frame.AppData+9, version, 6);
				InsertOneFrame(&frame);
		}
		else if (FirmUpgrade.FotaState == FOTA_SUCCESS || FirmUpgrade.FotaState == FOTA_FAILED || FirmUpgrade.FotaState == FOTA_TOO_LOSS ||
						 FirmUpgrade.FotaState == FOTA_TIMEOUT || FirmUpgrade.FotaState == FOTA_NO_START || FirmUpgrade.FotaState == FOTA_OUTDO_SIZE ||
						 FirmUpgrade.FotaState == FOTA_NO_APPEUI || FirmUpgrade.FotaState == FOTA_NO_VERSION || FirmUpgrade.FotaState == FOTA_PACKET_ERROR)
		{	
				
				FirmUpgrade.FotaFlag = false;
				TimerStop(&ReceivePackTimeoutTimer);
			
				if (FirmUpgrade.FotaState != FOTA_SUCCESS)
				{
						StreetLightParam.TxCycleCount = 1;	
						SetTxDutyCycle(randr( 6000, StreetLightParam.TxCycleTimeLag[0] * 60000 )); //6s~2min
						OffLineTimerReset();

#if 0					
						mibReq.Type = MIB_RX2_CHANNEL;
            mibReq.Param.Rx2Channel = ( Rx2ChannelParams_t ){ RX2_FQ, RX2_DR };
            LoRaMacMibSetRequestConfirm( &mibReq );
#endif
						
						if (ClassAMode_flag == true)
						{
								mibReq.Type = MIB_DEVICE_CLASS;
								mibReq.Param.Class = CLASS_A;
								LoRaMacMibSetRequestConfirm( &mibReq );
								Uart1.PrintfLog("FOTA failed,ClassC switch to ClassA successfully\r\n");
						}
				}
				
				frame.port = FOTA_REPORT_DATA_PORT;
				frame.IsTxConfirmed = 1;
				frame.nbRetries = 8;
				frame.AppDataSize = 2;
				frame.AppData[0] = FOTA_ACK;
				switch(FirmUpgrade.FotaState)
				{
						case FOTA_SUCCESS:    	frame.AppData[1] = FOTA_SUCCESS;   	  break;
						case FOTA_FAILED:     	frame.AppData[1] = FOTA_FAILED;    	  break;
						case FOTA_TOO_LOSS:   	frame.AppData[1] = FOTA_TOO_LOSS;  	  break;
						case FOTA_TIMEOUT:    	frame.AppData[1] = FOTA_TIMEOUT;    	break; 
						case FOTA_NO_START: 		frame.AppData[1] = FOTA_NO_START;   	break; 
						case FOTA_OUTDO_SIZE: 	frame.AppData[1] = FOTA_OUTDO_SIZE; 	break; 
						case FOTA_NO_APPEUI:  	frame.AppData[1] = FOTA_NO_APPEUI;  	break; 
						case FOTA_NO_VERSION: 	frame.AppData[1] = FOTA_NO_VERSION; 	break; 
						case FOTA_PACKET_ERROR: frame.AppData[1] = FOTA_PACKET_ERROR; break; 
						default : break;
				}
				InsertOneFrame(&frame);
		}
		else if (FirmUpgrade.FotaState == FOTA_LOSS)
		{
				memset(tempBuf, 0, FOTA_TEMP_BUFFER_SIZE);
				count = DataProcess(FirmUpgrade.LosePackBuf, tempBuf, FirmUpgrade.LossPackTotal);
			
				frame.port = FOTA_REPORT_DATA_PORT;
				frame.IsTxConfirmed = 1;
				frame.nbRetries = 8;
				frame.AppData[0] = FOTA_ACK;
				frame.AppData[1] = FOTA_LOSS;
				frame.AppData[2] = (FirmUpgrade.LossPackTotal & 0xff00)>>8;
				frame.AppData[3] = (FirmUpgrade.LossPackTotal & 0x00ff);
			
				mibReq.Type = MIB_CHANNELS_DATARATE;
				LoRaMacMibGetRequestConfirm( &mibReq );
				Uart1.PrintfLog("CurDR:%d\r\n", mibReq.Param.ChannelsDatarate);
			
#if defined (REGION_EU868) || (REGION_AS923) || (REGION_CN470) || (REGION_AU915) || (REGION_KR920) || (REGION_EU433) || (REGION_CN779)
				switch (mibReq.Param.ChannelsDatarate)
				{
						case DR_0: 
						case DR_1:
						case DR_2:
								payloadSize = 45;   //30 SF12/125K~SF10/125K
								break;
						case DR_3:
							  payloadSize = 90;   //60 SF9/125K
								break;
						case DR_4: 
						case DR_5:
						case DR_6:
								payloadSize = 150;  //100 SF8/125k~SF7/250K
								break;
						default :
							  payloadSize = 45;   //30
								break;
				}	
#elif defined(REGION_US915)
				switch (mibReq.Param.ChannelsDatarate)
				{
						case DR_0: 
								payloadSize = 6; 
								break;
						case DR_1:
								payloadSize = 45; 
								break;
						case DR_2:
								payloadSize = 105; 
								break;
						case DR_3:
						case DR_4:
							  payloadSize = 150; 
								break;
						default :
							  payloadSize = 6; 
								break;
				}	
#endif
			
				if (count > payloadSize)
				{
						frame.AppDataSize = 4+payloadSize;
						memcpy(frame.AppData+4, tempBuf, payloadSize);
				}
				else
				{
						frame.AppDataSize = 4+count;
						memcpy(frame.AppData+4, tempBuf, count);
				}
				InsertOneFrame(&frame);
		}
		else
		{
				Uart1.PrintfLog("No effective data reporting!!!\r\n");
		}
	
#if 0
		Uart1.PrintfLog("\r\n----------------\r\n");
		for (int i=0; i<frame.AppDataSize; i++)
		{
				Uart1.PrintfLog("%02X ", frame.AppData[i]);
		}
		Uart1.PrintfLog("\r\n----------------\r\n");
#endif					
}

//George@20181011:FOTA flash erase
static bool FLASH_If_Erase(uint32_t start, uint32_t end)
{
		FLASH_EraseInitTypeDef desc;
		uint32_t result = true;

		HAL_FLASH_Unlock();

		desc.PageAddress = start;
		desc.TypeErase = FLASH_TYPEERASE_PAGES;

		if (start < end)
		{
			desc.NbPages = (end - start) / FLASH_PAGE_SIZE;
			if (HAL_FLASHEx_Erase(&desc, &result) != HAL_OK)
				result = false;
		}
		
		HAL_FLASH_Lock();

		return result;
}	

//Geore@20181011:Write flash by half page
static bool WriteFlashByHalfPage(uint32_t destination, uint32_t *p_source, uint32_t length)
{
  bool status = true;
  uint32_t *p_actual = p_source; 
	
	//Uart1.PrintfLog("0x%08X\r\n", destination);

  HAL_FLASH_Unlock();
	
  while (p_actual < (uint32_t*)(p_source + length))
  {    
			if (HAL_FLASHEx_HalfPageProgram( destination, p_actual ) == HAL_OK)
			{
					for (int i = 0; i < 16; i++) //16 words in half page
					{
							if ((*(uint32_t*)(destination + 4 * i)) != p_actual[i])
							{
									Uart1.PrintfLog("Write flash error\r\n");
									status = false;
									break;
							}
					}
					destination += 16*4;
					p_actual += 16;
			}
			else
			{
					status = false;
			}
  }
	
  HAL_FLASH_Lock();
	
  return status;
}	

//Geore@20181011:Write flash by word
static bool WriteFlashByWord(uint32_t destaddr, uint32_t *p_source, uint32_t length)
{
		uint32_t *p_actual = p_source;
		bool Write_flag = true;
	
		//Uart1.PrintfLog("\r\naddress:0x%08X\r\n", destaddr);
		HAL_FLASH_Unlock();
	
		while (p_actual < (uint32_t*)(p_source + length))
		{
			HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, destaddr, *p_actual);

			if (*(uint32_t*)destaddr != *p_actual)
			{
					Uart1.PrintfLog("Write flash error\r\n");
					Write_flag = false;
					break;
			}

			destaddr += 4;
			p_actual++;
		}	

		HAL_FLASH_Lock();
		
		return Write_flag;
}

//Geore@20181011:Read flash by word
static void ReadFlashByWord(uint32_t destaddr, uint32_t *p_source, uint32_t length)
{
		//Uart1.PrintfLog("\r\naddress:0x%08X\r\n", destaddr);
		
		HAL_FLASH_Unlock();
	
		for (int i=0; i<length; i++)
		{
				p_source[i] = *(__IO uint32_t*)destaddr;
				destaddr += 4;
				//Uart1.PrintfLog("0x%08X ", p_source[i]);
		}
		//Uart1.PrintfLog("\r\n");
		
		HAL_FLASH_Lock();
}

//Geore@20181011:FOTA flash write
static bool FLASH_If_Write(uint32_t destaddr, uint32_t *p_source, uint32_t length)
{		
		bool result= false;
		uint32_t TempBuf[10];
		uint32_t address = destaddr;
		
		if (destaddr < FLASH_FOTA_BACKUP_START && destaddr > FLASH_FOTA_BACKUP_END) 
		{
				Uart1.PrintfLog("Flash writr address mismatch!!!");
				return result;
		}

		ReadFlashByWord(address, TempBuf, length);

		if (memcmp(p_source, TempBuf, length*4) != 0)
		{
				if (WriteFlashByWord(destaddr, p_source, length) == true)
				{
						result = true;
				}
		}
		//ReadFlashByWord(address, TempBuf, length);
		
		return result;
}

//George@20181011:FOTA Packet processing
void FOTA_PacketProcess(uint8_t *pdata, uint8_t size)
{
		uint8_t Lastversion[8];
		static uint8_t TempBuf[50];
		static uint8_t EndCount = 0;
	
#if 0
		Uart1.PrintfLog("---------------\r\n");
		Uart1.PrintfLog("Payload Size:%d\r\n", size);
		for (int i=0; i<size; i++)
		{
				Uart1.PrintfLog("0x%02X ", pdata[i]);
		}
		printf("\r\n");
#endif
		
		if (memcmp(pdata, TempBuf, size) == 0)
			return; 
		else
			memcpy(TempBuf, pdata, size);
		
		if (FirmUpgrade.FotaFlag == false)
		{
				if (pdata[0] == FOTA_SOH && size >= 21)
				{
						FirmUpgrade.FotaSize = (pdata[15]<<16)|(pdata[16]<<8)|(pdata[17]);
					
						if (FirmUpgrade.FotaSize < FLASH_FOTA_SIZE)
						{	
								if (memcmp(pdata+1, AppEui, 8) == 0)
								{
										memset(FirmUpgrade.VersionName, 0 ,6);
										memcpy(FirmUpgrade.VersionName, pdata+9, 6);
									
										if (size >= 22)
										{
												if ((pdata[21] & 0x80) == 0x80)
													FirmUpgrade.UpdateBootFlag = true;
												else
													FirmUpgrade.UpdateBootFlag = false;
												
												FirmUpgrade.MaxReissuePack = pdata[21]&0x7f;
										}
										else
										{
												FirmUpgrade.UpdateBootFlag = false;
												FirmUpgrade.MaxReissuePack = FOTA_DEFAULT_REIS_PACK;
										}
									
										if (FirmUpgrade.UpdateBootFlag == true)
											ReadBytesFromMcuEEPROM(BOOT_FIRMWARE_VERSION_START, Lastversion, 6);	
										else
											ReadBytesFromMcuEEPROM(APP_FIRMWARE_VERSION_START, Lastversion, 6);
									
										if (memcmp(FirmUpgrade.VersionName, Lastversion, 6) != 0)
										{
												TimerStop(&TxNextPacketTimer);  //stop report data
												TimerStop(&DeviceOfflineTimer); //stop off-line
											
												if (ClassAMode_flag != true)
												{
														TimerInit(&ReceivePackTimeoutTimer, ReceivePackTimeoutEvent); 
														TimerSetValue(&ReceivePackTimeoutTimer, FOTA_RECEIVE_TIMEOUT); 
														TimerStart(&ReceivePackTimeoutTimer);
												}
											
												FirmUpgrade.FotaFlag = true;
												FirmUpgrade.FotaState = FOTA_IDLE;
											
												FirmUpgrade.PackTotal = (pdata[18]<<8)|(pdata[19]);
												FirmUpgrade.PackLen = pdata[20];
												FirmUpgrade.DataLen = FirmUpgrade.PackLen - 3;				
											
												//clear
												EndCount = 0; 
												FirmUpgrade.FrameSum = 0;
												FirmUpgrade.LossPackTotal = 0;
												memset(FirmUpgrade.LosePackBuf, 0, FOTA_MAX_LOSS_PACK_NUM);
											
												FLASH_If_Erase(FLASH_FOTA_BACKUP_START, FLASH_FOTA_BACKUP_END);
											
												Uart1.PrintfLog("-------------------------\r\n");
												if (FirmUpgrade.UpdateBootFlag == true)
													Uart1.PrintfLog("UpdateObject:Bootloader\r\n");
												else
													Uart1.PrintfLog("UpdateObject:Appliction\r\n");
											
												PRINTF("FirmwareVer: V%s\r\n",FirmUpgrade.VersionName);
												PRINTF("FirmwareSize:%d\r\n", FirmUpgrade.FotaSize);
												PRINTF("PackTotal:   %d\r\n", FirmUpgrade.PackTotal);
												PRINTF("PackLength:  %d\r\n", FirmUpgrade.PackLen);
												PRINTF("MaxReissue:  %d\r\n", FirmUpgrade.MaxReissuePack);
												PRINTF("-------------------------\r\n");
										}
										else
										{
												FirmUpgrade.FotaState = FOTA_NO_VERSION;
												SetandStartReportTimer(FOTA_REPORT_STATE_TIME);
												Uart1.PrintfLog("Firmware version mismatch!!!\r\n");
										}
								}
								else
								{
										FirmUpgrade.FotaState = FOTA_NO_APPEUI;
										SetandStartReportTimer(FOTA_REPORT_STATE_TIME);
										Uart1.PrintfLog("Firmware appeui mismatch!!!\r\n");
								}
						}
						else
						{
								FirmUpgrade.FotaState = FOTA_OUTDO_SIZE;
								SetandStartReportTimer(FOTA_REPORT_STATE_TIME);
								Uart1.PrintfLog("Firmware size mismatch!!!\r\n");
						}
				}
				else
				{
						if ( FirmUpgrade.FotaState == FOTA_IDLE)
						{
								if (pdata[0] == FOTA_STX)
								{
										FirmUpgrade.FotaState = FOTA_NO_START;
										SetandStartReportTimer(FOTA_REPORT_STATE_TIME);
										Uart1.PrintfLog("No start frame received!!!\r\n");
								}
								else
								{
										Uart1.PrintfLog("Start Frame mismatch!!!\r\n");
								}
						}
				}
		}
		else
		{
				if (pdata[0] == FOTA_STX && size == FirmUpgrade.PackLen)  
				{	
						TimerSetValue(&ReceivePackTimeoutTimer, FOTA_RECEIVE_TIMEOUT); 
						TimerStart(&ReceivePackTimeoutTimer);
					
						FirmUpgrade.FrameNum =(pdata[1] & 0x7f)<<8 | pdata[2];
						memset(FirmUpgrade.PackPayload, 0, 48);
						memcpy(FirmUpgrade.PackPayload, pdata+3, FirmUpgrade.DataLen);
					
						if (FirmUpgrade.FrameNum > FirmUpgrade.PackTotal)
						{	
								Uart1.PrintfLog("Frame number mismatch!!!\r\n");
								return;
						}
					
						if ((pdata[1] & 0x80) == 0)
						{		
								Uart1.PrintfLog("NorPack:%d\r\n", FirmUpgrade.FrameNum);
						}
						else
						{
								Uart1.PrintfLog("ReiPack:%d\r\n", FirmUpgrade.FrameNum);
						}
								
						FirmUpgrade.DestAddr = FLASH_FOTA_BACKUP_START + (FirmUpgrade.FrameNum - 1) * FirmUpgrade.DataLen;
									
						if (FLASH_If_Write(FirmUpgrade.DestAddr, (uint32_t *)FirmUpgrade.PackPayload, FirmUpgrade.DataLen/4) == true)
						{
								FirmUpgrade.FrameSum++; 
								Uart1.PrintfLog("WriPack:%d\r\n", FirmUpgrade.FrameSum);
							
								if ((pdata[1] & 0x80) == 0) //normal packet
								{
										while(FirmUpgrade.FrameNum - FirmUpgrade.TempNum > 1)
										{
												FirmUpgrade.LosePackBuf[FirmUpgrade.LossPackTotal] = ++FirmUpgrade.TempNum; 
										
												if (++FirmUpgrade.LossPackTotal > FOTA_MAX_LOSS_PACK_NUM) 
												{
														FirmUpgrade.FotaFlag = false;
														FirmUpgrade.FotaState = FOTA_TOO_LOSS;
														SetandStartReportTimer(FOTA_REPORT_STATE_TIME);
														Uart1.PrintfLog("Packet lose too much exit FOTA!!!\r\n");
														break;
												}
												Uart1.PrintfLog("LossPackTota:%d CurLossPack:%d\r\n", FirmUpgrade.LossPackTotal, FirmUpgrade.TempNum);
										}
										FirmUpgrade.TempNum = FirmUpgrade.FrameNum;
								}
								else												//reissue packet
								{
										ArrayDeleteProcess(FirmUpgrade.LosePackBuf, &FirmUpgrade.LossPackTotal, FirmUpgrade.FrameNum);
								}
						}
						else
						{
								Uart1.PrintfLog("SamePack\r\n");
						}	
				}
				else if (pdata[0] == FOTA_END && pdata[1] == 0x01 && size >= 3) 
				{
						Uart1.PrintfLog("Fota end:%d\r\n", ++EndCount);	

						if (EndCount == 1)
						{
								while(FirmUpgrade.PackTotal != FirmUpgrade.TempNum)
								{
											FirmUpgrade.LosePackBuf[FirmUpgrade.LossPackTotal] = ++FirmUpgrade.TempNum; 
									
											if (++FirmUpgrade.LossPackTotal > FOTA_MAX_LOSS_PACK_NUM) 
											{
													FirmUpgrade.FotaFlag = false;
													FirmUpgrade.FotaState = FOTA_TOO_LOSS;
													Uart1.PrintfLog("Packet lose too much exit FOTA!!!\r\n");
													break;
											}
											Uart1.PrintfLog("Loss PackTotal:%d CurLossNum:%d\r\n", FirmUpgrade.LossPackTotal, FirmUpgrade.TempNum);
								}
								
								if (FirmUpgrade.FrameSum+FirmUpgrade.LossPackTotal != FirmUpgrade.PackTotal)
								{
										FirmUpgrade.FotaState = FOTA_PACKET_ERROR;
										SetandStartReportTimer(FOTA_REPORT_STATE_TIME);
										Uart1.PrintfLog("Packet lose statistics error!!!\r\n");
										return;
								}
						}

						if (FirmUpgrade.FrameSum == FirmUpgrade.PackTotal)
						{
								FirmUpgrade.FotaState = FOTA_SUCCESS;
								if (FirmUpgrade.UpdateBootFlag == true)
									WriteBytesToMcuEEPROM(BOOT_FIRMWARE_VERSION_START, FirmUpgrade.VersionName, 6); //save booloader version
								else
									WriteBytesToMcuEEPROM(APP_FIRMWARE_VERSION_START, FirmUpgrade.VersionName, 6); //save appliction version

								Uart1.PrintfLog("FOTA SUCCESS\r\n");
						}
						else
						{
								if (EndCount >= FirmUpgrade.MaxReissuePack+1)
								{
										FirmUpgrade.FotaState = FOTA_FAILED;
										Uart1.PrintfLog("FOTA FAILED\r\n");
								}
								else
								{
										FirmUpgrade.FotaState = FOTA_LOSS;
										Uart1.PrintfLog("Loss pack total number:%d list:", FirmUpgrade.LossPackTotal);
										for (int j=0; j<FirmUpgrade.LossPackTotal; j++)
										{
												Uart1.PrintfLog("%d ", FirmUpgrade.LosePackBuf[j]);
										}
										Uart1.PrintfLog("\r\n");
								}
						}
						
						SetandStartReportTimer(FOTA_REPORT_STATE_TIME);
				}				
				else
				{
						Uart1.PrintfLog("Packet Frame mismatch!!!\r\n");
				}
		}
}

//George@20181025:get version
void FOTA_GetVersionNum(uint8_t *pdata, uint8_t size)
{
		if (pdata[0] == FOTA_GET && size >= 2)
		{
				if (pdata[1] == 0x01)
					FirmUpgrade.GetBootVerFlag = true;
				else 
					FirmUpgrade.GetAppVerFlag = true;
				
				SetandStartReportTimer(FOTA_REPORT_STATE_TIME);	
		}
		else
		{
				Uart1.PrintfLog("Fota get version failed!!!\r\n");
		}
}

//George@20181015:bootloader 
void FOTA_SystemBootloader(uint8_t *pdata, uint8_t size)
{	
		if (pdata[0] == FOTA_RST && pdata[1] == 0x01 && size >= 2)
		{
				if (FirmUpgrade.FotaState == FOTA_SUCCESS)
				{
						if (FirmUpgrade.UpdateBootFlag == true)
							UpdataBootFirmware();
						else
							SetDataToFalsh(IAP_FLASH_MARKADDESS, FOTA_DOWNLOAD_FLAG);		
						
						Uart1.PrintfLog("Fota success.system reset\r\n\r\n");
						NVIC_SystemReset();
				}
				else
				{
						FirmUpgrade.FotaState = FOTA_IDLE;
						Uart1.PrintfLog("Fota failed.wait next time update\r\n");
				}
		}
}

//George@20181107:fota add temp multicast
void FOTA_AddTempMulticast(uint8_t *pdata, uint8_t size)
{
		MibRequestConfirm_t mibReq;
		frame_t frame;
		uint8_t KeyIndex;
		uint32_t GroupAddr;
		uint32_t FotaFreq = 0xffff;
		uint8_t FotaDR = 0xff;
	
		if(size >= 4)
		{
				KeyIndex = pdata[0]&0x0f;
				GroupAddr = (pdata[1]<<16)|(pdata[2]<<8)|(pdata[3])|0xff000000;
			
#if 0
				if (update_checkBatteryCapacity() != true)
				{
						frame.port = FOTA_ADD_MULTICAST_PORT;
						frame.IsTxConfirmed = 1;
						frame.nbRetries = 3;
						frame.AppDataSize = 1;	
						frame.AppData[0] = 2;

						InsertOneFrame(&frame);
						Uart1.PrintfLog("Device battery power low\r\n");
						return;
				}
#endif
			
				if(AddGroupAddrToMulticastListUnSave(KeyIndex, GroupAddr, 0) == true)
				{
						Uart1.PrintfLog("Fota add group address successfully %X-%d\r\n", GroupAddr, KeyIndex);
					
						mibReq.Type = MIB_DEVICE_CLASS;
						LoRaMacMibGetRequestConfirm( &mibReq );
			
						if (mibReq.Param.Class == CLASS_A)
						{
								ClassAMode_flag = true;
								uint16_t WaitTime = (pdata[4]<<8)|pdata[5];
														
								TimerInit(&SwitchWorkModeTimer, SwitchWorkModeEvent);
								TimerSetValue(&SwitchWorkModeTimer, WaitTime*1000);	
								TimerStart(&SwitchWorkModeTimer);
							
								Uart1.PrintfLog("Fota ClassA mode %d\r\n", WaitTime);
						}
						else
						{
								Uart1.PrintfLog("Fota ClassC mode\r\n");
						}
					
						frame.port = FOTA_ADD_MULTICAST_PORT;
						frame.IsTxConfirmed = 1;
						frame.nbRetries = 3;
						frame.AppDataSize = 1;	
						frame.AppData[0] = 1;

						InsertOneFrame(&frame);

#if 0	//fota new frequency from web set 				
						if (size >= 9)
						{
								FotaFreq = ((pdata[6]<<8)|pdata[7])*100000;
								FotaDR = pdata[8];
						}

						if (FotaFreq == 0xffff)
						{
#if   defined( REGION_EU868 ) 
								FotaFreq = 869300000;
#elif defined( REGION_AS923 ) 
			#if defined (AS923_INDONESIA_ENABLE)
								FotaFreq = 924800000;
			#else
								FotaFreq = 920200000;
			#endif
#elif defined( REGION_US915 ) 
								FotaFreq = 927500000;
#elif defined( REGION_AU915 )
								FotaFreq = 927500000;
#elif defined( REGION_KR920 ) 
								FotaFreq = 921500000;
#elif	defined( REGION_IN865 )
								FotaFreq = 866900000;
#elif defined( REGION_RU864 ) 
								FotaFreq = 869700000;
#elif defined( REGION_CN470 ) 
								FotaFreq = 505100000;
#else 
	          #error "No Region"
#endif
						}
						
						mibReq.Type = MIB_RX2_CHANNEL;
						LoRaMacMibGetRequestConfirm( &mibReq );
						RX2_FQ = mibReq.Param.Rx2Channel.Frequency;
					  RX2_DR = mibReq.Param.Rx2Channel.Datarate;
						
						if (FotaDR == 0xff)
							FotaDR = RX2_DR;
						
						mibReq.Type = MIB_RX2_CHANNEL;
            mibReq.Param.Rx2Channel = ( Rx2ChannelParams_t ){ FotaFreq, FotaDR };
            LoRaMacMibSetRequestConfirm( &mibReq );
#endif
				}
				else
				{
						Uart1.PrintfLog("Linked add multicast failed\r\n");
				}
		}
		else
		{
				Uart1.PrintfLog("Fota add multicast failed!!!\r\n");
		}
}
#endif

//end
