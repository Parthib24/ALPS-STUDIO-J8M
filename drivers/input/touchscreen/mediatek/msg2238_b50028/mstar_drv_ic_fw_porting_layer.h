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
 * @file    mstar_drv_ic_fw_porting_layer.h
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

#ifndef __MSTAR_DRV_IC_FW_PORTING_LAYER_H__
#define __MSTAR_DRV_IC_FW_PORTING_LAYER_H__

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/

#include "mstar_drv_common.h"
#if defined(CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC)
#include "mstar_drv_mutual_fw_control.h"
#ifdef CONFIG_ENABLE_ITO_MP_TEST
#include "mstar_drv_mutual_mp_test.h"
#endif //CONFIG_ENABLE_ITO_MP_TEST
#elif defined(CONFIG_ENABLE_TOUCH_DRIVER_FOR_SELF_IC)
#include "mstar_drv_self_fw_control.h"
#ifdef CONFIG_ENABLE_ITO_MP_TEST
#include "mstar_drv_self_mp_test.h"
#endif //CONFIG_ENABLE_ITO_MP_TEST
#endif

/*--------------------------------------------------------------------------*/
/* PREPROCESSOR CONSTANT DEFINITION                                         */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* GLOBAL FUNCTION DECLARATION                                              */
/*--------------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
extern void DrvIcFwLyrOpenGestureWakeup(unsigned int *pWakeupMode);
extern void DrvIcFwLyrCloseGestureWakeup(void);

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
extern void DrvIcFwLyrOpenGestureDebugMode(unsigned char nGestureFlag);
extern void DrvIcFwLyrCloseGestureDebugMode(void);
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

extern unsigned int DrvIcFwLyrReadDQMemValue(unsigned short nAddr);
extern void DrvIcFwLyrWriteDQMemValue(unsigned short nAddr, unsigned int nData);

extern unsigned short DrvIcFwLyrChangeFirmwareMode(unsigned short nMode);
extern void DrvIcFwLyrGetFirmwareInfo(FirmwareInfo_t *pInfo);
#if defined(CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC) // use for MSG26XXM only
extern unsigned short DrvIcFwLyrGetFirmwareMode(void);
#endif //CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC
extern void DrvIcFwLyrRestoreFirmwareModeToLogDataMode(void);

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
extern void DrvIcFwLyrCheckFirmwareUpdateBySwId(void);
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

extern void DrvIcFwLyrVariableInitialize(void);
extern void DrvIcFwLyrOptimizeCurrentConsumption(void);
extern unsigned char DrvIcFwLyrGetChipType(void);
extern void DrvIcFwLyrGetCustomerFirmwareVersion(unsigned short *pMajor, unsigned short *pMinor, unsigned char **ppVersion);
extern void DrvIcFwLyrGetPlatformFirmwareVersion(unsigned char **ppVersion);
extern void DrvIcFwLyrHandleFingerTouch(unsigned char *pPacket, unsigned short nLength);
extern unsigned int DrvIcFwLyrIsRegisterFingerTouchInterruptHandler(void);
extern signed int DrvIcFwLyrUpdateFirmware(unsigned char szFwData[][1024], EmemType_e eEmemType);
extern signed int DrvIcFwLyrUpdateFirmwareBySdCard(const char *pFilePath);

#ifdef CONFIG_ENABLE_ITO_MP_TEST
extern void DrvIcFwLyrCreateMpTestWorkQueue(void);
extern void DrvIcFwLyrScheduleMpTestWork(ItoTestMode_e eItoTestMode);
extern void DrvIcFwLyrGetMpTestDataLog(ItoTestMode_e eItoTestMode, unsigned char *pDataLog, unsigned int *pLength);
extern void DrvIcFwLyrGetMpTestFailChannel(ItoTestMode_e eItoTestMode, unsigned char *pFailChannel, unsigned int *pFailChannelCount);
extern signed int DrvIcFwLyrGetMpTestResult(void);
#if defined(CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC)
extern void DrvIcFwLyrGetMpTestScope(TestScopeInfo_t *pInfo);
#endif //CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC
#endif //CONFIG_ENABLE_ITO_MP_TEST
        
#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
#if defined(CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC)
extern void DrvIcFwLyrGetTouchPacketAddress(unsigned short *pDataAddress, unsigned short *pFlagAddress);
#endif //CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
extern signed int DrvIcFwLyrEnableProximity(void);
extern signed int DrvIcFwLyrDisableProximity(void);
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#ifdef CONFIG_ENABLE_GLOVE_MODE
extern void DrvIcFwLyrOpenGloveMode(void);
extern void DrvIcFwLyrCloseGloveMode(void);
extern void DrvIcFwLyrGetGloveInfo(unsigned char *pGloveMode);
#endif //CONFIG_ENABLE_GLOVE_MODE

#endif  /* __MSTAR_DRV_IC_FW_PORTING_LAYER_H__ */
