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
#include <string.h>
#include <math.h>
#include "board.h"

#include "LoRaMac.h"
#include "Commissioning.h"
#include "multicast.h"
#include "internal-eeprom.h"

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

#if defined( USE_BAND_868 )

#include "LoRaMacTest.h"

/*!
 * LoRaWAN ETSI duty cycle control enable/disable
 *
 * \remark Please note that ETSI mandates duty cycled transmissions. Use only for test purposes
 */
#define LORAWAN_DUTYCYCLE_ON                        true

#define USE_SEMTECH_DEFAULT_CHANNEL_LINEUP          1

#if( USE_SEMTECH_DEFAULT_CHANNEL_LINEUP == 1 )

#define LC4                { 867100000, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC5                { 867300000, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC6                { 867500000, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC7                { 867700000, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC8                { 867900000, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC9                { 868800000, { ( ( DR_7 << 4 ) | DR_7 ) }, 2 }
#define LC10               { 868300000, { ( ( DR_6 << 4 ) | DR_6 ) }, 1 }

#endif

#endif

/*!
 * LoRaWAN application port
 */
#define LORAWAN_APP_PORT		2
#define LIGHT_STATUS_PORT		3
#define LIGHT_CTRL_PORT			4

#define PROFILE1_PORT				100
#define PROFILE2_PORT				101
#define TIME_SYNC_PORT			150

/*!
 * User application data buffer size
 */
#if defined( USE_BAND_433 ) || defined( USE_BAND_470 ) || defined( USE_BAND_780 ) || defined( USE_BAND_868 )

#define LORAWAN_APP_DATA_SIZE                       16

#elif defined( USE_BAND_915 ) || defined( USE_BAND_915_HYBRID )

#define LORAWAN_APP_DATA_SIZE                       11

#endif

static uint8_t DevEui[] = LORAWAN_DEVICE_EUI;
static uint8_t AppEui[] = LORAWAN_APPLICATION_EUI;
static uint8_t AppKey[] = LORAWAN_APPLICATION_KEY;

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
#define LORAWAN_APP_DATA_MAX_SIZE                           64

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
static TimerEvent_t TxNextPacketTimer;



/*!
 * Indicates if a new packet can be sent
 */
static bool NextTx = true;

static bool JoinNetOk = false;

static int16_t LastRssi = 0;
static uint16_t LastFcnt = 0;	

/*+++++++++++++++++++++++++++++++++*/
Gpio_t Led3;
extern uint32_t LoRaMacState;
RN820x_t RN280xInfo;
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
    DEVICE_STATE_SLEEP
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

/*!
 * \brief   Prepares the payload of the frame
 */
static void PrepareTxFrame( uint8_t port )
{
		int16_t LastRssiTmp;
	
		switch( port )
    {
    case 2:
        {
#if defined( USE_BAND_433 ) || defined( USE_BAND_470 ) || defined( USE_BAND_780 ) || defined( USE_BAND_868 )
            
            /*AppData[0] = (uint8_t)(0-LastRssi);
            AppData[1] = (LastFcnt>>8)&0xff;
            AppData[2] = LastFcnt&0xff;*/
						LastRssiTmp = LastRssi;
					
						if(LastRssiTmp < 0)
						{							
							LastRssiTmp = 0 - LastRssiTmp;
						}							

						AppData[0] = ((uint8_t)(LastRssiTmp))/100;
						
						AppData[1] = (uint8_t)((((uint8_t)(LastRssiTmp))%100)/10)*16+(((uint8_t)(LastRssiTmp))%10);
						AppData[2] = (uint8_t)(LastFcnt/10000);
						AppData[3] = ((uint8_t)(((uint8_t)((LastFcnt%10000)/100))/10))*16+(((uint8_t)((LastFcnt%10000)/100))%10);
            AppData[4] = ((uint8_t)((LastFcnt%100)/10))*16+(LastFcnt%10);
						AppDataSize = 5;
#elif defined( USE_BAND_915 ) || defined( USE_BAND_915_HYBRID )
            
            AppData[0] = 0x30;
            AppData[1] = 0x31;                                           // Signed degrees celsius in half degree units. So,  +/-63 C
            AppData[2] = 0x32;                                          // Per LoRaWAN spec; 0=Charging; 1...254 = level, 255 = N/A
            AppData[3] = 0x33;
            AppData[4] = 0x34;
            AppData[5] = 0x35;
            AppData[6] = 0x36;
            AppData[7] = 0x37;
            AppData[8] = 0x38;
            AppData[9] = 0x39;
            AppData[10] = 0x3A;
#endif
        }
        break;
		case LIGHT_STATUS_PORT:
		case LIGHT_CTRL_PORT:
					AppDataSize = 11;
					AppData[0] = ((StreetLightParam.ctrlMode<<7)&0x80)|(StreetLightParam.pwmDuty&0x7f);
					AppData[1] = (((uint32_t)(StreetLightParam.lux*100))>>16)&0xff;
					AppData[2] = (((uint32_t)(StreetLightParam.lux*100))>>8)&0xff;
					AppData[3] = ((uint32_t)(StreetLightParam.lux*100))&0xff;
		
					StreetLightParam.kwh += CalculateKwh(StreetLightParam.prePower);
					AppData[4] = (((uint16_t)StreetLightParam.current)>>8)&0xff;
					AppData[5] = ((uint16_t)StreetLightParam.current)&0xff;
					AppData[6] = (((uint16_t)(StreetLightParam.voltage*10))>>8)&0xff;
					AppData[7] = ((uint16_t)(StreetLightParam.voltage*10))&0xff;
					AppData[8] = (((uint32_t)StreetLightParam.kwh)>>16)&0xff;
					AppData[9] = (((uint32_t)StreetLightParam.kwh)>>8)&0xff;
					AppData[10] = ((uint32_t)StreetLightParam.kwh)&0xff;
					PRINTF("I:%f, V:%f, P:%f, Kwh:%f\r\n", StreetLightParam.current, StreetLightParam.voltage, StreetLightParam.power, StreetLightParam.kwh);
			break;
		case MULTICAST_ADD_SETTING_PORT:
		case MULTICAST_REMOVE_SETTING_PORT:
		case MULTICAST_SETTING_PORT:	
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
 * \brief Function executed on TxNextPacket Timeout event
 */
static void OnTxNextPacketTimerEvent( void )
{
    MibRequestConfirm_t mibReq;
    LoRaMacStatus_t status;
		frame_t frame;
	
		//TimerStop( &TxNextPacketTimer );
		PRINTF("light:%flux\r\n", StreetLightParam.lux);
		//ReadOpt3001Reg(0x01);

    mibReq.Type = MIB_NETWORK_JOINED;
    status = LoRaMacMibGetRequestConfirm( &mibReq );

    if( status == LORAMAC_STATUS_OK )
    {
        if( mibReq.Param.IsNetworkJoined == true )
        {
            /*DeviceState = DEVICE_STATE_SEND;
            NextTx = true;*/
						GetRn820xParam(&RN280xInfo);
						
						if((RN280xInfo.power<ABNORMAL_POWER_LEVEL && StreetLightParam.pwmDuty>ABNORMAL_PWM_LEVEL) || (StreetLightParam.pwmDuty<ABNORMAL_PWM_LEVEL && RN280xInfo.power>ABNORMAL_POWER_LEVEL))
							StreetLightParam.AbnormalState = 1;
						else
							StreetLightParam.AbnormalState = 0;
						
						frame.port = LIGHT_STATUS_PORT;
						frame.IsTxConfirmed = 1;
						frame.nbRetries = 3;//3;	Modified by Steven for test in 2017/12/26.
						frame.AppDataSize = 13;
							
						//frame.AppData[0] = ((StreetLightParam.ctrlMode<<7)&0x80)|(StreetLightParam.pwmDuty&0x7f);	//Modified by Steven for new protocol in 2017/12/27.
						frame.AppData[0] = ((StreetLightParam.TimeSync<<7)&0x80)|(StreetLightParam.pwmDuty&0x7f);
						frame.AppData[1] = (((uint32_t)(StreetLightParam.lux*100))>>16)&0xff;
						frame.AppData[2] = (((uint32_t)(StreetLightParam.lux*100))>>8)&0xff;
						frame.AppData[3] = ((uint32_t)(StreetLightParam.lux*100))&0xff;
				
						frame.AppData[4] = (((uint16_t)RN280xInfo.current)>>8)&0xff;
						frame.AppData[5] = ((uint16_t)RN280xInfo.current)&0xff;
							
						frame.AppData[6] = (((uint16_t)(RN280xInfo.voltage*10))>>8)&0xff;
						frame.AppData[7] = ((uint16_t)(RN280xInfo.voltage*10))&0xff;
							
						frame.AppData[8] = ((((uint16_t)(RN280xInfo.power*10))>>8)&0x7f)|((StreetLightParam.AbnormalState<<7)&0x80);
						frame.AppData[9] = ((uint16_t)(RN280xInfo.power*10))&0xff;
							
						frame.AppData[10] = (((uint32_t)(RN280xInfo.kWh*1000))>>16)&0xff;
						frame.AppData[11] = (((uint32_t)(RN280xInfo.kWh*1000))>>8)&0xff;
						frame.AppData[12] = ((uint32_t)(RN280xInfo.kWh*1000))&0xff;
						//PRINTF("I:%f, V:%f, P:%f, Kwh:%f\r\n", StreetLightParam.current, StreetLightParam.voltage, StreetLightParam.power, StreetLightParam.kwh);	//Remove by Steven for test in 2017/12/26.
							
						InsertOneFrame(&frame);
        }
        else
        {
            DeviceState = DEVICE_STATE_JOIN;
        }
    }
		
		TimerSetValue( &TxNextPacketTimer, APP_TX_DUTYCYCLE + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND ) );
		TimerStart( &TxNextPacketTimer );
}

/*!
 * \brief   MCPS-Confirm event function
 *
 * \param   [IN] mcpsConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void McpsConfirm( McpsConfirm_t *mcpsConfirm )
{
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
	
		if( mcpsIndication->Status != LORAMAC_EVENT_INFO_STATUS_OK )
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
		LastRssi = mcpsIndication->Rssi;
		LastFcnt = mcpsIndication->DownLinkCounter;
		

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
							/*TimerSetValue( &TxNextPacketTimer, 500 );
							TimerStart( &TxNextPacketTimer );*/
							GetRn820xParam(&RN280xInfo);
							
							if(RN280xInfo.power<ABNORMAL_POWER_LEVEL && StreetLightParam.pwmDuty>ABNORMAL_PWM_LEVEL)
								StreetLightParam.AbnormalState = 1;
							else
								StreetLightParam.AbnormalState = 0;
							
							frame.port = LIGHT_STATUS_PORT;
							frame.IsTxConfirmed = 1;
							frame.nbRetries = 3;//3;	//Modified by Steven for test in 2017/12/26.
							frame.AppDataSize = 13;
							
							//frame.AppData[0] = ((StreetLightParam.ctrlMode<<7)&0x80)|(StreetLightParam.pwmDuty&0x7f);	//Modified by Steven for new protocol in 2017/12/27.
							frame.AppData[0] = ((StreetLightParam.TimeSync<<7)&0x80)|(StreetLightParam.pwmDuty&0x7f);
							frame.AppData[1] = (((uint32_t)(StreetLightParam.lux*100))>>16)&0xff;
							frame.AppData[2] = (((uint32_t)(StreetLightParam.lux*100))>>8)&0xff;
							frame.AppData[3] = ((uint32_t)(StreetLightParam.lux*100))&0xff;
				
							frame.AppData[4] = (((uint16_t)RN280xInfo.current)>>8)&0xff;
							frame.AppData[5] = ((uint16_t)RN280xInfo.current)&0xff;
							
							frame.AppData[6] = (((uint16_t)(RN280xInfo.voltage*10))>>8)&0xff;
							frame.AppData[7] = ((uint16_t)(RN280xInfo.voltage*10))&0xff;
							
							frame.AppData[8] = ((((uint16_t)(RN280xInfo.power*10))>>8)&0x7f)|((StreetLightParam.AbnormalState<<7)&0x80);
							frame.AppData[9] = ((uint16_t)(RN280xInfo.power*10))&0xff;
							
							frame.AppData[10] = (((uint32_t)(RN280xInfo.kWh*1000))>>16)&0xff;
							frame.AppData[11] = (((uint32_t)(RN280xInfo.kWh*1000))>>8)&0xff;
							frame.AppData[12] = ((uint32_t)(RN280xInfo.kWh*1000))&0xff;
							//PRINTF("I:%f, V:%f, P:%f, Kwh:%f\r\n", StreetLightParam.current, StreetLightParam.voltage, StreetLightParam.power, StreetLightParam.kwh);	//Removed by Steven for test in 2017/12/26.
							
							InsertOneFrame(&frame);
						}
						break;
				case LIGHT_CTRL_PORT:
						StreetLightParam.RecvFlag = 1;
						StreetLightParam.RecvData[0] = mcpsIndication->Buffer[0];
				
						if(StreetLightParam.RecvData[0]&0x80)
						{
							StreetLightParam.ctrlMode = MANUAL_DIMMING_MODE;
							StreetLightParam.pwmDuty = StreetLightParam.RecvData[0]&0x7f;
						}
						else
							StreetLightParam.ctrlMode = AUTO_DIMMING_MODE;
						
						/*TimerSetValue( &TxNextPacketTimer, 500 );
						TimerStart( &TxNextPacketTimer );*/	//Removed in 2017/10/12.
						TimerSetValue( &TxNextPacketTimer, 8000 + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND ) );
						TimerStart( &TxNextPacketTimer );
						break;
				case PROFILE1_PORT:
						AddProfile1(mcpsIndication->Buffer, mcpsIndication->BufferSize, 1);
						break;
				case PROFILE2_PORT:
						AddProfile2(mcpsIndication->Buffer, mcpsIndication->BufferSize, 1);
						break;
				case TIME_SYNC_PORT:
						if(mcpsIndication->BufferSize == 3)
						{
							SyncTimeValue = ((mcpsIndication->Buffer[0]<<16)|(mcpsIndication->Buffer[1]<<8)|(mcpsIndication->Buffer[2]))&0x01ffff;
							TimeSyncFunc(SyncTimeValue);
						}
						
						break;
				case MULTICAST_ADD_SETTING_PORT:
					
						if(mcpsIndication->BufferSize >= 4)
						{
							GroupAddr = (mcpsIndication->Buffer[1]<<16)|(mcpsIndication->Buffer[2]<<8)|(mcpsIndication->Buffer[3])|0xff000000;
							KeyIndex = mcpsIndication->Buffer[0]&0x0f;
						}
						else
						{
							PRINTF("Param error!\r\n");
							return;
						}
				
						if(AddGroupAddrToMulticastList(KeyIndex, GroupAddr) == true)
						{
							PRINTF("Added group addr %X %d successfully.\r\n", GroupAddr, KeyIndex);
							AppPort = MULTICAST_ADD_SETTING_PORT;
							/*TimerSetValue( &TxNextPacketTimer, 1000 );
							TimerStart( &TxNextPacketTimer );*/
							frame.port = MULTICAST_ADD_SETTING_PORT;
							frame.IsTxConfirmed = 1;
							frame.nbRetries = 3;
							frame.AppDataSize = 1;							
							frame.AppData[0] = 1;
							
							InsertOneFrame(&frame);
						}
						else
						{
							PRINTF("Added group addr %X failed.\r\n",GroupAddr);
						}						
						
						break;
				case MULTICAST_REMOVE_SETTING_PORT:
					
						if(mcpsIndication->BufferSize >= 3)
						{
							GroupAddr = (mcpsIndication->Buffer[0]<<16)|(mcpsIndication->Buffer[1]<<8)|(mcpsIndication->Buffer[2])|0xff000000;
						}
						else
						{
							PRINTF("Param error!\r\n");
							return;
						}
						
						if(RemoveGroupAddrFromMulticastList(GroupAddr) == true)
						{
							PRINTF("Removed group addr %X successfully.\r\n", GroupAddr);
							AppPort = MULTICAST_REMOVE_SETTING_PORT;
							/*TimerSetValue( &TxNextPacketTimer, 1000 );
							TimerStart( &TxNextPacketTimer );*/
							frame.port = MULTICAST_REMOVE_SETTING_PORT;
							frame.IsTxConfirmed = 1;
							frame.nbRetries = 3;
							frame.AppDataSize = 1;							
							frame.AppData[0] = 1;
							InsertOneFrame(&frame);
						}
						else
						{
							PRINTF("Removed group addr %X failed.\r\n", GroupAddr);
						}
						
						//GroupAddrIndex = mcpsIndication->Buffer[0];
				
						break;
				case MULTICAST_SETTING_PORT:
						
						if(SetMulticastAddr(mcpsIndication->Buffer[0]) == true)
						{
							PRINTF("Set group addr %X successfully.\r\n", mcpsIndication->Buffer[0]);
							AppPort = MULTICAST_SETTING_PORT;
							/*TimerSetValue( &TxNextPacketTimer, 1000 );
							TimerStart( &TxNextPacketTimer );*/
							frame.port = MULTICAST_SETTING_PORT;
							frame.IsTxConfirmed = 1;
							frame.nbRetries = 3;
							frame.AppDataSize = 1;							
							frame.AppData[0] = mcpsIndication->Buffer[0];
							InsertOneFrame(&frame);
						}
						else
						{
							PRINTF("Set group addr %X failed.\r\n", mcpsIndication->Buffer[0]);
						}
						//GroupAddrIndex = mcpsIndication->Buffer[0];
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

#if defined( USE_BAND_868 )
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
#if defined( USE_BAND_868 )
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
                    for( uint8_t i = 1; i < AppDataSize; i++ )
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
#if defined( USE_BAND_868 )
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
		switch( mlmeConfirm->MlmeRequest )
    {
        case MLME_JOIN:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
                // Status is OK, node has joined the network
                /*TimerSetValue( &TxNextPacketTimer, 1000 );
                TimerStart( &TxNextPacketTimer );*/
								
								TimerSetValue( &TxNextPacketTimer, 500 );
                TimerStart( &TxNextPacketTimer );
								
								SetNetStateLed(1);
								
								JoinNetOk = true;
								PRINTF("Join net OK!\r\n");
            }
            else
            {
                // Join was not successful. Try to join again
                DeviceState = DEVICE_STATE_JOIN;
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
 * Main application entry point.
 */
int main( void )
{
    LoRaMacPrimitives_t LoRaMacPrimitives;
    LoRaMacCallback_t LoRaMacCallbacks;
    MibRequestConfirm_t mibReq;

    BoardInitMcu( );
    BoardInitPeriph( );
		//EraseMcuEEPROM();
		LoadMulticastFromFlash();

    DeviceState = DEVICE_STATE_INIT;

    while( 1 )
    {
				UartTimeoutHandler();	//For rn820x celibration.
			
				switch( DeviceState )
        {
            case DEVICE_STATE_INIT:
            {
                LoRaMacPrimitives.MacMcpsConfirm = McpsConfirm;
                LoRaMacPrimitives.MacMcpsIndication = McpsIndication;
                LoRaMacPrimitives.MacMlmeConfirm = MlmeConfirm;
                LoRaMacCallbacks.GetBatteryLevel = BoardGetBatteryLevel;
                LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks );

                TimerInit( &TxNextPacketTimer, OnTxNextPacketTimerEvent );

                mibReq.Type = MIB_ADR;
                mibReq.Param.AdrEnable = LORAWAN_ADR_ON;
                LoRaMacMibSetRequestConfirm( &mibReq );

                mibReq.Type = MIB_PUBLIC_NETWORK;
                mibReq.Param.EnablePublicNetwork = LORAWAN_PUBLIC_NETWORK;
                LoRaMacMibSetRequestConfirm( &mibReq );
							
								mibReq.Type = MIB_DEVICE_CLASS;
                mibReq.Param.Class = CLASS_C;
                LoRaMacMibSetRequestConfirm( &mibReq );

#if defined( USE_BAND_868 )
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
                mibReq.Param.Rx2DefaultChannel = ( Rx2ChannelParams_t ){ 869525000, DR_3 };
                LoRaMacMibSetRequestConfirm( &mibReq );

                mibReq.Type = MIB_RX2_CHANNEL;
                mibReq.Param.Rx2Channel = ( Rx2ChannelParams_t ){ 869525000, DR_3 };
                LoRaMacMibSetRequestConfirm( &mibReq );
#endif

#endif
                /*Opt3001Init();
								ReadOpt3001Reg(0x01);*/
								/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
								GpioInit( &Led3, PA_12, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
								/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
								DeviceState = DEVICE_STATE_JOIN;
                break;
            }
            case DEVICE_STATE_JOIN:
            {
#if( OVER_THE_AIR_ACTIVATION != 0 )
                MlmeReq_t mlmeReq;

                // Initialize LoRaMac device unique ID
                BoardGetUniqueId( DevEui );

                mlmeReq.Type = MLME_JOIN;

                mlmeReq.Req.Join.DevEui = DevEui;
                mlmeReq.Req.Join.AppEui = AppEui;
                mlmeReq.Req.Join.AppKey = AppKey;
                mlmeReq.Req.Join.NbTrials = 3;
								
								PRINTF("OTAA\n\r"); 
								PRINTF("DevEui= %02X", DevEui[0]) ;for(int i=1; i<8 ; i++) {PRINTF("-%02X", DevEui[i]); }; PRINTF("\n\r");
								PRINTF("AppEui= %02X", AppEui[0]) ;for(int i=1; i<8 ; i++) {PRINTF("-%02X", AppEui[i]); }; PRINTF("\n\r");
								PRINTF("AppKey= %02X", AppKey[0]) ;for(int i=1; i<16; i++) {PRINTF(" %02X", AppKey[i]); }; PRINTF("\n\n\r");

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
								
								PRINTF("ABP\n\r"); 
								PRINTF("DevEui= %02X", DevEui[0]) ;for(int i=1; i<8 ; i++) {PRINTF("-%02X", DevEui[i]); }; PRINTF("\n\r");
								PRINTF("DevAdd=  %08X\n\r", DevAddr) ;
								PRINTF("NwkSKey= %02X", NwkSKey[0]) ;for(int i=1; i<16 ; i++) {PRINTF(" %02X", NwkSKey[i]); }; PRINTF("\n\r");
								PRINTF("AppSKey= %02X", AppSKey[0]) ;for(int i=1; i<16 ; i++) {PRINTF(" %02X", AppSKey[i]); }; PRINTF("\n\r");

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
							
								JoinNetOk = true;
								PRINTF("Join net OK!\r\n");
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
							if(!JoinNetOk)
                TimerLowPowerHandler( );
							
                break;
            }
            default:
            {
                DeviceState = DEVICE_STATE_INIT;
                break;
            }
        }
    }
}
