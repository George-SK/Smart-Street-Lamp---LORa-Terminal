#include "test.h"
#include "upgrad.h"

extern void GetLuxValue(float *LuxValue);
extern bool Calibration_flag;
extern RN820x_t RN820xInfo;
extern uint8_t RssiValue, SnrValue;
extern TimerEvent_t NetLedStateTimer;
extern TimerEvent_t NetRejoinTimer;

bool RelayTestMode_flag = false;
bool DownlinkAck_flag = false;

static TimerEvent_t StreetLightStateTimer;
static TimerEvent_t RelayStateTimer;

/**
  * @brief  Convert an Integer to a string
  * @param  p_str: The string output pointer
  * @param  intnum: The integer to be converted
  * @retval None
  */
void Int2Str(uint8_t *p_str, uint32_t intnum)
{
  uint32_t i, divider = 1000000000, pos = 0, status = 0;

  for (i = 0; i < 10; i++)
  {
    p_str[pos++] = (intnum / divider) + 48;

    intnum = intnum % divider;
    divider /= 10;
    if ((p_str[pos-1] == '0') & (status == 0))
    {
      pos = 0;
    }
    else
    {
      status++;
    }
  }
}

/**
  * @brief  Convert a string to an integer
  * @param  p_inputstr: The string to be converted
  * @param  p_intnum: The integer value
  * @retval 1: Correct
  *         0: Error
  */
uint32_t Str2Int(uint8_t *p_inputstr, uint32_t *p_intnum)
{
  uint32_t i = 0, res = 0;
  uint32_t val = 0;

  if ((p_inputstr[0] == '0') && ((p_inputstr[1] == 'x') || (p_inputstr[1] == 'X')))
  {
    i = 2;
    while ( ( i < 11 ) && ( p_inputstr[i] != '\0' ) )
    {
      if (ISVALIDHEX(p_inputstr[i]))
      {
        val = (val << 4) + CONVERTHEX(p_inputstr[i]);
      }
      else
      {
        /* Return 0, Invalid input */
        res = 0;
        break;
      }
      i++;
    }

    /* valid result */
    if (p_inputstr[i] == '\0')
    {
      *p_intnum = val;
      res = 1;
    }
  }
  else /* max 10-digit decimal input */
  {
    while ( ( i < 11 ) && ( res != 1 ) )
    {
      if (p_inputstr[i] == '\0')
      {
        *p_intnum = val;
        /* return 1 */
        res = 1;
      }
      else if (((p_inputstr[i] == 'k') || (p_inputstr[i] == 'K')) && (i > 0))
      {
        val = val << 10;
        *p_intnum = val;
        res = 1;
      }
      else if (((p_inputstr[i] == 'm') || (p_inputstr[i] == 'M')) && (i > 0))
      {
        val = val << 20;
        *p_intnum = val;
        res = 1;
      }
      else if (ISVALIDDEC(p_inputstr[i]))
      {
        val = val * 10 + CONVERTDEC(p_inputstr[i]);
      }
      else
      {
        /* return 0, Invalid input */
        res = 0;
        break;
      }
      i++;
    }
  }

  return res;
}

//George@20180607:Empty function
int EmptyFunction(const char * format, ... )
{
	return 0;
}

//George@20180702: relay state callback function
static void RelayStaticEvent(void)
{
	GetRn820xParam(&RN820xInfo);
	Uart1.PrintfTestLog("+CRST:%f\r\n", RN820xInfo.current/1000.0); 
}

//George@20180702: StreetLight state callback function
static void StreetLightStaticEvent(void)
{
	GetRn820xParam(&RN820xInfo);
	Uart1.PrintfTestLog("+LCDT:%f\r\n", RN820xInfo.current/1000.0);
}	

//George@20180630:get network join status
static NetJoinStatus GetNetJoinStatus(void)
{
  MibRequestConfirm_t mibReq;
  LoRaMacStatus_t status;

  mibReq.Type = MIB_NETWORK_JOINED;
  status = LoRaMacMibGetRequestConfirm(&mibReq);

  if (status == LORAMAC_STATUS_OK)
  {
			if (mibReq.Param.IsNetworkJoined == true)
			{
					return NetJoin_ON;
			}
  }
	
	return NetJoin_OFF;
}

//George@20180629:get uplink frame number
static int GetUplinkCounter(void)
{
  MibRequestConfirm_t mib;
  LoRaMacStatus_t status;

  mib.Type = MIB_UPLINK_COUNTER;
  status = LoRaMacMibGetRequestConfirm(&mib);
	
	if (status == LORAMAC_STATUS_OK)
	{
			return mib.Param.UpLinkCounter;
	}
	
  return 0;
}

//George@20180629:get downlink frame number
static int GetDownlinkCounter(void)
{
  MibRequestConfirm_t mib;
  LoRaMacStatus_t status;

  mib.Type = MIB_UPLINK_COUNTER;
  status = LoRaMacMibGetRequestConfirm(&mib);
	
	if (status == LORAMAC_STATUS_OK)
	{
			return mib.Param.DownLinkCounter;
	}
	
  return 0;
}

//George@20180630:downlink 
void DownlinkReceiveEvent(void)
{
	int FrameNum;
	
	if (AutoTestMode_flag == true && DownlinkAck_flag == true) 
	{
			DownlinkAck_flag = false;
			FrameNum = GetDownlinkCounter();
			Uart1.PrintfTestLog("+LUDCT:%d,%d,%d\r\n", RssiValue, SnrValue, FrameNum);
	}
}

//George@20180710:relay test count
void RelayTestCount(void)
{
	uint32_t count = 0, num = 0;
	
	if (AutoTestMode_flag == true && RelayTestMode_flag == true)
	{
			count = GetWordDataFromFlash(RELAY_COUNT_FLASH);
		
			while(1)
			{
					SetRelay(1);
					HAL_Delay(3000); //3s
				  GetRn820xParam(&RN820xInfo);
					if (RN820xInfo.current > 10) //10mA
					{
						 count++;
						 num++;
						 if (num >= 60) //10min 60
						 {
								num = 0;
							  SetDataToFalsh(RELAY_COUNT_FLASH, count);
						 }
					}

					Uart1.PrintfTestLog("+RCNT:%d\r\n", count); 
					SetRelay(0);
					HAL_Delay(7000); //7s
			}
	}
}

//George@20180614:test mode
void ProductionTestMode(uint8_t *RxBuffer, uint8_t Length)
{
	float LuxValue;
	frame_t frame;
	int FrameNum;
	 
	if (memcmp(RxBuffer, "AT+", 3) == 0)
	{
		if (memcmp(RxBuffer+3, "NJOIN", Length-3) == 0 && Length == strlen("AT+NJOIN"))
		{
				TimerSetValue( &NetRejoinTimer, 500 );	
				TimerStart( &NetRejoinTimer );
		}
		else if (memcmp(RxBuffer+3, "CRST=", Length-4) == 0 && Length == 9)
		{
				if ( RxBuffer[8] == '0')
						SetRelay(0);    //close relay
				else if (RxBuffer[8] == '1')
						SetRelay(1);    //open relay
				else
						Uart1.PrintfTestLog("AT PARAM ERROR\r\n");
				
				TimerInit(&RelayStateTimer, RelayStaticEvent);
				TimerSetValue( &RelayStateTimer, 5000); //5s
				TimerStart(&RelayStateTimer);
		}
		else if (memcmp(RxBuffer+3, "OEDT=?", Length-4) == 0 && Length == 9)
		{
				GetRn820xParam(&RN820xInfo);
				Uart1.PrintfTestLog("+OEDT:%f,%f,%f,%f,%f\r\n", RN820xInfo.voltage, RN820xInfo.current/1000.0, 
														RN820xInfo.power, RN820xInfo.powerFactor, RN820xInfo.kWh);
		}
		else if (memcmp(RxBuffer+3, "LCDT=", strlen("LCDT=")) == 0)
		{
				uint8_t tbuf[4];
				uint8_t count; 
				uint32_t value = 0;
			
				if (Length == strlen("AT+LCDT=1"))
					count = 1;
				else if (Length == strlen("AT+LCDT=10"))
					count = 2;
				else if (Length == strlen("AT+LCDT=100"))
					count = 3;
				else
				{
					Uart1.PrintfTestLog("AT PARAM ERROR\r\n");
					return;
				}

				memset(tbuf, 0, 4);
				memcpy(tbuf, RxBuffer+strlen("AT+LCDT="), count);
				Str2Int(tbuf, &value);
				
				if (value > 100)
					value = 100;
				
				Uart1.PrintfTestLog("brightness:%d\r\n", value);
				SetBrightness(value);
				
				TimerInit(&StreetLightStateTimer, StreetLightStaticEvent);
				TimerSetValue( &StreetLightStateTimer, 5000); //5s
				TimerStart(&StreetLightStateTimer);
		}
		else if (memcmp(RxBuffer+3, "LUDCT=?", Length-3) == 0 && Length == 10)
		{	
				if (GetNetJoinStatus() == NetJoin_ON)
				{
						FrameNum = GetUplinkCounter();
						FrameNum += 1;
					
						frame.IsTxConfirmed = 1;
						frame.AppDataSize = 8;
						frame.nbRetries = 3;
		
					  memcpy(frame.AppData, "+LUDCT:", 7);
						memcpy(frame.AppData+7, &FrameNum, 1);

						InsertOneFrame(&frame);
				}
				else
					  Uart1.PrintfTestLog("+LUDCT:NETWORK JOIN FAILED\r\n");
		}
		else if (memcmp(RxBuffer+3, "LST=?", Length-3) == 0 && Length == 8)
		{
				GetLuxValue(&LuxValue);
				Uart1.PrintfTestLog("+LST:%f\r\n", LuxValue);
		}
		else if (memcmp(RxBuffer+3, "RCNT=", Length-4) == 0 && Length == 9)
		{
				if (RxBuffer[8] == '1')
				{
						RelayTestMode_flag = true;
						Uart1.PrintfTestLog("+RCNT:ON\r\n"); 
				}
				else if (RxBuffer[8] == '0')
				{
						RelayTestMode_flag = false;
						Uart1.PrintfTestLog("+RCNT:OFF\r\n"); 
				}
		}
		else if (memcmp(RxBuffer+3, "CLEAR", Length-3) == 0 && Length == 8)
		{
				SetDataToFalsh(RELAY_COUNT_FLASH, 0);
				Uart1.PrintfTestLog("+CLEAR:OK\r\n");
		}
		else if (memcmp(RxBuffer+3, "ERROR", Length-3) == 0 && Length == 8)
		{
				SetNetStateLed(0);
				TimerStop( &NetLedStateTimer );
			  Uart1.PrintfTestLog("+ERROR:OK\r\n");
		}
		else if (memcmp(RxBuffer+3, "CHECK", Length-3) == 0 && Length == 8)
		{
				if (Calibration_flag == true)
					Uart1.PrintfTestLog("+CHECK:OK\r\n");
				else
					Uart1.PrintfTestLog("+CHECK:FAIL\r\n");
		}
	}	
}

//George@20180712:get tx time lag from flash
void GetTxCycleTimeLagFromEEPROM(void)
{
	uint8_t TxTime1, TxTime2;
	
	if (GetDataFromEEPROM(TX_TIME_FLASH_START, &TxTime1, &TxTime2) == true)
	{
			if (TxTime1 == 0)
				TxTime1 = 1;
			
			if (TxTime2 == 0)
				TxTime2 = 1;
		
			StreetLightParam.TxCycleTimeLag[0] = TxTime2 & 0x0f;
			StreetLightParam.TxCycleTimeLag[1] = (TxTime2>>4) & 0x0f;
			StreetLightParam.TxCycleTimeLag[2] = TxTime1 & 0x0f;
			StreetLightParam.TxCycleTimeLag[3] = (TxTime1>>4) & 0x0f;
		
			Uart1.PrintfLog("Use Flash Tx Cycle Time:%d,%d,%d,%d\r\n", StreetLightParam.TxCycleTimeLag[0], StreetLightParam.TxCycleTimeLag[1], 
																																 StreetLightParam.TxCycleTimeLag[2], StreetLightParam.TxCycleTimeLag[3]);
	}
	else //Default value
	{
			StreetLightParam.TxCycleTimeLag[0] = 2;
			StreetLightParam.TxCycleTimeLag[1] = 3;
			StreetLightParam.TxCycleTimeLag[2] = 5;
			StreetLightParam.TxCycleTimeLag[3] = 5;
		  Uart1.PrintfLog("Use default Tx Cycle Time\r\n");
	}
}

//George@20180713:clear tx cycle time
void EraseTxCycleTime(void)
{
	EraseMcuEEPROMByWords(TX_TIME_FLASH_START, 1);
}

//George@20180714:calculate random value
uint32_t CalculateRandomVal(uint8_t i)
{
		uint32_t randomVal = 0;
	
		if ((StreetLightParam.TxCycleTimeLag[i] > StreetLightParam.TxCycleTimeLag[i-1]) && i >= 1) 
		{
				if (StreetLightParam.TxCycleTimeLag[i] - StreetLightParam.TxCycleTimeLag[i-1] > 0)
				{
						randomVal = StreetLightParam.TxCycleTimeLag[i] * 60000 - randr(0, (StreetLightParam.TxCycleTimeLag[i] - StreetLightParam.TxCycleTimeLag[i-1]) * 60000); //uplink tx cycle 6s~2min
					  //Uart1.PrintfLog("Random %d %d %d\r\n", (StreetLightParam.TxCycleTimeLag[i] - StreetLightParam.TxCycleTimeLag[i-1]) * 10000, StreetLightParam.TxCycleTimeLag[i] * 10000, randomVal);
				}
		}
		else
		{
				randomVal = StreetLightParam.TxCycleTimeLag[i] * 60000;
		}
		
		return randomVal;
}
