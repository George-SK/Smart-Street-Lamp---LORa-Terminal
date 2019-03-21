/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Driver for the EEPROM IO expander

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#ifndef __EEPROM_H__
#define __EEPROM_H__


#define EEPROM_I2C_ADDRESS                          0x54

/*!
 * \brief Initializes the device
 */
void EEPROMInit( void );

/*!
 * \brief Resets the device
 *
 * \retval status [SUCCESS, FAIL]
 */
uint8_t EEPROMReset( void );

/*!
 * \brief Writes a byte at specified address in the device
 *
 * \param [IN]: addr
 * \param [IN]: data
 * \retval status [SUCCESS, FAIL]
 */
uint8_t EEPROMWrite( uint16_t addr, uint8_t data );

/*!
 * \brief Writes a buffer at specified address in the device
 *
 * \param [IN]: addr
 * \param [IN]: data
 * \param [IN]: size
 * \retval status [SUCCESS, FAIL]
 */
uint8_t EEPROMWriteBuffer( uint16_t addr, uint8_t *data, uint8_t size );

/*!
 * \brief Reads a byte at specified address in the device
 *
 * \param [IN]: addr
 * \param [OUT]: data
 * \retval status [SUCCESS, FAIL]
 */
uint8_t EEPROMRead( uint16_t addr, uint8_t *data );

/*!
 * \brief Reads a buffer at specified address in the device
 *
 * \param [IN]: addr
 * \param [OUT]: data
 * \param [IN]: size
 * \retval status [SUCCESS, FAIL]
 */
uint8_t EEPROMReadBuffer( uint16_t addr, uint8_t *data, uint8_t size );

/*!
 * \brief Sets the I2C device slave address
 *
 * \param [IN]: addr
 */
void EEPROMSetDeviceAddr( uint8_t addr );

/*!
 * \brief Gets the I2C device slave address
 *
 * \retval: addr Current device slave address
 */
uint8_t EEPROMGetDeviceAddr( void );

#endif  // __EEPROM_H__
