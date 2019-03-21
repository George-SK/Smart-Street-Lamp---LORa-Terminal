/*******************************************************************************
  * @file    dali.h 
  * @author  
  * @version 
  * @date    
  * @brief   
  *****************************************************************************/	
#ifndef _DALI__H_
#define _DALI__H_
/* Exported include ----------------------------------------------------------*/
#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
/* Exported types ------------------------------------------------------------*/
#define DEVICE_TYPE_STM32_HAL
#define DALI_DEVICE_TYPE_6			// LED module

#if defined (STM32L073xx) || defined (STM32L072xx)
#include "stm32l0xx.h"
#elif defined (STM32L151xB)
#include "stm32l1xx.h"
#endif

#include "tim.h"
#include "board.h"
#include "gpio-board.h"

#define DALI_DEVICE_TYPE_MASTER
//#define DALI_DEVICE_TYPE_SLAVE

/* Exported constants --------------------------------------------------------*/
#define DALI_VERSION						"V1.0.07"

#define DALI_PULSE_WIDTH					416
#define	DALI_TIMER_INTERRUPT_INTERVAL		82		// uint us
#define DALI_FIFO_DATA_MAX_SIZE				10
/* Exported enum -------------------------------------------------------------*/
typedef enum _DALI_FUNCTION_e_
{
	DALI_FUNCTION_IDLE = 0,
	DALI_FUNCTION_RANDOM_ADDR_ASSIGNMENT,
	DALI_FUNCTION_SET_POWER,
	DALI_FUNCTION_QUERY_STATUS,
	DALI_FUNCTION_QUERY_VERSION,
}DALI_FUNCTION_e;

typedef enum _DALI_RESPONCE_TYPE_e_
{
	DALI_RESPONCE_YES	= 0xff,
	DALI_RESPONCE_NO	= 0x00,
	DALI_RESPONCE_OTHER,
}DALI_RESPONCE_TYPE_e;

typedef enum _DALI_COMMAND_e_
{
	DALI_COMMAND_N	 = 0x0000,				// YAAA AAA0 XXXX XXXX 
	DALI_COMMAND_0	 = 0x0100,				// YAAA AAA1 0000 0000 
	DALI_COMMAND_1	 = 0x0101,				// YAAA AAA1 0000 0001 
	DALI_COMMAND_2	 = 0x0102,				// YAAA AAA1 0000 0010 
	DALI_COMMAND_3	 = 0x0103,				// YAAA AAA1 0000 0011 
	DALI_COMMAND_4	 = 0x0104,				// YAAA AAA1 0000 0100 
	DALI_COMMAND_5   = 0x0105,				// YAAA AAA1 0000 0101	Return to max power level
	DALI_COMMAND_6   = 0x0106,				// YAAA AAA1 0000 0110	Return to min power level
	DALI_COMMAND_7	 = 0x0107,				// YAAA AAA1 0000 0111 
	DALI_COMMAND_8	 = 0x0108,				// YAAA AAA1 0000 1000 
	DALI_COMMAND_9	 = 0x0109,				// YAAA AAA1 0000 1001 

	DALI_COMMAND_32	 = 0x0120,				// YAAA AAA1 0010 0000
	DALI_COMMAND_33	 = 0x0121,				// YAAA AAA1 0010 0001

	DALI_COMMAND_42	 = 0x012a,				// YAAA AAA1 0010 1010
	DALI_COMMAND_43	 = 0x012b,				// YAAA AAA1 0010 1011
	DALI_COMMAND_44	 = 0x012c,				// YAAA AAA1 0010 1100
	DALI_COMMAND_45	 = 0x012d,				// YAAA AAA1 0010 1101
	DALI_COMMAND_46	 = 0x012e,				// YAAA AAA1 0010 1110
	DALI_COMMAND_47	 = 0x012f,				// YAAA AAA1 0010 1111

	DALI_COMMAND_64_79		= 0x0140,		// YAAA AAA1 0100 XXXX
	DALI_COMMAND_80_95		= 0x0150,		// YAAA AAA1 0101 XXXX
	DALI_COMMAND_96_111		= 0x0160,		// YAAA AAA1 0110 XXXX
	DALI_COMMAND_112_127	= 0x0170,		// YAAA AAA1 0111 XXXX
	DALI_COMMAND_128 = 0x0180,				// YAAA AAA1 1000 0000
	DALI_COMMAND_129 = 0x0181,				// YAAA AAA1 1000 0001
	
	DALI_COMMAND_144 = 0x0190,				// YAAA AAA1 1001 0000	Query status
	DALI_COMMAND_145 = 0x0191,				// YAAA AAA1 1001 0001	Query control appliance
	DALI_COMMAND_146 = 0x0192,				// YAAA AAA1 1001 0010	Query lamp-failure
	DALI_COMMAND_147 = 0x0193,				// YAAA AAA1 1001 0011	Query lamp power on source
	DALI_COMMAND_148 = 0x0194,				// YAAA AAA1 1001 0100	Query limit error
	DALI_COMMAND_149 = 0x0195,				// YAAA AAA1 1001 0101	Query reset status
	DALI_COMMAND_150 = 0x0196,				// YAAA AAA1 1001 0110	Query missing short addr
	DALI_COMMAND_151 = 0x0197,				// YAAA AAA1 1001 0111	Query version
	DALI_COMMAND_152 = 0x0198,				// YAAA AAA1 1001 1000	Query DTR storage infomation
	DALI_COMMAND_153 = 0x0199,				// YAAA AAA1 1001 1001	Query device type
	DALI_COMMAND_154 = 0x019a,				// YAAA AAA1 1001 1010	Query physical min power level
	DALI_COMMAND_155 = 0x019b,				// YAAA AAA1 1001 1011	Query power supply malfunction
	DALI_COMMAND_156 = 0x019c,				// YAAA AAA1 1001 1100	Query DTR1 storage infomation
	DALI_COMMAND_157 = 0x019d,				// YAAA AAA1 1001 1101	Query DTR2 storage infomation
	DALI_COMMAND_160 = 0x01a0,				// YAAA AAA1 1010 0000
	DALI_COMMAND_161 = 0x01a1,				// YAAA AAA1 1010 0001
	DALI_COMMAND_162 = 0x01a2,				// YAAA AAA1 1010 0010
	DALI_COMMAND_163 = 0x01a3,				// YAAA AAA1 1010 0011
	DALI_COMMAND_164 = 0x01a4,				// YAAA AAA1 1010 0100
	DALI_COMMAND_165 = 0x01a5,				// YAAA AAA1 1010 0101
#ifdef DALI_DEVICE_TYPE_6
	DALI_COMMAND_224 = 0x01e0,				// YAAA AAA1 1110 0000
	DALI_COMMAND_225 = 0x01e1,				// YAAA AAA1 1110 0001
	DALI_COMMAND_226 = 0x01e2,				// YAAA AAA1 1110 0010
	DALI_COMMAND_227 = 0x01e3,				// YAAA AAA1 1110 0011
	DALI_COMMAND_228 = 0x01e4,				// YAAA AAA1 1110 0100
	DALI_COMMAND_229 = 0x01e5,				// YAAA AAA1 1110 0101

	DALI_COMMAND_236 = 0x01ec,				// YAAA AAA1 1110 1100
	DALI_COMMAND_237 = 0x01ed,				// YAAA AAA1 1110 1101
	DALI_COMMAND_238 = 0x01ee,				// YAAA AAA1 1110 1110
	DALI_COMMAND_239 = 0x01ef,				// YAAA AAA1 1110 1111
	DALI_COMMAND_240 = 0x01f0,				// YAAA AAA1 1111 0000
	DALI_COMMAND_241 = 0x01f1,				// YAAA AAA1 1111 0001
	DALI_COMMAND_242 = 0x01f2,				// YAAA AAA1 1111 0010
	DALI_COMMAND_243 = 0x01f3,				// YAAA AAA1 1111 0011
	DALI_COMMAND_244 = 0x01f4,				// YAAA AAA1 1111 0100
	DALI_COMMAND_245 = 0x01f5,				// YAAA AAA1 1111 0101
	DALI_COMMAND_246 = 0x01f6,				// YAAA AAA1 1111 0110
	DALI_COMMAND_247 = 0x01f7,				// YAAA AAA1 1111 0111
	DALI_COMMAND_248 = 0x01f8,				// YAAA AAA1 1111 1001
	DALI_COMMAND_249 = 0x01f9,				// YAAA AAA1 1111 1010
	DALI_COMMAND_250 = 0x01fa,				// YAAA AAA1 1111 1011
	DALI_COMMAND_251 = 0x01fb,				// YAAA AAA1 1111 1100
	DALI_COMMAND_252 = 0x01fc,				// YAAA AAA1 1111 1101
	DALI_COMMAND_253 = 0x01fd,				// YAAA AAA1 1111 1110
	DALI_COMMAND_254 = 0x01ff,				// YAAA AAA1 1111 1111

	DALI_COMMAND_257 = 0xa300,				// 1010 0011 XXXX XXXX
#endif
	DALI_COMMAND_256 = 0xa100,				// 1010 0001 0000 0000	stop
	DALI_COMMAND_258 = 0xa500,				// 1010 0101 XXXX XXXX	Initialize
	DALI_COMMAND_259 = 0xa700,				// 1010 0111 0000  0000	Randomize
	DALI_COMMAND_260 = 0xa900,				// 1010 1001 0000  0000	compare
	DALI_COMMAND_261 = 0xab00,				// 1010 1011 0000  0000	exit compare

	DALI_COMMAND_264 = 0xb100,				// 1011 0001 HHHH HHHH
	DALI_COMMAND_265 = 0xb300,				// 1011 0001 MMMM MMMM
	DALI_COMMAND_266 = 0xb500,				// 1011 0001 LLLL LLLL
	DALI_COMMAND_267 = 0xb701,				// 1011 0111 0AAA AAA1	Config short address
	DALI_COMMAND_268 = 0xb901,				// 1011 1001 0AAA AAA1	Verify short address
	DALI_COMMAND_269 = 0xbb00,				// 1011 1011 0000 0000	Query short address
	DALI_COMMAND_270 = 0xbd00,				// 1011 1101 0000 0000	Physical selection
	
	DALI_COMMAND_272 = 0xc100,				// 1100 0001 XXXX XXXX	
	DALI_COMMAND_273 = 0xc300,				// 1100 0011 XXXX XXXX	
	DALI_COMMAND_274 = 0xc500,				// 1100 0101 XXXX XXXX	
	DALI_COMMAND_275 = 0xc700,				// 1100 0111 XXXX XXXX	
}DALI_COMMAND_e;

typedef enum _DALI_STATE_e_
{
	DALI_IDLE = 0,
	DALI_START,
	DALI_DATA,
	DALI_STOP,
}DALI_STATE_e;


typedef enum _DALI_DATA_LEN_e_
{
	DALI_DATA_8BIT = 8,
	DALI_DATA_16BIT = 16,
}DALI_DATA_LEN_e;

typedef enum _GPIO_LEVEL_e_
{
	GPIO_LEVEL_LOW = 0,
	GPIO_LEVEL_HIGH,
}GPIO_LEVEL_e;

typedef enum _DALI_ADDR_TYPE_e_
{
	DALI_ADDR_TYPE_SHORT = 0,
	DALI_ADDR_TYPE_GROUP,
	DALI_ADDR_TYPE_BOARDCAST,
}DALI_ADDR_TYPE_e;


/* Exported config ----------------------------------------------------------*/
//config by user
#define dali_tx_pro							HAL_TIM_DaliTx
#define dali_rx_pro							HAL_TIM_DaliRx
#define dali_rx_timeout_pro			HAL_TIM_DaliRxTimeOut
#define dali_timer_interrupt		TIM3_interrupt
#ifdef DEVICE_TYPE_STM32_HAL
#define	dali_out_level(level)		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, (GPIO_PinState)level)
#define	dali_in_level						HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5)
#endif
/* Exported variable ---------------------------------------------------------*/
/* Exported struct -----------------------------------------------------------*/
typedef struct _DALI_FIFO_DATA_s_
{
	uint8_t data[DALI_FIFO_DATA_MAX_SIZE];
	uint8_t data_len;
}DALI_FIFO_DATA_s;

typedef struct _DALI_FIFO_s_
{
	uint8_t head;
	uint8_t tail;
	uint8_t fifo_cycle;
	DALI_FIFO_DATA_s pack[105];
}DALI_FIFO_s;

typedef struct _DALI_s_
{
	uint8_t SlaveDeviceNum;
	uint8_t ShortAddr;
	uint8_t GroupAddr;
	char random_addr_assignment_step;
	char physical_addr_assignment_step;
	uint8_t timer_interrupt_open_status;
	uint8_t CurrentArcPower;
	uint8_t RelayFlag;
	bool ledNeedToOffFlag;
}DALI_s;

typedef struct _DALI_GPIO_s_
{
	void *port;
	char  pin;
	bool  level;
}DALI_GPIO_s;

typedef struct _DALI_TX_s_
{
	DALI_FUNCTION_e function_type;
	DALI_ADDR_TYPE_e addr_type;
	uint8_t tx_step;
	uint16_t tx_data;
	uint8_t wait_responce_times;
	uint8_t tx_data_len;
	uint8_t tx_pack_bit;
	bool half_bit;	//	0:send first half bit   1:send second half bit
	uint16_t command_id;
}DALI_TX_s;
extern DALI_TX_s dali_tx_s;

typedef struct _DALI_RX_s_
{
	uint8_t rx_step;
	uint16_t rx_data;
	uint8_t rx_pack_bit;
	bool half_bit;	//	0:send first half bit   1:send second half bit
	bool rx_gpio_level;
	uint8_t recv_time;
}DALI_RX_s;

typedef struct _ROLA_DALI_s_
{
	uint8_t rx_data[20];
	uint8_t rx_data_len;
}ROLA_DALI_s;

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void dali_init(void);
void dali_send_function(void);
void dali_recv_function(void);
void dali_rx_timeout_reset(void);
void receive_rola_data_pro(uint8_t* data, uint8_t data_len);
void RecvRolaPowerCMD(uint8_t power);



#endif /* _DALI__H_ */

