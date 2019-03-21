#include <string.h>
#include <math.h>
#include "board.h"

#include "LoRaMac.h"
#include "multicast.h"
#include "internal-eeprom.h"

#define MULTICAST_LIST_LEN	16
#define MULTICAST_MAX_LEN		100

uint8_t GroupAddrIndex = 0;
uint8_t CtrlGroupAddrFlag = 0;
uint32_t TimeSyncDownCountTmp = 0;
static MulticastParams_t *MulticastList = NULL;

static const MulticastMember_t MulticastMember[MULTICAST_LIST_LEN] = {
	{0xf0000000, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x30}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xd0}},
	{0xf0000001, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x31}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xd1}},
	{0xf0000002, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x32}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xd2}},
	{0xf0000003, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x33}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xd3}},
	{0xf0000004, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x34}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xd4}},
	{0xf0000005, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x35}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xd5}},
	{0xf0000006, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x36}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xd6}},
	{0xf0000007, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x37}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xd7}},
	{0xf0000008, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x38}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xd8}},
	{0xf0000009, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x39}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xd9}},
	{0xf000000a, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x3a}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xda}},
	{0xf000000b, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x3b}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xdb}},
	{0xf000000c, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x3c}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xdc}},
	{0xf000000d, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x3d}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xdd}},
	{0xf000000e, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x3e}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xde}},
	{0xf000000f, {0x06, 0x91, 0xc2, 0x8c, 0xd2, 0x86, 0x1e, 0xcb, 0xb8, 0xb1, 0xf5, 0x33, 0x66, 0x31, 0x15, 0x3f}, {0x8a, 0xed, 0x39, 0xc1, 0x64, 0xa6, 0x98, 0x92, 0x47, 0x27, 0xce, 0xd7, 0x8e, 0xe4, 0xea, 0xdf}},
		
};

bool CreateMulticastNode(MulticastParams_t **node)
{
	*node = (MulticastParams_t *)malloc(sizeof(MulticastParams_t));
	
	if(!(*node))
	{
		return false;
	}
	
	(*node)->Next = NULL;
	
	return true;
}

bool CreateMulticastList(void)
{
	if(CreateMulticastNode(&MulticastList))
	{
		Uart1.PrintfLog("Create muticast list successfully\r\n");
		return true;
	}
	else
	{
		Uart1.PrintfLog("Create muticast list failed\r\n");
		return false;
	}	
}

bool MulticastNodeIsExist(uint32_t addr, MulticastParams_t *list)
{	
	if(list == NULL)
		return false;
	
	if(addr == list->Address)
		return true;
	
	MulticastParams_t *cur = list;

	// Search the node in the list
	while( cur && cur->Address != addr )
	{
		cur = cur->Next;
	}
	// Found the node.
	if( cur )
	{
		return true;
	}
	
	return false;
}

bool SaveMulticastToFlash(MulticastParams_t *list)
{	
	uint32_t DataBuf[MULTICAST_MAX_LEN];
	uint32_t DataLen = 0, DataValue;
	uint8_t KeyIndex = 0xff, i, j;
	
//	if(list == NULL)
//		return false;
	
	MulticastParams_t *cur = list;

	// Search the node in the list
	while( cur )
	{
		for(i = 0; i < MULTICAST_LIST_LEN; i++)	//Find the session key index;
		{
			for(j = 0; j < 16; j++)	//Compare NwkSkey and AppSKey;
			{
				if(cur->NwkSKey[j] != MulticastMember[i].NwkSKey[j] || cur->AppSKey[j] != MulticastMember[i].AppSKey[j])
					break;
			}
			
			if(j == 16)	//We found it.
			{
				KeyIndex = i;
				break;
			}
		}
		
		DataValue = cur->Address&0x00ffffff;
		DataValue |= ((KeyIndex&0x0f)<<24);
		DataValue |= 0x80000000;
		
		if(i < MULTICAST_MAX_LEN)
		{
			DataBuf[DataLen++] = DataValue;
			DataBuf[DataLen++] = cur->DownLinkCounter | 0x80000000;
		}
		
		cur = cur->Next;
	}
	
	if(WriteMcuEEPROM(DataBuf, DataLen) == false)
		Uart1.PrintfLog("Save failed!\r\n");
	else
		return true;
	
	return false;
}

void LoadMulticastFromFlash(void)
{
	uint32_t DataBuf[MULTICAST_MAX_LEN];
	uint32_t DataLen, DataValue, DownLinkCounter;
	uint8_t KeyIndex = 0xff, i, TimeSyncMulticastExist = 0;
	
	ReadMcuEEPROM(DataBuf, &DataLen);
	
	if(DataLen > 0)
	{
		for(i = 0; i < DataLen; i+=2)
		{
			KeyIndex = (DataBuf[i]&0x0f000000)>>24;
			DataValue = (DataBuf[i]&0x00ffffff)|0xff000000;
			DownLinkCounter = DataBuf[i+1]&0x7fffffff;
			
#if   defined( REGION_CN470 )
			if(DataValue == 0xffffffff)
#elif defined( REGION_EU868 )
			if(DataValue == 0xfffffffe)
#elif defined( REGION_AU915 ) 
			if(DataValue == 0xfffffffd)
#elif defined( REGION_US915 ) 
			if(DataValue == 0xfffffffc)
#elif defined( REGION_AS923 ) 
			if(DataValue == 0xfffffffb)
#endif
			{
				TimeSyncMulticastExist = 1;
				TimeSyncDownCountTmp = DownLinkCounter;
			}
			else
				CtrlGroupAddrFlag = 1;
			
			AddGroupAddrToMulticastListUnSave(KeyIndex, DataValue, DownLinkCounter);
			Uart1.PrintfLog("Load group addr:%d-%X-%d\r\n", KeyIndex, DataValue, DownLinkCounter);
		}
	}
	else
	{
		Uart1.PrintfLog("No data to load from flash.\r\n");
	}
	
	if(!TimeSyncMulticastExist)	//TimeSync isn't exist.
	{
	//George@20180711:default initialization multicast
#if   defined( REGION_CN470 ) 
		KeyIndex = 0; 
		DataValue = 0xffffffff;
#elif defined( REGION_EU868 )
		KeyIndex = 1; 
		DataValue = 0xfffffffe;
#elif defined( REGION_AU915 ) 
		KeyIndex = 2; 
		DataValue = 0xfffffffd;
#elif defined( REGION_US915 ) 
		KeyIndex = 3; 
		DataValue = 0xfffffffc;
#elif defined( REGION_AS923 ) 
		KeyIndex = 4; 
		DataValue = 0xfffffffb;
#endif
	
		AddGroupAddrToMulticastList(KeyIndex, DataValue, 0);
		Uart1.PrintfLog("Add TimeSync multicast address:%d-%X\r\n", KeyIndex, DataValue);
	}
}

bool CheckMulticastInFlash(uint8_t KeyIndex, uint32_t addr, uint32_t DownLinkCounter)
{
	uint32_t DataBuf[MULTICAST_MAX_LEN];
	uint32_t DataLen;
	
	ReadMcuEEPROM(DataBuf, &DataLen);
	
	if(DataLen > 0)
	{
		for(uint8_t i = 0; i < DataLen; i+=2)
		{
			if(KeyIndex == ((DataBuf[i]&0x0f000000)>>24) && addr == ((DataBuf[i]&0x00ffffff)|0xff000000) && DownLinkCounter == (DataBuf[i+1]&0x7fffffff))
				return true;
		}
	}
	
	return false;
}

MulticastParams_t *FindNodeAddrByGroupAddr(uint32_t addr)
{
	MibRequestConfirm_t mibReq;
	LoRaMacStatus_t status;	
	MulticastParams_t *cur = NULL;
	
	if(MulticastList == NULL)
		return false;	

	mibReq.Type = MIB_MULTICAST_CHANNEL;
	status = LoRaMacMibGetRequestConfirm( &mibReq );
	
	if( status == LORAMAC_STATUS_OK )
	{
		MulticastList = mibReq.Param.MulticastList;
	}
	else
		return NULL;
	
	cur = MulticastList;
	
	// Search the node in the list
	while( cur && cur->Address != addr )
	{
		cur = cur->Next;
	}
	// If we found the node, return its address.
	if( cur )
	{
		return cur;
	}
	
	return NULL;	
}

bool FillMulticastMemberToNode(MulticastParams_t *node, uint8_t index, uint32_t addr)
{
	if(node == NULL)
		return false;
	
	node->Address = addr;
	memcpy1(node->AppSKey, MulticastMember[index].AppSKey, 16);
	memcpy1(node->NwkSKey, MulticastMember[index].NwkSKey, 16);
	node->DownLinkCounter = 0;	//Steven@20180906.
	
	return true;
}

bool AddGroupAddrToMulticastList(uint8_t index, uint32_t addr, uint32_t DownLinkCounter)
{
	MulticastParams_t *tmp = NULL;
	MibRequestConfirm_t mibReq;
	LoRaMacStatus_t status;
	
	if(MulticastNodeIsExist(addr, MulticastList) == true)
		return true;
	
	if(CreateMulticastNode(&tmp))
	{
		FillMulticastMemberToNode(tmp, index, addr);
		if(LoRaMacMulticastChannelLink(tmp) != LORAMAC_STATUS_OK)
		{
			return false;
		}
		
		tmp->DownLinkCounter = DownLinkCounter;
	}
	else
		return false;
	
	mibReq.Type = MIB_MULTICAST_CHANNEL;
	status = LoRaMacMibGetRequestConfirm( &mibReq );
	
	if( status == LORAMAC_STATUS_OK )
	{
		MulticastList = mibReq.Param.MulticastList;
		SaveMulticastToFlash(MulticastList);	//Save the new multicast to flash;
	}
	else
		return false;
	
	return true;
}

bool AddGroupAddrToMulticastListUnSave(uint8_t index, uint32_t addr, uint32_t DownLinkCounter)
{
	MulticastParams_t *tmp = NULL;
	MibRequestConfirm_t mibReq;
	LoRaMacStatus_t status;
	
	if(MulticastNodeIsExist(addr, MulticastList) == true)
		return true;
	
	if(CreateMulticastNode(&tmp))
	{
		FillMulticastMemberToNode(tmp, index, addr);
		if(LoRaMacMulticastChannelLink(tmp) != LORAMAC_STATUS_OK)
		{
			return false;
		}
		
		tmp->DownLinkCounter = DownLinkCounter;
	}
	else
		return false;
	
	mibReq.Type = MIB_MULTICAST_CHANNEL;
	status = LoRaMacMibGetRequestConfirm( &mibReq );
	
	if( status == LORAMAC_STATUS_OK )
	{
		MulticastList = mibReq.Param.MulticastList;
	}
	else
		return false;
	
	return true;
}

bool RemoveGroupAddrFromMulticastList(uint32_t addr)
{
	MulticastParams_t *tmp = NULL;
	MibRequestConfirm_t mibReq;
	LoRaMacStatus_t status;
	
	if(MulticastNodeIsExist(addr, MulticastList) == false)
		return true;
	
	tmp = FindNodeAddrByGroupAddr(addr);
	
	if(LoRaMacMulticastChannelUnlink(tmp) != LORAMAC_STATUS_OK)
		return false;
	
	free(tmp);
	
	mibReq.Type = MIB_MULTICAST_CHANNEL;
	status = LoRaMacMibGetRequestConfirm( &mibReq );
	if( status == LORAMAC_STATUS_OK )
	{
		MulticastList = mibReq.Param.MulticastList;
		SaveMulticastToFlash(MulticastList);	//Save the new multicast to flash;
	}
	else
		return false;
	
	return true;
}

void SaveMulticastDownlinkCounter(uint32_t addr, uint32_t counter)
{
	uint32_t ReadData = 0, WriteData = 0;
	
	for(uint32_t i = DATA_EEPROM_START_ADDR; i < DATA_EEPROM_END_ADDR; i+=8)
	{
		ReadWordsFromMcuEEPROM(i, &ReadData, 1);
		
		if((ReadData&0x00ffffff) == (addr&0x00ffffff))
		{
			WriteData = counter|0x80000000;
			
			if(i+4 >= DATA_EEPROM_END_ADDR)
				break;
			
			WriteWordsToMcuEEPROM(i+4, &WriteData, 1);	//Save multicast downlink counter.
			break;
		}
	}
}

bool SetMulticastDownlinkCounter(uint32_t addr, uint32_t counter)
{
	MulticastParams_t *tmp = NULL;
	
	tmp = FindNodeAddrByGroupAddr(addr);
	
	if(tmp != NULL)
	{
		tmp->DownLinkCounter = counter;
		SaveMulticastDownlinkCounter(addr, counter);
	}
	else
		return false;
	
	return true;
}

void SaveAllDownLinkCounter(void)
{	
	MibRequestConfirm_t mibReq;
	LoRaMacStatus_t status;	
	MulticastParams_t *cur = NULL;
	
	if(MulticastList == NULL)
		return;	

	mibReq.Type = MIB_MULTICAST_CHANNEL;
	status = LoRaMacMibGetRequestConfirm( &mibReq );
	
	if( status == LORAMAC_STATUS_OK )
	{
		MulticastList = mibReq.Param.MulticastList;
	}
	else
		return;
	
	cur = MulticastList;
	
	// Search the node in the list
	while( cur )
	{
		SaveMulticastDownlinkCounter(cur->Address, cur->DownLinkCounter);
		cur = cur->Next;
	}
}

uint8_t GetDownLinkCounterByAddr(uint32_t addr, uint32_t *counter)
{
	MibRequestConfirm_t mibReq;
	LoRaMacStatus_t status;	
	MulticastParams_t *cur = NULL;
	
	if(MulticastList == NULL)
		return 0;	

	mibReq.Type = MIB_MULTICAST_CHANNEL;
	status = LoRaMacMibGetRequestConfirm( &mibReq );
	
	if( status == LORAMAC_STATUS_OK )
	{
		MulticastList = mibReq.Param.MulticastList;
	}
	else
		return 0;
	
	cur = MulticastList;
	
	// Search the node in the list
	while( cur )
	{
		if(addr == cur->Address)
		{
			*counter = cur->DownLinkCounter;
			return 1;		
		}
		
		cur = cur->Next;
	}
	
	return 0;
}

void MulticastTest(void)
{
	AddGroupAddrToMulticastList(0, 0xff000002, 0);
	AddGroupAddrToMulticastList(2, 0xff000005, 0);
	AddGroupAddrToMulticastList(3, 0xff000004, 0);
	AddGroupAddrToMulticastList(5, 0xff000001, 0);
	
	RemoveGroupAddrFromMulticastList(0xff000004);	
	Uart1.PrintfLog("remove 4\r\n");
	
	for(uint32_t i = 0xff000000; i < 0xff000000+MULTICAST_LIST_LEN; i++)
	{
		if(MulticastNodeIsExist(i, MulticastList))
			Uart1.PrintfLog("%X exist\r\n", i);
	}
	
	RemoveGroupAddrFromMulticastList(0xff000005);	
	Uart1.PrintfLog("remove 5\r\n");
	
	for(uint32_t i = 0xff000000; i < 0xff000000+MULTICAST_LIST_LEN; i++)
	{
		if(MulticastNodeIsExist(i, MulticastList))
			Uart1.PrintfLog("%X exist\r\n", i);
	}
	
	RemoveGroupAddrFromMulticastList(0xff000001);
	Uart1.PrintfLog("remove 1\r\n");
	
	for(uint32_t i = 0xff000000; i < 0xff000000+MULTICAST_LIST_LEN; i++)
	{
		if(MulticastNodeIsExist(i, MulticastList))
			Uart1.PrintfLog("%X exist\r\n", i);
	}
	
	RemoveGroupAddrFromMulticastList(0xff000002);
	Uart1.PrintfLog("remove 2\r\n");
	
	for(uint32_t i = 0xff000000; i < 0xff000000+MULTICAST_LIST_LEN; i++)
	{
		if(MulticastNodeIsExist(i, MulticastList))
			Uart1.PrintfLog("%X exist\r\n", i);
	}
	
}

uint8_t GetMulticastInfo(uint8_t *buffer)
{
	MibRequestConfirm_t mibReq;
	LoRaMacStatus_t status;	
	MulticastParams_t *cur = NULL;
	uint8_t DataLen = 0;
	
	if(buffer == NULL)
		return 0;

	mibReq.Type = MIB_MULTICAST_CHANNEL;
	status = LoRaMacMibGetRequestConfirm( &mibReq );
	
	if( status == LORAMAC_STATUS_OK )
	{
		cur = mibReq.Param.MulticastList;
	}
	else
		return 0;
	
	// Search the node in the list
	while( cur)
	{
		buffer[DataLen++] = ((cur->Address)>>24)&0xff;
		buffer[DataLen++] = ((cur->Address)>>16)&0xff;
		buffer[DataLen++] = ((cur->Address)>>8)&0xff;
		buffer[DataLen++] = (cur->Address)&0xff;
		buffer[DataLen++] = cur->AppSKey[15];
		buffer[DataLen++] = cur->NwkSKey[15];
		buffer[DataLen++] = ((cur->DownLinkCounter)>>24)&0xff;
		buffer[DataLen++] = ((cur->DownLinkCounter)>>16)&0xff;
		buffer[DataLen++] = ((cur->DownLinkCounter)>>8)&0xff;
		buffer[DataLen++] = (cur->DownLinkCounter)&0xff;
		cur = cur->Next;
	}
	
	return DataLen;	
}

