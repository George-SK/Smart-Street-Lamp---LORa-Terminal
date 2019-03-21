#include "ds1302.h"
#include "board.h"

void delay_us(uint16_t tim)
{
		for (int i=0; i<1*tim; i++) //1us
		{
				__nop();
		}
}

uint8_t BCD2HEX(uint8_t bcd_data)
{
		uint8_t temp;
		temp = (bcd_data/16*10 + bcd_data%16);
		return temp;
}

uint8_t HEX2BCD(uint8_t hex_data)
{
		uint8_t temp;
		temp = (hex_data/10*16 + hex_data%10);
		return temp;
}

void DS1302_IO_Init(void)
{
		GPIO_InitTypeDef GPIO_InitStructure;
		
		__HAL_RCC_GPIOA_CLK_ENABLE( );
		__HAL_RCC_GPIOB_CLK_ENABLE( );
	
		//IO
		GPIO_InitStructure.Pin =  DS1302_DATA_IO;
		GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD; //Open drain output Use two-way IO port
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
	
		//SCLK
		GPIO_InitStructure.Pin =  DS1302_SCLK_IO;
		GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
	
		//CE
		GPIO_InitStructure.Pin = DS1302_CE_IO;
		GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void DS1302WriteByte(uint8_t dat)
{
		uint8_t i = 0;
	
		DS1302_WRITE_SCLK_RESET;
		delay_us(2);
	
		for (i=0; i<8; i++) //write address
		{
				if (dat & 0x01)
						DS1302_WRITE_DATA_SET;
				else
						DS1302_WRITE_DATA_RESET;
				
				dat = dat>>1;
				DS1302_WRITE_SCLK_SET;
				delay_us(2);
				DS1302_WRITE_SCLK_RESET;
				delay_us(2);
		}
		DS1302_WRITE_DATA_SET;
		delay_us(2);
}

uint8_t DS1302ReadByte(void)
{
		uint8_t i =0;
		uint8_t temp = 1;
	
		delay_us(2);
		for (i=0; i<8; i++) 
		{
				temp >>= 1;
				if (DS1302_READ_DATA() == 1)  //LSB
						temp |= 0x80;
				
				DS1302_WRITE_SCLK_SET;
				delay_us(2);
				DS1302_WRITE_SCLK_RESET;
				delay_us(2);
		}
		return temp;
}

void DS1302_Write(uint8_t addr, uint8_t dat)
{
		DS1302_WRITE_CE_RESET;
		delay_us(2);
		DS1302_WRITE_SCLK_RESET;
		delay_us(2);
		DS1302_WRITE_CE_SET;
		delay_us(2);
		DS1302WriteByte(addr);
		DS1302WriteByte(dat);
		DS1302_WRITE_SCLK_SET;
		delay_us(2);
		DS1302_WRITE_CE_RESET;
		delay_us(2);
}

uint8_t DS1302_Read(uint8_t addr)
{
		uint8_t data;
	
		DS1302_WRITE_CE_RESET;
		delay_us(2);
		DS1302_WRITE_SCLK_RESET;
		delay_us(2);
		DS1302_WRITE_CE_SET;
		delay_us(2);
		DS1302WriteByte(addr|0x01); 
		delay_us(2);
		data = DS1302ReadByte();
		DS1302_WRITE_SCLK_SET;
		delay_us(2);
		DS1302_WRITE_CE_RESET;
		delay_us(2);
	
		return data;
}

//w=y+[y/4]+[c/4]-2c+[26(m+1)/10]+d-1 
int8_t DS1302GetWeek(uint8_t year, uint8_t mon, uint8_t day)
{
		uint16_t _year = 2000+year;
		uint16_t y, c;
		int8_t  m, d, w;
		
		if (mon >= 3)
		{
				m = mon;
				y = _year%100;
				c = _year/100;
				d = day;
		}
		else
		{
				m = mon+12;
				y = (_year-1)%100;
				c = (_year-1)/100;
				d = day;
		}
		
		w = y+y/4+c/4-2*c+26*(m+1)/10+d-1;
		
		if (w == 0)
			w = 7; 
		else if (w < 0)
			w = 7-(-w)%7;
		else
			w %= 7;
		
		return w;
}

void DS1302Initialize(void)
{
		DS1302_IO_Init();
	
		DS1302_WRITE_CE_RESET;
		delay_us(2);
		DS1302_WRITE_SCLK_RESET; 
		delay_us(2);
	
		if ((DS1302_Read(DS1302_SECOND_REG) & 0x80) != 0)
		{
				DS1302_Write(DS1302_PROTECT_REG, 0x00);
				
				DS1302_Write(DS1302_YEAR_REG, 	HEX2BCD(18)); 
				DS1302_Write(DS1302_MONTH_REG,	HEX2BCD(1));
				DS1302_Write(DS1302_DAY_REG, 		HEX2BCD(1));
				DS1302_Write(DS1302_HOUR_REG, 	HEX2BCD(0));
				DS1302_Write(DS1302_MINUTE_REG, HEX2BCD(0));
				DS1302_Write(DS1302_SECOND_REG, HEX2BCD(0));
				DS1302_Write(DS1302_WEEK_REG, 	HEX2BCD(1));
		}
		
		DS1302_Write(DS1302_PROTECT_REG, 0x80);
		DS1302_Write(DS1302_SECOND_REG, CLOCK_START);
}

void DS1302RTCSetTime(DS1302_t time)
{
		uint8_t week;
		
		week = DS1302GetWeek(time.year, time.month, time.day);
		
		DS1302_Write(DS1302_PROTECT_REG, 0x00);      //Write off protection
		DS1302_Write(DS1302_SECOND_REG, CLOCK_STOP); //DS1302 stop
		
		DS1302_Write(DS1302_YEAR_REG, 	HEX2BCD(time.year)); 
		DS1302_Write(DS1302_MONTH_REG,	HEX2BCD(time.month));
		DS1302_Write(DS1302_DAY_REG, 		HEX2BCD(time.day));
		DS1302_Write(DS1302_HOUR_REG, 	HEX2BCD(time.hour));
		DS1302_Write(DS1302_MINUTE_REG, HEX2BCD(time.min));
		DS1302_Write(DS1302_SECOND_REG, HEX2BCD(time.sec));
		DS1302_Write(DS1302_WEEK_REG, 	HEX2BCD(week));
		
		DS1302_Write(DS1302_PROTECT_REG, 0x80);     //Write on protection
}

DS1302_t DS1302RTCGetTime(void)
{
		DS1302_t Rds1302;
	
		Rds1302.year  = BCD2HEX(DS1302_Read(DS1302_YEAR_REG)); 
		Rds1302.month = BCD2HEX(DS1302_Read(DS1302_MONTH_REG)); 
		Rds1302.day  	= BCD2HEX(DS1302_Read(DS1302_DAY_REG));   
	  Rds1302.hour  = BCD2HEX(DS1302_Read(DS1302_HOUR_REG)); 
	  Rds1302.min   = BCD2HEX(DS1302_Read(DS1302_MINUTE_REG)); 
		Rds1302.sec   = BCD2HEX(DS1302_Read(DS1302_SECOND_REG)); 
		Rds1302.week  = BCD2HEX(DS1302_Read(DS1302_WEEK_REG)); 
	
		return Rds1302;
}

void DS1302SetTimeSync(DS1302_TimeSync tim)
{
		DS1302_Write(DS1302_PROTECT_REG, 0x00);      //Write off protection
		DS1302_Write(DS1302_SECOND_REG, CLOCK_STOP); //DS1302 stop
	
		DS1302_Write(DS1302_HOUR_REG, 	HEX2BCD(tim.hour));
		DS1302_Write(DS1302_MINUTE_REG, HEX2BCD(tim.min));
		DS1302_Write(DS1302_SECOND_REG, HEX2BCD(tim.sec));
	
		DS1302_Write(DS1302_PROTECT_REG, 0x80);     //Write on protection
}

DS1302_TimeSync DS1302GetTimeSync(void)
{
		DS1302_TimeSync tim;
	
		tim.hour  = BCD2HEX(DS1302_Read(DS1302_HOUR_REG)); 
	  tim.min   = BCD2HEX(DS1302_Read(DS1302_MINUTE_REG)); 
		tim.sec   = BCD2HEX(DS1302_Read(DS1302_SECOND_REG)); 
		
		return tim;
}
