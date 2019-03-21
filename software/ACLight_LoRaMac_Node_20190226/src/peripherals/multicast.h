#ifndef _MULTICAST_H_
#define _MULTICAST_H_

#include "stdint.h"
#include "stdbool.h"

typedef struct sMulticastMember
{
    /*!
     * Address
     */
    uint32_t Address;
    /*!
     * Network session key
     */
    uint8_t NwkSKey[16];
    /*!
     * Application session key
     */
    uint8_t AppSKey[16];
}MulticastMember_t;

extern uint8_t GroupAddrIndex;
extern uint8_t CtrlGroupAddrFlag;
extern uint32_t TimeSyncDownCountTmp;

void MulticastTest(void);
bool AddGroupAddrToMulticastList(uint8_t index, uint32_t addr, uint32_t DownLinkCounter);
bool AddGroupAddrToMulticastListUnSave(uint8_t index, uint32_t addr, uint32_t DownLinkCounter);
bool RemoveGroupAddrFromMulticastList(uint32_t addr);
bool SetMulticastAddr(uint16_t index);
void LoadMulticastFromFlash(void);
bool SetMulticastDownlinkCounter(uint32_t addr, uint32_t counter);
bool CheckMulticastInFlash(uint8_t KeyIndex, uint32_t addr, uint32_t DownLinkCounter);

void EraseMultcastAddress(void);  //George@20180713:Erase multcast address
void SaveAllDownLinkCounter(void);
uint8_t GetDownLinkCounterByAddr(uint32_t addr, uint32_t *counter);
uint8_t GetMulticastInfo(uint8_t *buffer);
void SaveMulticastDownlinkCounter(uint32_t addr, uint32_t counter);

#endif
