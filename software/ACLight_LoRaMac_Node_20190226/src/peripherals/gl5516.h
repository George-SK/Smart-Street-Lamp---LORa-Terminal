/*******************************************************************************
  * @file    GL5516.h 
  * @author  
  * @version 
  * @date    
  * @brief   
  *****************************************************************************/	
#ifndef _GL5516_H_
#define _GL5516_H_

/* Exported include ----------------------------------------------------------*/
#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
/* Exported types ------------------------------------------------------------*/
#if defined (STM32L073xx)
#include "stm32l0xx.h"
#elif defined (STM32L151xB)
#include "stm32l1xx.h"
#endif

#include "board.h"

void GL5516_init(void);
uint16_t GL5516GetVoltage(void);
float GL5516VoltageConvertToLumen(void);
	
#endif
