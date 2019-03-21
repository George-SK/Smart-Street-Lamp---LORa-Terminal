#include "rn820x.h"
#include "board.h"
#include "math.h"
#include "internal-eeprom.h"
#include "uart-board.h"

#define HFCONST			0x02
#define PFCNT				0x20
#define CURRENT_A		0x22
#define	CURRENT_B		0x23
#define	VOLTAGE			0x24
#define POWER_A			0x26
#define POWER_B			0x27
#define POWER_Q			0x28
#define ENERGY_P		0x29

#define Un			220.0	//220V
#define Ib			0.5	//0.5A
#define EC			1000
#define Vu			0.1358
#define Vi			0.032
#define Ireg		0x020ed0//0x01BD65	//AC14:0x020a66 0x120547 0.911 AC8:0x021e18 0x120b0d 0.876 AC7:0x020e60 0x120bf4 0.901 AC2->AC24:0x020ed0 0x120300 0.953
#define Ureg		0x120300//0x139A41	//AC10:0x02094d 0x120a5f 0.910 AC11:0x022224 0x121163 0.868 AC13:0x02273a 0x120b42 0.861 AC15:0x020cc5 0x122212 0.899 AC-xx:0x020f41 0x120b7c 0.9
/*Un=238V  Ib=0.424A
AC19: 0x01CBA4 0x1395A3 1.138
AC17: 0x01C68A 0x1394A3 1.153
AC18: 0x01C4D4 0x139A41 1.154
*/
#define KpGain	0.953//1.173
#define Kv			(Un*1.0/Ureg)
#define Ki			(Ib*1.0/Ireg)	//AC2: Ireg:0x0660E8 Vreg:0x1220B7	AC6: Ireg:0x067BAF Vreg:0x120BFC  AC7: Ireg:0x062505 Vreg:0x120CB0. Old AC7:Vreg:0x1209d6 Ireg:0x062536
#define HFConst	(uint16_t)((16.1079*Vu*Vi*1e11 )/(Un*Ib*EC))
#define Kp			(3.22155e12/(pow(2,32)*HFConst*EC)*KpGain)

//Gain calibration
#define S_test		((Ureg*1.0/pow(2,23))*(Ireg*1.0/pow(2,23)))
#define S_standard	(Un*Ib*1.0/Kp/pow(2,31))
#define GAIN_ERR	((S_test-S_standard)/S_standard)
#define PGAIN			((-GAIN_ERR)*1.0/(1+GAIN_ERR))

#define MAX_TX_TIMEOUT	100
#define MAX_RX_SIZE			20
#define CELIBRATE_PAYLOAD_LEN	72

#define UART2_TX                                     PA_2
#define UART2_RX                                     PA_3

static UART_HandleTypeDef Uart2Handle;

static Gpio_t TxGpio, RxGpio;
//RN820x_t RN820xInfo;
CelibratePro_t CelibrateProInfo;

bool Calibration_flag = false; //George:20180723:Check for calibration flag

void WriteKwhToEeprom(uint32_t *data);
uint32_t ReadKwhFromEeprom(void);
void EraseKwhInFlash(void);

void Rn820xUart(void)
{
	__HAL_RCC_USART2_FORCE_RESET( );
	__HAL_RCC_USART2_RELEASE_RESET( );
	__HAL_RCC_USART2_CLK_ENABLE( );

#if defined (STM32L073xx) || defined (STM32L072xx)
	GpioInit( &TxGpio, UART2_TX, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF4_USART2 );
	GpioInit( &RxGpio, UART2_RX, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF4_USART2 );
#else
	GpioInit( &TxGpio, UART2_TX, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART2 );
	GpioInit( &RxGpio, UART2_RX, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART2 );
#endif
	
	Uart2Handle.Instance        = USART2;

  Uart2Handle.Init.BaudRate   = 4800;
  Uart2Handle.Init.WordLength = UART_WORDLENGTH_9B;
  Uart2Handle.Init.StopBits   = UART_STOPBITS_1;
  Uart2Handle.Init.Parity     = UART_PARITY_EVEN;
  Uart2Handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
  Uart2Handle.Init.Mode       = UART_MODE_TX_RX;
	
  if(HAL_UART_DeInit(&Uart2Handle) != HAL_OK)
  {
    
  }
  
  if(HAL_UART_Init(&Uart2Handle) != HAL_OK)
  {
    
  }
}

bool SendCmd(const uint8_t *cmdBuf, uint16_t bufLen)
{	
	uint8_t CheckSum = 0, TxBuf[20], TxLen;
	
	DelayMs(1);	//Modified by Steven in 2018/01/24. Old: DelayMs(100);
	
	if(bufLen < 2 || bufLen >= 20)
		return false;
	
	for(uint8_t i = 0; i < bufLen; i++)
		CheckSum += cmdBuf[i];
	
	CheckSum = (uint8_t)(~CheckSum);
	
	memcpy1(TxBuf, cmdBuf, bufLen);
	TxBuf[bufLen] = CheckSum;
	TxLen = bufLen + 1;
	//Log show.
	/*Uart1.PrintfLog("Tx %d bytes to Rn820x\r\n", TxLen);
	for(uint8_t i = 0; i < TxLen; i++)
		Uart1.PrintfLog("%X ", TxBuf[i]);
	Uart1.PrintfLog("\r\n");*/
	
	if(HAL_UART_Transmit(&Uart2Handle, TxBuf, TxLen, MAX_TX_TIMEOUT) == HAL_OK)
	{
		return true;
	}
	
	return false;
}

bool ReadRegValue(uint8_t reg, uint8_t *data, uint8_t *len)
{
	if(HAL_UART_Transmit(&Uart2Handle, &reg, 1, MAX_TX_TIMEOUT) == HAL_OK)
	{
		HAL_UART_Receive(&Uart2Handle, data, MAX_RX_SIZE, 3);	//Modified by Steven in 2018/01/25. Old:300
		
		if((MAX_RX_SIZE-Uart2Handle.RxXferCount-1) <= 1)
			return false;
		
		*len = MAX_RX_SIZE-Uart2Handle.RxXferCount-1;		
		
		if(CheckData(reg, data, *len))
		{
			*len = (*len)-1;
			return true;
		}
		
		return false;
	}
	
	return false;
}

bool CheckData(uint8_t cmd, uint8_t *data, uint8_t len)
{
	uint8_t CheckSum = 0;
	
	if(len < 2)
		return false;
	
	for(uint8_t i = 0; i < len-1; i++)
		CheckSum += data[i];
	
	CheckSum += cmd;
	
	if(CheckSum == (uint8_t)(~data[len-1]))
		return true;
	
	return false;
}

static void swap(int32_t *a, int32_t *b)  
{  
	int32_t temp; 
	
	temp = *a;  
	*a = *b;  
	*b = temp;  
}  

static void BubbleSort(int32_t a[],uint8_t len)    
{  
	uint8_t i,j;
	
	for(i = 0; i < len; i++)  
	{  
		for(j = i+1; j < len; j++)  
		{  
			if(a[i] > a[j])  
				swap(&a[i], &a[j]);  
		}  
	}  
}  

static uint32_t Esum = 0, Etmp = 0;
static uint16_t EtmpM = 0;	

bool GetRn820xParam(RN820x_t *RN820xParam)
{
	uint8_t RxData[20], len;
	uint32_t Itmp, Vtmp;	
	int32_t ReadValueTmp[3] = {0, 0, 0};
	uint8_t ReadCount = 0;
	uint8_t ReadFailCount = 0;
	
	int Ptmp;
	
	ReadCount = 0;
	
	for(uint8_t RepeatCount = 0; RepeatCount < 3; RepeatCount++)
	{
		if(ReadRegValue(CURRENT_A, RxData, &len))
		{
			ReadValueTmp[RepeatCount] = (RxData[0]<<16)|(RxData[1]<<8)|RxData[2];
			ReadCount++;
		}
		else
		{
			ReadFailCount++;
			if(ReadFailCount >= 2)
			{
				//SaveRn820xParam(RN820xParam);
				RN820xInfo.kWhInFlash = RN820xInfo.kWh;
				ReadFailCount = 0;
				PRINTF("Read failed and reset Rn820x!\r\n");				
				Rn820xInit();
				
				if(ReadRegValue(CURRENT_A, RxData, &len))
				{
					RN820xParam->current = ((RxData[0]<<16)|(RxData[1]<<8)|RxData[2])*CelibrateProInfo.Kia*1000;
				}
			}
			
		}
	}
	
	if(ReadCount >= 2)
	{
		BubbleSort(ReadValueTmp, 3);
		Itmp = ReadValueTmp[1];
		RN820xParam->current = Itmp*CelibrateProInfo.Kia*1000;	//mA	
	}	
	
	ReadValueTmp[0] = 0;
	ReadValueTmp[1] = 0;
	ReadValueTmp[2] = 0;	
	ReadCount = 0;
	
	for(uint8_t RepeatCount = 0; RepeatCount < 3; RepeatCount++)
	{
		if(ReadRegValue(VOLTAGE, RxData, &len))
		{
			ReadValueTmp[RepeatCount] = (RxData[0]<<16)|(RxData[1]<<8)|RxData[2];
			ReadCount++;						
		}
	}
	
	if(ReadCount >= 2)
	{
		BubbleSort(ReadValueTmp, 3);
		Vtmp = ReadValueTmp[1];
		RN820xParam->voltage = Vtmp*CelibrateProInfo.Ku;	//V
	}	
	
	ReadValueTmp[0] = 0;
	ReadValueTmp[1] = 0;
	ReadValueTmp[2] = 0;	
	ReadCount = 0;
	
	for(uint8_t RepeatCount = 0; RepeatCount < 3; RepeatCount++)
	{
		if(ReadRegValue(POWER_A, RxData, &len))
		{
			ReadValueTmp[RepeatCount] = (RxData[0]<<24)|(RxData[1]<<16)|(RxData[2]<<8)|RxData[3];
			ReadCount++;						
		}
	}
	
	if(ReadCount >= 2)
	{
		BubbleSort(ReadValueTmp, 3);
		Ptmp = ReadValueTmp[1];
		
		if(Ptmp < 0)
			Ptmp = -Ptmp;
		
		RN820xParam->power = Ptmp*CelibrateProInfo.Kpp;	//w		
	}	
	
	ReadValueTmp[0] = 0;
	ReadValueTmp[1] = 0;
	ReadValueTmp[2] = 0;
	ReadCount = 0;
	
	for(uint8_t RepeatCount = 0; RepeatCount < 3; RepeatCount++)
	{
		if(ReadRegValue(ENERGY_P, RxData, &len))
		{
			ReadValueTmp[RepeatCount] = (RxData[0]<<16)|(RxData[1]<<8)|RxData[2];
			ReadCount++;						
		}
	}
	
	if(ReadCount >= 2)
	{
		BubbleSort(ReadValueTmp, 3);
		if(Etmp > ReadValueTmp[1])	//Overflow.
			EtmpM++;
		
		Etmp = ReadValueTmp[1];
		Esum = EtmpM*0xffffff + Etmp;
		RN820xParam->kWh = RN820xInfo.kWhInFlash + (Esum*1.0/CelibrateProInfo.Ec);	//kWh
		SaveRn820xParam(RN820xParam);
	}
	
	if(RN820xParam->current == 0 || RN820xParam->voltage == 0)
    RN820xParam->powerFactor = 0;
  else
    RN820xParam->powerFactor = RN820xParam->power/(RN820xParam->current/1000.0*RN820xParam->voltage);    
	
	if(RN820xParam->powerFactor > 1)
		RN820xParam->powerFactor = 1;
	
	Uart1.PrintfLog("***I:%f V:%f P:%f PF:%f kWh:%f\r\n", RN820xParam->current/1000.0, RN820xParam->voltage, RN820xParam->power, RN820xParam->powerFactor, RN820xParam->kWh);
	Uart1.PrintfLog("***Ireg:%X Vreg:%X Preg:%X\r\n", Itmp, Vtmp, Ptmp);
	
	return true;
}

void SaveRn820xParam(RN820x_t *RN820xParam)
{
	uint32_t ParamToSave[2];	
	uint32_t ParamToRead[2];
	int32_t DiffAbs = 0;
	
	//ReadWordsFromMcuEEPROM(RN820X_FLASH_START, ParamToRead, 1);
	ParamToRead[0] = ReadKwhFromEeprom();
	
	ParamToSave[0] = (uint32_t)(RN820xParam->kWh*1000);
	
	DiffAbs = ParamToSave[0]-ParamToRead[0];
	
	if(DiffAbs > 10000 || DiffAbs < -10000)	//10kWh.
		return;
	
	for(uint8_t i = 0; i < 3; i++)
	{
		//WriteWordsToMcuEEPROM(RN820X_FLASH_START, ParamToSave, 1);	
		//ReadWordsFromMcuEEPROM(RN820X_FLASH_START, ParamToRead, 1);
		WriteKwhToEeprom(ParamToSave);
		ParamToRead[0] = ReadKwhFromEeprom();
		if(ParamToSave[0] == ParamToRead[0])	//Write and read verify.
			break;
	}	
}

void GetRn820xParamFromFlash(RN820x_t *RN820xParam)
{
	uint32_t ParamToRead[2];
	
	//ReadWordsFromMcuEEPROM(RN820X_FLASH_START, ParamToRead, 1);
	ParamToRead[0] = ReadKwhFromEeprom();
	
	RN820xParam->kWhInFlash = ParamToRead[0]/1000.0;
}

void PowerGainCalibration(void)
{
	uint8_t data[10];
	uint16_t GpqaValue;
	
	if(PGAIN > 0)
	{
		GpqaValue = (uint16_t)(PGAIN*pow(2, 15));
	}
	else if(PGAIN < 0)
	{
		GpqaValue = (uint16_t)(PGAIN*pow(2, 15) + pow(2, 16));
	}
		
	data[0] = 0x85;	//Set GPQA.
	data[1] = (uint8_t)((GpqaValue>>8)&0xff);
	data[2] = (uint8_t)(GpqaValue&0xff);
	SendCmd(data, 3);
}

void Rn820xInit(void)
{
	uint8_t data[10], len;
	
	Rn820xUart();	

	data[0] = 0xEA;	//Reset.
	data[1] = 0xFA;
	SendCmd(data, 2);
	
	data[0] = 0xEA;	//Write enable.
	data[1] = 0xE5;
	SendCmd(data, 2);
	
	data[0] = 0x82;	//Set HFConst.
	data[1] = (uint8_t)((HFConst>>8)&0xff);
	data[2] = (uint8_t)(HFConst&0xff);
	SendCmd(data, 3);
	
	if(ReadRegValue(0x7f, data, &len))
	{
		Uart1.PrintfLog("Rn820x ID\r\n");
		for(uint8_t i = 0; i < len; i++)
			Uart1.PrintfLog("%X ", data[i]);
		Uart1.PrintfLog("\r\n");
	}
	//PowerGainCalibration();
	Esum = 0;
	Etmp = 0;
	EtmpM = 0;	
	
	GetRn820xParamFromFlash(&RN820xInfo);
	Uart1.PrintfLog("Get Rn820x Param From Flash. kWhInFlash:%f\r\n", RN820xInfo.kWhInFlash);
	
	CelibrateInit();
	GetCelibrateFactorFromFlash();
}

void Rn820xTest(void)
{
	uint8_t data[10], RxData[20], len;
	
	Rn820xUart();
	
	data[0] = 0xEA;
	data[1] = 0xE5;
	SendCmd(data, 2);
	
	data[0] = 0x82;
	data[1] = 0x5A;
	data[2] = 0xA5;
	SendCmd(data, 3);
	
	if(ReadRegValue(0x02, RxData, &len))
	{
		Uart1.PrintfLog("Rx %d bytes from Rn820x\r\n", len);
		for(uint8_t i = 0; i < len; i++)
			Uart1.PrintfLog("%X ", RxData[i]);
		Uart1.PrintfLog("\r\n");
	}
}

void CelibrateInit(void)
{
	CelibrateProInfo.StartChar1 = 0x68;
	CelibrateProInfo.Addr[0] = 0x11;
	CelibrateProInfo.Addr[1] = 0x11;
	CelibrateProInfo.Addr[2] = 0x11;
	CelibrateProInfo.Addr[3] = 0x11;
	CelibrateProInfo.Addr[4] = 0x11;
	CelibrateProInfo.Addr[5] = 0x11;
	CelibrateProInfo.StartChar2 = 0x68;
	CelibrateProInfo.EndChar = 0x16;
	
	CelibrateProInfo.FlagCode[0] = 0x33;
	CelibrateProInfo.FlagCode[1] = 0x4A;
	CelibrateProInfo.FlagCode[2] = 0x2B;
	CelibrateProInfo.FlagCode[3] = 0x37;
	
	CelibrateProInfo.Password[0] = 0x35;
	CelibrateProInfo.Password[1] = 0x33;
	CelibrateProInfo.Password[2] = 0x33;
	CelibrateProInfo.Password[3] = 0x33;
	CelibrateProInfo.Password[4] = 0x33;
	CelibrateProInfo.Password[5] = 0x33;
	CelibrateProInfo.Password[6] = 0x33;
	CelibrateProInfo.Password[7] = 0x33;

	CelibrateProInfo.UartSend = UartPutString;
}

bool CelibrateCheckSum(uint8_t *data, uint8_t len, uint8_t CheckSum)
{
	uint8_t sum = 0;
	
	for(uint8_t i = 0; i < len; i++)
	{
		sum += data[i];
	}
	
	if(CheckSum == sum)
		return true;
	else
		return false;
}

bool CheckFrame(uint8_t *data, uint8_t DataLen)
{
	bool state;
	
	if(CelibrateProInfo.StartChar1 == data[0] && data[7] == data[0] && CelibrateProInfo.EndChar == data[DataLen-1])
	{
		if(CelibrateCheckSum(data, DataLen-2, data[DataLen-2]) == true)
		{
			if(memcmp(&data[1], CelibrateProInfo.Addr, 6) == 0)
			{
				state = true;
			}
			else 
				state = false;
		}
		else
			state = false;
	}
	else
		state = false;
	
	return state;
}

bool CheckPayloadOfHost(uint8_t *data, uint8_t DataLen)
{
	uint16_t DataToCheck;
	
	if(DataLen < CELIBRATE_PAYLOAD_LEN)
		return false;
	
	DataToCheck = (data[5]<<8)|data[4];
	if(DataToCheck == 0)
		return false;
	
	DataToCheck = (data[11]<<8)|data[10];
	if(DataToCheck == 0x8000)
		return false;
	
	DataToCheck = (data[13]<<8)|data[12];
	if(DataToCheck == 0x8000)
		return false;
	
	DataToCheck = (data[31]<<8)|data[30];
	if(DataToCheck == 0x8000)
		return false;
	
	return true;
}

bool CheckPassword(uint8_t *data, uint8_t dataLen)
{
	if(dataLen < CELIBRATE_PAYLOAD_LEN+15)
		return false;
	
	if(memcmp(&data[14], CelibrateProInfo.Password, 8) == 0)
		return true;
	
	return false;
}

bool WriteCelibrateDataToFlash(uint8_t *data, uint8_t dataLen)
{
	uint32_t DataToSave[CELIBRATE_PAYLOAD_LEN];
	bool WriteStatus;
	
	if(dataLen > CELIBRATE_PAYLOAD_LEN)
		return false;
	
	memset(DataToSave, 0, CELIBRATE_PAYLOAD_LEN);
	memcpy(DataToSave, data, dataLen);
	//Log show.
	/*Uart1.PrintfLog("DataS:");
	for(uint8_t i = 0; i < dataLen/4 + (dataLen%4!=0); i++)
		Uart1.PrintfLog("%.8X ", DataToSave[i]);
	Uart1.PrintfLog("\r\n");*/
	
	WriteStatus = WriteWordsToMcuEEPROM(RN820X_FLASH_START+4, DataToSave, dataLen/4 + (dataLen%4!=0));
	
	return WriteStatus;
}

void ReadCelibrateDataFromFlash(uint8_t *data, uint8_t dataLen)
{
	uint32_t DataToRead[CELIBRATE_PAYLOAD_LEN];
	
	if(dataLen > CELIBRATE_PAYLOAD_LEN)
		return;
	
	memset(DataToRead, 0, CELIBRATE_PAYLOAD_LEN);	
	
	ReadWordsFromMcuEEPROM(RN820X_FLASH_START+4, DataToRead, dataLen/4 + (dataLen%4!=0));
	
	memcpy(data, DataToRead, dataLen);
}

void ConvertLittleEndian(uint8_t *data, uint8_t len)
{
	uint8_t tmp;
	
	for(uint8_t i = 0; i < len/2; i++)
	{
		tmp = data[i];
		data[i] = data[len-1-i];
		data[len-1-i] = tmp;
	}
}


CmdType ParseHostData(uint8_t *data, uint8_t DataLen)
{	
	CmdType cmd;
	uint8_t payload[CELIBRATE_PAYLOAD_LEN];
	uint8_t CmdData[10], CmdLen = 0, PayloadIndex = 0;
	float kia, ku, kpp;
	uint16_t ec;
	bool CalibrateMode_flag = false; 
	
	switch(DataLen)
	{
		case 19:
			if(data[11] == 0x11 && data[12] == 0x04)
			{
				if(CheckFrame(&data[3], DataLen-3) == false)
					return DATA_ERR;
				
				cmd = HOST_READ;
				
				//George@20180803:auto enter the calibration 
				if (CalibrateMode_flag == false)
				{
						CalibrateMode_flag = true;
						extern int EmptyFunction(const char * format, ... );
						Uart1.PrintfLog = EmptyFunction;
				}
				//end
			}				
			else
				cmd = DATA_ERR;
			
			break;
			
		case (24+CELIBRATE_PAYLOAD_LEN):
			
			if(CheckFrame(data, DataLen) == false)
				return DATA_ERR;
		
			if(data[8] == 0x14 && data[9] == 0x0C+CELIBRATE_PAYLOAD_LEN)	//Check the CtrlCode and length of data.
			{
				
				memcpy1(payload, &data[22], CELIBRATE_PAYLOAD_LEN);
				PayloadIndex = 0;
				
				Reduce33Hex(payload, CELIBRATE_PAYLOAD_LEN);
				
				if(CheckPayloadOfHost(payload, CELIBRATE_PAYLOAD_LEN) == true)	//Check the payload from host.
				{
					for(uint8_t i = 0; i <= 0x17; i++)	//Write data to registers.
					{
						CmdLen = 0;
						CmdData[CmdLen++] = i+0x80;
						
						if(i == 0x07 || i == 0x08)
						{
							memcpy1(&CmdData[CmdLen], &payload[PayloadIndex], 1);
							CmdLen++;
							PayloadIndex++;
						}
						else
						{
							memcpy1(&CmdData[CmdLen], &payload[PayloadIndex], 2);
							ConvertLittleEndian(&CmdData[CmdLen], 2);	//Convert to little-endian.
							CmdLen += 2;
							PayloadIndex += 2;
						}
						
						SendCmd(CmdData, CmdLen);
					}
					
					//Save params to flash.
					kia = (payload[63]<<24) | (payload[62]<<16) | (payload[61]<<8) | payload[60];
					ku = (payload[67]<<24) | (payload[66]<<16) | (payload[65]<<8) | payload[64];
					kpp = (payload[71]<<24) | (payload[70]<<16) | (payload[69]<<8) | payload[68];
					ec = (payload[55]<<8) | payload[54];
	
					if(kia==0 || ku==0 || kpp==0 || ec==0)
					{
						
					}
					else
					{
						WriteCelibrateDataToFlash(payload, CELIBRATE_PAYLOAD_LEN);
					}
					
					//Log show.
					/*Uart1.PrintfLog("Write C:");
					for(uint8_t i = 0; i < CELIBRATE_PAYLOAD_LEN; i++)
						Uart1.PrintfLog("%.2X ", (uint8_t)(payload[i]));
					Uart1.PrintfLog("\r\n");*/
					cmd = HOST_WRITE;
					
					//George@20180803:auto enter the calibration 
					if (CalibrateMode_flag == false)
					{
							CalibrateMode_flag = true;
							extern int EmptyFunction(const char * format, ... );
							Uart1.PrintfLog = EmptyFunction;
					}
					//end
				}
				else
					cmd = DATA_INVALID;
			}
			else
				cmd = DATA_ERR;
			
			break;
		default:
			cmd = DATA_ERR;
			break;
	}

	return cmd;
}

uint8_t SlaveUpload(int type)
{
	uint8_t payload[CELIBRATE_PAYLOAD_LEN+16];
	uint8_t PayloadIndex = 0, len = 0;
	uint8_t CheckSum = 0;
	
	switch(type)
	{
		case HOST_READ:
			payload[PayloadIndex++] = CelibrateProInfo.StartChar1;
			memcpy1(&payload[PayloadIndex], CelibrateProInfo.Addr, 6);	//6 bytes address.
			PayloadIndex += 6;
			payload[PayloadIndex++] = CelibrateProInfo.StartChar2;
			payload[PayloadIndex++] = 0x91;	//CtrlCode.
			payload[PayloadIndex++] = 4+CELIBRATE_PAYLOAD_LEN;	//Data length.
			memcpy1(&payload[PayloadIndex], CelibrateProInfo.FlagCode, 4);	//4 bytes FlagCode.
			PayloadIndex += 4;
			
			//Register data.
			for(uint8_t i = 0; i <= 0x17; i++)
			{
				ReadRegValue(i, &payload[PayloadIndex], &len);
				ConvertLittleEndian(&payload[PayloadIndex], len);	//Convert to little-endian.
				//Add33Hex(&payload[PayloadIndex], len);
				PayloadIndex += len;
			}
			
			ReadRegValue(0x2d, &payload[PayloadIndex], &len);
			ConvertLittleEndian(&payload[PayloadIndex], len);	//Convert to little-endian.
			//Add33Hex(&payload[PayloadIndex], len);
			PayloadIndex += len;
			
			for(uint8_t i = 0x22; i <= 0x28; i++)
			{
				ReadRegValue(i, &payload[PayloadIndex], &len);
				ConvertLittleEndian(&payload[PayloadIndex], len);	//Convert to little-endian.
				//Add33Hex(&payload[PayloadIndex], len);
				PayloadIndex += len;
			}
			
			Add33Hex(&payload[14], CELIBRATE_PAYLOAD_LEN);
			
			for(uint8_t i = 0; i < PayloadIndex; i++)	//Checksum.
				CheckSum += payload[i];
			
			payload[PayloadIndex++] = CheckSum;
			payload[PayloadIndex++] = CelibrateProInfo.EndChar;	//End char.
			break;
			
		case ACK_NORMAL:
			payload[PayloadIndex++] = CelibrateProInfo.StartChar1;
			memcpy1(&payload[PayloadIndex], CelibrateProInfo.Addr, 6);	//6 bytes address.
			PayloadIndex += 6;
			payload[PayloadIndex++] = CelibrateProInfo.StartChar2;
			payload[PayloadIndex++] = 0x94;	//CtrlCode.
			payload[PayloadIndex++] = 0;	//Data length.
		
			for(uint8_t i = 0; i < PayloadIndex; i++)	//Checksum.
				CheckSum += payload[i];			
		
			payload[PayloadIndex++] = CheckSum;
			payload[PayloadIndex++] = CelibrateProInfo.EndChar;	//End char.
			break;
		
		case ACK_ABNORMAL:
			payload[PayloadIndex++] = CelibrateProInfo.StartChar1;
			memcpy1(&payload[PayloadIndex], CelibrateProInfo.Addr, 6);	//6 bytes address.
			PayloadIndex += 6;
			payload[PayloadIndex++] = CelibrateProInfo.StartChar2;
			payload[PayloadIndex++] = 0xD4;	//CtrlCode.
			payload[PayloadIndex++] = 1;	//Data length.
			payload[PayloadIndex++] = 0x01;	//ERR.
		
			for(uint8_t i = 0; i < PayloadIndex; i++)	//Checksum.
				CheckSum += payload[i];
			
			payload[PayloadIndex++] = CheckSum;
			payload[PayloadIndex++] = CelibrateProInfo.EndChar;	//End char.
			break;
		
		default:
			break;
	}
	
	if(PayloadIndex > 0)
	{
		if(CelibrateProInfo.UartSend != NULL)
			CelibrateProInfo.UartSend(payload, PayloadIndex, 10);
	}
	
	return PayloadIndex;
}

void GetCelibrateFactorFromFlash(void)
{
	uint8_t FlashData[CELIBRATE_PAYLOAD_LEN];
	float kia, ku, kpp;
	uint16_t ec;
	uint8_t CmdData[10], CmdLen = 0, PayloadIndex = 0;
	
	ReadCelibrateDataFromFlash(FlashData, CELIBRATE_PAYLOAD_LEN);
	
	//Write celibration data to register.
	if(CheckPayloadOfHost(FlashData, CELIBRATE_PAYLOAD_LEN) == true)	//Check the data.
	{
		for(uint8_t i = 0; i <= 0x17; i++)	//Write data to registers.
		{
			CmdLen = 0;
			CmdData[CmdLen++] = i+0x80;
						
			if(i == 0x07 || i == 0x08)
			{
				memcpy1(&CmdData[CmdLen], &FlashData[PayloadIndex], 1);
				CmdLen++;
				PayloadIndex++;
			}
			else
			{
				memcpy1(&CmdData[CmdLen], &FlashData[PayloadIndex], 2);
				ConvertLittleEndian(&CmdData[CmdLen], 2);	//Convert to little-endian.
				CmdLen += 2;
				PayloadIndex += 2;
			}
						
			SendCmd(CmdData, CmdLen);
		}
	}
	else
	{
		Uart1.PrintfLog("Write register failed from flash\r\n");
	}
	
	CmdData[0] = 0x81;	//EMUCON
	CmdData[1] = 0x00;
	CmdData[2] = 0x03;
	SendCmd(CmdData, 3);
	
	kia = ((FlashData[63]<<24) | (FlashData[62]<<16) | (FlashData[61]<<8) | FlashData[60])*1.0/pow(2, 23)/1000;
	ku = ((FlashData[67]<<24) | (FlashData[66]<<16) | (FlashData[65]<<8) | FlashData[64])*1.0/pow(2, 23)/1000;
	kpp = ((FlashData[71]<<24) | (FlashData[70]<<16) | (FlashData[69]<<8) | FlashData[68])*1.0/pow(2, 31);
	ec = (FlashData[55]<<8) | FlashData[54];
	
	if(kia==0 || ku==0 || kpp==0 || ec==0)
	{
		kia = Ki;
		ku = Kv;
		kpp = Kp;
		ec = EC;
		Uart1.PrintfLog("Celibration data in flash is invalid. Use default value.\r\n");
		Calibration_flag = false; 
	}
	else
	{
		Uart1.PrintfLog("Celibration data in flash is valid\r\n");
		
		if (ec == 1000 || ec == 2400)
			Calibration_flag = true;
	}
	
	CelibrateProInfo.Kia = kia;
	CelibrateProInfo.Ku = ku;
	CelibrateProInfo.Kpp = kpp;
	CelibrateProInfo.Ec = ec;
	
	Uart1.PrintfLog("kia:%f, ku:%f, kpp:%f, ec:%d\r\n", CelibrateProInfo.Kia, CelibrateProInfo.Ku, CelibrateProInfo.Kpp, CelibrateProInfo.Ec);
}

void CelibrateHandler(uint8_t *data, uint8_t len)
{
	int cmdType;
	
	cmdType = ParseHostData(data, len);
	
	switch(cmdType)
	{
		case HOST_READ:
			SlaveUpload(HOST_READ);
			break;
		
		case HOST_WRITE:
			SlaveUpload(ACK_NORMAL);
			break;
		
		default:
			SlaveUpload(ACK_ABNORMAL);
			break;
	}
}

void Add33Hex(uint8_t *data, uint8_t len)
{
	for(uint8_t i = 0; i < len; i++)
		data[i] += 0x33;
}

void Reduce33Hex(uint8_t *data, uint8_t len)
{
	for(uint8_t i = 0; i < len; i++)
		data[i] -= 0x33;
}

void ClearKwhInFlash(void)
{
	uint8_t data[10], len;
	uint16_t RegValue = 0;	
	
	if(ReadRegValue(0x01, data, &len))	//Read EMUCON.
	{
		RegValue = (data[0]<<8) | data[1];		
		RegValue |= (1<<15);	//Set EnergyCLR bit.
		
		data[0] = 0x81;	//Set EMUCON(EnergyCLR).
		data[1] = (RegValue>>8)&0xff;
		data[2] = RegValue&0xff;
		SendCmd(data, 3);
		
		for(uint8_t i = 0; i < 3; i++)
		{
			if(ReadRegValue(0x29, data, &len))	//Reading this register will clear it.
			{
				//Uart1.PrintfLog("Clear EnergyP\r\n");
				break;
			}
		}
		
		RegValue &= ~(1<<15);	//Clear EnergyCLR bit.
		
		data[0] = 0x81;	
		data[1] = (RegValue>>8)&0xff;
		data[2] = RegValue&0xff;
		SendCmd(data, 3);
	}
	
	//EraseMcuEEPROMByWords(RN820X_FLASH_START, 1);
	EraseKwhInFlash();
	
	RN820xInfo.kWhInFlash = 0;
	Esum = 0;
	Etmp = 0;
	EtmpM = 0;	
}

void WriteKwhToEeprom(uint32_t *data)
{
	uint32_t BlockData[3] = {0, 0, 0};
	uint32_t addrTmp = 0;
	
	for(uint32_t addr = KWH_FLASH_STATR; addr < KWH_FLASH_END; addr+=4)
	{
		ReadWordsFromMcuEEPROM(addr, BlockData, 1);
		
		if(BlockData[0] != 0)
		{
			if(addr+4 < KWH_FLASH_END)
			{
				addrTmp = addr+4;				
			}
			else
			{
				addrTmp = KWH_FLASH_STATR;
			}			
			
			WriteWordsToMcuEEPROM(addrTmp, data, 1);
			ReadWordsFromMcuEEPROM(addrTmp, BlockData+1, 1);
			
			if(*data == BlockData[1])	//Write successfully and erase the current value in flash.
			{
				EraseMcuEEPROMByWords(addr, 1);
			}
			
			return;
		}
	}
	
	WriteWordsToMcuEEPROM(KWH_FLASH_STATR, data, 1);
}

uint32_t ReadKwhFromEeprom(void)
{
	uint32_t BlockData[2] = {0, 0};
	
	for(uint32_t addr = KWH_FLASH_STATR; addr < KWH_FLASH_END; addr+=4)
	{
		ReadWordsFromMcuEEPROM(addr, BlockData, 1);
		
		if(BlockData[0] != 0)
			return BlockData[0];
	}
	
	return 0;
}

void EraseKwhInFlash(void)
{
	uint32_t BlockData[2] = {0, 0};
	
	for(uint32_t addr = KWH_FLASH_STATR; addr < KWH_FLASH_END; addr+=4)
	{
		ReadWordsFromMcuEEPROM(addr, BlockData, 1);
		
		if(BlockData[0] != 0)
			EraseMcuEEPROMByWords(addr, 1);
	}
}

