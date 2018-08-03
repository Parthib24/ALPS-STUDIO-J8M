////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2012 MStar Semiconductor, Inc.
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
 * @file    mstar_drv_hotknot.h
 *
 * @brief   This file defines the hotknot functions
 *
 *
 */

#ifndef __MSTAR_DRV_HOTKNOT_H__
#define __MSTAR_DRV_HOTKNOT_H__


////////////////////////////////////////////////////////////
/// Included Files
////////////////////////////////////////////////////////////
#include "mstar_drv_common.h"
#include "mstar_drv_hotknot_queue.h"


#ifdef CONFIG_ENABLE_HOTKNOT
////////////////////////////////////////////////////////////
/// Data Types
////////////////////////////////////////////////////////////
//typedef int bool;

//#define false   0
//#define true    !false


////////////////////////////////////////////////////////////
/// Function Prototype
////////////////////////////////////////////////////////////
extern void CreateHotKnotMem(void);
extern void DeleteHotKnotMem(void);


////////////////////////////////////////////////////////////
/// Data Types
////////////////////////////////////////////////////////////
typedef struct
{
    unsigned char          nCmdId;
    unsigned char         *pSndData;      //send data to fw  
    unsigned short         nSndLen;
    unsigned char         *pRcvData;      //receive data from fw
    unsigned short         nRcvLen;
    unsigned short        *pFwMode;
    signed int         nTimeOut;
} DrvCmd_t;


//hotknot cmd
typedef struct
{
    unsigned char         nHeader;
    unsigned char         nInstruction;
    unsigned char         szReserved[2];     
} HotKnotCmd_t;

typedef struct
{
    unsigned char         nHeader;
    unsigned char         nInstruction;
	unsigned char         nResult;
    unsigned char         szReserved[38];     
	unsigned char         nIdentify;    	
	unsigned char         nCheckSum;
} DemoHotKnotCmdRet_t;

typedef struct
{
    unsigned char         nHeader;
	unsigned char         nPacketLen_H;  
	unsigned char         nPacketLen_L;
	unsigned char         nType;    
    unsigned char         nInstruction;
	unsigned char         nResult;
  //unsigned char         szReserved[40];    //unknown size   	
  //unsigned char         nCheckSum;
} DebugHotKnotCmdRet_t;


//hotknot auth
typedef struct
{
    unsigned char         nHeader;
    unsigned char         nInstruction;
    unsigned char         szReserved[2];     
} HotKnotAuth_t;


//hotknot writecipher
typedef struct
{
    unsigned char         nHeader;
    unsigned char         nInstruction;      
	unsigned short        nSMBusAdr;
    unsigned char         szData[16];     
	unsigned char         nCheckSum;
} HotKnotWriteCipher_t;
//return 0 if success, return 1 if fail


//hotknot send
typedef struct
{
    unsigned char         nHeader;
    unsigned char         nInstruction;      
	unsigned short        nSMBusAdr;
    unsigned char         szData[129];     
	unsigned char         nDataLen_H;  
	unsigned char         nDataLen_L;  	
	unsigned char         nCheckSum;
} HotKnotSnd_t;

typedef struct
{
    unsigned char         nHeader;
	unsigned char         nPacketLen_H;  
	unsigned char         nPacketLen_L;  		
    unsigned char         nType;      	
    unsigned char         nInstruction;      
	unsigned char         nResult;
    unsigned char         szReserved[143];     
	unsigned char         nCheckSum;
} DemoHotKnotSndRet_t;

typedef struct
{
    unsigned char         nHeader;
	unsigned char         nPacketLen_H;  
	unsigned char         nPacketLen_L;  		
    unsigned char         nType;      	
    unsigned char         nInstruction;      
	unsigned char         nResult;
    unsigned char         szReserved[143];
    unsigned char         szDebug[100];    
	unsigned char         nCheckSum;
} DebugHotKnotSndRet_t;


//hotknot receive
typedef struct
{
    unsigned char         nHeader;
    unsigned char         nInstruction;
	unsigned char         nRequireDataLen_H;  
	unsigned char         nRequireDataLen_L;  	
} HotKnotRcv_t;

typedef struct
{
    unsigned char         nHeader;      	     
	unsigned char         nActualHotKnotLen_H;  
	unsigned char         nActualHotKnotLen_L;   
    unsigned char         szData[146];    
	unsigned char         nCheckSum;
} DemoHotKnotLibRcvRet_t;

typedef struct
{
    unsigned char         nHeader;
	unsigned char         nPacketLen_H;  
	unsigned char         nPacketLen_L;  		
    unsigned char         nType;      	
    unsigned char         szData[143];     
	unsigned char         nActualDataLen_H;  
	unsigned char         nActualDataLen_L;  		
	unsigned char         nCheckSum;
} DemoHotKnotRcvRet_t;

typedef struct
{
    unsigned char         nHeader;
	unsigned char         nPacketLen_H;  
	unsigned char         nPacketLen_L;  		
    unsigned char         nType;      	
    unsigned char         szData[143];     
	unsigned char         nActualDataLen_H;  
	unsigned char         nActualDataLen_L;
    unsigned char         szDebug[100];    
	unsigned char         nCheckSum;
} DebugHotKnotRcvRet_t;


//hotknot get queue data
typedef struct
{
    unsigned char         nHeader;
    unsigned char         nInstruction;
	unsigned char         nRequireDataLen_H;  
	unsigned char         nRequireDataLen_L;  	
} HotKnotGetQ_t;

typedef struct
{
    unsigned char         nHeader;
	unsigned short        nFront;  
	unsigned short        nRear;  		     	
    unsigned char         szData[HOTKNOT_QUEUE_SIZE];     
	unsigned char         nCheckSum;
} DemoHotKnotGetQRet_t;


////////////////////////////////////////////////////////////
/// Variables
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Macro
////////////////////////////////////////////////////////////
#define HOTKNOT_IOCTL_BASE               99
#define HOTKNOT_IOCTL_RUN_CMD            _IOWR(HOTKNOT_IOCTL_BASE, 0, long)
#define HOTKNOT_IOCTL_QUERY_VENDOR       _IOR('G', 28, char[30])


#define HOTKNOT_CMD                      0x60
#define HOTKNOT_AUTH                     0x61
#define HOTKNOT_SEND                     0x62
#define HOTKNOT_RECEIVE                  0x63


//HotKnot CMD
#define ENABLE_HOTKNOT                   0xA0 
#define DISABLE_HOTKNOT                  0xA1
#define ENTER_MASTER_MODE                0xA2 
#define EXIT_MASTER_MODE                 0xA3 
#define ENTER_SLAVE_MODE                 0xA4 
#define EXIT_SLAVE_MODE                  0xA5 
#define READ_PAIR_STATE                  0xA6 
#define EXIT_READ_PAIR_STATE             0xA7 
#define ENTER_TRANSFER_MODE              0xA8 
#define EXIT_TRANSFER_MODE               0xA9 
#define READ_DEPART_STATE                0xAA 

//HotKnot AUTH
#define AUTH_INIT                        0xB0 
#define AUTH_GETKEYINDEX                 0xB1 
#define AUTH_READSCRAMBLECIPHER          0xB2 

//Hotknot QueryVersion
#define QUERY_VERSION                    0xB3

//HotKnot Send
#define AUTH_WRITECIPHER                 0xC0 
#define SEND_DATA                        0xC1 
#define ADAPTIVEMOD_BEGIN                0xC3    //notice fw to begin adaptive modulation

//HotKnot Receive
#define RECEIVE_DATA                     0xD0 

//HotKnot Send Test
#define SEND_DATA_TEST                   0xC2 

//HotKnot Get Queue
#define GET_QUEUE                        0xD1    //decide the size to get queue data

//
#define DEMO_PD_PACKET_ID                0x5A    //only for PD state in demo mode
#define DEMO_PD_PACKET_IDENTIFY          0xC0    //only for PD state in demo mode
#define HOTKNOT_PACKET_ID                0xA7
#define HOTKNOT_PACKET_TYPE              0x41    //for command and send packet
#define HOTKNOT_RECEIVE_PACKET_TYPE      0x40

//send length
#define HOTKNOT_CMD_LEN                  4
#define HOTKNOT_AUTH_LEN                 4
#define HOTKNOT_WRITECIPHER_LEN          21
#define HOTKNOT_SEND_LEN                 136
#define HOTKNOT_RECEIVE_LEN              4      //TBD
#define HOTKNOT_MAX_DATA_LEN             128 
#define HOTKNOT_GETQUEUE_LEN             4 

//receive length
#define KEYINDEX_LEN                     4
#define QUERYVERSION_LEN                 4 
#define CIPHER_LEN                       16
#define DEMO_PD_PACKET_RET_LEN           43
#define MAX_PD_PACKET_RET_LEN            100    //TBD  
#define DEMO_HOTKNOT_SEND_RET_LEN        150
#define DEMO_HOTKNOT_RECEIVE_RET_LEN     150
#define DEBUG_HOTKNOT_SEND_RET_LEN       250
#define DEBUG_HOTKNOT_RECEIVE_RET_LEN    250

//result
#define RESULT_OK                        0
#define RESULT_FAIL                      1
#define RESULT_TIMEOUT                   2

//hotknot mode
#define HOTKNOT_BEFORE_TRANS_STATE       0x91    //read_pair_state success, fw send 43 bytes to driver
#define HOTKNOT_TRANS_STATE              0x92    //enter_transfer_mode success, fw send 150 bytes to driver
#define HOTKNOT_AFTER_TRANS_STATE        0x93    //exit_transfer_mode, fw send 43 bytes to driver
#define HOTKNOT_NOT_TRANS_STATE          0x94    //enter_slave_mode success, fw send 43 bytes to driver



////////////////////////////////////////////////////////////
/// COMPILE OPTION DEFINITION                                        
////////////////////////////////////////////////////////////
//#define CONFIG_ENABLE_HOTKNOT_RCV_BLOCKING



////////////////////////////////////////////////////////////
/// Function Prototypes
////////////////////////////////////////////////////////////
extern void ReportHotKnotCmd(unsigned char *pPacket, unsigned short nLength);
extern long HotKnotIoctl( struct file *pFile, unsigned int nCmd, unsigned long nArg );


#endif //CONFIG_ENABLE_HOTKNOT
#endif // __MSTAR_DRV_HOTKNOT_H__
