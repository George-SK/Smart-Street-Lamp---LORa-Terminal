#ifndef _DS1302_H_
#define _DS1302_H_

#include <string.h>
#include <math.h>
#include "stdint.h" 

#define DS1302_DATA_IO	GPIO_PIN_7
#define DS1302_SCLK_IO  GPIO_PIN_6
#define DS1302_CE_IO 		GPIO_PIN_5

#define DS1302_WRITE_DATA_SET    	HAL_GPIO_WritePin(GPIOB, DS1302_DATA_IO, GPIO_PIN_SET)
#define DS1302_WRITE_DATA_RESET   HAL_GPIO_WritePin(GPIOB, DS1302_DATA_IO, GPIO_PIN_RESET)
#define DS1302_WRITE_SCLK_SET   	HAL_GPIO_WritePin(GPIOB, DS1302_SCLK_IO, GPIO_PIN_SET)
#define DS1302_WRITE_SCLK_RESET   HAL_GPIO_WritePin(GPIOB, DS1302_SCLK_IO, GPIO_PIN_RESET)
#define DS1302_WRITE_CE_SET    		HAL_GPIO_WritePin(GPIOA, DS1302_CE_IO, GPIO_PIN_SET)
#define DS1302_WRITE_CE_RESET    	HAL_GPIO_WritePin(GPIOA, DS1302_CE_IO, GPIO_PIN_RESET)
#define DS1302_READ_DATA()				HAL_GPIO_ReadPin(GPIOB, DS1302_DATA_IO)

//DS1302 Chip register address
#define CLOCK_START               0x00
#define CLOCK_STOP						    0x80
#define DS1302_PROTECT_REG        0x8E

#define DS1302_SECOND_REG         0x80
#define DS1302_MINUTE_REG         0x82
#define DS1302_HOUR_REG           0x84
#define DS1302_DAY_REG            0x86
#define DS1302_MONTH_REG          0x88
#define DS1302_WEEK_REG           0x8A
#define DS1302_YEAR_REG           0x8C

typedef struct 
{           
    uint8_t  year; //0~99
    uint8_t  month;
    uint8_t  day;
		uint8_t  hour;
    uint8_t  min;
    uint8_t  sec; 
    uint8_t  week;       
}DS1302_t;

typedef struct 
{           
		uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;       
}DS1302_TimeSync;


void DS1302Initialize(void);
void DS1302RTCSetTime(DS1302_t time);
DS1302_t DS1302RTCGetTime(void);
void DS1302SetTimeSync(DS1302_TimeSync tim);
DS1302_TimeSync DS1302GetTimeSync(void);

#endif
