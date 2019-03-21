#ifndef _MACRO_DEFINITION_H_
#define _MACRO_DEFINITION_H_

/*!
 * LoRaWAN application port
 */
#define LORAWAN_APP_PORT		    				2
#define LIGHT_STATUS_PORT		    				3
#define LIGHT_CTRL_PORT			    				4
#define LIGHT_STATUS_3_4_PORT   				5
#define LIGHT_STATUS_F1_2_PORT  				6
#define LIGHT_STATUS_B1_2_PORT  				7
#define DALI_CTRL_PORT									8

#define PROFILE1_PORT										100
#define PROFILE2_PORT										101
#define ERASE_FLASH_PORT								102

#define SET_REPORT_STATUS_PORT  				103
#define GET_REPORT_STATUS_PORT  				104
#define QUERY_REPORT_RSSI_PORT  				105

#define TIME_SYNC_PORT			    				150
#define ADR_JUST_RECEIVE_PORT   				151
#define ADR_JUST_SEND_PORT      				152
#define FRAME_NUM_SETTING_PORT					153

#define FOTA_RECEIVE_DATA_PORT     			154
#define FAULT_PARAM_PORT					 			155
#define FOTA_REPORT_DATA_PORT      			156
#define FOTA_GET_VERSION_PORT      			157
#define FOTA_BOOTLOADER_PORT       			158

#define MULTICAST_ADD_SETTING_PORT			200
#define MULTICAST_REMOVE_SETTING_PORT		201
#define FOTA_ADD_MULTICAST_PORT    			202

#define SYSTEM_RESET_PORT               210
#define CHEAK_CUR_MULTICAST_PORT        211

/*!
 * Bootloader firmware vertion.
 */
#define BOOT_VERSION  "STM32L0_BOOT_V1.0.0"

/*!
 * Application firmware vertion.
 */
#define APP_VERSION   "EMLLC_B10_V1.6.6"  

/*!
 * Active mode.
 */
//#define ACTIVE_MODE_2		1

/*!
 * Watch dog enable.
 */
#define WATCH_DOG_ENABLE

/*!
 * AS923 configuration.
 */
#define AS923_INDONESIA_ENABLE 

/*!
 * CN470 configuration.
 */
//#define CN470_NEW_CHANNEL_ENABLE

/*!
 * Debug mode.
 */
//#define DEBUG_MODE_ENABLE

/*!
 * Fault handling enable.
 */
//#define FAULT_HANDLE_ENABLE

/*!
 * DS1302 RTC enable.
 */
//#define DS1302_RTC_ENABLE

/*!
 * FOTA enable.
 */
#define FOTA_ENABLE

/*!
 * GL5516 photoresistance enable.
 */
//#define GL5516_PHOTOREISTANCE_ENABLE

#endif
