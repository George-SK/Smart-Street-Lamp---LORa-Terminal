/*******************************************************************************
  * @file    dali.c
  * @author  
  * @version 
  * @date    
  * @brief   
  *****************************************************************************/	
/* Exported include ------------------------------------------------------------*/
#include "dali.h"
/* Exported types -------------------------------------------------------------*/
//#define DALI_DEBUG_PRINTF
//#define DALI_CODE_PRINTF

#ifdef DALI_DEBUG_PRINTF
	#define DEBUG_PRINTF printf
#else 
	#define DEBUG_PRINTF(...)
#endif

#ifdef DALI_CODE_PRINTF
	#define DALI_PRINTF printf
#else
	#define DALI_PRINTF(...)
#endif
/* Exported constants ---------------------------------------------------------*/
/* Exported variable -----------------------------------------------------------*/
#if 1
uint8_t dali_power[][2] = {
{0, 0}, 	{1, 179},	{2, 180},	{3, 181},	{4, 182},
{5, 183},	{6, 184},	{7, 185},	{8, 186},	{9, 187},
{10,188},	{11,189},	{12,190},	{13,191},	{14,192},
{15,193},	{16,194},	{17,195},	{18,196},	{19,197},
{20,198},	{21,199},	{22,200},	{23,201},	{24,202},	
{25,203},	{26,204},	{27,205},	{28,206},	{29,207}, 
{30,208},	{31,209},	{32,210},	{33,211},	{34,212},
{35,213},	{36,214},	{37,215},	{38,216},	{39,217}, 
{40,218},	{41,219},	{42,220},	{43,221},	{44,222}, 
{45,223},	{46,224},	{47,225},	{48,226},	{49,227}, 
{50,228},	{51,229},	{52,230},	{53,231},	{54,232}, 
{55,232},	{56,233},	{57,233},	{58,234},	{59,235}, 
{60,235},	{61,236},	{62,237},	{63,237},	{64,238}, 
{65,238},	{66,239},	{67,239},	{68,240},	{69,241}, 
{70,241},	{71,242},	{72,242},	{73,243},	{74,243}, 
{75,244},	{76,244},	{77,245},	{78,245},	{79,246}, 
{80,246},	{81,247},	{82,247},	{83,248},	{73,248}, 
{85,248},	{86,249},	{87,249},	{88,250},	{89,250}, 
{90,250},	{91,251},	{92,251},	{93,252},	{94,252}, 
{95,252},	{96,253},	{97,253},	{98,253},	{99,254}, 
{100,254},
};
#else
uint8_t dali_power[][2] = {
{0, 0}, 	{1, 2},		{2, 5},		{3, 7},		{4, 10},
{5, 12},	{6, 15},	{7, 17},	{8, 20},	{9, 22},
{10,25},	{11,27},	{12,30},	{13,32},	{14,35},
{15,37},	{16,40},	{17,42},	{18,45},	{19,47},
{20,50},	{21,52},	{22,55},	{23,57},	{24,60},	
{25,62},	{26,65},	{27,67},	{28,70},	{29,72}, 
{30,75},	{31,77},	{32,80},	{33,82},	{34,85},
{35,87},	{36,90},	{37,92},	{38,95},	{39,97}, 
{40,100},	{41,102},	{42,105},	{43,107},	{44,110}, 
{45,112},	{46,115},	{47,117},	{48,120},	{49,122}, 
{50,125},	{51,127},	{52,130},	{53,132},	{54,135}, 
{55,137},	{56,140},	{57,142},	{58,145},	{59,147}, 
{60,150},	{61,152},	{62,155},	{63,157},	{64,160}, 
{65,162},	{66,165},	{67,167},	{68,170},	{69,172}, 
{70,175},	{71,177},	{72,180},	{73,182},	{74,185}, 
{75,187},	{76,190},	{77,192},	{78,195},	{79,197}, 
{80,200},	{81,202},	{82,205},	{83,207},	{73,210}, 
{85,212},	{86,215},	{87,217},	{88,220},	{89,222}, 
{90,225},	{91,230},	{92,232},	{93,235},	{94,237}, 
{95,240},	{96,242},	{97,245},	{98,248},	{99,251}, 
{100,254},
};
#endif 
/* Exported enum -------------------------------------------------------------*/
/* Exported struct -------------------------------------------------------------*/
DALI_s dali_s;
DALI_TX_s dali_tx_s;
DALI_RX_s dali_rx_s;
ROLA_DALI_s rola_dali_s;
DALI_FIFO_s DaliTxMsg;
DALI_FIFO_s DaliRxMsg;
DALI_FIFO_s LoraToDaliMsg;
DALI_FIFO_s DaliToLoraMsg;
//TimerEvent_t DaliSendTimer;
/* Exported macro -------------------------------------------------------------*/
/* Exported functions ----------------------------------------------------------*/
void dali_app_SetArcPower(uint8_t AddrField, uint8_t setting_power);
/*************************************************************
* \fn    					
* \brief 					
* \param[in] 		
* \param[out] 
* \return 
**************************************************************/
void Dali_FifoInit (DALI_FIFO_s* p, uint8_t PackNum)
{
    p->head = 0;
    p->tail = 0;
	p->fifo_cycle = PackNum;
}
/*************************************************************
* \fn    					
* \brief 					
* \param[in] 		
* \param[out] 
* \return 
**************************************************************/
char Dali_EntryFifo(DALI_FIFO_s* p, uint8_t* data, uint8_t data_len)
{
	if ((p->head+1) % p->fifo_cycle == p->tail) 			return 0;
	if (data_len > DALI_FIFO_DATA_MAX_SIZE)					return 0;
	
	memcpy(p->pack[p->head].data, data, data_len);
	p->pack[p->head].data_len = data_len;
	
	p->head++;
	p->head %= p->fifo_cycle;
	return 1;
}
/*************************************************************
* \fn    					
* \brief 					
* \param[in] 		
* \param[out] 
* \return 
**************************************************************/
char Dali_OutFifo(DALI_FIFO_s* p, uint8_t* data, uint8_t* data_len)
{
	if (p->head % p->fifo_cycle == p->tail) return 0;

	memcpy(data, p->pack[p->tail].data, p->pack[p->tail].data_len);
	*data_len = p->pack[p->tail].data_len;
	
	p->tail++;
	p->tail %= p->fifo_cycle;
	return 1;
}

/*************************************************************
* \fn    					
* \brief 	
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void dali_gpio_config(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	//dali out pin
	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	//dali in pin
	GPIO_InitStruct.Pin = GPIO_PIN_5;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);

//	HAL_NVIC_SetPriority(EXTI4_15_IRQn, 1, 0);
//	HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
	DALI_PRINTF("dali gpio config complete\r\n");
}
/*************************************************************
* \fn    					
* \brief 	
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void dali_rx_timeout_reset(void)
{
	dali_rx_s.rx_data = 0x00;
	dali_rx_s.rx_step = DALI_START;
	dali_rx_s.recv_time = 0;
	dali_rx_s.half_bit = 0;
	dali_rx_s.rx_pack_bit = 0;

//	DEBUG_PRINTF("dali rx timeout,reset rx sequence\r\n");
}

/*************************************************************
* \fn    					
* \brief 				callback in 416us interrupt	
					send 	start(1 byte) + data + stop(2 byte) 
					manchester encode
* \param[in] 			
* \param[out] 
* \return 			
**************************************************************/
void dali_tx(void)
{
	switch(dali_tx_s.tx_step) {
		case DALI_IDLE:
			break;
		
		case DALI_START:
			if (!dali_tx_s.half_bit) {
				dali_out_level(GPIO_LEVEL_LOW);
				dali_tx_s.half_bit = 1;
			} else {
				dali_out_level(GPIO_LEVEL_HIGH);
				dali_tx_s.half_bit = 0;
				dali_tx_s.tx_pack_bit--;
				dali_tx_s.tx_step = DALI_DATA;
			}
			break;
			
		case DALI_DATA:
		{
			#ifdef DALI_DEVICE_TYPE_MASTER
				uint16_t compare_number = 0x8000;
			#elif DALI_DEVICE_TYPE_SLAVE
				uint8_t compare_number = 0x80;
			#endif

			if (dali_tx_s.tx_data & compare_number) {
				if (!dali_tx_s.half_bit) {
					dali_out_level(GPIO_LEVEL_LOW);
					dali_tx_s.half_bit = 1;
				} else if (dali_tx_s.half_bit == 1){
					dali_out_level(GPIO_LEVEL_HIGH);
					dali_tx_s.half_bit = 0;
					dali_tx_s.tx_data <<= 1;
					dali_tx_s.tx_pack_bit--;
				}
			} else {
				if (!dali_tx_s.half_bit) {
					dali_out_level(GPIO_LEVEL_HIGH);
					dali_tx_s.half_bit = 1;
				} else if (dali_tx_s.half_bit == 1){
					dali_out_level(GPIO_LEVEL_LOW);
					dali_tx_s.half_bit = 0;
					dali_tx_s.tx_data <<= 1;
					dali_tx_s.tx_pack_bit--;
				}
			}	

			if (dali_tx_s.tx_pack_bit == 2) {
				dali_tx_s.tx_step = DALI_STOP;
			}
			break;
		}
		
		case DALI_STOP:
			dali_out_level(GPIO_LEVEL_HIGH);
			if (!dali_tx_s.half_bit) {
				dali_tx_s.half_bit = 1;
			} else if (dali_tx_s.half_bit == 1){
				dali_tx_s.half_bit = 0;
				dali_tx_s.tx_pack_bit--;
			}
			if (!dali_tx_s.tx_pack_bit) {
				dali_tx_s.tx_step = DALI_IDLE;
				dali_timer_interrupt(0, 1);
				dali_rx_s.rx_step = DALI_START;
//				DEBUG_PRINTF("dali send data complete\r\n");
			}
			break;
			
		default:
			break;
	}
}

/*************************************************************
* \fn    					
* \brief 							callback in timer 52us interrupt
								manchester decode
* \param[in] 			
* \param[out] 
* \return 				0: null    1:receive start 2:level change 3:level no change
**************************************************************/
char dali_rx(void)
{
	char ReturnValue = 0;
	
	switch(dali_rx_s.rx_step) {
			case DALI_IDLE:
				break;
				
			case DALI_START:
				if ((dali_in_level == GPIO_LEVEL_LOW) && (!dali_rx_s.half_bit)) {
					dali_rx_s.half_bit = 1;
					ReturnValue = 1;
				} else if ((dali_in_level == GPIO_LEVEL_HIGH) && dali_rx_s.half_bit && (!dali_rx_s.recv_time)) {
					ReturnValue = 2;
					dali_rx_s.recv_time = 1;
				}
				break;
				
			case DALI_DATA:
			{
				if (dali_in_level == GPIO_LEVEL_HIGH) {
					if (!dali_rx_s.half_bit) {
						dali_rx_s.half_bit = 1;
						dali_rx_s.rx_gpio_level = GPIO_LEVEL_HIGH;
//					} else if (dali_rx_s.rx_gpio_level) {
//						ReturnValue = 3;
					} else if ((dali_rx_s.rx_gpio_level == GPIO_LEVEL_LOW) && (!dali_rx_s.recv_time)){
						dali_rx_s.rx_data |= 1;
						dali_rx_s.recv_time = 1;
						ReturnValue = 2;
					}
				} else if (dali_in_level == GPIO_LEVEL_LOW) {
					if (!dali_rx_s.half_bit) {
						dali_rx_s.half_bit = 1;
						dali_rx_s.rx_gpio_level = GPIO_LEVEL_LOW;
//					} else if (!dali_rx_s.rx_gpio_level){
//						ReturnValue = 3;
					} else if ((dali_rx_s.rx_gpio_level == GPIO_LEVEL_HIGH) && (!dali_rx_s.recv_time)) {
						dali_rx_s.recv_time = 1;
						ReturnValue = 2;
					}
				}
				break;
			}
			
			case DALI_STOP:
				break;
			default:
				break;
	}

	if (dali_rx_s.recv_time) {
		if (++dali_rx_s.recv_time >= DALI_PULSE_WIDTH/DALI_TIMER_INTERRUPT_INTERVAL + 2) {
			dali_rx_s.recv_time = 0;
			dali_rx_s.half_bit = 0;
			dali_rx_s.rx_pack_bit++;
			ReturnValue = 2;

			if (dali_rx_s.rx_step == DALI_START) {
				dali_rx_s.rx_step = DALI_DATA;
			} else {
			#ifdef DALI_DEVICE_TYPE_MASTER
				if (dali_rx_s.rx_pack_bit >= DALI_DATA_8BIT+1) {
					dali_rx_s.rx_step = DALI_IDLE;
					dali_timer_interrupt(0, 1);
					ReturnValue = 3;
				} else {
					dali_rx_s.rx_data <<= 1;
				}
			#elif  DALI_DEVICE_TYPE_SLAVE
				if (dali_rx_s.rx_pack_bit >= DALI_DATA_16BIT+1) {
					dali_rx_s.rx_step = DALI_IDLE;
					dali_timer_interrupt(0, 1);
					ReturnValue = 3;
				} else {
					dali_rx_s.rx_data <<= 1;
				}
			#endif
			}
				
		}
	}
	
	return ReturnValue;
}

/*************************************************************
* \fn    					
* \brief 			//data will send by timer 416us timeout interrupt
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void dali_SendCommand(uint16_t command)
{
	dali_tx_s.tx_data = command;
	
	#ifdef DALI_DEVICE_TYPE_MASTER
		dali_tx_s.tx_data_len = DALI_DATA_16BIT;
		dali_tx_s.tx_pack_bit = DALI_DATA_16BIT+3;		// + 1 start + 2 stop bit
	#else
		dali_tx_s.tx_data_len = DALI_DATA_8BIT;
		dali_tx_s.tx_pack_bit = DALI_DATA_8BIT+3;
	#endif

	dali_tx_s.tx_step = DALI_START;

	dali_timer_interrupt(1, 0);
}
/*************************************************************
* \fn    					
* \brief 	
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void GetDeviceAddr(uint8_t addr_type, uint8_t* addr)
{
	bool y_addr = 0;
	uint8_t command_addr = 0;

	if (addr_type == DALI_ADDR_TYPE_SHORT) {
		y_addr = 0;
		command_addr = (y_addr << 7) | (dali_s.ShortAddr << 1);
	} else if (addr_type == DALI_ADDR_TYPE_GROUP) {
		y_addr = 1;
		command_addr = (y_addr << 7) | (dali_s.GroupAddr << 1); 
	} else {
		command_addr = 0xFE;
	}

	*addr = command_addr;
}
/*************************************************************
* \fn    					
* \brief 	
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void dali_PutTxCMDToFIFO(DALI_COMMAND_e command, uint8_t AddrField, uint8_t data, uint16_t command_id, uint8_t wait_responce_times)
{
	uint8_t temp[5] = {0};
	temp[0] = ((command>>8)&0xff) | AddrField;
	temp[1] = (command&0xff) | data;
	temp[2] = (command_id>>8)&0xff;
	temp[3] = command_id&0xff;
	temp[4] = wait_responce_times;
	
	if(Dali_EntryFifo(&DaliTxMsg, temp, 5) == 0) {
		DALI_PRINTF("Put dali command to FIFO fail\r\n");
	} else {
//		DEBUG_PRINTF("Put dali command %d to FIFO success\r\n", command_id);
	}
}
/*************************************************************
* \fn    					
* \brief 	
* \param[in] 			
* \param[out] 
* \return 				0: get a valid pack
**************************************************************/
char dali_GetTxCMDFromFIFO(uint16_t* CMD)
{	
	uint8_t ReturnValue = 1;
	uint8_t data[5] = {0};
	uint8_t data_len = 0;
	uint16_t command = 0;
	
	if(Dali_OutFifo(&DaliTxMsg, data, &data_len) == 1) {
		command = (data[0]<<8) | data[1];
		dali_tx_s.command_id = (data[2]<<8) | data[3];
		dali_tx_s.wait_responce_times = data[4];
		if ((dali_tx_s.command_id==350) && dali_s.RelayFlag && (!data[1])) {
			dali_s.ledNeedToOffFlag = 1;
			DEBUG_PRINTF("Get turn off command from fifo, data[1]:0x%x\r\n", data[1]);
		}

		*CMD = command;
		ReturnValue = 0;
	}

	return ReturnValue;
}

/*************************************************************
* \fn    					
* \brief 	
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void dali_PutRxDataToFIFO(uint16_t command_id, uint8_t data)
{
	uint8_t temp[3] = {0};
	
	temp[0] = (command_id>>8)&0xff;
	temp[1] = command_id&0xff;
	temp[2] = data;
	
	if(Dali_EntryFifo(&DaliRxMsg, temp, 3) == 0) {
		DALI_PRINTF("Put dali Rx data to FIFO fail\r\n");
	} else {
		DEBUG_PRINTF("Put dali Rx data to FIFO success, command id:%d\r\n", command_id);
	}
}
/*************************************************************
* \fn    					
* \brief 	
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
char dali_GetRxDataFromFIFO(uint16_t* CMD, uint8_t* RxData)
{	
	uint8_t ReturnValue = 1;
	
	uint8_t data[5] = {0};
	uint8_t data_len = 0;
	uint16_t command = 0;
	
	if(Dali_OutFifo(&DaliRxMsg, data, &data_len) == 1) {
		command = (data[0]<<8) | data[1];
		*CMD = command;
		*RxData = data[2];
	
		DEBUG_PRINTF("read a dali rx data success\r\n");
		ReturnValue = 0;
	}

	return ReturnValue;
}
/*************************************************************
* \fn    					
* \brief
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void Dali_QueryStatus(uint8_t AddrField)
{
	dali_PutTxCMDToFIFO(DALI_COMMAND_144, AddrField, 0, 144, 1);
}
/*************************************************************
* \fn    					
* \brief
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void Dali_QueryShortAddr(void)
{
	dali_PutTxCMDToFIFO(DALI_COMMAND_269, NULL, 0, 269, 1);
}
/*************************************************************
* \fn    					
* \brief
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void Dali_VerifyShortAddr(uint8_t addr)
{
	dali_PutTxCMDToFIFO(DALI_COMMAND_268, NULL, addr<<1, 268, 1);
}
/*************************************************************
* \fn    					
* \brief
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void Dali_ReConfig(uint8_t AddrField, uint8_t responce_request)
{
	dali_PutTxCMDToFIFO(DALI_COMMAND_32, AddrField, 0x00, 32, responce_request);
}
/*************************************************************
* \fn    					
* \brief
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void Dali_QueryVersion(uint8_t AddrField)
{
	dali_PutTxCMDToFIFO(DALI_COMMAND_151, AddrField, 0, 151, 1);
}

/*************************************************************
* \fn    					
* \brief 	
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
char dali_RandomAddrChkNextBit(int8_t bits, uint32_t* Addr, uint8_t num)
{
	char ReturnValue = 1;
	
	if (bits<0) {
		dali_s.ShortAddr = num;
		dali_PutTxCMDToFIFO(DALI_COMMAND_264, NULL, (*Addr>>16)&0xff, 264, 0);
		dali_PutTxCMDToFIFO(DALI_COMMAND_265, NULL, (*Addr>>8)&0xff, 265, 0);
		dali_PutTxCMDToFIFO(DALI_COMMAND_266, NULL, *Addr&0xff, 266, 0);
		dali_PutTxCMDToFIFO(DALI_COMMAND_260, NULL, 0x00, 260, 0);
		
		dali_PutTxCMDToFIFO(DALI_COMMAND_267, NULL, 0xff, 267, 0);			//delete short addr
		dali_PutTxCMDToFIFO(DALI_COMMAND_267, NULL, dali_s.ShortAddr<<1, 267, 0);
		dali_PutTxCMDToFIFO(DALI_COMMAND_268, NULL, dali_s.ShortAddr<<1, 268, 1);
		Dali_QueryShortAddr();
		DEBUG_PRINTF("check random addr over, SearchAddr = 0x%x, dali_s.ShortAddr = %d, \
			bits = %d\r\n", *Addr, dali_s.ShortAddr, bits);
//		dali_app_SetArcPower(0xfe, 0);
//		DALI_PRINTF("Close all dali LED\r\n");
		ReturnValue = 0;
	} else {
		(*Addr) &= ~(1<<bits);
		dali_PutTxCMDToFIFO(DALI_COMMAND_264, NULL, (*Addr>>16)&0xff, 264, 0);
		dali_PutTxCMDToFIFO(DALI_COMMAND_265, NULL, (*Addr>>8)&0xff, 265, 0);
		dali_PutTxCMDToFIFO(DALI_COMMAND_266, NULL, *Addr&0xff, 266, 0);
		dali_PutTxCMDToFIFO(DALI_COMMAND_260, NULL, 0x00, 260, 2);
		DEBUG_PRINTF("check bit = %d\r\n", bits);
	}
	return ReturnValue;
}
/*************************************************************
* \fn    					
* \brief 				reference to GB/T XXXXX.102¡ª201X/IEC 62386-102£º2009  page 111	
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void dali_random_address_assignment(void)
{
	char ret = 1;
	uint16_t command_id = 0;
	uint8_t RxData = 0;
	static uint8_t device_num = 0;
	static int8_t CheckAddrBit = 0;
	static uint32_t SearchAddr = 0;

	switch(dali_s.random_addr_assignment_step) {
		case 0:
			
			break;
			
		case 1:
			device_num = 0;										
			CheckAddrBit = 24;
			SearchAddr = 0x00ffffff;

			Dali_ReConfig(0xfe, 0);
			Dali_ReConfig(0xfe, 1);
			dali_PutTxCMDToFIFO(DALI_COMMAND_257, NULL, 0xff, 257, 0);
			dali_PutTxCMDToFIFO(DALI_COMMAND_128, 0xfe, 0x00, 128, 0);
			dali_PutTxCMDToFIFO(DALI_COMMAND_128, 0xfe, 0x00, 128, 0);
			dali_PutTxCMDToFIFO(DALI_COMMAND_256, NULL, 0x00, 256, 0);
			
			dali_PutTxCMDToFIFO(DALI_COMMAND_258, NULL, 0x00, 258, 0);
			dali_PutTxCMDToFIFO(DALI_COMMAND_258, NULL, 0x00, 258, 1);
			dali_PutTxCMDToFIFO(DALI_COMMAND_259, NULL, 0x00, 259, 0);
			dali_PutTxCMDToFIFO(DALI_COMMAND_259, NULL, 0x00, 259, 1);
			dali_PutTxCMDToFIFO(DALI_COMMAND_264, NULL, (SearchAddr>>16)&0xff, 264, 0);
			dali_PutTxCMDToFIFO(DALI_COMMAND_265, NULL, (SearchAddr>>8)&0xff, 265, 0);
			dali_PutTxCMDToFIFO(DALI_COMMAND_266, NULL, SearchAddr&0xff, 266, 0);
			dali_PutTxCMDToFIFO(DALI_COMMAND_260, NULL, 0x00, 260, 2);
			dali_s.random_addr_assignment_step++;
			break;

		case 2:
			ret = dali_GetRxDataFromFIFO(&command_id, &RxData);
			if (!ret) {
				if (command_id == 260) {
					if (RxData == DALI_RESPONCE_YES) {
						DEBUG_PRINTF("Recv responce yes\r\n");
						CheckAddrBit--;
						ret = dali_RandomAddrChkNextBit(CheckAddrBit, &SearchAddr, device_num);
						if (!ret) {
							CheckAddrBit = 0;
						}
					} else if (RxData == DALI_RESPONCE_NO) {
						DEBUG_PRINTF("Recv responce no\r\n");
						if (SearchAddr == 0xffffff) {
							dali_s.random_addr_assignment_step++;
						} else {
							SearchAddr |= 1<<CheckAddrBit;
							CheckAddrBit--;
							ret = dali_RandomAddrChkNextBit(CheckAddrBit, &SearchAddr, device_num);
							if (!ret) {
								CheckAddrBit = 0;
						}
						}
					}
				} else if (command_id == 268) {
					if (RxData == DALI_RESPONCE_YES) {
						dali_PutTxCMDToFIFO(DALI_COMMAND_261, NULL, 0x00, 261, 0);
						device_num++;	
					} else {
						DEBUG_PRINTF("Device %d verify short addr error\r\n", device_num);
					}
					CheckAddrBit = 24;
					SearchAddr = 0x00ffffff;

					dali_PutTxCMDToFIFO(DALI_COMMAND_264, NULL, (SearchAddr>>16)&0xff, 264, 0);
					dali_PutTxCMDToFIFO(DALI_COMMAND_265, NULL, (SearchAddr>>8)&0xff, 265, 0);
					dali_PutTxCMDToFIFO(DALI_COMMAND_266, NULL, SearchAddr&0xff, 266, 0);
					dali_PutTxCMDToFIFO(DALI_COMMAND_260, NULL, 0x00, 260, 2);
				}
			}
			
			break;
			
		case 3:
			dali_PutTxCMDToFIFO(DALI_COMMAND_256, NULL, 0x00, 256, 0);
			DEBUG_PRINTF("random addr assignment complete, exit\r\n");
			dali_tx_s.addr_type = DALI_ADDR_TYPE_SHORT;
			device_num = 0;
			dali_s.random_addr_assignment_step = 0;
			break;
			
		default:
			
			break;
	}
}


/*************************************************************
* \fn    					
* \brief 	
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
/*
void dali_physical_address_assignment(void)
{
	DALI_PRINTF("start dali_physical_address_assignment\r\n");
	dali_PutTxCMDToFIFO(DALI_COMMAND_258, NULL, 0x00, 258, 0);
	dali_PutTxCMDToFIFO(DALI_COMMAND_258, NULL, 0x00, 258, 0);
	dali_PutTxCMDToFIFO(DALI_COMMAND_270, NULL, 0x00, 270, 0);
	dali_PutTxCMDToFIFO(DALI_COMMAND_269, NULL, 0x00, 269, 0);
	dali_PutTxCMDToFIFO(DALI_COMMAND_267, NULL, 0x7F<<1, 267, 0);
	dali_PutTxCMDToFIFO(DALI_COMMAND_267, NULL, 0x24<<1, 267, 0);
	dali_PutTxCMDToFIFO(DALI_COMMAND_5, 0x00, 0x00, 5, 0);
	dali_PutTxCMDToFIFO(DALI_COMMAND_6, 0x00, 0x00, 6, 0);
	dali_PutTxCMDToFIFO(DALI_COMMAND_256, NULL, 0x00, 256, 0);
}
*/
/*************************************************************
* \fn    					
* \brief				page8 table 1
					20% -------- 196
					40% -------- 221
					60% -------- 235
					80% -------- 246
					100% ------- 254
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void Dali_SetArcPower(uint8_t AddrField, uint8_t power)
{
	uint8_t PowerValue = 0;
	
	for (uint8_t i=0; i<sizeof(dali_power)/sizeof(dali_power[0]); i++) {
		if (power == dali_power[i][0]) {
			PowerValue = dali_power[i][1];
			dali_PutTxCMDToFIFO(DALI_COMMAND_N, AddrField, PowerValue, 350, 0);
//			DEBUG_PRINTF("PowerValue = %d\r\n", PowerValue);
			break;
		}
	}
}
/*************************************************************
* \fn    					
* \brief 	
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void dali_app_SetArcPower(uint8_t AddrField, uint8_t setting_power)
{
	uint8_t i = 0;

	if (dali_s.CurrentArcPower+1 < setting_power) {
		for(i=dali_s.CurrentArcPower+1; i<setting_power; i++) {
			Dali_SetArcPower(AddrField, i);
		}	
	} else if (dali_s.CurrentArcPower > setting_power) {
		for(i=dali_s.CurrentArcPower; i>setting_power; i--) {
			Dali_SetArcPower(AddrField, i);
		}
	} 

	dali_s.CurrentArcPower = setting_power;
	Dali_SetArcPower(AddrField, dali_s.CurrentArcPower);
}
/*************************************************************
* \fn    					
* \brief 	
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void slave_response_data_pro(uint8_t data)
{
	switch(dali_tx_s.command_id) {
		case 144:
			DALI_PRINTF("command 144 responce status:0x%x\r\n", data);
			break;

		case 151:
			DALI_PRINTF("command 151 responce version:0x%x\r\n", data);
			break;

		case 260:
			if (!dali_tx_s.wait_responce_times)		break;				//the lastest result of random addr compare is invalid
			dali_PutRxDataToFIFO(260, data);
			DALI_PRINTF("command 260 responce compare result:0x%x\r\n", data);
			break;

		case 268:
			dali_PutRxDataToFIFO(268, data);
			DALI_PRINTF("command 268 responce compare result:0x%x\r\n", data);
			break;
			
		case 269:
			DALI_PRINTF("command 269 responce short addr:0x%x\r\n", data);
			break;
		
		default:
			break;
	}

	dali_tx_s.wait_responce_times = 0;
}
//================================================================

/*************************************************************
* \fn    					
* \brief 	
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void dali_init(void)
{
	DALI_PRINTF("dali version:%s\r\n", DALI_VERSION);
	
	memset(&dali_s, 0, sizeof(dali_s));
	memset(&dali_tx_s, 0, sizeof(dali_tx_s));
	memset(&dali_rx_s, 0, sizeof(dali_rx_s));

	Dali_FifoInit(&DaliTxMsg, 106);
	Dali_FifoInit(&DaliRxMsg, 10);
//	CreateQueue(&LoraToDaliMsg, MESSAGE_MAX_SIZE);
//	CreateQueue(&DaliToLoraMsg, MESSAGE_MAX_SIZE);

	dali_gpio_config();
	dali_tx_pro = dali_tx;
	dali_rx_pro = dali_rx;
	dali_rx_timeout_pro = dali_rx_timeout_reset;
//	TimerInit(&DaliSendTimer, dali_send_function);
//	TimerSetValue(&DaliSendTimer, 1);
//	TimerStart(&DaliSendTimer);

		dali_s.RelayFlag = 1;
		dali_s.CurrentArcPower = 100;
//	SetRelay(1);				
//	dali_app_SetArcPower(0xfe, 100);
//	DALI_PRINTF("Open all dali LEDs\r\n");
//	dali_PutTxCMDToFIFO(DALI_COMMAND_257, NULL, 0x00, 257, 0);				// set min power level to DTR when power on
//	dali_PutTxCMDToFIFO(DALI_COMMAND_45, 0xfe, 0x00, 45, 0);	

	for (uint8_t i=0; i<2; i++) {
		Dali_QueryVersion(0xfe);
	}

//	dali_s.random_addr_assignment_step = 1;

	DALI_PRINTF("dali init complete\r\n");
}
/*************************************************************
* \fn    					
* \brief 	
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void dali_send_function(void)
{
	char ret = 1;
	static uint16_t command = 0;
	static uint32_t tick = 0;
	uint32_t timeout_tick = 40;

//	TimerSetValue(&DaliSendTimer, 1);
//	TimerStart(&DaliSendTimer);

	if ((dali_rx_s.rx_step != DALI_IDLE) && (dali_rx_s.rx_step != DALI_START)) {
		tick = 0;
		return;
	}

	if ((dali_tx_s.command_id==32) && dali_tx_s.wait_responce_times) {			// 300ms+wait time ,may be is 300ms +710ms
		timeout_tick = 2500;
	}
	
	if (((dali_tx_s.command_id==258)||(dali_tx_s.command_id==259))
		&& dali_tx_s.wait_responce_times) {			// 300ms+wait time ,may be is 300ms +710ms
		timeout_tick = 2500;
	}

	if (dali_s.ledNeedToOffFlag) {
		timeout_tick = 550;
	}
	
//	HAL_GetTick();

	if (HAL_GetTick()-tick < timeout_tick) return;			// timeout_tick=3000, less than 100ms in normal mode
	
	tick = HAL_GetTick();
	
	dali_rx_s.rx_step = DALI_IDLE;

	dali_random_address_assignment();

	if (dali_tx_s.wait_responce_times) {		//no responce, send previous command again
		dali_tx_s.wait_responce_times--;
		if ((dali_tx_s.command_id == 260) && (!dali_tx_s.wait_responce_times)) {
			DALI_PRINTF("command %d no responce, simulate device's responce\r\n", dali_tx_s.command_id);
			dali_PutRxDataToFIFO(260, 0x00);	// simulate device's responce 
		} else if ((dali_tx_s.command_id == 32) 
				|| (dali_tx_s.command_id == 258)
				|| (dali_tx_s.command_id == 259)) {
			//do nothing
		} else {
			dali_SendCommand(command);
			DALI_PRINTF("command %d no responce, dali sending the provious command\r\n", dali_tx_s.command_id);
		}
	} else if (dali_s.ledNeedToOffFlag) {
		dali_s.ledNeedToOffFlag = 0;
		dali_s.RelayFlag = 0;
		SetRelay(0);		// close relay for save power when turn off led
		DALI_PRINTF("Dali turn off led complete, close relay\r\n");
	} else {
//		Dali_QueryStatus(0xFE);
		ret = dali_GetTxCMDFromFIFO(&command);
		if (!ret) {
			dali_SendCommand(command);
//			DALI_PRINTF("dali sending a new command\r\n");
		}
	}
}
/*************************************************************
* \fn    					
* \brief 	
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void dali_recv_function(void)
{
//	static uint32_t tick = 0;
	char ret = 0;
	
	if (dali_rx_s.rx_pack_bit && (dali_rx_s.rx_step == DALI_IDLE)) {
		#ifdef DALI_DEVICE_TYPE_MASTER
			slave_response_data_pro((uint8_t)dali_rx_s.rx_data);
		#else
		
		#endif
		
		dali_rx_s.rx_pack_bit = 0;
		
		DEBUG_PRINTF("dali_rx_s.rx_data = 0x%x\r\n", dali_rx_s.rx_data);
		dali_rx_s.rx_data = 0;
	} else {
//		if (++tick < 100000) return;
	
//		tick = 0;
		
//		DEBUG_PRINTF("dali_rx_s.rx_pack_bit = %d\r\n", dali_rx_s.rx_pack_bit);	
//		DEBUG_PRINTF("dali_rx_s.rx_data = %d\r\n", dali_rx_s.rx_data);
	}
		
	if ((dali_rx_s.rx_step == DALI_START) && (!dali_rx_s.half_bit)) {
		ret = dali_rx();
		if (ret == 1) {
			dali_timer_interrupt(1, 1);
			DALI_PRINTF("received dali Rx signal\r\n");
		}
		
	}
}


/*************************************************************
* \fn    					
* \brief 				data will process in dali_send_function poll function
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void receive_rola_data_pro(uint8_t* data, uint8_t data_len)
{

	uint8_t DeviceAddr = 0;
	
	dali_tx_s.addr_type = DALI_ADDR_TYPE_BOARDCAST;
	GetDeviceAddr(dali_tx_s.addr_type, &DeviceAddr);

	dali_tx_s.function_type = (DALI_FUNCTION_e) data[0];

	switch(dali_tx_s.function_type) {
		case DALI_FUNCTION_RANDOM_ADDR_ASSIGNMENT:
			dali_s.random_addr_assignment_step = 1;
			break;
			
		case DALI_FUNCTION_SET_POWER:
		{
			uint8_t power = data[1];
			
			DALI_PRINTF("Recv Rola set power CMD,Device addr:0x%x, power:%d, open relay\r\n", DeviceAddr, power);
			if (!dali_s.RelayFlag) {
				dali_s.RelayFlag = 1;
				SetRelay(1);
			}
			dali_app_SetArcPower(DeviceAddr, power);
			break;
		}

		case DALI_FUNCTION_QUERY_STATUS:
			DALI_PRINTF("Recv Rola Query status CMD\r\n");
			Dali_QueryStatus(DeviceAddr);
			break;

		case DALI_FUNCTION_QUERY_VERSION:
			DALI_PRINTF("Recv Rola Query version CMD, DeviceAddr:0x%x\r\n", DeviceAddr);
			Dali_QueryVersion(DeviceAddr);
			break;
			
		default:
			break;
	}

	
}
/*************************************************************
* \fn    					
* \brief 				data will process in dali_send_function poll function
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void RecvRolaPowerCMD(uint8_t power)
{
	DALI_PRINTF("Rola set dali power:%d\r\n", power);
	if ((!dali_s.RelayFlag) && (power||dali_s.CurrentArcPower)) {
		dali_s.CurrentArcPower = 100;
		dali_s.RelayFlag = 1;
		SetRelay(1);
		DALI_PRINTF("open relay\r\n");
	}
	
	if (!power) {
		dali_PutTxCMDToFIFO(DALI_COMMAND_257, NULL, 0x00, 257, 0);				// set min power level to DTR
		dali_PutTxCMDToFIFO(DALI_COMMAND_45, 0xfe, 0x00, 45, 0);	
	}
	dali_app_SetArcPower(0xfe, power);
}

/*************************************************************
* \fn    					
* \brief 				data will process in dali_send_function poll function
* \param[in] 			
* \param[out] 
* \return 
**************************************************************/
void timing_running_function(void)
{
	
}



