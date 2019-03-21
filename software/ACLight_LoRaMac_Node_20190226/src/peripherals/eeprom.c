#include "board.h"
#include "eeprom.h"
#include "i2c-board.h"

static uint8_t I2cDeviceAddr = 0;

static uint8_t EEPROMWriteBuf[10] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
static uint8_t EEPROMReadBuf[10];

static bool EEPROMInitialized = false;

extern I2cAddrSize I2cInternalAddrSize;

void EEPROMInit( void )
{
		int status;
		uint8_t i;
	
		if( EEPROMInitialized == false )
    {   
        EEPROMSetDeviceAddr( EEPROM_I2C_ADDRESS );
        EEPROMInitialized = true;
    }
		
		//EEPROM test.
		I2cInternalAddrSize = I2C_ADDR_SIZE_16;
		
		DelayMs(50);
		status = EEPROMWriteBuffer(0x1212, EEPROMWriteBuf, 10);
		
		if(status)
		{
			PRINTF("EEPROM write at address 0x1212: ");
			for(i = 0; i < 10; i++)
				PRINTF("%#X ", EEPROMWriteBuf[i]);
			PRINTF("\r\n");
		}
		else
			PRINTF("EEPROM write failed!\r\n");
		
		DelayMs(50);	//Note: must do a delay.
		status = EEPROMReadBuffer(0x1212, EEPROMReadBuf, 10);		
		
		if(status)
		{
			PRINTF("EEPROM read at address 0x1212: ");
			for(i = 0; i < 10; i++)
				PRINTF("%#X ", EEPROMReadBuf[i]);
			PRINTF("\r\n");
		}
		else
			PRINTF("EEPROM read failed!\r\n");
		
		for(i = 0; i < 10; i++)
		{
			if(EEPROMWriteBuf[i] != EEPROMReadBuf[i])
			{
				PRINTF("EEPROM verify error!\r\n");
				break;
			}
		}
		
		if(i == 10)
			PRINTF("EEPROM verify OK!\r\n");
		
		I2cInternalAddrSize = I2C_ADDR_SIZE_8;
}

uint8_t EEPROMWrite( uint16_t addr, uint8_t data )
{
    return EEPROMWriteBuffer( addr, &data, 1 );
}

uint8_t EEPROMWriteBuffer( uint16_t addr, uint8_t *data, uint8_t size )
{
    return I2cWriteBuffer( &I2c, I2cDeviceAddr << 1, addr, data, size );
}

uint8_t EEPROMRead( uint16_t addr, uint8_t *data )
{
    return EEPROMReadBuffer( addr, data, 1 );
}

uint8_t EEPROMReadBuffer( uint16_t addr, uint8_t *data, uint8_t size )
{
    return I2cReadBuffer( &I2c, I2cDeviceAddr << 1, addr, data, size );
}

void EEPROMSetDeviceAddr( uint8_t addr )
{
    I2cDeviceAddr = addr;
}

uint8_t EEPROMGetDeviceAddr( void )
{
    return I2cDeviceAddr;
}
