/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: LoRaMac classA device implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/

/*! \file classA/SensorNode/main.c */

#include <string.h>
#include <math.h>
#include "board.h"

#include "LoRaMac.h"
#include "Region.h"
#include "Commissioning.h"
#include "multicast.h"
#include "internal-eeprom.h"
#include "profile.h"
#include "iwdg.h"
#include "LoRaMacTest.h"
#include "fault-handler.h"
#include "street-light.h"
#include "upgrad.h"
#include "test.h"
#include "ds1302.h"
#include "macro-definition.h"
#include "fota.h"

/*!
 * Defines the application data transmission duty cycle. 120s, value in [ms].
 */
#define APP_TX_DUTYCYCLE                            120000//120000

/*!
 * Defines a random delay for application data transmission duty cycle. 1s,
 * value in [ms].
 */
#define APP_TX_DUTYCYCLE_RND                        1000

/*!
 * Default datarate
 */
#define LORAWAN_DEFAULT_DATARATE                    DR_0

/*!
 * LoRaWAN confirmed messages
 */
#define LORAWAN_CONFIRMED_MSG_ON                    true

/*!
 * LoRaWAN Adaptive Data Rate
 *
 * \remark Please note that when ADR is enabled the end-device should be static
 */
#define LORAWAN_ADR_ON                              1

#define LORAWAN_DUTYCYCLE_ON                        false//true	//Modified by Steven.

#if defined( REGION_EU868 )

#include "LoRaMacTest.h"

/*!
 * LoRaWAN ETSI duty cycle control enable/disable
 *
 * \remark Please note that ETSI mandates duty cycled transmissions. Use only for test purposes
 */

#define USE_SEMTECH_DEFAULT_CHANNEL_LINEUP          1

#if( USE_SEMTECH_DEFAULT_CHANNEL_LINEUP == 1 )

#define LC4                { 867100000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC5                { 867300000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC6                { 867500000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC7                { 867700000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC8                { 867900000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC9                { 868800000, 0, { ( ( DR_7 << 4 ) | DR_7 ) }, 2 }
#define LC10               { 868300000, 0, { ( ( DR_6 << 4 ) | DR_6 ) }, 1 }

#endif

#endif

/*!
 * User application data buffer size
 */
#if defined( REGION_CN470 ) || defined( REGION_CN779 ) || defined( REGION_EU433 ) || defined( REGION_EU868 ) || defined( REGION_IN865 ) || defined( REGION_KR920 )

#define LORAWAN_APP_DATA_SIZE                       16

#elif defined( REGION_AS923 ) || defined( REGION_AU915 ) || defined( REGION_US915 ) || defined( REGION_US915_HYBQUERYRID )

#define LORAWAN_APP_DATA_SIZE                       20	//Modified by Steven.

#else

#error "Please define a region in the compiler options."

#endif

uint8_t DevEui[] = LORAWAN_DEVICE_EUI;
uint8_t AppEui[] = LORAWAN_APPLICATION_EUI;
uint8_t AppKey[] = LORAWAN_APPLICATION_KEY;

#if( OVER_THE_AIR_ACTIVATION == 0 )

static uint8_t NwkSKey[] = LORAWAN_NWKSKEY;
static uint8_t AppSKey[] = LORAWAN_APPSKEY;

/*!
 * Device address
 */
static uint32_t DevAddr = LORAWAN_DEVICE_ADDRESS;

#endif

/*!
 * Application port
 */
static uint8_t AppPort = LIGHT_STATUS_PORT;

/*!
 * User application data size
 */
static uint8_t AppDataSize = LORAWAN_APP_DATA_SIZE;

/*!
 * User application data buffer size
 */
#define LORAWAN_APP_DATA_MAX_SIZE                           242

/*!
 * User application data
 */
static uint8_t AppData[LORAWAN_APP_DATA_MAX_SIZE];

/*!
 * Indicates if the node is sending confirmed or unconfirmed messages
 */
static uint8_t IsTxConfirmed = LORAWAN_CONFIRMED_MSG_ON;

/*!
 * Defines the application data transmission duty cycle
 */
static uint32_t TxDutyCycleTime;

/*!
 * Timer to handle the application data transmission duty cycle
 */
TimerEvent_t TxNextPacketTimer;

/*!
 * Indicates if a new packet can be sent
 */
static bool NextTx = true;

TimerEvent_t NetRejoinTimer;
TimerEvent_t NetLedStateTimer;

void SetTxDutyCycle(uint32_t time);
void EraseTxCycleTime(void);

/*+++++++++++++++++++++++++++++++++*/
Gpio_t Led3;
extern uint32_t LoRaMacState;
RN820x_t RN820xInfo; 
uint8_t RssiValue = 0, SnrValue = 0;	//Added by Steven in 2018/04/04.

DS1302_TimeSync DS1302_Set; 	//George@20181022:set time sync to ds1302

TimerEvent_t DeviceOfflineTimer;
static TimerEvent_t ADRJustModeExitTimer;

static uint8_t OffLineJudgeStage = 0;

bool ADRjustMode_flag = false;
uint8_t TxCycleConfirmed_flag = 0;
bool OffLineState_flag = false;

extern uint8_t SensorState;

/*+++++++++++++++++++++++++++++++++*/

/*!
 * Device states
 */
static enum eDeviceState
{
    DEVICE_STATE_INIT,
    DEVICE_STATE_JOIN,
    DEVICE_STATE_SEND,
    DEVICE_STATE_CYCLE,
    DEVICE_STATE_SLEEP,
	  DEVICE_STATE_IDLE
}DeviceState;

/*!
 * LoRaWAN compliance tests support data
 */
struct ComplianceTest_s
{
    bool Running;
    uint8_t State;
    bool IsTxConfirmed;
    uint8_t AppPort;
    uint8_t AppDataSize;
    uint8_t *AppDataBuffer;
    uint16_t DownLinkCounter;
    bool LinkCheck;
    uint8_t DemodMargin;
    uint8_t NbGateways;
}ComplianceTest;


//George@20180711:device lost connect 
static void DeviceDropStateEvent(void)
{	
	TimerStop(&DeviceOfflineTimer);
	
	if (FirmUpgrade.FotaFlag != true)
	{
			if(OffLineJudgeStage == 0)
			{		
				OffLineJudgeStage = 1;
				TimerSetValue(&DeviceOfflineTimer, randr(5000, 300000)); //5s~5min
				TimerStart(&DeviceOfflineTimer);		
			}
			else if(OffLineJudgeStage == 1)
			{
				frame_t frame;
				
				OffLineJudgeStage = 2;
				
				frame.port = FRAME_NUM_SETTING_PORT;
				frame.IsTxConfirmed = 1;
				frame.nbRetries = 3;
				frame.AppDataSize = 1;							
				frame.AppData[0] = 0x01;
				InsertOneFrame(&frame);
				
				TimerSetValue(&DeviceOfflineTimer, 60000); //1min
				TimerStart(&DeviceOfflineTimer);
			}
			else
			{
				MibRequestConfirm_t mibReq;
				
				OffLineJudgeStage = 0;
				TimerStop(&TxNextPacketTimer);	//Stop the periodic reporting.
				SetNetStateLed(0);
				mibReq.Type = MIB_NETWORK_JOINED;
				mibReq.Param.IsNetworkJoined = false;
				LoRaMacMibSetRequestConfirm(&mibReq);
				TimerStart(&NetLedStateTimer);
				StreetLightParam.JoinState = 0;
				StreetLightParam.ReportFlag = 0;
				
#if defined (DS1302_RTC_ENABLE)  //George@20181023:If no profile, enter auto dimming mode.
				if(LoadProfileFromFlash() != NO_PROFILE)	
				{
						EnterDimmingMode(PROFILE_DIMMING_MODE, 0); 
						Uart1.PrintfLog("Enter profile dimming mode and wait rejoin the network\r\n");
				}
				else
				{
						EnterDimmingMode(AUTO_DIMMING_MODE, 0); 
						Uart1.PrintfLog("Enter auto dimming mode and wait rejoin the network\r\n");
				}
#else
				EnterDimmingMode(AUTO_DIMMING_MODE, 0); 
				Uart1.PrintfLog("Enter auto dimming mode and wait rejoin the network\r\n");
#endif
					
				SetDataToEEPROM(DEVICE_CONTRAL_MODE_START, StreetLightParam.ctrlMode|0x80, StreetLightParam.pwmDuty); //George@20181008:set offline flag and reset
				OffLineState_flag = true;
				BoardDisableIrq();
				NVIC_SystemReset();
				BoardEnableIrq();
			}	
	}
}

void OffLineTimerReset(void)
{
	TimerSetValue(&DeviceOfflineTimer, 1200000); //20min
	TimerStart(&DeviceOfflineTimer);
	OffLineJudgeStage = 0;	
}

//George@20180718:ADR just mode timeout exit event
void ADRJustModeTimeoutEvent(void)
{
	ADRjustMode_flag = false;
	StreetLightParam.TxCycleCount = 1;	
	SetTxDutyCycle(randr( 6000, StreetLightParam.TxCycleTimeLag[0] * 60000 )); //6s~2min
	Uart1.PrintfLog("Eixt ADR just mode\r\n");	
}

/*!
 * \brief   Prepares the payload of the frame
 */
static void PrepareTxFrame( uint8_t port )
{
    switch( port )
    {
			case 2:
					{
							AppData[0] = 0x01;
							AppData[1] = 0x02;
							AppData[2] = 0x03;
							AppData[3] = 0x04;
							AppData[4] = 0x05;
							AppData[5] = 0x06;
							AppData[6] = 0x07;
							AppData[7] = 0x08;
							AppData[8] = 0x09;
							AppData[9] = 0x0a;
							AppData[10] = 0x0b;
							AppData[11] = 0x0c;
							AppData[12] = 0x0d;
							AppData[13] = 0x0e;
							AppData[14] = 0x0f;
							AppData[15] = 0x10;
					}
					break;
			case LIGHT_STATUS_PORT:
			case LIGHT_CTRL_PORT:
				break;
			case MULTICAST_ADD_SETTING_PORT:
			case MULTICAST_REMOVE_SETTING_PORT:
					AppData[0] = GroupAddrIndex;
					AppDataSize = 1;
					break;
			case 224:
					if( ComplianceTest.LinkCheck == true )
					{
							ComplianceTest.LinkCheck = false;
							AppDataSize = 3;
							AppData[0] = 5;
							AppData[1] = ComplianceTest.DemodMargin;
							AppData[2] = ComplianceTest.NbGateways;
							ComplianceTest.State = 1;
					}
					else
					{
							switch( ComplianceTest.State )
							{
							case 4:
									ComplianceTest.State = 1;
									break;
							case 1:
									AppDataSize = 2;
									AppData[0] = ComplianceTest.DownLinkCounter >> 8;
									AppData[1] = ComplianceTest.DownLinkCounter;
									break;
							}
					}
					break;
			default:
					break;
    }
}

/*!
 * \brief   Prepares the payload of the frame
 *
 * \retval  [0: frame could be send, 1: error]
 */
static bool SendFrame( void )
{
    McpsReq_t mcpsReq;
    LoRaMacTxInfo_t txInfo;

    if( LoRaMacQueryTxPossible( AppDataSize, &txInfo ) != LORAMAC_STATUS_OK )
    {
        // Send empty frame in order to flush MAC commands
        mcpsReq.Type = MCPS_UNCONFIRMED;
        mcpsReq.Req.Unconfirmed.fBuffer = NULL;
        mcpsReq.Req.Unconfirmed.fBufferSize = 0;
        mcpsReq.Req.Unconfirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
    }
    else
    {
        if( IsTxConfirmed == false )
        {
            mcpsReq.Type = MCPS_UNCONFIRMED;
            mcpsReq.Req.Unconfirmed.fPort = AppPort;
            mcpsReq.Req.Unconfirmed.fBuffer = AppData;
            mcpsReq.Req.Unconfirmed.fBufferSize = AppDataSize;
            mcpsReq.Req.Unconfirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
        }
        else
        {
            mcpsReq.Type = MCPS_CONFIRMED;
            mcpsReq.Req.Confirmed.fPort = AppPort;
            mcpsReq.Req.Confirmed.fBuffer = AppData;
            mcpsReq.Req.Confirmed.fBufferSize = AppDataSize;
            mcpsReq.Req.Confirmed.NbTrials = 3;
            mcpsReq.Req.Confirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
        }
    }

    if( LoRaMacMcpsRequest( &mcpsReq ) == LORAMAC_STATUS_OK )
    {
        return false;
    }
    return true;
}

/*!
 * \brief Function executed on rejoining event
 */
static void OnNetRejoinTimerEvent( void )
{
	DeviceState = DEVICE_STATE_JOIN;
	TimerStop( &NetRejoinTimer );
}

/*!
 * \brief Function executed on LED state event
 */
static void OnNetLedStateEvent( void )
{
	MibRequestConfirm_t mibReq;
	LoRaMacStatus_t status;
	static uint8_t LedState = 1;
	
	mibReq.Type = MIB_NETWORK_JOINED;
	status = LoRaMacMibGetRequestConfirm( &mibReq );
	
	if( status == LORAMAC_STATUS_OK )
	{
		if( mibReq.Param.IsNetworkJoined == true )
		{
			SetNetStateLed(1);
			TimerStop( &NetLedStateTimer );
		}
		else
		{
			SetNetStateLed(LedState);
			LedState = !LedState;
			TimerStart( &NetLedStateTimer );
		}
	}
}

/*!
 * \brief Function executed on TxNextPacket Timeout event
 */
void OnTxNextPacketTimerEvent( void )
{
    MibRequestConfirm_t mibReq;
    LoRaMacStatus_t status;
		frame_t frame;
		static uint32_t TxCycleCnt = 0;
		static float AcVoltageTmp = 8888.0;
		static uint8_t AcVoltageRecheckCount = 0;
	
		Uart1.PrintfLog("---------------------------\r\n");
		Uart1.PrintfLog("light:%flux\r\n", StreetLightParam.lux);

    mibReq.Type = MIB_NETWORK_JOINED;
    status = LoRaMacMibGetRequestConfirm( &mibReq );
	
		if (ADRjustMode_flag == true)
		{
				if (TxCycleConfirmed_flag == 1)
						frame.IsTxConfirmed = 1;
				else if (TxCycleConfirmed_flag == 2)	
						frame.IsTxConfirmed = 0;
				else if (TxCycleConfirmed_flag == 3)	
						frame.IsTxConfirmed = 1;
				else if (TxCycleConfirmed_flag == 4)	
						frame.IsTxConfirmed = 0;
				
				frame.port = ADR_JUST_SEND_PORT;
				frame.nbRetries = 3;
				frame.AppDataSize = 2;							
				frame.AppData[0] = RssiValue;
				frame.AppData[1] = SnrValue;
				InsertOneFrame(&frame);
					
				uint8_t hour, minute, second;
				uint32_t CurrentTime;
				CurrentTime = GetCurrentTime();
				hour = CurrentTime/3600;
				minute = (CurrentTime-(hour*3600))/60;
				second = CurrentTime%60;
				Uart1.PrintfLog("TIME[%d:%d:%d]\r\n", hour, minute, second);	
				
				if (TxCycleConfirmed_flag == 3 || TxCycleConfirmed_flag == 4)
				{
						SetTxDutyCycle(StreetLightParam.ADRJustTxCycleTime * 60000);
				}
				Uart1.PrintfLog("ADR SEND OK\r\n");	
		}
		else
		{
				if( status == LORAMAC_STATUS_OK )
				{
						if( mibReq.Param.IsNetworkJoined == true )
						{
								GetRn820xParam(&RN820xInfo);
								
								//Steven@20181219.
								if(AcVoltageRecheckCount < 3)
								{
									if(AcVoltageTmp != 8888.0)
									{
										if(AcVoltageTmp > RN820xInfo.voltage && (RN820xInfo.voltage/AcVoltageTmp < 0.82))
										{
											TimerSetValue( &TxNextPacketTimer, 5000 );
											TimerStart( &TxNextPacketTimer );
											AcVoltageRecheckCount++;
											return;
										}
										else if(AcVoltageTmp < RN820xInfo.voltage && (AcVoltageTmp/RN820xInfo.voltage < 0.82))
										{
											TimerSetValue( &TxNextPacketTimer, 5000 );
											TimerStart( &TxNextPacketTimer );
											AcVoltageRecheckCount++;
											return;
										}
									}
								}
								
								AcVoltageRecheckCount = 0;
								AcVoltageTmp = RN820xInfo.voltage;								
							
								StreetLightParam.ReportFlag = 1;
								
								if((RN820xInfo.power<ABNORMAL_POWER_LEVEL && StreetLightParam.pwmDuty>ABNORMAL_PWM_LEVEL) || (StreetLightParam.pwmDuty<ABNORMAL_PWM_LEVEL && RN820xInfo.power>ABNORMAL_POWER_LEVEL))
									StreetLightParam.AbnormalState = 1;
								else
									StreetLightParam.AbnormalState = 0;
								
								frame.IsTxConfirmed = 0;
								frame.nbRetries = 5;//3;	Modified by Steven for test in 2017/12/26
								
								//George@20180711:new protocol
								if(RN820xInfo.current < 40)
								{
									if(StreetLightParam.pwmDuty == 0)
									{
										RN820xInfo.current = 0;
										RN820xInfo.power = 0;
										RN820xInfo.powerFactor = 0;
									}
								}
								
								uint8_t Mode;
								if(StreetLightParam.ctrlMode == AUTO_DIMMING_MODE)
									Mode = 0x00;	 //00_xxxxxx
								else if(StreetLightParam.ctrlMode == MANUAL_DIMMING_MODE)
									Mode = 0x80;	 //10_xxxxxx
								else if(StreetLightParam.ctrlMode == PROFILE_DIMMING_MODE)
									Mode = 0x40;	 //01_xxxxxx
								
								uint16_t ProfileId = GetProfileId();
								uint8_t DataLen = 0;
								
								if (StreetLightParam.lux > 0xffff) 
										StreetLightParam.lux = 0xffff;
											
								if(StreetLightParam.TxCycleCount < 3)
								{
									TxCycleCnt = 0;
									
									frame.AppData[DataLen++] = ((StreetLightParam.TimeSync<<7)&0x80)|(StreetLightParam.pwmDuty&0x7f);
									
									frame.AppData[DataLen++] = (uint8_t)(RN820xInfo.powerFactor*100);
						
									frame.AppData[DataLen++] = Mode|(((uint16_t)RN820xInfo.current>>8)&0x3f);
									frame.AppData[DataLen++] = ((uint16_t)RN820xInfo.current)&0xff;
									
									frame.AppData[DataLen++] = (((uint16_t)(RN820xInfo.voltage*10))>>8)&0xff;
									frame.AppData[DataLen++] = ((uint16_t)(RN820xInfo.voltage*10))&0xff;
									
									frame.AppData[DataLen++] = (((uint32_t)(RN820xInfo.kWh*1000))>>16)&0xff;
									frame.AppData[DataLen++] = (((uint32_t)(RN820xInfo.kWh*1000))>>8)&0xff;
									frame.AppData[DataLen++] = ((uint32_t)(RN820xInfo.kWh*1000))&0xff;
									
									if (StreetLightParam.TxCycleCount == 0)
									{
											StreetLightParam.TxCycleTime = StreetLightParam.TxCycleTimeLag[0] * 60000;	      //default 2 min
											frame.port = LIGHT_STATUS_3_4_PORT;
											frame.AppData[DataLen++] = ProfileId;
									}	
									else if (StreetLightParam.TxCycleCount == 1)
									{
											StreetLightParam.TxCycleTime = CalculateRandomVal(StreetLightParam.TxCycleCount);	//default 3 min
											frame.port = LIGHT_STATUS_3_4_PORT;
											frame.AppData[DataLen++] = ProfileId;
									}
									else
									{				
											StreetLightParam.TxCycleTime = CalculateRandomVal(StreetLightParam.TxCycleCount);	//default 5 min
											frame.port = LIGHT_STATUS_PORT;
											frame.AppData[DataLen++]  = (((uint16_t)StreetLightParam.lux)>>8)&0xff;
											frame.AppData[DataLen++] = ((uint16_t)StreetLightParam.lux)&0xff;
										
											frame.AppData[DataLen++] = ProfileId;
									}
									
									StreetLightParam.TxCycleCount++;
								}
								else
								{
									//StreetLightParam.TxCycleTime = StreetLightParam.TxCycleTimeLag[3] * 60000;	    //uplink tx cycle 5min
									StreetLightParam.TxCycleTime = 6 * 60000 - randr(0, 60000);                     //George@20180911:5min~6min rand number
									
									if (TxCycleCnt % 2 == 0)
									{
											frame.port = LIGHT_STATUS_F1_2_PORT;
											frame.AppData[DataLen++] = ((StreetLightParam.TimeSync<<7)&0x80)|(StreetLightParam.pwmDuty&0x7f);
										
											frame.AppData[DataLen++] = (uint8_t)(RN820xInfo.powerFactor*100);
										
											frame.AppData[DataLen++] = Mode|(((uint16_t)RN820xInfo.current>>8)&0x3f);
											frame.AppData[DataLen++] = ((uint16_t)RN820xInfo.current)&0xff;
										
											frame.AppData[DataLen++] = (((uint16_t)(RN820xInfo.voltage*10))>>8)&0xff;
											frame.AppData[DataLen++] = ((uint16_t)(RN820xInfo.voltage*10))&0xff;
									}	
									else
									{
											frame.port = LIGHT_STATUS_B1_2_PORT;
											frame.AppData[DataLen++] = (((uint32_t)(RN820xInfo.kWh*1000))>>16)&0xff;
											frame.AppData[DataLen++] = (((uint32_t)(RN820xInfo.kWh*1000))>>8)&0xff;
											frame.AppData[DataLen++] = ((uint32_t)(RN820xInfo.kWh*1000))&0xff;
										
											frame.AppData[DataLen++] = (((uint16_t)StreetLightParam.lux)>>8)&0xff;
											frame.AppData[DataLen++] = ((uint16_t)StreetLightParam.lux)&0xff;
										
											frame.AppData[DataLen++] = ProfileId;
									}	
									
									TxCycleCnt++;
								}
#if defined (FAULT_HANDLE_ENABLE)								
								frame.AppData[DataLen++] = LightFault.AttributeId;
								
								if(SensorState)
									LightFault.fault &= ~(0x01<<5);
								else
									LightFault.fault |= (0x01<<5);//Sensor Failure.
								
								frame.AppData[DataLen++] = LightFault.fault;
#endif
								frame.AppDataSize = DataLen;
											
								uint8_t hour, minute, second;
								uint32_t CurrentTime;
								CurrentTime = GetCurrentTime();
								hour = CurrentTime/3600;
								minute = (CurrentTime-(hour*3600))/60;
								second = CurrentTime%60;
								Uart1.PrintfLog("TIME[%d:%d:%d]\r\n", hour, minute, second);	
								
								if(!CtrlGroupAddrFlag)
								{
#if defined(ACTIVE_MODE_2)
									//Cover the previous setting and send the activation request.
									frame.port = 4;
									frame.AppData[0] = 0x01;
									frame.AppDataSize = 1;
#endif
									
									if(StreetLightParam.TxCycleCount == 1)
										StreetLightParam.TxCycleTime = 180000;
									else if(StreetLightParam.TxCycleCount == 2)
										StreetLightParam.TxCycleTime = 120000;
									else if(StreetLightParam.TxCycleCount == 3)
									{
										CtrlGroupAddrFlag = 1;		//Recover the normal state report.
										StreetLightParam.TxCycleCount = 0;
										StreetLightParam.TxCycleTime = 60000;
									}
								}
								
								if (AutoTestMode_flag == false)  //George@20180629:add auto test flag 
								{
									StreetLightParam.FeedDogCount = 0;
									InsertOneFrame(&frame);
								}
									
								TimerSetValue( &TxNextPacketTimer, StreetLightParam.TxCycleTime );
								TimerStart( &TxNextPacketTimer );
						}
						else
						{
								DeviceState = DEVICE_STATE_JOIN;
						}
				}		
		}
}

/*!
 * \brief   MCPS-Confirm event function
 *
 * \param   [IN] mcpsConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void McpsConfirm( McpsConfirm_t *mcpsConfirm )
{
	//MibRequestConfirm_t mibReq;
	
	if( mcpsConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
    {
        switch( mcpsConfirm->McpsRequest )
        {
            case MCPS_UNCONFIRMED:
            {
                // Check Datarate
                // Check TxPower
                break;
            }
            case MCPS_CONFIRMED:
            {
                // Check Datarate
                // Check TxPower
                // Check AckReceived
                // Check NbTrials								
                break;
            }
            case MCPS_PROPRIETARY:
            {
                break;
            }
            default:
                break;
        }
    }
		
		if(mcpsConfirm->AckReceived)
		{
			StreetLightParam.FailureCounter = 0;
			DownlinkAck_flag = true;  //George@20180630:add Downlink ack receive flag
			
			//If received any ACK, reset the off-line timer counter.
			OffLineTimerReset();
		}
		
    NextTx = true;
}

/*!
 * \brief   MCPS-Indication event function
 *
 * \param   [IN] mcpsIndication - Pointer to the indication structure,
 *               containing indication attributes.
 */
static void McpsIndication( McpsIndication_t *mcpsIndication )
{
		frame_t frame;
		uint32_t GroupAddr;
		uint8_t KeyIndex;
		uint32_t SyncTimeValue;
		MibRequestConfirm_t mibReq;
    LoRaMacStatus_t status;
	
		bool EraseOK_flag = false;
		uint32_t DataLen, DataBuf[100];
		uint32_t TxReportTime;
	  uint8_t profileType;
	
		if( mcpsIndication->Status != LORAMAC_EVENT_INFO_STATUS_OK )
    {
        return;
    }
		
		mibReq.Type = MIB_NETWORK_JOINED;
    status = LoRaMacMibGetRequestConfirm( &mibReq );

    if( status != LORAMAC_STATUS_OK || mibReq.Param.IsNetworkJoined == false)	//If not joined, it can't be controlled through Multicast.
    {
        return;
		}
		
    switch( mcpsIndication->McpsIndication )
    {
        case MCPS_UNCONFIRMED:
        {
            break;
        }
        case MCPS_CONFIRMED:
        {
            break;
        }
        case MCPS_PROPRIETARY:
        {
            break;
        }
        case MCPS_MULTICAST:
        {
            break;
        }
        default:
            break;
    }

    // Check Multicast
    // Check Port
    // Check Datarate
    // Check FramePending
    // Check Buffer
    // Check BufferSize
    // Check Rssi
    // Check Snr
    // Check RxSlot
		
		if(mcpsIndication->Rssi > 0)	//Added by Steven in 2018/04/04.
			RssiValue = 0;
		else
			RssiValue = -mcpsIndication->Rssi;
		
		SnrValue = mcpsIndication->Snr;
	
    if( ComplianceTest.Running == true )
    {
        ComplianceTest.DownLinkCounter++;
    }

    if( mcpsIndication->RxData == true )
    {
        switch( mcpsIndication->Port )
        {
        case 1: 
        case 2:
						//To do...
            break;
				case LIGHT_STATUS_PORT:
						if(mcpsIndication->Buffer[0] != 0)
						{
							GetRn820xParam(&RN820xInfo);
							
							if(RN820xInfo.power<ABNORMAL_POWER_LEVEL && StreetLightParam.pwmDuty>ABNORMAL_PWM_LEVEL)
								StreetLightParam.AbnormalState = 1;
							else
								StreetLightParam.AbnormalState = 0;
							
							//George@20180711:new protocol
							if(RN820xInfo.current < 40)
							{
								if(StreetLightParam.pwmDuty == 0)
								{
									RN820xInfo.current = 0;
									RN820xInfo.power = 0;
									RN820xInfo.powerFactor = 0;
								}
							}
							
							uint8_t Mode;
							if(StreetLightParam.ctrlMode == AUTO_DIMMING_MODE)
								Mode = 0x00;	 //00_xxxxxx
							else if(StreetLightParam.ctrlMode == MANUAL_DIMMING_MODE)
								Mode = 0x80;	 //10_xxxxxx
							else if(StreetLightParam.ctrlMode == PROFILE_DIMMING_MODE)
								Mode = 0x40;	 //01_xxxxxx
							
							uint16_t ProfileId = GetProfileId();
							
							if (StreetLightParam.lux > 0xffff) 
									StreetLightParam.lux = 0xffff;
							
							frame.port = LIGHT_STATUS_PORT;
							frame.IsTxConfirmed = 1;
							frame.nbRetries = 3;
							frame.AppDataSize = 12;
							
							frame.AppData[0] = ((StreetLightParam.TimeSync<<7)&0x80)|(StreetLightParam.pwmDuty&0x7f);
							
							frame.AppData[1] = (uint8_t)(RN820xInfo.powerFactor*100);
				
							frame.AppData[2] = Mode|(((uint16_t)RN820xInfo.current>>8)&0x3f);
							frame.AppData[3] = ((uint16_t)RN820xInfo.current)&0xff;
							
							frame.AppData[4] = (((uint16_t)(RN820xInfo.voltage*10))>>8)&0xff;
							frame.AppData[5] = ((uint16_t)(RN820xInfo.voltage*10))&0xff;
							
							frame.AppData[6] = (((uint32_t)(RN820xInfo.kWh*1000))>>16)&0xff;
							frame.AppData[7] = (((uint32_t)(RN820xInfo.kWh*1000))>>8)&0xff;
							frame.AppData[8] = ((uint32_t)(RN820xInfo.kWh*1000))&0xff;
							
							frame.AppData[9]  = (((uint16_t)StreetLightParam.lux)>>8)&0xff;
							frame.AppData[10] = ((uint16_t)StreetLightParam.lux)&0xff;
							
							frame.AppData[11] = ProfileId;
						
							InsertOneFrame(&frame);
							//end
						}
						break;
				case LIGHT_CTRL_PORT:
						StreetLightParam.RecvFlag = 1;
						StreetLightParam.RecvData[0] = mcpsIndication->Buffer[0];
#if defined (DS1302_RTC_ENABLE)
						SaveCtrModeAndBrightness(mcpsIndication->Buffer);
#endif
						break;
				case DALI_CTRL_PORT: //shengyu@20180806:add dali dimming
					  Uart1.PrintfLog("port:%d, data:0x%x\r\n", DALI_CTRL_PORT, mcpsIndication->Buffer[0]);
					  receive_rola_data_pro(mcpsIndication->Buffer, mcpsIndication->BufferSize);
					break;
				case PROFILE1_PORT:
						if(StreetLightParam.TimeSync)
						{
							AddProfile1(mcpsIndication->Buffer, mcpsIndication->BufferSize, 1);
							EnterDimmingMode(PROFILE_DIMMING_MODE, 1);
							Uart1.PrintfLog("Enter profile dimming mode\r\n");
						}
						else
						{
							if(mcpsIndication->BufferSize >= 7)
							{
								SaveProfileToFlash(PROFILE1_TYPE, mcpsIndication->Buffer, mcpsIndication->BufferSize);
							}
						}
						break;
				case PROFILE2_PORT:
						if(StreetLightParam.ctrlMode == PROFILE_DIMMING_MODE)	//Modified by Steven in 2018/06/07.
						{
							AddProfile2(mcpsIndication->Buffer, mcpsIndication->BufferSize, 1);
						}
						else
						{
							if(mcpsIndication->BufferSize >= 3)
							{
								SaveProfileToFlash(PROFILE2_TYPE, mcpsIndication->Buffer, mcpsIndication->BufferSize);
							}
						}
						break;
				case ERASE_FLASH_PORT:
						if(mcpsIndication->BufferSize == 1)
						{
							if(mcpsIndication->Buffer[0] == 0x01)
							{
									RemoveProfile();
								  if (GetProfileFlash() == 0)
										EraseOK_flag = true; 
							}
							else if(mcpsIndication->Buffer[0] == 0x02)
							{
								  for(int i=0; i<3; i++)
									{
										ClearKwhInFlash();
										GetRn820xParam(&RN820xInfo);
										if (RN820xInfo.kWh <= 0.005)
										{
											EraseOK_flag = true; 
											break;
										}
									}
							}
							else if (mcpsIndication->Buffer[0] == 0x03)  //George@20180713:erase report cycle for e2prom
							{
									EraseTxCycleTime();
									ReadWordsFromMcuEEPROM(TX_TIME_FLASH_START, &TxReportTime, 1);
								  if (TxReportTime == 0)
										EraseOK_flag = true; 
							}
							else if (mcpsIndication->Buffer[0] == 0x04)  //George@20180713:Restore factory settings
							{
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

									ReadMcuEEPROM(DataBuf, &DataLen);
									profileType = GetProfileFlash();
									ReadWordsFromMcuEEPROM(TX_TIME_FLASH_START, &TxReportTime, 1);
								
									if (DataLen == 0 && profileType == 0 && RN820xInfo.kWh <= 0.005 && TxReportTime == 0)
									{
											EraseOK_flag = true; 
										  Uart1.PrintfLog("Restore factory settings\r\n");
									}
							}
									
							frame.port = ERASE_FLASH_PORT;
							frame.IsTxConfirmed = 1;
							frame.nbRetries = 3;
							frame.AppDataSize = 1;			
							if (EraseOK_flag == true)
								frame.AppData[0] = 0x01;
							else
								frame.AppData[0] = 0x00;
							
							InsertOneFrame(&frame);
						}
						break;
				//George@20180711:new port
			  case SET_REPORT_STATUS_PORT: 
					  if(mcpsIndication->BufferSize == 2)
						{
								if (mcpsIndication->Buffer[0] > 0 && mcpsIndication->Buffer[1] > 0)
								{
										StreetLightParam.TxCycleTimeLag[0] = mcpsIndication->Buffer[1] & 0x0f;
										StreetLightParam.TxCycleTimeLag[1] = (mcpsIndication->Buffer[1]>>4) & 0x0f;
										StreetLightParam.TxCycleTimeLag[2] = mcpsIndication->Buffer[0] & 0x0f;
										StreetLightParam.TxCycleTimeLag[3] = (mcpsIndication->Buffer[0]>>4) & 0x0f;
									
										SetDataToEEPROM(TX_TIME_FLASH_START, mcpsIndication->Buffer[0], mcpsIndication->Buffer[1]);
									  Uart1.PrintfLog("Set Tx cycle time:%d,%d,%d,%d\r\n", StreetLightParam.TxCycleTimeLag[0], StreetLightParam.TxCycleTimeLag[1], 
																																				 StreetLightParam.TxCycleTimeLag[2], StreetLightParam.TxCycleTimeLag[3]);
										StreetLightParam.TxCycleCount = 1;
										SetTxDutyCycle(randr( 6000, StreetLightParam.TxCycleTimeLag[0] * 60000 )); //6s~2min 
								}
						}
						break;
				case GET_REPORT_STATUS_PORT:
					  if(mcpsIndication->BufferSize == 1)
						{
								if(mcpsIndication->Buffer[0] == 0x01)
								{
										GetTxCycleTimeLagFromEEPROM();
									
										frame.port = GET_REPORT_STATUS_PORT;
										frame.IsTxConfirmed = 1;
										frame.nbRetries = 3;
										frame.AppDataSize = 2;							
										frame.AppData[0] = StreetLightParam.TxCycleTimeLag[2] | (StreetLightParam.TxCycleTimeLag[3]<<4);
										frame.AppData[1] = StreetLightParam.TxCycleTimeLag[0] | (StreetLightParam.TxCycleTimeLag[1]<<4);
										InsertOneFrame(&frame);
								}
						}
						break;
				case QUERY_REPORT_RSSI_PORT:
					  if(mcpsIndication->BufferSize == 1)
						{
								if(mcpsIndication->Buffer[0] == 0x01)
								{
										frame.port = QUERY_REPORT_RSSI_PORT;
										frame.IsTxConfirmed = 1;
										frame.nbRetries = 3;
										frame.AppDataSize = 2;							
										frame.AppData[0] = RssiValue;
										frame.AppData[1] = SnrValue;
										InsertOneFrame(&frame);
										Uart1.PrintfLog("RSSI:%02X SNR:%02X\r\n", frame.AppData[0], frame.AppData[1]);
								}
						}
						break;
				case ADR_JUST_RECEIVE_PORT:
					  if(mcpsIndication->BufferSize == 1)
						{
								if (mcpsIndication->Buffer[0] & 0x80)
								{
										ADRjustMode_flag = true;
									  if ((mcpsIndication->Buffer[0] & 0x60) == 0x00)
												TxCycleConfirmed_flag = 1;
										else if ((mcpsIndication->Buffer[0] & 0x60) == 0x20) 
												TxCycleConfirmed_flag = 2;
										else if ((mcpsIndication->Buffer[0] & 0x60) == 0x40)
												TxCycleConfirmed_flag = 3;
										else if ((mcpsIndication->Buffer[0] & 0x60) == 0x60)
												TxCycleConfirmed_flag = 4;
										
										StreetLightParam.ADRJustTxCycleTime = (uint8_t)(mcpsIndication->Buffer[0] & 0x1f);	
										
										SetTxDutyCycle(randr( 6000, StreetLightParam.ADRJustTxCycleTime * 60000 )); //6s~xxmin
										
										TimerSetValue( &ADRJustModeExitTimer, 7200000); //7200s==2h
										TimerStart(&ADRJustModeExitTimer);
										Uart1.PrintfLog("Enter ADR just mode\r\n");	
								}
								else 
								{
										ADRjustMode_flag = false;
										StreetLightParam.TxCycleCount = 1;	
										SetTxDutyCycle(randr( 6000, StreetLightParam.TxCycleTimeLag[0] * 60000 )); //6s~2min
									  Uart1.PrintfLog("Eixt ADR just mode\r\n");	
								}
						}
						break;
				//end
				case TIME_SYNC_PORT:
						if(mcpsIndication->BufferSize >= 3)
						{
							SyncTimeValue = ((mcpsIndication->Buffer[0]<<16)|(mcpsIndication->Buffer[1]<<8)|(mcpsIndication->Buffer[2]))&0x01ffff;
							TimeSyncFunc(SyncTimeValue);
							
#if defined (DS1302_RTC_ENABLE)  //George@20181022:set time sync to ds1302
							DS1302_Set.hour = SyncTimeValue/3600;
							DS1302_Set.min  = (SyncTimeValue-(DS1302_Set.hour*3600))/60;
							DS1302_Set.sec  = SyncTimeValue%60;
							DS1302SetTimeSync(DS1302_Set);
#endif
							
							OffLineTimerReset();
							
							uint32_t multicastAddr = 0xfffffffe, DCounter = 0, DCounterDiff = 0;
							
#if   defined( REGION_CN470 )
							multicastAddr = 0xffffffff;
#elif defined( REGION_EU868 )
							multicastAddr = 0xfffffffe;
#elif defined( REGION_AU915 ) 
							multicastAddr = 0xfffffffd;
#elif defined( REGION_US915 ) 
							multicastAddr = 0xfffffffc;
#elif defined( REGION_AS923 )
							multicastAddr = 0xfffffffb;
#endif
							
							if(GetDownLinkCounterByAddr(multicastAddr, &DCounter))
							{
								DCounterDiff = DCounter - TimeSyncDownCountTmp;
								
								if(DCounterDiff > 1000)
								{
									SaveAllDownLinkCounter();	
									TimeSyncDownCountTmp = DCounter;
								}
							}
						}
						
						break;
				case MULTICAST_ADD_SETTING_PORT:
					
						frame.port = MULTICAST_ADD_SETTING_PORT;
						frame.IsTxConfirmed = 1;
						frame.nbRetries = 3;
						frame.AppDataSize = 1;	
						
						if(mcpsIndication->BufferSize >= 9)
						{
							GroupAddr = (mcpsIndication->Buffer[1]<<16)|(mcpsIndication->Buffer[2]<<8)|(mcpsIndication->Buffer[3])|0xff000000;
							KeyIndex = mcpsIndication->Buffer[0]&0x0f;
							
							if(RemoveGroupAddrFromMulticastList(GroupAddr) == true)	//Steven@20180726: Must remove the same multicast adress first if existed. 
							{
								uint32_t GroupDownLinkCounter = 0;
									
								GroupDownLinkCounter = ((mcpsIndication->Buffer[4]<<8)|mcpsIndication->Buffer[5])<<8;
								
								if(AddGroupAddrToMulticastList(KeyIndex, GroupAddr, GroupDownLinkCounter) == true)
								{
									PRINTF("Add group addr %d-%X-%d successfully.\r\n", KeyIndex, GroupAddr, GroupDownLinkCounter);									
									
									if(CheckMulticastInFlash(KeyIndex, GroupAddr, GroupDownLinkCounter) == false)
									{
										frame.AppData[0] = 2;
										InsertOneFrame(&frame);
										return;
									}
								}
								else
								{
									frame.AppData[0] = 3;
									InsertOneFrame(&frame);
									return;
								}
							}
							else
							{
								frame.AppData[0] = 4;
								InsertOneFrame(&frame);
								return;
							}
							
							if(mcpsIndication->BufferSize >= 12)
							{
									GroupAddr = (mcpsIndication->Buffer[9]<<16)|(mcpsIndication->Buffer[10]<<8)|(mcpsIndication->Buffer[11])|0xff000000;
									KeyIndex = (mcpsIndication->Buffer[0]&0xf0)>>4;
							
									if(RemoveGroupAddrFromMulticastList(GroupAddr) == true)	//Steven@20180726: Must remove the same multicast adress first if existed. 
									{
										uint32_t GroupDownLinkCounter = 0;
										
										if(AddGroupAddrToMulticastList(KeyIndex, GroupAddr, GroupDownLinkCounter) == true)
										{
											PRINTF("Add fault group addr %d-%X-%d successfully.\r\n", KeyIndex, GroupAddr, GroupDownLinkCounter);									
											
											if(CheckMulticastInFlash(KeyIndex, GroupAddr, GroupDownLinkCounter) == false)
											{
												frame.AppData[0] = 5;
												InsertOneFrame(&frame);
												return;
											}
										}
										else
										{
											frame.AppData[0] = 6;
											InsertOneFrame(&frame);
											return;
										}
									}
									else
									{
										frame.AppData[0] = 7;
										InsertOneFrame(&frame);
										return;
									}
							}
							
							uint32_t TimeSyncDownLinkCounter = 0;
							
							TimeSyncDownLinkCounter = (mcpsIndication->Buffer[6]<<16)|(mcpsIndication->Buffer[7]<<8)|mcpsIndication->Buffer[8];
							
#if   defined( REGION_CN470 ) 
							KeyIndex = 0; 
							GroupAddr = 0xffffffff;
#elif defined( REGION_EU868 )
							KeyIndex = 1; 
							GroupAddr = 0xfffffffe;
#elif defined( REGION_AU915 ) 
							KeyIndex = 2; 
							GroupAddr = 0xfffffffd;
#elif defined( REGION_US915 ) 
							KeyIndex = 3; 
							GroupAddr = 0xfffffffc;
#elif defined( REGION_AS923 ) 
							KeyIndex = 4; 
							GroupAddr = 0xfffffffb;
#endif
							
							TimeSyncDownCountTmp = TimeSyncDownLinkCounter;
							SetMulticastDownlinkCounter(GroupAddr, TimeSyncDownLinkCounter);
									
							if(CheckMulticastInFlash(KeyIndex, GroupAddr, TimeSyncDownLinkCounter) == false)
							{
								frame.AppData[0] = 8;
								InsertOneFrame(&frame);
								return;
							}
							
							PRINTF("Add timesync group addr %d-%X-%d successfully.\r\n", KeyIndex, GroupAddr, TimeSyncDownLinkCounter);
							
							StreetLightParam.TxCycleCount = 1;
							SetTxDutyCycle(randr( 6000, StreetLightParam.TxCycleTimeLag[0] * 60000 )); //6s~2min
												
							frame.AppData[0] = 1;
							InsertOneFrame(&frame);
						}
						else
						{
							frame.AppData[0] = 9;
							InsertOneFrame(&frame);
						}
						
						break;
				case MULTICAST_REMOVE_SETTING_PORT:
					
						if(mcpsIndication->BufferSize >= 3)
						{
							GroupAddr = (mcpsIndication->Buffer[0]<<16)|(mcpsIndication->Buffer[1]<<8)|(mcpsIndication->Buffer[2])|0xff000000;
						}
						else
						{
								Uart1.PrintfLog("Param error!\r\n");
								return;
						}
						
						if(RemoveGroupAddrFromMulticastList(GroupAddr) == true)
						{
								AppPort = MULTICAST_REMOVE_SETTING_PORT;
								frame.port = MULTICAST_REMOVE_SETTING_PORT;
								frame.IsTxConfirmed = 1;
								frame.nbRetries = 3;
								frame.AppDataSize = 1;							
								frame.AppData[0] = 1;
								InsertOneFrame(&frame);
								Uart1.PrintfLog("Removed group addr %X successfully.\r\n", GroupAddr);
						}
						else
						{
								Uart1.PrintfLog("Removed group addr %X failed.\r\n", GroupAddr);
						}
						break;
				case FRAME_NUM_SETTING_PORT:
					
						if(mcpsIndication->BufferSize >= 9)
						{
							uint32_t TimeSyncDownLinkCounter = 0, TimeSyncAddr, GroupDownLinkCounter = 0;
							
							GroupAddr = (mcpsIndication->Buffer[1]<<16)|(mcpsIndication->Buffer[2]<<8)|(mcpsIndication->Buffer[3])|0xff000000;
							GroupDownLinkCounter = ((mcpsIndication->Buffer[4]<<8)|mcpsIndication->Buffer[5])<<8;								
							TimeSyncDownLinkCounter = (mcpsIndication->Buffer[6]<<16)|(mcpsIndication->Buffer[7]<<8)|mcpsIndication->Buffer[8];
							
#if   defined( REGION_CN470 ) 
							TimeSyncAddr = 0xffffffff;
#elif defined( REGION_EU868 )
							TimeSyncAddr = 0xfffffffe;
#elif defined( REGION_AU915 ) 
							TimeSyncAddr = 0xfffffffd;
#elif defined( REGION_US915 ) 
							TimeSyncAddr = 0xfffffffc;
#elif defined( REGION_AS923 ) 
							TimeSyncAddr = 0xfffffffb;
#endif
							Uart1.PrintfLog("TS counter:%d GD counter:%d\r\n", TimeSyncDownLinkCounter, GroupDownLinkCounter);
							SetMulticastDownlinkCounter(TimeSyncAddr, TimeSyncDownLinkCounter);
							SetMulticastDownlinkCounter(GroupAddr, GroupDownLinkCounter);
						}
						
						OffLineTimerReset();						
						
						break;
				case FAULT_PARAM_PORT:
						
#if defined (FAULT_HANDLE_ENABLE)	
						if(mcpsIndication->BufferSize == 9)
						{
							PRINTF("FAULT_PARAM_SETTING\r\n");
							SetFaultThreshold(mcpsIndication->Buffer, mcpsIndication->BufferSize);
						}
						else
							PRINTF("FAULT_PARAM_ERROR\r\n");
#endif
						break;
#if defined (FOTA_ENABLE) //George@20181026:FOTA
				case FOTA_ADD_MULTICAST_PORT:
						FOTA_AddTempMulticast(mcpsIndication->Buffer, mcpsIndication->BufferSize);
						break;
				case FOTA_RECEIVE_DATA_PORT:
					  FOTA_PacketProcess(mcpsIndication->Buffer, mcpsIndication->BufferSize);
						break;
				case FOTA_GET_VERSION_PORT:
						FOTA_GetVersionNum(mcpsIndication->Buffer, mcpsIndication->BufferSize);
						break;
				case FOTA_BOOTLOADER_PORT:
						FOTA_SystemBootloader(mcpsIndication->Buffer, mcpsIndication->BufferSize);
						break;
#endif
				case SYSTEM_RESET_PORT:	
						System_SoftReset(mcpsIndication->Buffer, mcpsIndication->BufferSize);
						break;
				case CHEAK_CUR_MULTICAST_PORT:
						Check_MulticastAddr(mcpsIndication->Buffer, mcpsIndication->BufferSize);
						break;
        case 224:
            if( ComplianceTest.Running == false )
            {
                // Check compliance test enable command (i)
                if( ( mcpsIndication->BufferSize == 4 ) &&
                    ( mcpsIndication->Buffer[0] == 0x01 ) &&
                    ( mcpsIndication->Buffer[1] == 0x01 ) &&
                    ( mcpsIndication->Buffer[2] == 0x01 ) &&
                    ( mcpsIndication->Buffer[3] == 0x01 ) )
                {
                    IsTxConfirmed = false;
                    AppPort = 224;
                    AppDataSize = 2;
                    ComplianceTest.DownLinkCounter = 0;
                    ComplianceTest.LinkCheck = false;
                    ComplianceTest.DemodMargin = 0;
                    ComplianceTest.NbGateways = 0;
                    ComplianceTest.Running = true;
                    ComplianceTest.State = 1;

                    MibRequestConfirm_t mibReq;
                    mibReq.Type = MIB_ADR;
                    mibReq.Param.AdrEnable = true;
                    LoRaMacMibSetRequestConfirm( &mibReq );

#if defined( REGION_EU868 )
                    LoRaMacTestSetDutyCycleOn( false );
#endif
                }
            }
            else
            {
                ComplianceTest.State = mcpsIndication->Buffer[0];
                switch( ComplianceTest.State )
                {
                case 0: // Check compliance test disable command (ii)
                    IsTxConfirmed = LORAWAN_CONFIRMED_MSG_ON;
                    AppPort = LORAWAN_APP_PORT;
                    AppDataSize = LORAWAN_APP_DATA_SIZE;
                    ComplianceTest.DownLinkCounter = 0;
                    ComplianceTest.Running = false;

                    MibRequestConfirm_t mibReq;
                    mibReq.Type = MIB_ADR;
                    mibReq.Param.AdrEnable = LORAWAN_ADR_ON;
                    LoRaMacMibSetRequestConfirm( &mibReq );
#if defined( REGION_EU868 )
                    LoRaMacTestSetDutyCycleOn( LORAWAN_DUTYCYCLE_ON );
#endif
                    break;
                case 1: // (iii, iv)
                    AppDataSize = 2;
                    break;
                case 2: // Enable confirmed messages (v)
                    IsTxConfirmed = true;
                    ComplianceTest.State = 1;
                    break;
                case 3:  // Disable confirmed messages (vi)
                    IsTxConfirmed = false;
                    ComplianceTest.State = 1;
                    break;
                case 4: // (vii)
                    AppDataSize = mcpsIndication->BufferSize;

                    AppData[0] = 4;
                    for( uint8_t i = 1; i < MIN( AppDataSize, LORAWAN_APP_DATA_MAX_SIZE ); i++ )
                    {
                        AppData[i] = mcpsIndication->Buffer[i] + 1;
                    }
                    break;
                case 5: // (viii)
                    {
                        MlmeReq_t mlmeReq;
                        mlmeReq.Type = MLME_LINK_CHECK;
                        LoRaMacMlmeRequest( &mlmeReq );
                    }
                    break;
                case 6: // (ix)
                    {
                        MlmeReq_t mlmeReq;

                        // Disable TestMode and revert back to normal operation
                        IsTxConfirmed = LORAWAN_CONFIRMED_MSG_ON;
                        AppPort = LORAWAN_APP_PORT;
                        AppDataSize = LORAWAN_APP_DATA_SIZE;
                        ComplianceTest.DownLinkCounter = 0;
                        ComplianceTest.Running = false;

                        MibRequestConfirm_t mibReq;
                        mibReq.Type = MIB_ADR;
                        mibReq.Param.AdrEnable = LORAWAN_ADR_ON;
                        LoRaMacMibSetRequestConfirm( &mibReq );
#if defined( REGION_EU868 )
                        LoRaMacTestSetDutyCycleOn( LORAWAN_DUTYCYCLE_ON );
#endif
                        mlmeReq.Type = MLME_JOIN;

                        mlmeReq.Req.Join.DevEui = DevEui;
                        mlmeReq.Req.Join.AppEui = AppEui;
                        mlmeReq.Req.Join.AppKey = AppKey;
                        mlmeReq.Req.Join.NbTrials = 3;

                        LoRaMacMlmeRequest( &mlmeReq );
                        DeviceState = DEVICE_STATE_SLEEP;
                    }
                    break;
                case 7: // (x)
                    {
                        if( mcpsIndication->BufferSize == 3 )
                        {
                            MlmeReq_t mlmeReq;
                            mlmeReq.Type = MLME_TXCW;
                            mlmeReq.Req.TxCw.Timeout = ( uint16_t )( ( mcpsIndication->Buffer[1] << 8 ) | mcpsIndication->Buffer[2] );
                            LoRaMacMlmeRequest( &mlmeReq );
                        }
                        else if( mcpsIndication->BufferSize == 7 )
                        {
                            MlmeReq_t mlmeReq;
                            mlmeReq.Type = MLME_TXCW_1;
                            mlmeReq.Req.TxCw.Timeout = ( uint16_t )( ( mcpsIndication->Buffer[1] << 8 ) | mcpsIndication->Buffer[2] );
                            mlmeReq.Req.TxCw.Frequency = ( uint32_t )( ( mcpsIndication->Buffer[3] << 16 ) | ( mcpsIndication->Buffer[4] << 8 ) | mcpsIndication->Buffer[5] ) * 100;
                            mlmeReq.Req.TxCw.Power = mcpsIndication->Buffer[6];
                            LoRaMacMlmeRequest( &mlmeReq );
                        }
                        ComplianceTest.State = 1;
                    }
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
    }
}

/*!
 * \brief   MLME-Confirm event function
 *
 * \param   [IN] mlmeConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void MlmeConfirm( MlmeConfirm_t *mlmeConfirm )
{
		MibRequestConfirm_t mibReq;
	
    switch( mlmeConfirm->MlmeRequest )
    {
        case MLME_JOIN:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
                // Status is OK, node has joined the network
#ifdef GL5516_PHOTOREISTANCE_ENABLE
							StreetLightParam.lux = GL5516VoltageConvertToLumen();	
#else
							StreetLightParam.lux = ReadOpt3001Value();
#endif

							for(uint8_t i = 0; i < 3; i++)
							{
								StreetLightParam.CurrentLuxBuf[i] = StreetLightParam.lux;
							}
							
							if(CtrlGroupAddrFlag)
								TimerSetValue( &TxNextPacketTimer, 500 ); //Steven@20181011:500 //180000
							else
								TimerSetValue( &TxNextPacketTimer, 180000 ); //Steven@20181026
							
							TimerStart( &TxNextPacketTimer );
							OffLineTimerReset();
							
							SetNetStateLed(1);
							StreetLightParam.JoinState = 1;
							
							if (AutoTestMode_flag == true)
								Uart1.PrintfTestLog("+NJOIN:OK\r\n"); 
							else
								Uart1.PrintfLog("Join net OK!\r\n");
							
							mibReq.Type = MIB_DEVICE_CLASS;
              mibReq.Param.Class = CLASS_C;
              LoRaMacMibSetRequestConfirm( &mibReq ); //George@20181122:switch to ClassC mode
							Uart1.PrintfLog("ClassA switch to ClassC successfully\r\n");
							
#if defined( REGION_EU868 )	//Added by Steven in 2018/06/08.
							//MibRequestConfirm_t mibReq;
							GetPhyParams_t getPhy;
							PhyParam_t phyParam;
							
							getPhy.Attribute = PHY_DEF_TX_DR;
							phyParam = RegionGetPhyParam( LORAMAC_REGION_EU868, &getPhy );
							
							mibReq.Type = MIB_CHANNELS_DATARATE;
							mibReq.Param.ChannelsDatarate = phyParam.Value;
							Uart1.PrintfLog("ChannelsDatarate:%d\r\n", phyParam.Value);
							LoRaMacMibSetRequestConfirm( &mibReq );
#endif
							
#if defined ( REGION_AS923 ) //George@20180824:Set the rate of the first packet uplink.
							//MibRequestConfirm_t mibReq;
							GetPhyParams_t getPhy;
							PhyParam_t phyParam;
							
							getPhy.Attribute = PHY_DEF_TX_DR;
							phyParam = RegionGetPhyParam( LORAMAC_REGION_AS923, &getPhy );
							
							mibReq.Type = MIB_CHANNELS_DATARATE;
							mibReq.Param.ChannelsDatarate = phyParam.Value;
							Uart1.PrintfLog("ChannelsDatarate:%d\r\n", phyParam.Value);
							LoRaMacMibSetRequestConfirm( &mibReq );
#endif
						
#if defined( REGION_US915 )	//Added by Steven for the server error setting.
							//MibRequestConfirm_t mibReq;
							mibReq.Type = MIB_RX2_CHANNEL;
							LoRaMacMibGetRequestConfirm( &mibReq );
							mibReq.Param.Rx2Channel.Datarate = DR_8;
							LoRaMacMibSetRequestConfirm( &mibReq );
#endif
            }
            else
            {
								if (AutoTestMode_flag == true)
									Uart1.PrintfTestLog("+NJOIN:FAIL\r\n"); 
							
                // Join was not successful. Try to join again
                TimerSetValue( &NetRejoinTimer, 900000 );  //15min
                TimerStart( &NetRejoinTimer );	
							
#if defined (DS1302_RTC_ENABLE)  //George@20181023:If no profile, enter auto dimming mode.
								if(LoadProfileFromFlash() != NO_PROFILE)	
								{
										EnterDimmingMode(PROFILE_DIMMING_MODE, 0); 
										Uart1.PrintfLog("Join net failed! Enter profile dimming mode\r\n");
								}
								else
								{
										EnterDimmingMode(AUTO_DIMMING_MODE, 0); 
										Uart1.PrintfLog("Join net failed! NO profile enter auto dimming mode\r\n");
								}
#else
								EnterDimmingMode(AUTO_DIMMING_MODE, 0); 
								Uart1.PrintfLog("Join net failed! NO profile enter auto dimming mode\r\n");
#endif
            }
            break;
        }
        case MLME_LINK_CHECK:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
                // Check DemodMargin
                // Check NbGateways
                if( ComplianceTest.Running == true )
                {
                    ComplianceTest.LinkCheck = true;
                    ComplianceTest.DemodMargin = mlmeConfirm->DemodMargin;
                    ComplianceTest.NbGateways = mlmeConfirm->NbGateways;
                }
            }
            break;
        }
        default:
            break;
    }
    NextTx = true;
}

/**
 * Feed dog process.
 */
void FeedDogProcess(void)
{
	static uint32_t currentTime = 0, contextTime = 0;
	uint32_t timeOutValue = 0;	
	
	currentTime = RtcGetTimerValue();	
		
	if( currentTime < contextTime )
	{
		timeOutValue = currentTime + ( 0xFFFFFFFF - contextTime );
	}
	else
	{
		timeOutValue = currentTime - contextTime;
	}
		
	if( timeOutValue > 2000 )	//Timeout. 2s.
	{
		contextTime = currentTime;
		
#if defined (WATCH_DOG_ENABLE)	
		if(StreetLightParam.JoinState)
		{
			if(AutoTestMode_flag == false && FirmUpgrade.FotaFlag == false)
			{
				if(StreetLightParam.FeedDogCount < 600)	//If there is no any uplink within 20 minutes, stop feeding dog and reset.
				{
					HAL_IWDG_Refresh(&hiwdg);
					StreetLightParam.FeedDogCount++;
				}	
			}
			else
			{
				StreetLightParam.FeedDogCount = 0;
				HAL_IWDG_Refresh(&hiwdg);
			}
		}
		else
		{
			StreetLightParam.FeedDogCount = 0;
			HAL_IWDG_Refresh(&hiwdg);
		}
#endif
	}	
}

/**
 * Main application entry point.
 */
int main( void )
{
  LoRaMacPrimitives_t LoRaMacPrimitives;
  LoRaMacCallback_t LoRaMacCallbacks;
  MibRequestConfirm_t mibReq;
	uint32_t TxRandTime;								

  BoardInitMcu( );
  BoardInitPeriph( );
	//EraseMcuEEPROM();
	LoadMulticastFromFlash();
	
#if defined (WATCH_DOG_ENABLE)
	MX_IWDG_Init();            //shengyu@20180806:watchdog init
#if defined (STM32L073xx) || defined (STM32L072xx)
	HAL_IWDG_Start(&hiwdg);
#endif
#endif
	
	GetSoftwareVersion();
	Uart1.PrintfLog("[Boot Version]:%s\r\n", BOOT_VERSION);
	Uart1.PrintfLog("[App  Version]:%s\r\n", strVersion);
	
#if defined (FAULT_HANDLE_ENABLE)	
	FaultHandlerInit();
#endif
		
	/*Randomize the joining time.*/
  DeviceState = DEVICE_STATE_INIT;	

  while( 1 )
	{
		
		UartTimeoutHandler();	   //For rn820x calibration.
		
		DownlinkReceiveEvent();  //George@20180630:send test mode downlink value
		RelayTestCount();        //George@20180710:relay test count
		
		FeedDogProcess();	       //Feed dog.
		
		if (LightDimmingMode == DALI_dimming_mode)
		{
				dali_send_function();    //shengyu@20180806:add dali dimming
				dali_recv_function();
		}
			
#if defined (FAULT_HANDLE_ENABLE)
		if(StreetLightParam.ReportFlag)
			LightFaultsFsm();
#endif
		
		switch( DeviceState )
    {
            case DEVICE_STATE_INIT:
            {
                LoRaMacPrimitives.MacMcpsConfirm = McpsConfirm;
                LoRaMacPrimitives.MacMcpsIndication = McpsIndication;
                LoRaMacPrimitives.MacMlmeConfirm = MlmeConfirm;
                LoRaMacCallbacks.GetBatteryLevel = BoardGetBatteryLevel;
#if defined( REGION_AS923 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_AS923 );
#elif defined( REGION_AU915 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_AU915 );
#elif defined( REGION_CN470 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_CN470 );
#elif defined( REGION_CN779 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_CN779 );
#elif defined( REGION_EU433 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_EU433 );
#elif defined( REGION_EU868 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_EU868 );
#elif defined( REGION_IN865 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_IN865 );
#elif defined( REGION_KR920 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_KR920 );
#elif defined( REGION_US915 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_US915 );
#elif defined( REGION_US915_HYBRID )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_US915_HYBRID );
#else
    #error "Please define a region in the compiler options."
#endif
							
								TimerInit( &NetRejoinTimer, OnNetRejoinTimerEvent );
								TimerInit( &NetLedStateTimer, OnNetLedStateEvent );
								TimerSetValue( &NetLedStateTimer, 500 );
								TimerStart( &NetLedStateTimer );
								TimerInit( &TxNextPacketTimer, OnTxNextPacketTimerEvent );
								
								TimerInit(&DeviceOfflineTimer, DeviceDropStateEvent);         //George@20180711:init off-line state timer
								TimerInit(&ADRJustModeExitTimer, ADRJustModeTimeoutEvent);    //George@20180718:init ADR just timeout timer
#if defined (FOTA_ENABLE)
								TimerInit(&SendFOTAStateTimer, SendFOTAStateEvent);           //George@20181024:init FOTA report state
#endif

								GpioInit( &Led3, PA_12, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
								DeviceState = DEVICE_STATE_IDLE;
								
#ifdef DEBUG_MODE_ENABLE
								TxRandTime = randr( 20, 1000);
#else
								TxRandTime = randr( 20, 120000);
#endif
								Uart1.PrintfLog("rand:%d\r\n", TxRandTime);
								
								TimerSetValue( &NetRejoinTimer, TxRandTime );	//20ms~2min.
								TimerStart( &NetRejoinTimer );
								
                break;
            }
            case DEVICE_STATE_JOIN:
            {
#if defined( REGION_AS923 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_AS923 );
#elif defined( REGION_AU915 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_AU915 );
#elif defined( REGION_CN470 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_CN470 );
#elif defined( REGION_CN779 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_CN779 );
#elif defined( REGION_EU433 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_EU433 );
#elif defined( REGION_EU868 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_EU868 );
#elif defined( REGION_IN865 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_IN865 );
#elif defined( REGION_KR920 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_KR920 );
#elif defined( REGION_US915 )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_US915 );
#elif defined( REGION_US915_HYBRID )
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_US915_HYBRID );
#else
    #error "Please define a region in the compiler options."
#endif

                mibReq.Type = MIB_ADR;
                mibReq.Param.AdrEnable = LORAWAN_ADR_ON;
                LoRaMacMibSetRequestConfirm( &mibReq );

                mibReq.Type = MIB_PUBLIC_NETWORK;
                mibReq.Param.EnablePublicNetwork = LORAWAN_PUBLIC_NETWORK;
                LoRaMacMibSetRequestConfirm( &mibReq );
							
								mibReq.Type = MIB_DEVICE_CLASS;
                mibReq.Param.Class = CLASS_A;
                LoRaMacMibSetRequestConfirm( &mibReq );

#if defined( REGION_EU868 )
                LoRaMacTestSetDutyCycleOn( LORAWAN_DUTYCYCLE_ON );

#if( USE_SEMTECH_DEFAULT_CHANNEL_LINEUP == 1 )
                LoRaMacChannelAdd( 3, ( ChannelParams_t )LC4 );
                LoRaMacChannelAdd( 4, ( ChannelParams_t )LC5 );
                LoRaMacChannelAdd( 5, ( ChannelParams_t )LC6 );
                LoRaMacChannelAdd( 6, ( ChannelParams_t )LC7 );
                LoRaMacChannelAdd( 7, ( ChannelParams_t )LC8 );
                LoRaMacChannelAdd( 8, ( ChannelParams_t )LC9 );
                LoRaMacChannelAdd( 9, ( ChannelParams_t )LC10 );

                mibReq.Type = MIB_RX2_DEFAULT_CHANNEL;
                mibReq.Param.Rx2DefaultChannel = ( Rx2ChannelParams_t ){ 869525000, DR_0 };
                LoRaMacMibSetRequestConfirm( &mibReq );

                mibReq.Type = MIB_RX2_CHANNEL;
                mibReq.Param.Rx2Channel = ( Rx2ChannelParams_t ){ 869525000, DR_0 };
                LoRaMacMibSetRequestConfirm( &mibReq );
#endif

#endif
								
#if defined (REGION_AS923)  //George@20180824:add as923 channel
								LoRaMacTestSetDutyCycleOn( LORAWAN_DUTYCYCLE_ON );
#if defined (AS923_INDONESIA_ENABLE)
								LoRaMacChannelAdd(2, ( ChannelParams_t ){ 923600000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 });
								LoRaMacChannelAdd(3, ( ChannelParams_t ){ 923800000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 });
								LoRaMacChannelAdd(4, ( ChannelParams_t ){ 924000000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 });
								LoRaMacChannelAdd(5, ( ChannelParams_t ){ 924200000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 });
								LoRaMacChannelAdd(6, ( ChannelParams_t ){ 924400000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 });
								LoRaMacChannelAdd(7, ( ChannelParams_t ){ 924600000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 });
								//LoRaMacChannelAdd(8, ( ChannelParams_t ){ 924800000, 0, { ( ( DR_7 << 4 ) | DR_7 ) }, 2 }); //125k/FSK
								LoRaMacChannelAdd(9, ( ChannelParams_t ){ 924500000, 0, { ( ( DR_6 << 4 ) | DR_6 ) }, 1 }); //250k/LoRa
#else
								LoRaMacChannelAdd(2, ( ChannelParams_t ){ 922000000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 });
								LoRaMacChannelAdd(3, ( ChannelParams_t ){ 922200000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 });
								LoRaMacChannelAdd(4, ( ChannelParams_t ){ 922400000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 });
								LoRaMacChannelAdd(5, ( ChannelParams_t ){ 922600000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 });
								LoRaMacChannelAdd(6, ( ChannelParams_t ){ 922800000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 });
								LoRaMacChannelAdd(7, ( ChannelParams_t ){ 923000000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 });
								//LoRaMacChannelAdd(8, ( ChannelParams_t ){ 921800000, 0, { ( ( DR_7 << 4 ) | DR_7 ) }, 2 }); //125k/FSK
								LoRaMacChannelAdd(9, ( ChannelParams_t ){ 922100000, 0, { ( ( DR_6 << 4 ) | DR_6 ) }, 1 }); //250k/LoRa
#endif
								
#endif
							
#if( OVER_THE_AIR_ACTIVATION != 0 )
                MlmeReq_t mlmeReq;

                // Initialize LoRaMac device unique ID
                BoardGetUniqueId( DevEui );

                mlmeReq.Type = MLME_JOIN;

                mlmeReq.Req.Join.DevEui = DevEui;
                mlmeReq.Req.Join.AppEui = AppEui;
                mlmeReq.Req.Join.AppKey = AppKey;
							
								for(uint8_t i = 0; i < 8; i++)
								{
									AppKey[i] = DevEui[i];
								}
												
								mlmeReq.Req.Join.NbTrials = 35;//20;//5;	//Modified by Steven in 2018/06/08.
											
								Uart1.PrintfLog("OTAA\n\r"); 
								Uart1.PrintfLog("DevEui= %02X", DevEui[0]) ;for(int i=1; i<8 ; i++) {Uart1.PrintfLog("-%02X", DevEui[i]); }; Uart1.PrintfLog("\n\r");
								Uart1.PrintfLog("AppEui= %02X", AppEui[0]) ;for(int i=1; i<8 ; i++) {Uart1.PrintfLog("-%02X", AppEui[i]); }; Uart1.PrintfLog("\n\r");
								Uart1.PrintfLog("AppKey= %02X", AppKey[0]) ;for(int i=1; i<16; i++) {Uart1.PrintfLog(" %02X", AppKey[i]); }; Uart1.PrintfLog("\n\n\r");

                if( NextTx == true )
                {
                    LoRaMacMlmeRequest( &mlmeReq );
                }
                DeviceState = DEVICE_STATE_SLEEP;
#else
                // Choose a random device address if not already defined in Commissioning.h
                if( DevAddr == 0 )
                {
                    // Random seed initialization
                    srand1( BoardGetRandomSeed( ) );

                    // Choose a random device address
                    DevAddr = randr( 0, 0x01FFFFFF );
                }
								
								// Initialize LoRaMac device unique ID
                BoardGetUniqueId( DevEui );
								
								Uart1.PrintfLog("ABP\n\r"); 
								Uart1.PrintfLog("DevEui= %02X", DevEui[0]) ;for(int i=1; i<8 ; i++) {Uart1.PrintfLog("-%02X", DevEui[i]); }; Uart1.PrintfLog("\n\r");
								Uart1.PrintfLog("DevAdd=  %08X\n\r", DevAddr) ;
								Uart1.PrintfLog("NwkSKey= %02X", NwkSKey[0]) ;for(int i=1; i<16 ; i++) {Uart1.PrintfLog(" %02X", NwkSKey[i]); }; Uart1.PrintfLog("\n\r");
								Uart1.PrintfLog("AppSKey= %02X", AppSKey[0]) ;for(int i=1; i<16 ; i++) {Uart1.PrintfLog(" %02X", AppSKey[i]); }; Uart1.PrintfLog("\n\r");

                mibReq.Type = MIB_NET_ID;
                mibReq.Param.NetID = LORAWAN_NETWORK_ID;
                LoRaMacMibSetRequestConfirm( &mibReq );

                mibReq.Type = MIB_DEV_ADDR;
                mibReq.Param.DevAddr = DevAddr;
                LoRaMacMibSetRequestConfirm( &mibReq );

                mibReq.Type = MIB_NWK_SKEY;
                mibReq.Param.NwkSKey = NwkSKey;
                LoRaMacMibSetRequestConfirm( &mibReq );

                mibReq.Type = MIB_APP_SKEY;
                mibReq.Param.AppSKey = AppSKey;
                LoRaMacMibSetRequestConfirm( &mibReq );

                mibReq.Type = MIB_NETWORK_JOINED;
                mibReq.Param.IsNetworkJoined = true;
                LoRaMacMibSetRequestConfirm( &mibReq );

                DeviceState = DEVICE_STATE_SLEEP;
								TimerSetValue( &TxNextPacketTimer, 500 );
                TimerStart( &TxNextPacketTimer );
							
								StreetLightParam.JoinState = 1;
								Uart1.PrintfLog("Join net OK!\r\n");
#endif
                break;
            }
            case DEVICE_STATE_SEND:
            {
                if( NextTx == true )
                {
                    PrepareTxFrame( AppPort );

                    NextTx = SendFrame( );
                }
                if( ComplianceTest.Running == true )
                {
                    // Schedule next packet transmission
                    TxDutyCycleTime = 5000; // 5000 ms
                }
                else
                {
                    // Schedule next packet transmission
                    TxDutyCycleTime = APP_TX_DUTYCYCLE + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
                }
								
								if( NextTx == true )	//Transmission failed.
									DeviceState = DEVICE_STATE_SEND;
								else
								{
									DeviceState = DEVICE_STATE_CYCLE;
									
									if(AppPort != LIGHT_STATUS_PORT)
									{
										AppPort = LIGHT_STATUS_PORT;
									}
								}
								
                break;
            }
            case DEVICE_STATE_CYCLE:
            {
                DeviceState = DEVICE_STATE_SLEEP;

                // Schedule next packet transmission
								AppPort = LIGHT_STATUS_PORT;
                TimerSetValue( &TxNextPacketTimer, TxDutyCycleTime );
                TimerStart( &TxNextPacketTimer );
                break;
            }
            case DEVICE_STATE_SLEEP:
            {
								StreetLightFsm(); 
								if(LoRaMacState == 0 )
								{
									if(!EmptyQueue(&Qmsg))  
									{  
										SendQueueFrames();										
									} 
								}
				
								// Wake up through events
								if(StreetLightParam.JoinState == 0)
										TimerLowPowerHandler( );
								break;
            }
						case DEVICE_STATE_IDLE:
								//Do noting, only for the joining time randomization.
								break;
            default:
            {
                DeviceState = DEVICE_STATE_INIT;
                break;
            }
        }
    }
}

void SetTxDutyCycle(uint32_t time) //ms
{
	TimerSetValue( &TxNextPacketTimer, time );
	TimerStart( &TxNextPacketTimer );
}
