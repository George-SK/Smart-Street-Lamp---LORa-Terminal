#include "board.h"
#include "opt3001.h"
#include "math.h"

/*---------------------------------------------------------------------------*/
/* Slave address */
#define OPT3001_I2C_ADDR_W             0x88
#define OPT3001_I2C_ADDR_R             0x89
/*---------------------------------------------------------------------------*/
/* Register addresses */
#define REG_RESULT                      0x00
#define REG_CONFIGURATION               0x01
#define REG_LOW_LIMIT                   0x02
#define REG_HIGH_LIMIT                  0x03
 
#define REG_MANUFACTURER_ID             0x7E
#define REG_DEVICE_ID                   0x7F
/*---------------------------------------------------------------------------*/
/*
 * Configuration Register Bits and Masks.
 * We use uint16_t to read from / write to registers, meaning that the
 * register's MSB is the variable's LSB.
 */
#define CONFIG_RN      0x00F0 /* [15..12] Range Number */
#define CONFIG_CT      0x0008 /* [11] Conversion Time */
#define CONFIG_M       0x0006 /* [10..9] Mode of Conversion */
#define CONFIG_OVF     0x0001 /* [8] Overflow */
#define CONFIG_CRF     0x8000 /* [7] Conversion Ready Field */
#define CONFIG_FH      0x4000 /* [6] Flag High */
#define CONFIG_FL      0x2000 /* [5] Flag Low */
#define CONFIG_L       0x1000 /* [4] Latch */
#define CONFIG_POL     0x0800 /* [3] Polarity */
#define CONFIG_ME      0x0400 /* [2] Mask Exponent */
#define CONFIG_FC      0x0300 /* [1..0] Fault Count */
 
/* Possible Values for CT */
#define CONFIG_CT_100      0x0000
#define CONFIG_CT_800      CONFIG_CT
 
/* Possible Values for M */
#define CONFIG_M_CONTI     0x0004
#define CONFIG_M_SINGLE    0x0002
#define CONFIG_M_SHUTDOWN  0x0000
 
/* Reset Value for the register 0xC810. All zeros except: */
#define CONFIG_RN_RESET    0x00C0
#define CONFIG_CT_RESET    CONFIG_CT_800
#define CONFIG_L_RESET     0x1000
#define CONFIG_DEFAULTS    (CONFIG_RN_RESET | CONFIG_CT_100 | CONFIG_L_RESET)
 
/* Enable / Disable */
#define CONFIG_ENABLE_CONTINUOUS  (CONFIG_M_CONTI | CONFIG_DEFAULTS)
#define CONFIG_ENABLE_SINGLE_SHOT (CONFIG_M_SINGLE | CONFIG_DEFAULTS)
#define CONFIG_DISABLE             CONFIG_DEFAULTS
/*---------------------------------------------------------------------------*/
/* Register length */
#define REGISTER_LENGTH                 2
/*---------------------------------------------------------------------------*/
/* Sensor data size */
#define DATA_LENGTH                     2
/*---------------------------------------------------------------------------*/

#define OPT3001_INT_IO	PA_4

extern I2c_t I2c;

uint16_t ReadOpt3001Reg(uint16_t reg);
static uint8_t WriteOpt3001Reg(uint16_t reg, uint16_t value);

uint8_t SensorState = 1;

typedef struct LimitReg_s
{
	float lux;
	uint8_t LEValue;
}LimitReg_s;

static const LimitReg_s LuxLeverTable[12] = {
	{40.95, 0},
	{81.90, 1},
	{163.80, 2},
	{327.60, 3},
	{655.20, 4},
	{1310.40, 5},
	{2620.80, 6},
	{5241.60, 7},
	{10483.20, 8},
	{20966.40, 9},
	{41932.80, 10},
	{83865.60, 11}
};

Gpio_t Opt3001Int;

static uint8_t ComputeLE(float lux)
{
	uint8_t i;
	
	if(lux <= LuxLeverTable[0].lux)
		return LuxLeverTable[0].LEValue;
	
	for(i = 1; i < 12; i++)
	{
		if(lux > LuxLeverTable[i-1].lux && lux <= LuxLeverTable[i].lux)
			return LuxLeverTable[i].LEValue;
	}
	
	return LuxLeverTable[11].LEValue;
}

uint8_t SetHighLowLimit(float lux, uint8_t LowOrHigh)
{
	uint16_t LEValue;
	uint16_t TLValue;
	uint16_t RegAddr;
	
	LEValue = ComputeLE(lux);
	TLValue = lux/exp2(LEValue)*100;
	
	if(LowOrHigh)
		RegAddr = REG_HIGH_LIMIT;
	else
		RegAddr = REG_LOW_LIMIT;
	
	TLValue = TLValue|((LEValue<<12)&0xf000);//((((TLValue&0xff)<<8)|((TLValue>>8)&0xff))&0x0fff)|((LEValue<<12)&0xf000);
	TLValue = (TLValue<<8) | (TLValue>>8 & 0xFF);
	Uart1.PrintfLog("TLValue:%X\r\n", TLValue);
	
	return WriteOpt3001Reg(RegAddr, TLValue);
}

void Opt3001IrqHandler(void)
{
	if(GpioRead(&Opt3001Int)==0)
	{
		Uart1.PrintfLog("Irq low\r\n");
	}
	else
	{
		Uart1.PrintfLog("Irq high\r\n");
	}
	
	ReadOpt3001Reg(REG_CONFIGURATION);
}

void Opt3001Init(void)
{
	GpioInit( &Opt3001Int, OPT3001_INT_IO, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );  
  GpioSetInterrupt( &Opt3001Int, IRQ_RISING_FALLING_EDGE, IRQ_LOW_PRIORITY, Opt3001IrqHandler );
	SetHighLowLimit(55, 0);
	SetHighLowLimit(1000, 1);
	Uart1.PrintfLog("Low:%X\r\n", ReadOpt3001Reg(REG_LOW_LIMIT));
	Uart1.PrintfLog("High:%X\r\n", ReadOpt3001Reg(REG_HIGH_LIMIT));
}

/**
 * \brief Turn the sensor on/off
 * \param enable TRUE: on, FALSE: off
 */
void EnableOpt3001(bool enable)
{
  uint16_t val;
 
  if(enable) {
    val = CONFIG_ENABLE_CONTINUOUS;
 
  } else {
    val = CONFIG_DISABLE;
  }
 
  I2cWriteBuffer( &I2c, (uint8_t)OPT3001_I2C_ADDR_W, REG_CONFIGURATION, (uint8_t *)&val, REGISTER_LENGTH );
}

uint16_t ReadOpt3001Reg(uint16_t reg)
{
	uint8_t data[2];
	
	SensorState = I2cReadBuffer( &I2c, (uint8_t)(OPT3001_I2C_ADDR_R), reg, data, 2 );
	
	return ((uint16_t)(data[0]<<8)|data[1]);
}

static uint8_t WriteOpt3001Reg(uint16_t reg, uint16_t value)
{
	return I2cWriteBuffer( &I2c, (uint8_t)OPT3001_I2C_ADDR_W, reg, (uint8_t *)&value, REGISTER_LENGTH );
}

static float Convert(uint16_t raw_data)
{
  uint16_t e, m;
 
  m = raw_data & 0x0FFF;
  e = (raw_data & 0xF000) >> 12;
 
  return m * (0.01 * exp2(e));
}

float ReadOpt3001Value(void)
{
	uint16_t RegValue;
	
	RegValue = ReadOpt3001Reg(REG_RESULT);
	return Convert(RegValue);	
}
