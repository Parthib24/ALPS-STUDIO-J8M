////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2014 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (??MStar Confidential Information??) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_utility_adaption.h
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

#ifndef __MSTAR_DRV_UTILITY_ADAPTION_H__
#define __MSTAR_DRV_UTILITY_ADAPTION_H__ (1)


////////////////////////////////////////////////////////////
/// Included Files
////////////////////////////////////////////////////////////

#include "mstar_drv_common.h"

////////////////////////////////////////////////////////////
/// Constant
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Data Types
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Variables
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Macro
////////////////////////////////////////////////////////////
#define BK_REG8_WL(addr,val)    ( RegSetLByteValue( addr, val ) )
#define BK_REG8_WH(addr,val)    ( RegSetHByteValue( addr, val ) )
#define BK_REG16_W(addr,val)    ( RegSet16BitValue( addr, val ) )
#define BK_REG8_RL(addr)        ( RegGetLByteValue( addr ) )
#define BK_REG8_RH(addr)        ( RegGetHByteValue( addr ) )
#define BK_REG16_R(addr)        ( RegGet16BitValue( addr ) )

#define PRINTF_EMERG(fmt, ...)  printk(KERN_EMERG pr_fmt(fmt), ##__VA_ARGS__)
#define PRINTF_ALERT(fmt, ...)  printk(KERN_ALERT pr_fmt(fmt), ##__VA_ARGS__)
#define PRINTF_CRIT(fmt, ...)   printk(KERN_CRIT pr_fmt(fmt), ##__VA_ARGS__) 
#define PRINTF_ERR(fmt, ...)    printk(KERN_ERR pr_fmt(fmt), ##__VA_ARGS__)
#define PRINTF_WARN(fmt, ...)   printk(KERN_WARNING pr_fmt(fmt), ##__VA_ARGS__)
#define PRINTF_NOTICE(fmt, ...) printk(KERN_NOTICE pr_fmt(fmt), ##__VA_ARGS__)
#define PRINTF_INFO(fmt, ...)   printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#define PRINTF_DEBUG(fmt, ...)  printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__) 

////////////////////////////////////////////////////////////
/// Function Prototypes
////////////////////////////////////////////////////////////

#ifdef CONFIG_ENABLE_DMA_IIC
extern void DmaAlloc(void);
extern void DmaReset(void);
extern void DmaFree(void);
#endif //CONFIG_ENABLE_DMA_IIC
extern unsigned short  RegGet16BitValue(unsigned short nAddr);
extern unsigned char   RegGetLByteValue(unsigned short nAddr);
extern unsigned char   RegGetHByteValue(unsigned short nAddr);
extern void RegGetXBitValue(unsigned short nAddr, unsigned char * pRxData, unsigned short nLength, unsigned short nMaxI2cLengthLimit);
extern void RegSet16BitValue(unsigned short nAddr, unsigned short nData);
extern void RegSetLByteValue(unsigned short nAddr, unsigned char nData);
extern void RegSetHByteValue(unsigned short nAddr, unsigned char nData);
extern void RegSet16BitValueOn(unsigned short nAddr, unsigned short nData);
extern void RegSet16BitValueOff(unsigned short nAddr, unsigned short nData);
extern unsigned short  RegGet16BitValueByAddressMode(unsigned short nAddr, AddressMode_e eAddressMode);
extern void RegSet16BitValueByAddressMode(unsigned short nAddr, unsigned short nData, AddressMode_e eAddressMode);
extern void RegMask16BitValue(unsigned short nAddr, unsigned short nMask, unsigned short nData, AddressMode_e eAddressMode);
extern signed int DbBusEnterSerialDebugMode(void);
extern void DbBusExitSerialDebugMode(void);
extern void DbBusIICUseBus(void);
extern void DbBusIICNotUseBus(void);
extern void DbBusIICReshape(void);
extern void DbBusStopMCU(void);
extern void DbBusNotStopMCU(void);
extern void DbBusResetSlave(void);
extern void DbBusWaitMCU(void);
extern signed int IicWriteData(unsigned char nSlaveId, unsigned char* pBuf, unsigned short nSize);
extern signed int IicReadData(unsigned char nSlaveId, unsigned char* pBuf, unsigned short nSize);
extern signed int IicSegmentReadDataByDbBus(unsigned char nRegBank, unsigned char nRegAddr, unsigned char* pBuf, unsigned short nSize, unsigned short nMaxI2cLengthLimit);
extern signed int IicSegmentReadDataBySmBus(unsigned short nAddr, unsigned char* pBuf, unsigned short nSize, unsigned short nMaxI2cLengthLimit);
extern void mstpMemSet(void *pDst, signed char nVal, unsigned int nSize);
extern void mstpMemCopy(void *pDst, void *pSource, unsigned int nSize);
extern void mstpDelay(unsigned int nTime);

#endif // __MSTAR_DRV_UTILITY_ADAPTION_H__
