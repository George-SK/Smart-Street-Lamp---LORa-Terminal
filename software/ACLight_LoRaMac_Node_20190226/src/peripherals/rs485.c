#include "rs485.h"
#include "board.h"

#define UART3_TX	PB_10
#define UART3_RX	PB_11
#define RX_TX_EN	PA_3

#define FIFO_UART3_RX_SIZE	128
#define RS485_MAX_LEN				20

static UART_HandleTypeDef Uart3Handle;

static uint8_t UART3RxFIFO[FIFO_UART3_RX_SIZE];

static uint8_t Uart3RxBuffer[RS485_MAX_LEN], Uart3Len = 0;

Gpio_t RxTxEnPin;

void Uart3IrqNotify( UartNotifyId_t id );

void Rs485Init(void)
{	
	FifoInit( &Uart3.FifoRx, UART3RxFIFO, FIFO_UART3_RX_SIZE ); 
  Uart3.IrqNotify = Uart3IrqNotify;  
    
  UartInit( &Uart3, UART_3, UART3_TX, UART3_RX );
  UartConfig( &Uart3, RX_TX, 115200, UART_8_BIT, UART_1_STOP_BIT, NO_PARITY, NO_FLOW_CTRL );
	GpioInit( &RxTxEnPin, RX_TX_EN, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );	//RX
}

static void Uart3IrqNotify( UartNotifyId_t id )
{
    uint8_t data;
    if( id == UART_NOTIFY_RX )
    {
        if( UartGetChar( &Uart3, &data ) == 0)
        {
          if(Uart3Len < RS485_MAX_LEN)
						Uart3RxBuffer[Uart3Len++] = data;
					
					if(data == '\n')
					{
						Uart3RxBuffer[Uart3Len] = '\0';
						PRINTF("RS485 RECV:%s", Uart3RxBuffer);
						Uart3Len = 0;
					}						 
        }
    }  
}

void RS485SendData(uint8_t *data, uint8_t len)
{
	SetRxTxMode(0);	//Switch to TX mode.
	HAL_UART_Transmit(&Uart3.UartHandle, data, len, 20);
	SetRxTxMode(1); //Switch to RX mode.
}

//1---RX	0---TX
void SetRxTxMode(uint8_t mode)
{
	if(mode)
		GpioWrite( &RxTxEnPin, 1 );
	else
		GpioWrite( &RxTxEnPin, 0 );
}
