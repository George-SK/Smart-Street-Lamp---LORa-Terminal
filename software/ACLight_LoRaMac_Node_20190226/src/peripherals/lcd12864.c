#include "LCD12864.h"
#include "board.h"
#include "math.h"

#define LCD_MOSI                                    PA_7
#define LCD_MISO                                    PA_6
#define LCD_SCLK                                    PA_5
#define LCD_CS                                      IOE_4
#define LCD_RST                                     IOE_5

#define Dis_X_MAX  128-1
#define Dis_Y_MAX  64-1

/*
 * Private functions prototypes
 */

static void LcdWriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size );
static void LcdReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size );
static void LcdWrite( uint8_t addr, uint8_t data );
static uint8_t LcdRead( uint8_t addr );
static void SPI_SSSet(unsigned char Status);
static void SPI_Send(unsigned char Data);

/*
 * Private global variables
 */

Gpio_t LcdCsGpio, LcdRstGpio;
Spi_t LcdSpi;

void LcdInit(void)
{ 
  GpioInit( &LcdCsGpio, LCD_CS, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );  //LCD_CS = 1
  GpioInit( &LcdRstGpio, LCD_RST, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );  //LCD_RST = 1
  
  SpiInit( &LcdSpi, LCD_MOSI, LCD_MISO, LCD_SCLK, NC ); 
  
  GpioWrite( &LcdRstGpio, 0 );
  DelayMs(2);
  GpioWrite( &LcdRstGpio, 1 );
  DelayMs(12);
  
  PRINTF("lcd initialization!\r\n");
  
  ClrScreen();
  LcdWrite(0x8A, 50);
  FontSet_cn(0,1);
	PutString(41, 0, "E-MAGE:");
  PutString(0, 16, "Light sensor:");
  PutString(0, 28, "Gas sensor:");
  PutString(0, 40, "Temperature:");
  PutString(0, 52, "Humi:");
  ShowFloat(strlen("Light sensor:")*6, 16, 0, 1);
  PutChar(strlen("Gas sensor:")*6, 28, '0');
  ShowFloat(strlen("Temperature:")*6, 40, 0, 1);
  ShowFloat(strlen("Humi:")*6, 52, 0, 1);
}

static void LcdWriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
  uint8_t i;

  //NSS = 0;
  GpioWrite( &LcdCsGpio, 0 );

  SpiInOut( &LcdSpi, addr );
  for( i = 0; i < size; i++ )
  {
    SpiInOut( &LcdSpi, buffer[i] );
  }

  //NSS = 1;
  GpioWrite( &LcdCsGpio, 1 );
}

static void LcdReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
  uint8_t i;

  //NSS = 0;
  GpioWrite( &LcdCsGpio, 0 );

  SpiInOut( &LcdSpi, addr );

  for( i = 0; i < size; i++ )
  {
    buffer[i] = SpiInOut( &LcdSpi, 0 );
  }

  //NSS = 1;
  GpioWrite( &LcdCsGpio, 1 );
}

static void LcdWrite( uint8_t addr, uint8_t data )
{
  LcdWriteBuffer( addr, &data, 1 );
}

static uint8_t LcdRead( uint8_t addr )
{
  uint8_t data;
  
  LcdReadBuffer( addr, &data, 1 );
  
  return data;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


unsigned char X_Witch=6;
unsigned char Y_Witch=10;
unsigned char X_Witch_cn=16;
unsigned char Y_Witch_cn=16;
unsigned char Dis_Zero=0;


static void SPI_SSSet(unsigned char Status)
{
  if(Status)
    GpioWrite( &LcdCsGpio, 1 );
  else
    GpioWrite( &LcdCsGpio, 0 );
}

static void SPI_Send(unsigned char Data)
{
  SpiInOut( &LcdSpi, Data );
}

void FontSet(unsigned char Font_NUM,unsigned char Color)
{
  unsigned char ucTemp=0;
  
  if(Font_NUM==0)
  {
    X_Witch = 6;//7;
    Y_Witch = 10;
  }
  else
  {
    X_Witch = 8;
    Y_Witch = 16;
  }
  ucTemp = (Font_NUM<<4)|Color;
  SPI_SSSet(0);
  SPI_Send(0x81);
  SPI_Send(ucTemp);
  SPI_SSSet(1);
}

void FontMode(unsigned char Cover,unsigned char Color)
{
  unsigned char ucTemp=0;
  ucTemp = (Cover<<4)|Color;

  SPI_SSSet(0);
  SPI_Send(0x89);
  SPI_Send(ucTemp);
  SPI_SSSet(1);
}

void FontSet_cn(unsigned char Font_NUM,unsigned char Color)
{
  unsigned char ucTemp=0;
  
  if(Font_NUM==0)
  {
    X_Witch_cn = 12;
    Y_Witch_cn = 12;
  }
  else
  {
    X_Witch_cn = 16;
    Y_Witch_cn = 16;
  }
  ucTemp = (Font_NUM<<4)|Color;

  SPI_SSSet(0);
  SPI_Send(0x82);
  SPI_Send(ucTemp);
  SPI_SSSet(1);
}

void PutChar(unsigned char x,unsigned char y,unsigned char a) 
{
  SPI_SSSet(0);
  SPI_Send(7);
  SPI_Send(x);
  SPI_Send(y);
  SPI_Send(a);
  SPI_SSSet(1);
}

void PutString(unsigned char x,unsigned char y,unsigned char *p)
{
  while(*p!=0)
  {
    PutChar(x,y,*p);
    x += X_Witch;
    if((x + X_Witch) > Dis_X_MAX)
    {
      x = Dis_Zero;
      if((Dis_Y_MAX - y) < Y_Witch) break;
      else y += Y_Witch;
    }
    p++;
  }
}

void PutChar_cn(unsigned char x,unsigned char y,unsigned char * GB) 
{
  SPI_SSSet(0);
  SPI_Send(8);
  SPI_Send(x);
  SPI_Send(y);
  
  SPI_Send(*(GB++));
  SPI_Send(*GB);
  SPI_SSSet(1);
}

void PutString_cn(unsigned char x,unsigned char y,unsigned char *p)
{
  while(*p!=0)
  {
    if(*p<128)
    {
      PutChar(x,y,*p);
      x += X_Witch+1;
      if((x/* + X_Witch*/) > Dis_X_MAX)
      {
        x = Dis_Zero;
        if((Dis_Y_MAX - y) < Y_Witch) break;
        else y += Y_Witch_cn;
      }
      p+=1;
    }
    else
    {
      PutChar_cn(x,y,p);
      x += X_Witch_cn+1;
      if((x/* + X_Witch_cn*/) > Dis_X_MAX)
      {
        x = Dis_Zero;
        if((Dis_Y_MAX - y) < Y_Witch_cn) break;
        else y += Y_Witch_cn;
      }
      p+=2;
    }
  }
}

void SetPaintMode(unsigned char Mode,unsigned char Color)
{
  unsigned char ucTemp=0;
  ucTemp = (Mode<<4)|Color;
 
  SPI_SSSet(0);
  SPI_Send(0x83);
  SPI_Send(ucTemp);
  SPI_SSSet(1);
}

void PutPixel(unsigned char x,unsigned char y)
{
  SPI_SSSet(0);
  SPI_Send(1);
  SPI_Send(x);
  SPI_Send(y);
  SPI_SSSet(1);
}

void Line(unsigned char s_x,unsigned char  s_y,unsigned char  e_x,unsigned char  e_y)
{  
  SPI_SSSet(0);
  SPI_Send(2);
  SPI_Send(s_x);
  SPI_Send(s_y);
  SPI_Send(e_x);
  SPI_Send(e_y);
  SPI_SSSet(1);
}

void Circle(unsigned char x,unsigned char y,unsigned char r,unsigned char mode)
{
  SPI_SSSet(0);
  if(mode)
    SPI_Send(6);
  else
    SPI_Send(5);
  SPI_Send(x);
  SPI_Send(y);
  SPI_Send(r);
  SPI_SSSet(1);
}

void Rectangle(unsigned char left, unsigned char top, unsigned char right,
     unsigned char bottom, unsigned char mode)
{
  SPI_SSSet(0);
  if(mode)
    SPI_Send(4);
  else
    SPI_Send(3);
  SPI_Send(left);
  SPI_Send(top);
  SPI_Send(right);
  SPI_Send(bottom);
  SPI_SSSet(1);
}

void ClrScreen(void)
{
  SPI_SSSet(0);
  SPI_Send(0x80);
  SPI_SSSet(1);
}

void PutBitmap(unsigned char x,unsigned char y,unsigned char width,unsigned char high,unsigned char *p)
{
  unsigned short Dat_Num;
  unsigned char ucTemp=0;
  SPI_SSSet(0);
  SPI_Send(0x0e);  
  SPI_Send(x);
  SPI_Send(y);
  SPI_Send(width);
  SPI_Send(high);
  
  width = width+0x07;
  Dat_Num = (width>>3)*high;
  while(Dat_Num--)
  {
    ucTemp++;
  SPI_Send(*p);
  if(ucTemp>250)
  {
    DelayMs(2);
    ucTemp = 0;
  }
  p++;
  }
  SPI_SSSet(1);
}

void ShowChar(unsigned char x,unsigned char y,unsigned char a,unsigned char type) 
{
  SPI_SSSet(0);
  SPI_Send(11);
  SPI_Send(x);
  SPI_Send(y);
  SPI_Send(a);
  SPI_Send(type);
  SPI_SSSet(1);
}

void ShowShort(unsigned char x,unsigned char y,unsigned short a,unsigned char type) 
{
  SPI_SSSet(0);
  SPI_Send(12);
  SPI_Send(x);
  SPI_Send(y);
  SPI_Send((unsigned char)(a>>8));
  SPI_Send((unsigned char)a);
  SPI_Send(type);
  SPI_SSSet(1);
}

void SetBackLight(unsigned char Deg) 
{
  SPI_SSSet(0);
  SPI_Send(0x8a);
  SPI_Send(Deg);
  SPI_SSSet(1);
}



void ShowFloat(unsigned char x, unsigned char y, float num, unsigned char DecNum)
{
  int Int, Dec;
  unsigned char p[20], index = 0, endChar = '\0';  
  
  Int = num;
  Dec = (int)((num-Int)*pow(10, DecNum));
  if(num < 0)
  {
    Int = -Int;
    Dec = -Dec;
    p[0] = '-';
    index++;
  }
  sprintf(p+index, "%d.%d%c", Int, Dec, endChar);
  PutString(x , y, p);
}

