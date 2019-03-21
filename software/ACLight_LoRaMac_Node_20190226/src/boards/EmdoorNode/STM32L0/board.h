/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Target board general functions implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#ifndef __BOARD_H__
#define __BOARD_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "stm32l0xx.h"
#include "stm32l0xx_hal.h"
#include "utilities.h"
#include "timer.h"
#include "delay.h"
#include "gpio.h"
#include "adc.h"
#include "spi.h"
#include "uart.h"
#include "radio.h"
#include "i2c.h"	//Added by Steven in 2018/03/21.
#include "sx1276/sx1276.h"
#include "rtc-board.h"
#include "sx1276-board.h"
#include "uart-board.h"
#include "street-light.h"
#include "data-processing.h"
#include "rn820x.h"
#include "profile.h"
#include "tim.h"
#include "dali.h"
#include "gl5516.h"

/*!
 * Define indicating if an external IO expander is to be used
 */
////#define BOARD_IOE_EXT

/*!
 * Generic definition
 */
#ifndef SUCCESS
#define SUCCESS                                     1
#endif

#ifndef FAIL
#define FAIL                                        0
#endif

/*!
 * Board IO Extender pins definitions
 */


/*!
 * Board MCU pins definition
 * Based on LW470-01(LSD)
 */
//--------------------------------------------------------
#if defined (STM32L073xx)
#define RADIO_RESET                                 PB_10

#define RADIO_MOSI                                  PB_15
#define RADIO_MISO                                  PB_14
#define RADIO_SCLK                                  PB_13
#define RADIO_NSS                                   PB_12

#define RADIO_DIO_0                                 PB_11  // LW470-01
#define RADIO_DIO_1                                 PC_13
#define RADIO_DIO_2                                 PB_9
#define RADIO_DIO_3                                 PB_4
#define RADIO_DIO_4                                 PB_3
#define RADIO_DIO_5                                 PA_15

#define RADIO_ANT_SWITCH_RX                         PB_7
#define RADIO_ANT_SWITCH_TX                         PB_6

#define RADIO_ANT_SWITCH                      			PA_1	//Added by Steven.

#define OSC_LSE_IN                                  PC_14
#define OSC_LSE_OUT                                 PC_15

#define SWCLK                                       PA_14
#define SWDAT                                       PA_13

#define UART_TX                                     PA_9//PC_10		//PA_2  // USART2	//Modified by Steven.
#define UART_RX                                     PA_10//PC_11		//PA_3  // USART2

#define BAT_LEVEL_PIN                               PA_0
#define BAT_LEVEL_CHANNEL                           ADC_CHANNEL_0

#define PHOTORESISTOR_PIN                           PC_4
#define PHOTORESISTOR_CHANNEL												ADC_CHANNEL_10

#define I2C1_SCL                                    PB_6
#define I2C1_SDA                                    PB_7

#define DALI_OUT_PIN																PC_7
#define DALI_IN_PIN																	PA_5
//---------------------------------------------------------
#elif defined (STM32L072xx)
#define RADIO_RESET                                 PA_8

#define RADIO_MOSI                                  PB_15
#define RADIO_MISO                                  PB_14
#define RADIO_SCLK                                  PB_13
#define RADIO_NSS                                   PB_12

#define RADIO_DIO_0                                 PB_0  // LW470-01
#define RADIO_DIO_1                                 PB_1
#define RADIO_DIO_2                                 PB_2
#define RADIO_DIO_3                                 PA_15
#define RADIO_DIO_4                                 PB_5
#define RADIO_DIO_5                                 PB_9

#define RADIO_ANT_SWITCH_RX                         PB_7
#define RADIO_ANT_SWITCH_TX                         PB_6

#define RADIO_ANT_SWITCH                      		  PB_8	//Added by Steven.

#define OSC_LSE_IN                                  PC_14
#define OSC_LSE_OUT                                 PC_15

#define SWCLK                                       PA_14
#define SWDAT                                       PA_13

#define UART_TX                                     PA_9
#define UART_RX                                     PA_10

#define BAT_LEVEL_PIN                               PA_0
#define BAT_LEVEL_CHANNEL                           ADC_CHANNEL_0

#define PHOTORESISTOR_PIN                           PC_4
#define PHOTORESISTOR_CHANNEL												ADC_CHANNEL_10

#define I2C1_SCL                                    PB_6
#define I2C1_SDA                                    PB_7

#define DALI_OUT_PIN								                PA_11
#define DALI_IN_PIN									                PA_12
#endif
/* Exported macros -----------------------------------------------------------*/
#if 1
#define PRINTF(...)     printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*!
 * MCU objects
 */
extern Uart_t Uart1;
extern Adc_t Adc;

/*!
 * LED GPIO pins objects
 */
extern Gpio_t Led1;
extern Gpio_t Led2;
extern Gpio_t Led3;

/*!
 * Possible power sources
 */
enum BoardPowerSources
{
    USB_POWER = 0,
    BATTERY_POWER,
};

/*!
 * \brief Disable interrupts
 *
 * \remark IRQ nesting is managed
 */
void BoardDisableIrq( void );

/*!
 * \brief Enable interrupts
 *
 * \remark IRQ nesting is managed
 */
void BoardEnableIrq( void );

/*!
 * \brief Initializes the target board peripherals.
 */
void BoardInitMcu( void );

/*!
 * \brief Initializes the boards peripherals.
 */
void BoardInitPeriph( void );

/*!
 * \brief De-initializes the target board peripherals to decrease power
 *        consumption.
 */
void BoardDeInitMcu( void );

/*!
 * \brief Measure the Battery voltage
 *
 * \retval value  battery voltage in volts
 */
uint32_t BoardGetBatteryVoltage( void );

/*!
 * \brief Get the current battery level
 *
 * \retval value  battery level [  0: USB,
 *                                 1: Min level,
 *                                 x: level
 *                               254: fully charged,
 *                               255: Error]
 */
uint8_t BoardGetBatteryLevel( void );

/*!
 * Returns a pseudo random seed generated using the MCU Unique ID
 *
 * \retval seed Generated pseudo random seed
 */
uint32_t BoardGetRandomSeed( void );

/*!
 * \brief Gets the board 64 bits unique ID
 *
 * \param [IN] id Pointer to an array that will contain the Unique ID
 */
void BoardGetUniqueId( uint8_t *id );

/*!
 * \brief Get the board power source
 *
 * \retval value  power source [0: USB_POWER, 1: BATTERY_POWER]
 */
uint8_t GetBoardPowerSource( void );
void UartTimeoutHandler(void);

#endif // __BOARD_H__
