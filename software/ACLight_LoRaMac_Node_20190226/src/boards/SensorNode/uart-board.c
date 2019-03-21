/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: Board UART driver implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include "board.h"

#include "uart-board.h"
#include "stdarg.h"

#define BUFSIZE	128

static uint16_t iw;
static char buff[BUFSIZE+16];

void UartMcuInit( Uart_t *obj, uint8_t uartId, PinNames tx, PinNames rx )
{
    obj->UartId = uartId;

    if(uartId == UART_1)
		{
			__HAL_RCC_USART1_FORCE_RESET( );
			__HAL_RCC_USART1_RELEASE_RESET( );
			__HAL_RCC_USART1_CLK_ENABLE( );

			GpioInit( &obj->Tx, tx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART1 );
			GpioInit( &obj->Rx, rx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART1 );
		}
		else if(uartId == UART_2)
		{
			__HAL_RCC_USART2_FORCE_RESET( );
			__HAL_RCC_USART2_RELEASE_RESET( );
			__HAL_RCC_USART2_CLK_ENABLE( );

			GpioInit( &obj->Tx, tx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART2 );
			GpioInit( &obj->Rx, rx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART2 );
		}
		else if(uartId == UART_3)
		{
			__HAL_RCC_USART3_FORCE_RESET( );
			__HAL_RCC_USART3_RELEASE_RESET( );
			__HAL_RCC_USART3_CLK_ENABLE( );

			GpioInit( &obj->Tx, tx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART3 );
			GpioInit( &obj->Rx, rx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART3 );
		}
}

void UartMcuConfig( Uart_t *obj, UartMode_t mode, uint32_t baudrate, WordLength_t wordLength, StopBits_t stopBits, Parity_t parity, FlowCtrl_t flowCtrl )
{
    if(obj->UartId == UART_1)
			obj->UartHandle.Instance = USART1;
		else if(obj->UartId == UART_2)
			obj->UartHandle.Instance = USART2;
		else if(obj->UartId == UART_3)
			obj->UartHandle.Instance = USART3;
		
    obj->UartHandle.Init.BaudRate = baudrate;

    if( mode == TX_ONLY )
    {
        if( obj->FifoTx.Data == NULL )
        {
            assert_param( FAIL );
        }
        obj->UartHandle.Init.Mode = UART_MODE_TX;
    }
    else if( mode == RX_ONLY )
    {
        if( obj->FifoRx.Data == NULL )
        {
            assert_param( FAIL );
        }
        obj->UartHandle.Init.Mode = UART_MODE_RX;
    }
    else if( mode == RX_TX )
    {
        if( ( obj->FifoTx.Data == NULL ) || ( obj->FifoRx.Data == NULL ) )
        {
            assert_param( FAIL );
        }
        obj->UartHandle.Init.Mode = UART_MODE_TX_RX;
    }
    else
    {
       assert_param( FAIL );
    }

    if( wordLength == UART_8_BIT )
    {
        obj->UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    }
    else if( wordLength == UART_9_BIT )
    {
        obj->UartHandle.Init.WordLength = UART_WORDLENGTH_9B;
    }

    switch( stopBits )
    {
    case UART_2_STOP_BIT:
        obj->UartHandle.Init.StopBits = UART_STOPBITS_2;
        break;
    case UART_1_STOP_BIT:
    default:
        obj->UartHandle.Init.StopBits = UART_STOPBITS_1;
        break;
    }

    if( parity == NO_PARITY )
    {
        obj->UartHandle.Init.Parity = UART_PARITY_NONE;
    }
    else if( parity == EVEN_PARITY )
    {
        obj->UartHandle.Init.Parity = UART_PARITY_EVEN;
    }
    else
    {
        obj->UartHandle.Init.Parity = UART_PARITY_ODD;
    }

    if( flowCtrl == NO_FLOW_CTRL )
    {
        obj->UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    }
    else if( flowCtrl == RTS_FLOW_CTRL )
    {
        obj->UartHandle.Init.HwFlowCtl = UART_HWCONTROL_RTS;
    }
    else if( flowCtrl == CTS_FLOW_CTRL )
    {
        obj->UartHandle.Init.HwFlowCtl = UART_HWCONTROL_CTS;
    }
    else if( flowCtrl == RTS_CTS_FLOW_CTRL )
    {
        obj->UartHandle.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
    }

    obj->UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;

    if( HAL_UART_Init( &obj->UartHandle ) != HAL_OK )
    {
        assert_param( FAIL );
    }

    if(obj->UartId == UART_1)
		{
			HAL_NVIC_SetPriority( USART1_IRQn, 8, 0 );
			HAL_NVIC_EnableIRQ( USART1_IRQn );			
		}
		else if(obj->UartId == UART_2)
		{
			HAL_NVIC_SetPriority( USART2_IRQn, 8, 0 );
			HAL_NVIC_EnableIRQ( USART2_IRQn );
		}
		else if(obj->UartId == UART_3)
		{
			HAL_NVIC_SetPriority( USART3_IRQn, 6, 0 );
			HAL_NVIC_EnableIRQ( USART3_IRQn );
		}

		/* Enable the UART Data Register not empty Interrupt */
		HAL_UART_Receive_IT( &obj->UartHandle, &obj->RxData, 1 );
}

void UartMcuDeInit( Uart_t *obj )
{
    if(obj->UartId == UART_1)
		{
			__HAL_RCC_USART1_FORCE_RESET( );
			__HAL_RCC_USART1_RELEASE_RESET( );
			__HAL_RCC_USART1_CLK_DISABLE( );
		}			
		else if(obj->UartId == UART_2)
		{
			__HAL_RCC_USART2_FORCE_RESET( );
			__HAL_RCC_USART2_RELEASE_RESET( );
			__HAL_RCC_USART2_CLK_DISABLE( );
		}
		else if(obj->UartId == UART_3)
		{
			__HAL_RCC_USART3_FORCE_RESET( );
			__HAL_RCC_USART3_RELEASE_RESET( );
			__HAL_RCC_USART3_CLK_DISABLE( );
		}

    GpioInit( &obj->Tx, obj->Tx.pin, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &obj->Rx, obj->Rx.pin, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
}

uint8_t UartMcuPutChar( Uart_t *obj, uint8_t data )
{
    BoardDisableIrq( );
    //TxData = data;

    if( IsFifoFull( &obj->FifoTx ) == false )
    {
        FifoPush( &obj->FifoTx, obj->TxData );

        // Trig UART Tx interrupt to start sending the FIFO contents.
        __HAL_UART_ENABLE_IT( &obj->UartHandle, UART_IT_TC );

        BoardEnableIrq( );
        return 0; // OK
    }
    BoardEnableIrq( );
    return 1; // Busy
}

uint8_t UartMcuGetChar( Uart_t *obj, uint8_t *data )
{
    BoardDisableIrq( );

    if( IsFifoEmpty( &obj->FifoRx ) == false )
    {
        *data = FifoPop( &obj->FifoRx );
        BoardEnableIrq( );
        return 0;
    }
    BoardEnableIrq( );
    return 1;
}

void HAL_UART_TxCpltCallback( UART_HandleTypeDef *handle )
{
    if(handle->Instance == USART1)
		{
			if( IsFifoEmpty( &Uart1.FifoTx ) == false )
			{
					Uart1.TxData = FifoPop( &Uart1.FifoTx );
					//  Write one byte to the transmit data register
					HAL_UART_Transmit_IT( &Uart1.UartHandle, &Uart1.TxData, 1 );
			}

			if( Uart1.IrqNotify != NULL )
			{
					Uart1.IrqNotify( UART_NOTIFY_TX );
			}
		}
		else if(handle->Instance == USART2)
		{
			if( IsFifoEmpty( &Uart2.FifoTx ) == false )
			{
					Uart2.TxData = FifoPop( &Uart2.FifoTx );
					//  Write one byte to the transmit data register
					HAL_UART_Transmit_IT( &Uart2.UartHandle, &Uart2.TxData, 1 );
			}

			if( Uart2.IrqNotify != NULL )
			{
					Uart2.IrqNotify( UART_NOTIFY_TX );
			}
		}
		else if(handle->Instance == USART3)
		{
			if( IsFifoEmpty( &Uart3.FifoTx ) == false )
			{
					Uart3.TxData = FifoPop( &Uart3.FifoTx );
					//  Write one byte to the transmit data register
					HAL_UART_Transmit_IT( &Uart3.UartHandle, &Uart3.TxData, 1 );
			}

			if( Uart3.IrqNotify != NULL )
			{
					Uart3.IrqNotify( UART_NOTIFY_TX );
			}
		}
}

void HAL_UART_RxCpltCallback( UART_HandleTypeDef *handle )
{
    if(handle->Instance == USART1)
		{
			if( IsFifoFull( &Uart1.FifoRx ) == false )
			{
					// Read one byte from the receive data register
					FifoPush( &Uart1.FifoRx, Uart1.RxData );
			}

			if( Uart1.IrqNotify != NULL )
			{
					Uart1.IrqNotify( UART_NOTIFY_RX );
			}

			HAL_UART_Receive_IT( &Uart1.UartHandle, &Uart1.RxData, 1 );
		}
		else if(handle->Instance == USART2)
		{
			if( IsFifoFull( &Uart2.FifoRx ) == false )
			{
					// Read one byte from the receive data register
					FifoPush( &Uart2.FifoRx, Uart2.RxData );
			}

			if( Uart2.IrqNotify != NULL )
			{
					Uart2.IrqNotify( UART_NOTIFY_RX );
			}

			HAL_UART_Receive_IT( &Uart2.UartHandle, &Uart2.RxData, 1 );
		}
		else if(handle->Instance == USART3)
		{
			if( IsFifoFull( &Uart3.FifoRx ) == false )
			{
					// Read one byte from the receive data register
					FifoPush( &Uart3.FifoRx, Uart3.RxData );
			}

			if( Uart3.IrqNotify != NULL )
			{
					Uart3.IrqNotify( UART_NOTIFY_RX );
			}

			HAL_UART_Receive_IT( &Uart3.UartHandle, &Uart3.RxData, 1 );
		}
}

void HAL_UART_ErrorCallback( UART_HandleTypeDef *handle )
{
    if(handle->Instance == USART1)
			HAL_UART_Receive_IT( &Uart1.UartHandle, &Uart1.RxData, 1 );
		else if(handle->Instance == USART2)
			HAL_UART_Receive_IT( &Uart2.UartHandle, &Uart2.RxData, 1 );
		else if(handle->Instance == USART3)
			HAL_UART_Receive_IT( &Uart3.UartHandle, &Uart3.RxData, 1 );
}

void USART1_IRQHandler( void )
{
    HAL_UART_IRQHandler( &Uart1.UartHandle );
}

void USART2_IRQHandler( void )
{
    HAL_UART_IRQHandler( &Uart2.UartHandle );
}

void USART3_IRQHandler( void )
{
    HAL_UART_IRQHandler( &Uart3.UartHandle );
}

void UartPutString(uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    HAL_UART_Transmit(&Uart1.UartHandle,pData,Size,Timeout);
}

void vcom_Send( char *format, ... )
{
  va_list args;
  va_start(args, format);
  
  /*convert into string at buff[0] of length iw*/
  iw = vsprintf(&buff[0], format, args);
  
  HAL_UART_Transmit(&Uart1.UartHandle,(uint8_t *)&buff[0], iw, 300);
  
  va_end(args);
}
