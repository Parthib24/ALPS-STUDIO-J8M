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
 * @file    mstar_drv_mutual_fw_control.h
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

#ifndef __MSTAR_DRV_MUTUAL_FW_CONTROL_H__
#define __MSTAR_DRV_MUTUAL_FW_CONTROL_H__

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/

#include "mstar_drv_common.h"
#ifdef CONFIG_ENABLE_HOTKNOT
#include "mstar_drv_hotknot.h"
#endif //CONFIG_ENABLE_HOTKNOT

#if defined(CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC)

/*--------------------------------------------------------------------------*/
/* PREPROCESSOR CONSTANT DEFINITION                                         */
/*--------------------------------------------------------------------------*/

#define DEMO_MODE_PACKET_LENGTH    (43)
#define MAX_TOUCH_NUM           (10)     

#define MSG26XXM_FIRMWARE_MAIN_BLOCK_SIZE (32) //32K
#define MSG26XXM_FIRMWARE_INFO_BLOCK_SIZE (8) //8K
#define MSG26XXM_FIRMWARE_WHOLE_SIZE (MSG26XXM_FIRMWARE_MAIN_BLOCK_SIZE+MSG26XXM_FIRMWARE_INFO_BLOCK_SIZE) //40K

#define MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE (128) //128K
#define MSG28XX_FIRMWARE_INFO_BLOCK_SIZE (2) //2K
#define MSG28XX_FIRMWARE_WHOLE_SIZE (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+MSG28XX_FIRMWARE_INFO_BLOCK_SIZE) //130K

#define MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE  (128)
#define MSG28XX_EMEM_SIZE_BYTES_ONE_WORD  (4)

#define MSG28XX_EMEM_MAIN_MAX_ADDR  (0x3FFF) //0~0x3FFF = 0x4000 = 16384 = 65536/4
#define MSG28XX_EMEM_INFO_MAX_ADDR  (0x1FF) //0~0x1FF = 0x200 = 512 = 2048/4


#define MSG26XXM_FIRMWARE_MODE_UNKNOWN_MODE (0xFFFF)
#define MSG26XXM_FIRMWARE_MODE_DEMO_MODE    (0x0005)
#define MSG26XXM_FIRMWARE_MODE_DEBUG_MODE   (0x0105)

#define MSG28XX_FIRMWARE_MODE_UNKNOWN_MODE (0xFF)
#define MSG28XX_FIRMWARE_MODE_DEMO_MODE    (0x00)
#define MSG28XX_FIRMWARE_MODE_DEBUG_MODE   (0x01)


#define DEBUG_MODE_PACKET_LENGTH    (1280) //It is a predefined maximum packet length, not the actual packet length which queried from firmware.

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
#define UPDATE_FIRMWARE_RETRY_COUNT (2)
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#define FIRMWARE_GESTURE_INFORMATION_MODE_A	(0x00)
#define FIRMWARE_GESTURE_INFORMATION_MODE_B	(0x01)
#define FIRMWARE_GESTURE_INFORMATION_MODE_C	(0x02)
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

/*--------------------------------------------------------------------------*/
/* DATA TYPE DEFINITION                                                     */
/*--------------------------------------------------------------------------*/

typedef struct
{
    unsigned short nId;
    unsigned short nX;
    unsigned short nY;
    unsigned short nP;
} TouchPoint_t;

/// max 80+1+1 = 82 bytes
typedef struct
{
    unsigned char nCount;
    unsigned char nKeyCode;
    TouchPoint_t tPoint[MAX_TOUCH_NUM];
} TouchInfo_t;

typedef struct
{
    unsigned short nFirmwareMode;
    unsigned char nType;
    unsigned char nLogModePacketHeader;
    unsigned char nMy;
    unsigned char nMx;
    unsigned char nSd;
    unsigned char nSs;
    unsigned short nLogModePacketLength;
} FirmwareInfo_t;


#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
/*
 * Note.
 * 0x0000 and 0xFFFF are not allowed to be defined as SW ID.
 * SW_ID_UNDEFINED is a reserved enum value, do not delete it or modify it.
 * Please modify the SW ID of the below enum value depends on the TP vendor that you are using.
 */
typedef enum {
    MSG26XXM_SW_ID_XXXX = 0x0001,
    MSG26XXM_SW_ID_YYYY = 0x0002,
    MSG26XXM_SW_ID_UNDEFINED
} Msg26xxmSwId_e;

/*
 * Note.
 * 0x0000 and 0xFFFF are not allowed to be defined as SW ID.
 * SW_ID_UNDEFINED is a reserved enum value, do not delete it or modify it.
 * Please modify the SW ID of the below enum value depends on the TP vendor that you are using.
 */
typedef enum {
    MSG28XX_SW_ID_XXXX = 0x0001,
    MSG28XX_SW_ID_YYYY = 0x0002,
    MSG28XX_SW_ID_UNDEFINED
} Msg28xxSwId_e;
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

/*--------------------------------------------------------------------------*/
/* GLOBAL FUNCTION DECLARATION                                              */
/*--------------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
extern void DrvFwCtrlOpenGestureWakeup(unsigned int *pMode);
extern void DrvFwCtrlCloseGestureWakeup(void);

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
extern void DrvFwCtrlOpenGestureDebugMode(unsigned char nGestureFlag);
extern void DrvFwCtrlCloseGestureDebugMode(void);
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

extern unsigned int DrvFwCtrlReadDQMemValue(unsigned short nAddr);
extern void DrvFwCtrlWriteDQMemValue(unsigned short nAddr, unsigned int nData);

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
extern void DrvFwCtrlCheckFirmwareUpdateBySwId(void);
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

extern unsigned short DrvFwCtrlChangeFirmwareMode(unsigned short nMode);
extern void DrvFwCtrlGetFirmwareInfo(FirmwareInfo_t *pInfo);
extern unsigned short DrvFwCtrlGetFirmwareMode(void);
extern void DrvFwCtrlRestoreFirmwareModeToLogDataMode(void);

#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
extern void DrvFwCtrlGetTouchPacketAddress(unsigned short *pDataAddress, unsigned short *pFlagAddress);
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
extern signed int DrvFwCtrlEnableProximity(void);
extern signed int DrvFwCtrlDisableProximity(void);
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

extern void DrvFwCtrlVariableInitialize(void);
extern void DrvFwCtrlOptimizeCurrentConsumption(void);
extern unsigned char DrvFwCtrlGetChipType(void);
extern void DrvFwCtrlGetCustomerFirmwareVersionByDbBus(EmemType_e eEmemType, unsigned short *pMajor, unsigned short *pMinor, unsigned char **ppVersion);
extern void DrvFwCtrlGetCustomerFirmwareVersion(unsigned short *pMajor, unsigned short *pMinor, unsigned char **ppVersion);
extern void DrvFwCtrlGetPlatformFirmwareVersion(unsigned char **ppVersion);
extern void DrvFwCtrlHandleFingerTouch(void);
extern signed int DrvFwCtrlUpdateFirmware(unsigned char szFwData[][1024], EmemType_e eEmemType);
extern signed int DrvFwCtrlUpdateFirmwareBySdCard(const char *pFilePath);

#ifdef CONFIG_ENABLE_HOTKNOT
extern void ReportHotKnotCmd(unsigned char *pPacket, unsigned short nLength);
#endif //CONFIG_ENABLE_HOTKNOT

#ifdef CONFIG_ENABLE_GLOVE_MODE
extern void DrvFwCtrlOpenGloveMode(void);
extern void DrvFwCtrlCloseGloveMode(void);
extern void DrvFwCtrlGetGloveInfo(unsigned char *pGloveMode);
#endif //CONFIG_ENABLE_GLOVE_MODE

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
extern void DrvFwCtrlChargerDetection(unsigned char nChargerStatus);
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#endif //CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC
        
#endif  /* __MSTAR_DRV_MUTUAL_FW_CONTROL_H__ */
