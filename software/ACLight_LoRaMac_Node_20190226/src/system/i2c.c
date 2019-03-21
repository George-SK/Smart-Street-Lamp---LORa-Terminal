/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Implements the generic I2C driver

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include "board.h"
#include "i2c-board.h"

/* I2C TIMING Register define when I2C clock source is SYSCLK */
/* I2C TIMING is calculated in case of the I2C Clock source is the SYSCLK = 32 MHz */
//#define I2C_TIMING    0x10A13E56 /* 100 kHz with analog Filter ON, Rise Time 400ns, Fall Time 100ns */ 
#define I2C_TIMING      0x00B1112E /* 400 kHz with analog Filter ON, Rise Time 250ns, Fall Time 100ns */   

/*!
 * Flag to indicates if the I2C is initialized
 */
static bool I2cInitialized = false;

void I2cInit( I2c_t *obj, PinNames scl, PinNames sda )
{
    if( I2cInitialized == false )
    {
        I2cInitialized = true;

        I2cMcuInit( obj, scl, sda );
#if defined (STM32L073xx) || defined (STM32L072xx)
        I2cMcuFormat( obj, MODE_I2C, I2C_DUTY_CYCLE_2, true, I2C_ACK_ADD_7_BIT, I2C_TIMING );
#else
				I2cMcuFormat( obj, MODE_I2C, I2C_DUTY_CYCLE_2, true, I2C_ACK_ADD_7_BIT, 400000 );
#endif
    }
}

void I2cDeInit( I2c_t *obj )
{
    I2cInitialized = false;
    I2cMcuDeInit( obj );
}

void I2cResetBus( I2c_t *obj, PinNames scl, PinNames sda )
{
    I2cInitialized = false;
    I2cInit( obj, scl, sda );
}

uint8_t I2cWrite( I2c_t *obj, uint8_t deviceAddr, uint16_t addr, uint8_t data )
{
    if( I2cInitialized == true )
    {
        if( I2cMcuWriteBuffer( obj, deviceAddr, addr, &data, 1 ) == FAIL )
        {
            // if first attempt fails due to an IRQ, try a second time
            if( I2cMcuWriteBuffer( obj, deviceAddr, addr, &data, 1 ) == FAIL )
            {
                return FAIL;
            }
            else
            {
                return SUCCESS;
            }
        }
        else
        {
            return SUCCESS;
        }
    }
    else
    {
        return FAIL;
    }
}

uint8_t I2cWriteBuffer( I2c_t *obj, uint8_t deviceAddr, uint16_t addr, uint8_t *buffer, uint16_t size )
{
    if( I2cInitialized == true )
    {
        if( I2cMcuWriteBuffer( obj, deviceAddr, addr, buffer, size ) == FAIL )
        {
            // if first attempt fails due to an IRQ, try a second time
            if( I2cMcuWriteBuffer( obj, deviceAddr, addr, buffer, size ) == FAIL )
            {
                return FAIL;
            }
            else
            {
                return SUCCESS;
            }
        }
        else
        {
            return SUCCESS;
        }
    }
    else
    {
        return FAIL;
    }
}

uint8_t I2cRead( I2c_t *obj, uint8_t deviceAddr, uint16_t addr, uint8_t *data )
{
    if( I2cInitialized == true )
    {
        return( I2cMcuReadBuffer( obj, deviceAddr, addr, data, 1 ) );
    }
    else
    {
        return FAIL;
    }
}

uint8_t I2cReadBuffer( I2c_t *obj, uint8_t deviceAddr, uint16_t addr, uint8_t *buffer, uint16_t size )
{
    if( I2cInitialized == true )
    {
        return( I2cMcuReadBuffer( obj, deviceAddr, addr, buffer, size ) );
    }
    else
    {
        return FAIL;
    }
}
