#include "mmp510.h"
#include "board.h"
#include "string.h"

#define UART2_TX                                     PA_2
#define UART2_RX                                     PA_3
#define INT_OUT																			 PC_13//PA_0
#define INT_IN																			 PA_1
#define IO1																					 PA_0//PC_13
#define REST																				 PB_10

#define MAX_TIMEOUT		100

#define ANY_DATA_UPLINK		1

MMP510_t MMP510Info;

void Uart2IrqNotify( UartNotifyId_t id );
int MMP510MDataHandle(void);

const char* strErrorCMD = "ERROR CMD!";
const char* strRightCMD = "RECEIVED!";
const uint8_t maxRetryTimes = 3;
static UART_HandleTypeDef Uart2Handle;

Gpio_t IntOutGpio, IntInGpio, IO1Gpio;
Gpio_t TxGpio, RxGpio;

void Mmp510Init(void)
{
	Gpio_t IoPin;
	
	__HAL_RCC_USART2_FORCE_RESET( );
	__HAL_RCC_USART2_RELEASE_RESET( );
	__HAL_RCC_USART2_CLK_ENABLE( );

	GpioInit( &TxGpio, UART2_TX, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART2 );
	GpioInit( &RxGpio, UART2_RX, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART2 );
	
	Uart2Handle.Instance        = USART2;

  Uart2Handle.Init.BaudRate   = 115200;
  Uart2Handle.Init.WordLength = UART_WORDLENGTH_8B;
  Uart2Handle.Init.StopBits   = UART_STOPBITS_1;
  Uart2Handle.Init.Parity     = UART_PARITY_NONE;
  Uart2Handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
  Uart2Handle.Init.Mode       = UART_MODE_TX_RX;
  if(HAL_UART_DeInit(&Uart2Handle) != HAL_OK)
  {
    
  }  
  if(HAL_UART_Init(&Uart2Handle) != HAL_OK)
  {
    
  }
	
	//INT_OUT --- input
	GpioInit( &IntOutGpio, INT_OUT, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_DOWN, 0 );
	GpioSetInterrupt( &IntOutGpio, IRQ_RISING_EDGE, IRQ_LOW_PRIORITY, MMP510MDataHandler );
	HAL_NVIC_DisableIRQ( EXTI15_10_IRQn );	//Disable INT_OUT pin interrupt.
	//INT_IN --- output
	GpioInit( &IntInGpio, INT_IN, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
	//IO1 --- output
	GpioInit( &IO1Gpio, IO1, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );	
	
	//RST --- output
	GpioInit( &IoPin, REST, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
	DelayMs(10);
	GpioInit( &IoPin, REST, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
	DelayMs(10);
	GpioInit( &IoPin, REST, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
	
	//MMP510 variable initialization
	memset(&MMP510Info, 0, sizeof(MMP510_t));
	MMP510Info.parkStatus = INIT_STATUS;
	MMP510Info.RxLen = 36;
	
	DelayMs(1000);
	
	PRINTF("MMP510 init done\r\n");
	
	//SendCmdToMMP510M((uint8_t *)("SetHR=0=CMD"), strlen("SetHR=0=CMD"));
	//SendCmdToMMP510M((uint8_t *)("ClearParkTimes=CMD"), strlen("ClearParkTimes=CMD"));
}

void MMP510MDataHandler(void)
{
	if(MMP510Info.NeedUplink == 0)
		MMP510MDataHandle();
}

int MMP510MDataHandle(void)
{
	memset(MMP510Info.MMP510RxBuffer,0,MAX_BUF_LEN);
	HAL_UART_Receive(&Uart2Handle, MMP510Info.MMP510RxBuffer, MAX_BUF_LEN, 20);
	
	uint8_t idx = Uart2Handle.RxXferSize;
	
	while((MMP510Info.MMP510RxBuffer[idx]=='\0') || (MMP510Info.MMP510RxBuffer[idx]==' ') || (MMP510Info.MMP510RxBuffer[idx]=='\r') || (MMP510Info.MMP510RxBuffer[idx]=='\n') || (MMP510Info.MMP510RxBuffer[idx]=='\t'))
	{
		if((idx--) == 1)  
			return -1;
	}
	
	if(MMP510Info.MMP510RxBuffer[0] == 0XFB)
	{
		if( idx >= 35 && 0X22 == MMP510Info.MMP510RxBuffer[1] && 0XFE == MMP510Info.MMP510RxBuffer[35] && 0XA0 == MMP510Info.MMP510RxBuffer[2])
		{
			MMP510Info.RxLen = 36;
			
			if(MMP510Info.MMP510RxBuffer[3] == 0x01)
			{
#if !ANY_DATA_UPLINK
				if(MMP510Info.parkStatus != PARK_OFF && MMP510Info.parkStatus != PARK_CHANGE_OFF)
				{
					MMP510Info.parkStatus = PARK_CHANGE_OFF;
					MMP510Info.NeedUplink = 1;
				}
				else
					MMP510Info.parkStatus = PARK_OFF;
#else
				MMP510Info.NeedUplink = 1;
#endif
				PRINTF("OFF:");
			}
			else if(MMP510Info.MMP510RxBuffer[3] == 0x02)
			{
#if !ANY_DATA_UPLINK
				if(MMP510Info.parkStatus != PARK_ON && MMP510Info.parkStatus != PARK_CHANGE_ON)
				{
					MMP510Info.parkStatus = PARK_CHANGE_ON;
					MMP510Info.NeedUplink = 1;
				}
				else
					MMP510Info.parkStatus = PARK_ON;
#else
				MMP510Info.NeedUplink = 1;
#endif
				PRINTF("ON :");
			}
			else if(MMP510Info.MMP510RxBuffer[3] == 0x00)
			{
				MMP510Info.parkStatus = INIT_STATUS;
				PRINTF("INIT:");
			}
			
			for(uint8_t i = 0; i <= idx; i++)
				PRINTF("%#X ", MMP510Info.MMP510RxBuffer[i]);
			
			PRINTF("\r\n");			
		}
	}
	else if(strstr((char *)MMP510Info.MMP510RxBuffer,strErrorCMD))
	{
		PRINTF("%s\r\n", MMP510Info.MMP510RxBuffer);
		MMP510Info.CmdFeedback = 1;
		MMP510Info.RxLen = strlen((char *)MMP510Info.MMP510RxBuffer) - 1;
	}
	else if(strstr((char *)MMP510Info.MMP510RxBuffer,strRightCMD))
	{
		PRINTF("%s\r\n", MMP510Info.MMP510RxBuffer);
		MMP510Info.CmdFeedback = 1;
		MMP510Info.RxLen = strlen((char *)MMP510Info.MMP510RxBuffer) - 1;
	}
	else
	{
		return -1;
	}
	
	return 0;
}

int SendCmdToMMP510M(const uint8_t * cmdBuf,uint16_t bufLen)
{	
	int i = 0;
	int rev = -1;
	
	GpioWrite(&IntInGpio, 0);
	HAL_Delay(2);
	GpioWrite(&IntInGpio, 1);
	HAL_Delay(80);
	GpioWrite(&IntInGpio, 0);
	
	for(i = 0; i < maxRetryTimes; i++)
	{
		if(HAL_UART_Transmit(&Uart2Handle, (uint8_t*)cmdBuf,bufLen, MAX_TIMEOUT) == HAL_OK)
		{
			rev = 0;
			break;
		}
		
		HAL_Delay(2);
	}
	
	return rev;
}

void McuBusySet(uint8_t status)
{
	if(status)
	{
		HAL_NVIC_DisableIRQ( EXTI15_10_IRQn );
		GpioInit( &IO1Gpio, IO1, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
	}
	else
	{		
		GpioInit( &IO1Gpio, IO1, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_DOWN, 0 );
		HAL_NVIC_EnableIRQ( EXTI15_10_IRQn );
	}
}

uint8_t GetParkStatus(void)
{
	return MMP510Info.parkStatus;
}
