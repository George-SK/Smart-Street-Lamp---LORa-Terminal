#include "board.h"
#include "humiture-sensor.h"
#include <math.h>    
#include <stdio.h>

#define PIN_SCL                                     IOE_6
#define PIN_SDA                                     IOE_7

#if (USE_LCD12864)
#include "LCD12864.h"
#endif

/* |----------|
 * | Humiture |-----> IOE_6
 * |          |-----> IOE_7
 * |----------| */

#define noACK 0
#define ACK   1
                            
/*
 * Definition of registers and commands.
 */
#define STATUS_REG_W 0x06   //000   0011    0
#define STATUS_REG_R 0x07   //000   0011    1
#define MEASURE_TEMP 0x03   //000   0001    1
#define MEASURE_HUMI 0x05   //000   0010    1
#define RESET        0x1e   //000   1111    0

/*
 * Definition of CLOCK and DATA line.
 */
#define SCK_H   GpioWrite(&HumitureSclGpio, 1)
#define SCK_L   GpioWrite(&HumitureSclGpio, 0)
#define SDA_H   GpioWrite(&HumitureSdaGpio, 1)
#define SDA_L   GpioWrite(&HumitureSdaGpio, 0)

/*
 * Private functions prototypes
 */
void s_connectionreset(void);
char s_softreset(void);

/*
 * Private global variables.
 */
Gpio_t HumitureSclGpio, HumitureSdaGpio;
Humiture_t HumitureValue;

enum {TEMP,HUMI};

/**************************************************************************************************
* @fn      DelayUs
*
* @brief   wait for x us. @ 32MHz MCU clock it takes 32 "nop"s for 1 us delay.
*
* @param   x us. range[0-65536]
*
* @return  None
**************************************************************************************************/
void DelayUs(uint16_t microSecs)
{
  while(microSecs--)
  {
    /* 32 NOPs == 1 usecs */
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP();
  }
}

/**************************************************************************************************
* @fn      HumitureSensorInit
*
* @brief   Initialized the Humiture sensor.
*
* @param   None
*
* @return  None
**************************************************************************************************/
void HumitureSensorInit(void)
{
  GpioInit( &HumitureSclGpio, PIN_SCL, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );  //SCL = 0
  GpioInit( &HumitureSdaGpio, PIN_SDA, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );  //SDA = 1
  s_softreset();
}

/**************************************************************************************************
* @fn      SetSDAIn
*
* @brief   Set SDA gpio as input mode.
*
* @param   None
*
* @return  None
**************************************************************************************************/
void SetSDAIn(void)
{
  GpioInit( &HumitureSdaGpio, PIN_SDA, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
}

/**************************************************************************************************
* @fn      SetSDAOut
*
* @brief   Set SDA gpio as output mode.
*
* @param   None
*
* @return  None
**************************************************************************************************/
void SetSDAOut(void)
{
  GpioInit( &HumitureSdaGpio, PIN_SDA, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );  //SDA = 1
}

/**************************************************************************************************
* @fn      SetSDAValue
*
* @brief   Set the level of SDA Pin.
*
* @param   value: 0 or 1
*
* @return  None
**************************************************************************************************/
void SetSDAValue(uint8_t value)
{
  if(value)
    GpioWrite(&HumitureSdaGpio, 1);
  else
    GpioWrite(&HumitureSdaGpio, 0);
}

/**************************************************************************************************
* @fn      ReadSDAPin
*
* @brief   Read the level of SDA Pin.
*
* @param   None
*
* @return  the level(0 or 1) of SDA Pin.
**************************************************************************************************/
uint8_t ReadSDAPin(void)
{
  uint8_t value;
  
  SetSDAIn();
  value = GpioRead(&HumitureSdaGpio);
  SetSDAOut();
  return value;
}

/**************************************************************************************************
* @fn      s_write_byte
*
* @brief   Writes a byte on the Sensibus and checks the acknowledge.
*
* @param   value: The byte data to write.
*
* @return  error: "0" indicate write successfully, "1" indicate no acknowledge.
**************************************************************************************************/
char s_write_byte(unsigned char value)
{ 
  unsigned char i,error=0;  
  for (i=0x80;i>0;i/=2)             //shift bit for masking
  { if (i & value) SDA_H;          //masking value with i , write to SENSI-BUS
    else SDA_L;
    DelayUs(5);
    SCK_H;                          //clk for SENSI-BUS
    DelayUs(5);        //pulswith approx. 5 us  	
    SCK_L;
    DelayUs(5);
  }
  SDA_H;                           //release DATA-line
  SCK_H;                            //clk #9 for ack 
  error=ReadSDAPin();                       //check ack (DATA will be pulled down by SHT11)
  SCK_L;        
  return error;                     //error=1 in case of no acknowledge
}

/**************************************************************************************************
* @fn      s_read_byte
*
* @brief   Reads a byte form the Sensibus and gives an acknowledge in case of "ack=1".
*
* @param   ack: ACK or noACK.
*
* @return  The value to read.
**************************************************************************************************/
char s_read_byte(unsigned char ack) 
{ 
  unsigned char i,val=0;
  SDA_H;                           //release DATA-line
  for (i=0x80;i>0;i/=2)             //shift bit for masking
  { 
    DelayUs(5);
    SCK_H;                          //clk for SENSI-BUS
    DelayUs(5);
    if (ReadSDAPin()) val=(val | i);        //read bit  
    SCK_L;  					 
  }
  SetSDAValue(!ack);                        //in case of "ack==1" pull down DATA-Line
  DelayUs(5);
  SCK_H;                            //clk #9 for ack
  DelayUs(5);          //pulswith approx. 5 us 
  SCK_L;						    
  SDA_H;                           //release DATA-line
  return val;
}

/**************************************************************************************************
* @fn      s_transstart
*
* @brief   Generates a transmission start.
*
* @param   None.
*
* @return  None.
**************************************************************************************************/
//       _____         ________
// DATA:      |_______|
//           ___     ___
// SCK : ___|   |___|   |______

void s_transstart(void)
{  
   SDA_H; SCK_L;                   //Initial state
   DelayUs(5);
   SCK_H;
   DelayUs(5);
   SDA_L;
   DelayUs(5);
   SCK_L;  
   DelayUs(5);
   SCK_H;
   DelayUs(5);
   SDA_H;		   
   DelayUs(5);
   SCK_L;
   DelayUs(5);
}

/**************************************************************************************************
* @fn      s_connectionreset
*
* @brief   Communication reset: DATA-line=1 and at least 9 SCK cycles followed by transstart.
*
* @param   None.
*
* @return  None.
**************************************************************************************************/
//----------------------------------------------------------------------------------
// 
//       _____________________________________________________         ________
// DATA:                                                      |_______|
//          _    _    _    _    _    _    _    _    _        ___     ___
// SCK : __| |__| |__| |__| |__| |__| |__| |__| |__| |______|   |___|   |______

void s_connectionreset(void)
{  
  unsigned char i; 
  SDA_H; SCK_L;                    //Initial state
  for(i=0;i<9;i++)                  //9 SCK cycles
  { 
    SCK_H;
    DelayUs(5);
    SCK_L;
    DelayUs(5);
  }
  s_transstart();                   //transmission start
}

/**************************************************************************************************
* @fn      s_softreset
*
* @brief   Resets the sensor by a softreset.
*
* @param   None.
*
* @return  error=1 : no response form the sensor.
**************************************************************************************************/
char s_softreset(void) 
{ 
  unsigned char error=0;  
  s_connectionreset();              //reset communication
  error+=s_write_byte(RESET);       //send RESET-command to sensor
  return error;                     //error=1 in case of no response form the sensor
}

/**************************************************************************************************
* @fn      s_softreset
*
* @brief   Reads the status register with checksum (8-bit).
*
* @param   p_value: pointer to the value of status register.  p_checksum: pointer to the checksum.
*
* @return  error=1 : no response form the sensor.
**************************************************************************************************/
char s_read_statusreg(unsigned char *p_value, unsigned char *p_checksum)
{ 
  unsigned char error=0;
  s_transstart();                   //transmission start
  error=s_write_byte(STATUS_REG_R); //send command to sensor
  *p_value=s_read_byte(ACK);        //read status register (8-bit)
  *p_checksum=s_read_byte(noACK);   //read checksum (8-bit)  
  return error;                     //error=1 in case of no response form the sensor
}

/**************************************************************************************************
* @fn      s_write_statusreg
*
* @brief   Write the status register with checksum (8-bit).
*
* @param   p_value: pointer to the value of status register.
*
* @return  error>=1 : no response form the sensor.
**************************************************************************************************/
char s_write_statusreg(unsigned char *p_value)
{ 
  unsigned char error=0;
  s_transstart();                   //transmission start
  error+=s_write_byte(STATUS_REG_W);//send command to sensor
  error+=s_write_byte(*p_value);    //send value of status register
  return error;                     //error>=1 in case of no response form the sensor
}
 							   
/**************************************************************************************************
* @fn      s_measure
*
* @brief   Makes a measurement (humidity/temperature) with checksum.
*
* @param   p_value: pointer to the value of humidity/temperature. p_checksum: pointer to checksum. mode: TEMP/HUMI.
*
* @return  error>=1 : measure failed.
**************************************************************************************************/
char s_measure(unsigned char *p_value, unsigned char *p_checksum, unsigned char mode)
{ 
  unsigned error=0;
  unsigned long i;

  s_transstart();                   //transmission start
  switch(mode){                     //send command to sensor
    case TEMP	: error+=s_write_byte(MEASURE_TEMP); break;
    case HUMI	: error+=s_write_byte(MEASURE_HUMI); break;
    default     : break;	 
  }
  for (i=0;i<165535;i++) if(ReadSDAPin()==0) break; //wait until sensor has finished the measurement
  if(ReadSDAPin()) error+=1;                // or timeout (~2 sec.) is reached
  *(p_value)  =s_read_byte(ACK);    //read the first byte (MSB)
  *(p_value+1)=s_read_byte(ACK);    //read the second byte (LSB)
  *p_checksum =s_read_byte(noACK);  //read checksum
  return error;
}

/**************************************************************************************************
* @fn      calc_sth11
*
* @brief   Calculates temperature [¡ãC] and humidity [%RH].
*
* @param   input :      humi [Ticks] (12 bit) 
*                       temp [Ticks] (14 bit)
*          output:      humi [%RH]
*                       temp [¡ãC]
*
* @return  None.
**************************************************************************************************/
void calc_sth11(float *p_humidity ,float *p_temperature)
{ 
  const float C1=-4.0;              // for 12 Bit
  const float C2=+0.0405;           // for 12 Bit
  const float C3=-0.0000028;        // for 12 Bit
  const float T1=+0.01;             // for 14 Bit @ 5V
  const float T2=+0.00008;           // for 14 Bit @ 5V	

  float rh=*p_humidity;             // rh:      Humidity [Ticks] 12 Bit 
  float t=*p_temperature;           // t:       Temperature [Ticks] 14 Bit
  float rh_lin;                     // rh_lin:  Humidity linear
  float rh_true;                    // rh_true: Temperature compensated humidity
  float t_C;                        // t_C   :  Temperature [¡ãC]

  t_C=t*0.01 - 40;                  //calc. temperature from ticks to [¡ãC]
  rh_lin=C3*rh*rh + C2*rh + C1;     //calc. humidity from ticks to [%RH]
  rh_true=(t_C-25)*(T1+T2*rh)+rh_lin;   //calc. temperature compensated humidity [%RH]
  if(rh_true>100)rh_true=100;       //cut if the value is outside of
  if(rh_true<0.1)rh_true=0.1;       //the physical possible range

  *p_temperature=t_C;               //return temperature [¡ãC]
  *p_humidity=rh_true;              //return humidity[%RH]
}

/**************************************************************************************************
* @fn      calc_dewpoint
*
* @brief   calculates dew point.
*
* @param   h: humidity [%RH], t: temperature [¡ãC]
*
* @return  dew point [¡ãC].
**************************************************************************************************/
float calc_dewpoint(float h,float t)
{ 
  float logEx,dew_point;
  logEx=0.66077+7.5*t/(237.3+t)+(log10(h)-2);
  dew_point = (logEx - 0.66077)*237.3/(0.66077+7.5-logEx);
  return dew_point;
}

void ReadHumiture(void)
{
  unsigned char humi[2], temp[2];
  float dew_point;
  unsigned char error,checksum;
  
  error=0;
  error+=s_measure((unsigned char*) humi,&checksum,HUMI);  //measure humidity
  error+=s_measure((unsigned char*) temp,&checksum,TEMP);  //measure temperature
  if(error!=0) 
    s_connectionreset();                 //in case of an error: connection reset
  else
  { 
    HumitureValue.humi = (float)((humi[0]<<8)|humi[1]);       //converts integer to float
    HumitureValue.temp = (float)((temp[0]<<8)|temp[1]);       //converts integer to float
    calc_sth11(&HumitureValue.humi, &HumitureValue.temp);            //calculate humidity, temperature
    dew_point = calc_dewpoint(HumitureValue.humi, HumitureValue.temp); //calculate dew point
    PRINTF("temp:%5.1fC humi:%5.1f%% dew point:%5.1fC\r\n", HumitureValue.temp, HumitureValue.humi, dew_point);
#if (USE_LCD12864)
    ShowFloat(strlen("Temperature:")*6, 40, HumitureValue.temp, 1);
    ShowFloat(strlen("Humi:")*6, 52, HumitureValue.humi, 1);
#endif
  }
}

