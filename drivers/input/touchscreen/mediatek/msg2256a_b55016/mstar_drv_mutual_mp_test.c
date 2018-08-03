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
 * @file    mstar_drv_mutual_mp_test.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "mstar_drv_mutual_mp_test.h"
#include "mstar_drv_utility_adaption.h"
#include "mstar_drv_mutual_fw_control.h"
#include "mstar_drv_platform_porting_layer.h"

#if defined(CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC)
#ifdef CONFIG_ENABLE_ITO_MP_TEST

#include "msg26xxm_open_test_X.h"
#include "msg26xxm_short_test_X.h"
#include "msg28xx_mp_test_X.h"
#include "msg28xx_mp_test_Y.h"

/*=============================================================*/
// PREPROCESSOR CONSTANT DEFINITION
/*=============================================================*/

// Modify.
#define TP_TYPE_X    (1) //(2)
#define TP_TYPE_Y    (4)

/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

extern unsigned int SLAVE_I2C_ID_DBBUS;
extern unsigned int SLAVE_I2C_ID_DWI2C;

extern unsigned char g_ChipType;

/*=============================================================*/
// GLOBAL VARIABLE DEFINITION
/*=============================================================*/

unsigned int g_IsInMpTest = 0;

/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/

static unsigned int _gTestRetryCount = CTP_MP_TEST_RETRY_COUNT;
static ItoTestMode_e _gItoTestMode = 0;

static signed int _gCtpMpTestStatus = ITO_TEST_UNDER_TESTING;

static unsigned short _gSenseLineNum = 0;
static unsigned short _gDriveLineNum = 0;
static unsigned short _gWaterProofNum = 0;

static struct work_struct _gCtpItoTestWork;
static struct workqueue_struct *_gCtpMpTestWorkQueue = NULL;
 
static signed int _gDeltaC[MAX_MUTUAL_NUM] = {0};
static signed int _gResult[MAX_MUTUAL_NUM] = {0};
static signed int _gDeltaCWater[12] = {0};
static signed int _gResultWater[12] = {0};
//static unsigned char _gMode[MAX_MUTUAL_NUM] = {0};
static signed int _gSenseR[MAX_CHANNEL_NUM] = {0};
static signed int _gDriveR[MAX_CHANNEL_NUM] = {0};
static signed int _gGRR[MAX_CHANNEL_NUM] = {0};
static signed int _gTempDeltaC[MAX_MUTUAL_NUM] = {0};

static unsigned char _gTestFailChannel[MAX_MUTUAL_NUM] = {0};
static unsigned int _gTestFailChannelCount = 0;

static unsigned char _gShortTestChannel[MAX_CHANNEL_NUM] = {0};

TestScopeInfo_t g_TestScopeInfo = {0};

static unsigned char _gTestAutoSwitchFlag = 1;
static unsigned char _gTestSwitchMode = 0;

unsigned short _gMuxMem_20_3E_0_Settings[16] = {0};
unsigned short _gMuxMem_20_3E_1_Settings[16] = {0};
unsigned short _gMuxMem_20_3E_2_Settings[16] = {0};
unsigned short _gMuxMem_20_3E_3_Settings[16] = {0};
unsigned short _gMuxMem_20_3E_4_Settings[16] = {0};
unsigned short _gMuxMem_20_3E_5_Settings[16] = {0};
unsigned short _gMuxMem_20_3E_6_Settings[16] = {0};


//Msg28xx TP Type Defibition
static unsigned short SENSE_NUM = 0;
static unsigned short DRIVE_NUM = 0;
static unsigned short KEY_NUM = 0;
static unsigned short KEY_LINE = 0;
static unsigned short GR_NUM = 0;
static unsigned short CSUB_REF = 0;
static unsigned short SENSE_MUTUAL_SCAN_NUM = 0;
static unsigned short MUTUAL_KEY = 0;
static unsigned short PATTERN_TYPE = 0;

static unsigned short SHORT_N1_TEST_NUMBER = 0;
static unsigned short SHORT_N2_TEST_NUMBER = 0;
static unsigned short SHORT_S1_TEST_NUMBER = 0;
static unsigned short SHORT_S2_TEST_NUMBER = 0;
static unsigned short SHORT_TEST_5_TYPE = 0;
static unsigned short SHORT_X_TEST_NUMBER = 0;
                                                                                                                                    
static unsigned short * SHORT_N1_TEST_PIN = NULL;
static unsigned short * SHORT_N1_MUX_MEM_20_3E = NULL;
static unsigned short * SHORT_N2_TEST_PIN = NULL;
static unsigned short * SHORT_N2_MUX_MEM_20_3E = NULL;
static unsigned short * SHORT_S1_TEST_PIN = NULL;
static unsigned short * SHORT_S1_MUX_MEM_20_3E = NULL;
static unsigned short * SHORT_S2_TEST_PIN = NULL;
static unsigned short * SHORT_S2_MUX_MEM_20_3E = NULL;
static unsigned short * SHORT_X_TEST_PIN = NULL;
static unsigned short * SHORT_X_MUX_MEM_20_3E = NULL;                                                           
                               
static unsigned short * PAD_TABLE_DRIVE = NULL;
static unsigned short * PAD_TABLE_SENSE = NULL;
static unsigned short * PAD_TABLE_GR = NULL;
                               
static unsigned char * KEYSEN = NULL;
static unsigned char * KEYDRV = NULL;
                                                            
//static unsigned char ** g_MapVaMutual;
static unsigned short _gTpType = 0;
static signed char _gDeepStandby = 0;


/*=============================================================*/
// LOCAL FUNCTION DEFINITION
/*=============================================================*/

static void _DrvMpTestItoTestMsg26xxmSetToNormalMode(void)
{
    unsigned short nRegData = 0;
    unsigned short nTmpAddr = 0, nAddr = 0;
    unsigned short nDriveNumGeg = 0, nSenseNumGeg = 0;
    unsigned short i = 0;

    DBG("*** %s() ***\n", __func__);

    RegSet16BitValue(0x0FE6, 0x0001); 

    nRegData = RegGet16BitValue(0x110E); 
    
    if (nRegData == 0x1D08)
    {
        DBG("Wrong mode 0\n");
    }
    
    nRegData &= 0x0800;
    
    if (nRegData > 1)
    {
        DBG("Wrong mode\n");
    }
    
    RegSet16BitValueOff(0x110E, 0x0800); 
    RegSet16BitValueOn(0x1116, 0x0005); 
    RegSet16BitValueOn(0x114A, 0x0001); 

    for (i = 0; i < 7; i ++)
    {
        nTmpAddr = 0x3C + i;	
        nTmpAddr = nTmpAddr * 2;
        nAddr = (0x11 << 8) | nTmpAddr;
        RegSet16BitValue(nAddr, MSG26XXM_open_ANA1_N_X[i]); 
    }
    
    RegSet16BitValue(0x1E66, 0x0000); 
    RegSet16BitValue(0x1E67, 0x0000); 
    RegSet16BitValue(0x1E68, 0x0000); 
    RegSet16BitValue(0x1E69, 0x0000); 
    RegSet16BitValue(0x1E6A, 0x0000); 
    RegSet16BitValue(0x1E6B, 0x0000); 
    
    for (i = 0; i < 21; i ++)
    {
        nTmpAddr = 3 + i;	
        nTmpAddr = nTmpAddr * 2;
        nAddr = (0x10 << 8) | nTmpAddr;
        RegSet16BitValue(nAddr, MSG26XXM_open_ANA3_N_X[i]); 
    }
    
    nDriveNumGeg = ((MSG26XXM_DRIVE_NUM - 1) << 8 & 0xFF00);
    nSenseNumGeg = (MSG26XXM_SENSE_NUM & 0x00FF);
    
    RegSet16BitValue(0x1216, nDriveNumGeg); 
    RegSet16BitValue(0x102E, nSenseNumGeg); 

    RegSet16BitValue(0x0FE6, 0x0001); 

    DBG("Wrong mode correction\n");
}

/*
static void _DrvMpTestItoTestMsg26xxmSetToWaterProofMode(void)
{
    unsigned short nTmpAddr = 0, nAddr = 0;
    unsigned short i = 0;
    unsigned short nCsubWPdata = 0;

    DBG("*** %s() ***\n", __func__);

    nCsubWPdata = (WATERPROOF_CSUB_REF << 8) | WATERPROOF_CSUB_REF;

    for (i = 0; i < 7; i ++)
    {
        nTmpAddr = 0x3C + i;
        nTmpAddr = nTmpAddr * 2;
        nAddr = (0x11 << 8) | nTmpAddr;
        RegSet16BitValue(nAddr, MSG26XXM_WATERPROOF_ANA1_N_X[i]);
    }

    for (i = 0; i < 21; i ++)
    {
        nTmpAddr = 3 + i;
        nTmpAddr = nTmpAddr * 2;
        nAddr = (0x10 << 8) | nTmpAddr;
        RegSet16BitValue(nAddr, MSG26XXM_WATERPROOF_ANA3_N_X[i]);
    }

    // ana2 write
    RegSet16BitValue(0x1216, 0x0000);

    for (i = 0; i < 6; i ++)
    {
        nTmpAddr = 0x20 + i;
        nTmpAddr = nTmpAddr * 2;
        nAddr = (0x10 << 8) | nTmpAddr;
        RegSet16BitValue(nAddr, nCsubWPdata);
    }

    RegSet16BitValueOn(0x1116, 0x0005);
    RegSet16BitValueOn(0x114A, 0x0001);
    RegSet16BitValue(0x1208, 0x0002);

    DBG("WaterProof : Wrong mode correction\n");
}
*/

void _ItoTestDebugShowArray(void *pBuf, unsigned short nLen, int nDataType, int nCarry, int nChangeLine)
{
    unsigned char * pU8Buf = NULL;
    signed char * pS8Buf = NULL;
    unsigned short * pU16Buf = NULL;
    signed short * pS16Buf = NULL;
    unsigned int * pU32Buf = NULL;
    signed int * pS32Buf = NULL;
    int i;

    if(nDataType == 8)
        pU8Buf = (unsigned char *)pBuf;    
    else if(nDataType == -8)
        pS8Buf = (signed char *)pBuf;    
    else if(nDataType == 16)
        pU16Buf = (unsigned short *)pBuf;    
    else if(nDataType == -16)
        pS16Buf = (signed short *)pBuf;    
    else if(nDataType == 32)
        pU32Buf = (unsigned int *)pBuf;    
    else if(nDataType == -32)
        pS32Buf = (signed int *)pBuf;    

    for(i=0; i < nLen; i++)
    {
        if(nCarry == 16)    
        {
            if(nDataType == 8)        
                DBG("%02X ", pU8Buf[i]);
            else if(nDataType == -8)        
                DBG("%02X ", pS8Buf[i]);
            else if(nDataType == 16)        
                DBG("%04X ", pU16Buf[i]);
            else if(nDataType == -16)        
                DBG("%04X ", pS16Buf[i]);
            else if(nDataType == 32)        
                DBG("%08X ", pU32Buf[i]);            
            else if(nDataType == -32)        
                DBG("%08X ", pS32Buf[i]);            
        }    
        else if(nCarry == 10)
        {
            if(nDataType == 8)
                DBG("%6d ", pU8Buf[i]);
            else if(nDataType == -8)
                DBG("%6d ", pS8Buf[i]);
            else if(nDataType == 16)
                DBG("%6d ", pU16Buf[i]);
            else if(nDataType == -16)
                DBG("%6d ", pS16Buf[i]);
            else if(nDataType == 32)
                DBG("%6d ", pU32Buf[i]);
            else if(nDataType == -32)
                DBG("%6d ", pS32Buf[i]);
        }
 
        if(i%nChangeLine == nChangeLine-1){  
            DBG("\n");
        }
    }
    DBG("\n");    
}

void _ItoTestDebugShowS32Array(signed int *pBuf, unsigned short nRow, unsigned short nCol)
{
    int i, j;

    for(j=0; j < nRow; j++)
    {
        for(i=0; i < nCol; i++)    
        {
            DBG("%4d ", pBuf[i * nRow + j]);       
        }
        DBG("\n");
    }
    DBG("\n");    
}

static void _DrvMpTestItoTestMsg26xxmMcuStop(void)
{
    DBG("*** %s() ***\n", __func__);

    RegSet16BitValue(0x0FE6, 0x0001); //bank:mheg5, addr:h0073
}

static void _DrvMpTestItoTestMsg26xxmAnaSwitchToMutual(void)
{
    unsigned short nTemp = 0;

    DBG("*** %s() ***\n", __func__);

    nTemp = RegGet16BitValue(0x114A); //bank:ana, addr:h0025
    nTemp |= BIT0;
    RegSet16BitValue(0x114A, nTemp);
    nTemp = RegGet16BitValue(0x1116); //bank:ana, addr:h000b
    nTemp |= (BIT2 | BIT0);
    RegSet16BitValue(0x1116, nTemp);
}

static unsigned short _DrvMpTestItoTestAnaGetMutualChannelNum(void)
{
    unsigned short nSenseLineNum = 0;
    unsigned short nRegData = 0;

    DBG("*** %s() ***\n", __func__);

    nRegData = RegGet16BitValue(0x102E); //bank:ana3, addr:h0017
    nSenseLineNum = nRegData & 0x000F;

    DBG("nSenseLineNum = %d\n", nSenseLineNum);

    return nSenseLineNum;
}

static unsigned short _DrvMpTestItoTestAnaGetMutualSubFrameNum(void)
{
    unsigned short nDriveLineNum = 0;
    unsigned short nRegData = 0;

    DBG("*** %s() ***\n", __func__);

    nRegData = RegGet16BitValue(0x1216); //bank:ana2, addr:h000b
    nDriveLineNum = ((nRegData & 0xFF00) >> 8) + 1; //Since we only retrieve 8th~12th bit of reg_m_sf_num, 0xFF00 shall be changed to 0x1F00. 

    DBG("nDriveLineNum = %d\n", nDriveLineNum);

    return nDriveLineNum;
}

/*
static void _DrvMpTestItoOpenTestMsg26xxmAnaGetMutualCSub(unsigned char *pMode)
{
    unsigned short i, j;
    unsigned short nSenseLineNum;
    unsigned short nDriveLineNum;
    unsigned short nTotalNum;
    unsigned char szDataAna4[3];    
    unsigned char szDataAna3[3];    
    unsigned char szDataAna41[ANA4_MUTUAL_CSUB_NUMBER]; //200 = 392 - 192  
    unsigned char szDataAna31[ANA3_MUTUAL_CSUB_NUMBER]; //192 = 14 * 13 + 10   
    unsigned char szModeTemp[MAX_MUTUAL_NUM];

    DBG("*** %s() ***\n", __func__);

    nTotalNum = MAX_MUTUAL_NUM;

    nSenseLineNum = _DrvMpTestItoTestAnaGetMutualChannelNum();
    nDriveLineNum = _DrvMpTestItoTestAnaGetMutualSubFrameNum();

    if (ANA4_MUTUAL_CSUB_NUMBER > 0)
    {
        mdelay(100);
        for (i = 0; i < ANA4_MUTUAL_CSUB_NUMBER; i ++)
        {	
            szDataAna41[i] = 0;
        }

        szDataAna4[0] = 0x10;
        szDataAna4[1] = 0x15; //bank:ana4, addr:h0000
        szDataAna4[2] = 0x00;
        
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDataAna4[0], 3);
        IicReadData(SLAVE_I2C_ID_DBBUS, &szDataAna41[0], ANA4_MUTUAL_CSUB_NUMBER); //200

        nTotalNum -= (unsigned short)ANA4_MUTUAL_CSUB_NUMBER;
    }

    for (i = 0; i < nTotalNum; i ++)
    {
        szDataAna31[i] = 0;
    }

    mdelay(100);

    szDataAna3[0] = 0x10;
    szDataAna3[1] = 0x10; //bank:ana3, addr:h0020
    szDataAna3[2] = 0x40;

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDataAna3[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &szDataAna31[0], ANA3_MUTUAL_CSUB_NUMBER); //192

    for (i = 0; i < ANA3_MUTUAL_CSUB_NUMBER; i ++)
    {
        szModeTemp[i] = szDataAna31[i];
    }

    for (i = ANA3_MUTUAL_CSUB_NUMBER; i < (ANA3_MUTUAL_CSUB_NUMBER + ANA4_MUTUAL_CSUB_NUMBER); i ++)
    {
        szModeTemp[i] = szDataAna41[i - ANA3_MUTUAL_CSUB_NUMBER];
    }
    
    for (i = 0; i < nDriveLineNum; i ++)
    {
        for (j = 0; j < nSenseLineNum; j ++)
        {
            _gMode[j * nDriveLineNum + i] = szModeTemp[i * MAX_CHANNEL_SEN + j];

//            DBG("_gMode[%d] = %d\n", j * nDriveLineNum + i, _gMode[j * nDriveLineNum + i]);
        }
    }
}
*/

static void _DrvMpTestItoOpenTestMsg26xxmAnaSetMutualCSub(unsigned short nCSub)
{
    unsigned short i = 0;
    unsigned char szDbBusTxData[256] = {0};

    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x15; //bank:ana4, addr:h0000
    szDbBusTxData[2] = 0x00;

    for (i = 3; i < (3+ANA4_MUTUAL_CSUB_NUMBER); i ++)
    {
        szDbBusTxData[i] = (unsigned char)nCSub;             
    }

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+ANA4_MUTUAL_CSUB_NUMBER);

    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x10; //bank:ana3, addr:h0020
    szDbBusTxData[2] = 0x40;

    for (i = 3; i < 3+ANA3_MUTUAL_CSUB_NUMBER; i ++)
    {
        szDbBusTxData[i] = (unsigned char)nCSub;             
    }

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+ANA3_MUTUAL_CSUB_NUMBER);	
}

static void _DrvMpTestItoTestMsg26xxmDisableFilterNoiseDetect(void)
{
    unsigned short nTemp = 0;

    DBG("*** %s() ***\n", __func__);

    nTemp = RegGet16BitValue(0x1302); //bank:fir, addr:h0001
    nTemp &= (~(BIT2 | BIT1 | BIT0));
    RegSet16BitValue(0x1302, nTemp);
}

static void _DrvMpTestItoTestMsg26xxmAnaSwReset(void)
{
    DBG("*** %s() ***\n", __func__);

    RegSet16BitValue(0x1100, 0xFFFF); //bank:ana, addr:h0000
    RegSet16BitValue(0x1100, 0x0000);
    mdelay(100);
}

static void _DrvMpTestItoTestMsg26xxmEnableAdcOneShot(void)
{
    DBG("*** %s() ***\n", __func__);

    RegSet16BitValue(0x130C, BIT15); //bank:fir, addr:h0006
    RegSet16BitValue(0x1214, 0x0031);
}

static void _DrvMpTestItoTestGetMutualOneShotRawIir(unsigned short wszResultData[][MAX_CHANNEL_DRV], unsigned short nDriveLineNum, unsigned short nSenseLineNum)
{
    unsigned short nRegData;
    unsigned short i, j;
    unsigned short nTemp;
    unsigned short nReadSize;
    unsigned char szDbBusTxData[3];
    unsigned char szShotData1[FILTER1_MUTUAL_DELTA_C_NUMBER]; //190 = (6 * 14 + 11) * 2
    unsigned char szShotData2[FILTER2_MUTUAL_DELTA_C_NUMBER]; //594 = (MAX_MUTUAL_NUM - (6 * 14 + 11)) * 2

    DBG("*** %s() ***\n", __func__);

    nTemp = RegGet16BitValue(0x3D08); //bank:intr_ctrl, addr:h0004
    nTemp &= (~(BIT8 | BIT4));
    RegSet16BitValue(0x3D08, nTemp);

    _DrvMpTestItoTestMsg26xxmEnableAdcOneShot();
    
    nRegData = 0;
    while (0x0000 == (nRegData & BIT8))
    {
        nRegData = RegGet16BitValue(0x3D18); //bank:intr_ctrl, addr:h000c
    }

    for (i = 0; i < FILTER1_MUTUAL_DELTA_C_NUMBER; i ++)
    {
        szShotData1[i] = 0;
    }
    
    for (i = 0; i < FILTER2_MUTUAL_DELTA_C_NUMBER; i ++)
    {
        szShotData2[i] = 0;
    }

    mdelay(100);
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x13; //bank:fir, addr:h0021
    szDbBusTxData[2] = 0x42;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &szShotData1[0], FILTER1_MUTUAL_DELTA_C_NUMBER); //190

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    mdelay(100);
    nReadSize = IicSegmentReadDataByDbBus(0x20, 0x00, &szShotData2[0], FILTER2_MUTUAL_DELTA_C_NUMBER, MAX_I2C_TRANSACTION_LENGTH_LIMIT); //594
    DBG("*** nReadSize = %d ***\n", nReadSize); // add for debug
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
    mdelay(100);
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x20; //bank:fir2, addr:h0000
    szDbBusTxData[2] = 0x00;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &szShotData2[0], FILTER2_MUTUAL_DELTA_C_NUMBER); //594
#endif

    for (j = 0; j < nDriveLineNum; j ++)
    {
        for (i = 0; i < nSenseLineNum; i ++)
        {
            // FILTER1 : SF0~SF5, AFE0~AFE13; SF6, AFE0~AFE10
            if ((j <= 5) || ((j == 6) && (i <= 10)))
            {
                nRegData = (unsigned short)(szShotData1[(j * 14 + i) * 2] | szShotData1[(j * 14 + i) * 2 + 1] << 8);
                wszResultData[i][ j] = (short)nRegData;
            }
            else
            {
                // FILTER2 : SF6, AFE11~AFE13
                if ((j == 6) && (i > 10))
                {
                    nRegData = (unsigned short)(szShotData2[((j - 6) * 14 + (i - 11)) * 2] | szShotData2[((j - 6) * 14 + (i - 11)) * 2 + 1] << 8);
                    wszResultData[i][j] = (short)nRegData;
                }
                else
                {
                    nRegData = (unsigned short)(szShotData2[6 + ((j - 7) * 14 + i) * 2] | szShotData2[6 + ((j - 7) * 14 + i) * 2 + 1] << 8);
                    wszResultData[i][j] = (short)nRegData;
                }
            }
        }
    }

    nTemp = RegGet16BitValue(0x3D08); //bank:intr_ctrl, addr:h0004
    nTemp |= (BIT8 | BIT4);
    RegSet16BitValue(0x3D08, nTemp);
}

static void _DrvMpTestItoTestMsg26xxmGetDeltaC(signed int *pTarget)
{
    signed short nTemp;
    unsigned short wszRawData[MAX_CHANNEL_SEN][MAX_CHANNEL_DRV];
    unsigned short i, j;
    unsigned short nDriveLineNum = 0, nSenseLineNum = 0, nShift = 0;

    DBG("*** %s() ***\n", __func__);

    nSenseLineNum = _DrvMpTestItoTestAnaGetMutualChannelNum();
    nDriveLineNum = _DrvMpTestItoTestAnaGetMutualSubFrameNum();

    _DrvMpTestItoTestGetMutualOneShotRawIir(wszRawData, nDriveLineNum, nSenseLineNum);

    for (i = 0; i < nSenseLineNum; i ++)
    {
        for (j = 0; j < nDriveLineNum; j ++)
        {
            nShift = (unsigned short)(i * nDriveLineNum + j);
            nTemp = (signed short)wszRawData[i][j];
            pTarget[nShift] = nTemp;

//            DBG("wszRawData[%d][%d] = %d\n", i, j, nTemp);
        }
    }
}

static signed int _DrvMpTestItoTestMsg26xxmReadTrunkFwVersion(unsigned int* pVersion)
{
    unsigned short nMajor = 0;
    unsigned short nMinor = 0;
    unsigned char szDbBusTxData[3] = {0};
    unsigned char szDbBusRxData[4] = {0};

    DBG("*** %s() ***\n", __func__);

    szDbBusTxData[0] = 0x53;
    szDbBusTxData[1] = 0x00;
    szDbBusTxData[2] = 0x24;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);

    DBG("szDbBusRxData[0] = 0x%x\n", szDbBusRxData[0]); // add for debug
    DBG("szDbBusRxData[1] = 0x%x\n", szDbBusRxData[1]); // add for debug
    DBG("szDbBusRxData[2] = 0x%x\n", szDbBusRxData[2]); // add for debug
    DBG("szDbBusRxData[3] = 0x%x\n", szDbBusRxData[3]); // add for debug

    nMajor = (szDbBusRxData[1]<<8) + szDbBusRxData[0];
    nMinor = (szDbBusRxData[3]<<8) + szDbBusRxData[2];

    DBG("*** major = %x ***\n", nMajor);
    DBG("*** minor = %x ***\n", nMinor);

    *pVersion = (nMajor << 16) + nMinor;

    return 0;
}

static signed int _DrvMpTestItoTestMsg26xxmGetSwitchFlag(void)
{
    unsigned int nFwVersion = 0;

    DBG("*** %s() ***\n", __func__);

    if (_DrvMpTestItoTestMsg26xxmReadTrunkFwVersion(&nFwVersion) < 0)
    {
        _gTestSwitchMode = 0;
        return 0;
    }

    if (nFwVersion >= 0x10030000)
    {
        _gTestSwitchMode = 1;
    }
    else
    {
        _gTestSwitchMode = 0;
    }

    return 0;
}

static signed int _DrvMpTestItoTestMsg26xxmCheckSwitchStatus(void)
{
    unsigned int nRegData = 0;
    int nTimeOut = 100;
    int nT = 0;

    DBG("*** %s() ***\n", __func__);

    do
    {
        nRegData = RegGet16BitValue(0x3CE4);
        mdelay(20);
        nT++;
        if (nT > nTimeOut)
        {
            return -1;
        }
        DBG("*** %s() nRegData:%x***\n", __func__ , nRegData);

    } while (nRegData != 0x7447);

    return 0;
}

static signed int _DrvMpTestItoTestMsg26xxmSwitchFwMode(unsigned char nFWMode)
{
    DBG("*** %s() ***\n", __func__);

    RegSet16BitValue(0x0FE6, 0x0001);    //MCU_stop
    mdelay(150);

    RegSet16BitValue(0X3C60, 0xAA55);    // disable watch dog

    RegSet16BitValue(0X3D08, 0xFFFF);
    RegSet16BitValue(0X3D18, 0xFFFF);

    RegSet16BitValue(0x3CE4, 0x7474);

    //RegSet16BitValue(0x1E04, 0x7D60);
    RegSet16BitValue(0x1E04, 0x829F);
    RegSet16BitValue(0x0FE6, 0x0000);
    mdelay(150);

    if (_DrvMpTestItoTestMsg26xxmCheckSwitchStatus() < 0)
    {
        DBG("*** Msg26xx MP Test# CheckSwitchStatus failed! ***\n");
        return -1;
    }

    switch (nFWMode)
    {
        case MUTUAL:
            RegSet16BitValue(0x3CE4, 0x5705);
            break;

        case SELF:
            RegSet16BitValue(0x3CE4, 0x6278);
            break;

        case WATERPROOF:
            RegSet16BitValue(0x3CE4, 0x7992);
            DBG("*** Msg26xx MP Test# WATERPROOF mode***\n");
            break;

        default:
            return -1;
    }
    if (_DrvMpTestItoTestMsg26xxmCheckSwitchStatus() < 0)
    {
        DBG("*** Msg26xx MP Test# CheckSwitchStatus failed! ***\n");
        return -1;
    }

    RegSet16BitValue(0x0FE6, 0x0001);// stop mcu
    RegSet16BitValue(0x3D08, 0xFEFF);//open timer

    return 0;
}

static signed int _DrvMpTestItoTestMsg26xxmSwitchMode(unsigned char nSwitchMode, unsigned char nFMode)
{
    if (_gTestSwitchMode != 0)
    {
        if (_DrvMpTestItoTestMsg26xxmSwitchFwMode(nFMode) < 0)
        {
            if (nFMode == MUTUAL)
            {
                _DrvMpTestItoTestMsg26xxmSetToNormalMode();
            }
            else
            {
                //_DrvMpTestItoTestMsg26xxmSetToWaterProofMode();
                DBG("*** Msg26xx MP Test# _DrvMpTestItoTestMsg26xxmSwitchMode failed! ***\n");
                return -1;
            }
        }
    }
    else
    {
        if (nFMode == MUTUAL)
        {
            _DrvMpTestItoTestMsg26xxmSetToNormalMode();
        }
        else
        {
            //_DrvMpTestItoTestMsg26xxmSetToWaterProofMode();
            DBG("*** Msg26xx MP Test# _DrvMpTestItoTestMsg26xxmSwitchMode failed! ***\n");
            return -1;
        }
    }
    
    return 0;
}

signed int _DrvMpTestMsg26xxmItoOpenTestEntry(void)
{
    signed int nRetVal = 0;
    signed int nPrev = 0, nDelta = 0;
    unsigned short i = 0, j = 0;

    DBG("*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DrvPlatformLyrDisableFingerTouchReport();

    DrvPlatformLyrTouchDeviceResetHw();

    //auto detetc fw version to decide switch fw mode
    if (_gTestAutoSwitchFlag != 0)
    {
        _DrvMpTestItoTestMsg26xxmGetSwitchFlag();
    }

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    _DrvMpTestItoTestMsg26xxmSwitchMode(_gTestSwitchMode, MUTUAL);

    _DrvMpTestItoTestMsg26xxmMcuStop();
    mdelay(10);

    for (i = 0; i < MAX_MUTUAL_NUM; i ++)
    {
        _gTestFailChannel[i] = 0;
    }	

    _gTestFailChannelCount = 0; // Reset _gTestFailChannelCount to 0 before test start

    _gSenseLineNum = _DrvMpTestItoTestAnaGetMutualChannelNum();
    _gDriveLineNum = _DrvMpTestItoTestAnaGetMutualSubFrameNum();

    _DrvMpTestItoOpenTestMsg26xxmAnaSetMutualCSub(MSG26XXM_OPEN_CSUB_REF_X);
    _DrvMpTestItoTestMsg26xxmDisableFilterNoiseDetect();
    
    // Set charge&dump time 
    RegSet16BitValue(0x1224, 0xFFC0);
    RegSet16BitValue(0x122A, 0x0C0A);

    _DrvMpTestItoTestMsg26xxmAnaSwReset();
    _DrvMpTestItoTestMsg26xxmGetDeltaC(_gDeltaC);
    
    for (i = 0; i < _gSenseLineNum; i ++)
    {
        DBG("\nSense[%02d]\t", i);
        
        for (j = 0; j < _gDriveLineNum; j ++)
        {
            _gResult[i * _gDriveLineNum + j] = (4464*MSG26XXM_OPEN_CSUB_REF_X - _gDeltaC[i * _gDriveLineNum + j]);
            DBG("%d  %d  %d\t", _gResult[i * _gDriveLineNum + j], 4464*MSG26XXM_OPEN_CSUB_REF_X, _gDeltaC[i * _gDriveLineNum + j]);
        }
    }
    
    DBG("\n\n\n");

//    for (j = 0; j < _gDriveLineNum; j ++)
    for (j = 0; j < (_gDriveLineNum-1); j ++)
    {
        for (i = 0; i < _gSenseLineNum; i ++)
        {
            if (_gResult[i * _gDriveLineNum + j] < FIR_THRESHOLD)
            {
                _gTestFailChannel[i * _gDriveLineNum + j] = 1;
                _gTestFailChannelCount ++; 
                nRetVal = -1;
                DBG("\nSense%d, Drive%d, MIN_Threshold = %d\t", i, j, _gResult[i * _gDriveLineNum + j]);
            }

            if (i > 0)
            {
                nDelta = _gResult[i * _gDriveLineNum + j] > nPrev ? (_gResult[i * _gDriveLineNum + j] - nPrev) : (nPrev - _gResult[i * _gDriveLineNum + j]);
                if (nDelta > nPrev*FIR_RATIO/100)
                {
                    if (0 == _gTestFailChannel[i * _gDriveLineNum + j]) // for avoid _gTestFailChannelCount to be added twice
                    {
                        _gTestFailChannel[i * _gDriveLineNum + j] = 1;
                        _gTestFailChannelCount ++; 
                    }
                    nRetVal = -1;
                    DBG("\nSense%d, Drive%d, MAX_Ratio = %d,%d\t", i, j, nDelta, nPrev);
                }
            }
            nPrev = _gResult[i * _gDriveLineNum + j];
        }
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    
    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    DrvPlatformLyrEnableFingerTouchReport();

    return nRetVal;
}

static void _DrvMpTestItoOpenTestSetMutualCsubViaDBbus(signed short nCSub)
{   
    unsigned char nBaseLen = 6;
    unsigned short nFilter = 0x3F;
    unsigned short nLastFilter = 0xFFF;
    unsigned char nBasePattern = nCSub & nFilter; 
    unsigned char nPattern;
    unsigned short n16BitsPattern;
    unsigned short nCSub16Bits[5] = {0};
    int i;

    DBG("*** %s() ***\n", __func__);

    for(i=0; i<5; i++)
    {
        if(i == 0)
        {
            nPattern = nBasePattern;    //Patn => Pattern
        }
         
        n16BitsPattern = ((nPattern & 0xF) << nBaseLen*2) | (nPattern << nBaseLen) | nPattern;

        if(i == 4)
        {
            nCSub16Bits[i] = n16BitsPattern & nLastFilter;
        }
        else
        {
            nCSub16Bits[i] = n16BitsPattern;
        }
        nPattern = (unsigned char)((n16BitsPattern >> 4) & nFilter);
    }    
        
    RegSet16BitValue(0x215C, 0x1FFF);

    for (i = 0; i < 5; i++)
    {
        RegSet16BitValue(0x2148 + 2 * i, nCSub16Bits[i]);
        RegSet16BitValue(0x2152 + 2 * i, nCSub16Bits[i]);
    }     
}

/*
static void _DrvMpTestItoOpenTestAFEGainOne(void)
{
    unsigned char nRegData = 0;
    unsigned short nAFECoef = 0;

    DBG("*** %s() ***\n", __func__);

    // AFE gain = 1X
    RegSet16BitValue(0x1318, 0x4440);
    RegSet16BitValue(0x131A, 0x4444);
    RegSet16BitValue(0x13D6, 0x2000);
    
    RegSet16BitValue(0x2160, 0x0040);
    RegSet16BitValue(0x2162, 0x0040);
    RegSet16BitValue(0x2164, 0x0040);
    RegSet16BitValue(0x2166, 0x0040);
    RegSet16BitValue(0x2168, 0x0040);
    RegSet16BitValue(0x216A, 0x0040);
    RegSet16BitValue(0x216C, 0x0040);
    RegSet16BitValue(0x216E, 0x0040);
    RegSet16BitValue(0x2170, 0x0040);
    RegSet16BitValue(0x2172, 0x0040);
    RegSet16BitValue(0x2174, 0x0040);
    RegSet16BitValue(0x2176, 0x0040);
    RegSet16BitValue(0x2178, 0x0040);
    RegSet16BitValue(0x217A, 0x1FFF);
    RegSet16BitValue(0x217C, 0x1FFF);
    
    /// reg_hvbuf_sel_gain
    RegSet16BitValue(0x1564, 0x0077);
    
    /// all AFE Cfb use defalt (50p)
    RegSet16BitValue(0x1508, 0x1FFF);// all AFE Cfb: SW control
    RegSet16BitValue(0x1550, 0x0000);// all AFE Cfb use defalt (50p)
    
    ///ADC: AFE Gain bypass
    RegSet16BitValue(0x1260, 0x1FFF);

    //AFE coef
    nRegData = RegGetLByteValue(0x101A);
    nAFECoef = (unsigned short)(0x10000/nRegData);
    RegSet16BitValue(0x13D6, nAFECoef);        
}
*/

static void _DrvMpTestItoOpenTestAFEGainOne(void)
{
    // AFE gain = 1X
    unsigned short nAfeGain = 0;
    unsigned short nDriOpening = 0;
    unsigned char nRegData = 0;
    unsigned short nAfeCoef = 0;
    unsigned short i = 0;


    //proto.MstarReadReg(loopDevice, (uint)0x1312, ref regdata); //get dri num
    nRegData = RegGetLByteValue(0x1312);    
    nDriOpening = nRegData;

    ///filter unit gain
    if (nDriOpening == 11 || nDriOpening == 15)
    {
        RegSet16BitValue(0x1318, 0x4470);
    }
    else if (nDriOpening == 7)
    {
        RegSet16BitValue(0x1318, 0x4460);
    }

    //proto.MstarWriteReg_16(loopDevice, 0x131A, 0x4444);
    RegSet16BitValue(0x131A, 0x4444);

    ///AFE coef
    //proto.MstarReadReg(loopDevice, (uint)0x101A, ref regdata);
    nRegData = RegGetLByteValue(0x101A); 
    nAfeCoef = 0x10000 / nRegData;
    //proto.MstarWriteReg_16(loopDevice, (uint)0x13D6, AFE_coef);
    RegSet16BitValue(0x13D6, nAfeCoef);

    ///AFE gain
    if (nDriOpening == 7 || nDriOpening == 15)
    {
        nAfeGain = 0x0040;
    }    
    else if (nDriOpening == 11)
    {
        nAfeGain = 0x0055;
    }
    
    for (i = 0; i < 13; i++)
    {
        RegSet16BitValue(0x2160 + 2 * i, nAfeGain);
    }

    ///AFE gain: over write enable
    RegSet16BitValue(0x217A, 0x1FFF);
    RegSet16BitValue(0x217C, 0x1FFF);

    /// all AFE Cfb use defalt (50p)
    RegSet16BitValue(0x1508, 0x1FFF);// all AFE Cfb: SW control
    RegSet16BitValue(0x1550, 0x0000);// all AFE Cfb use defalt (50p)

    /// reg_hvbuf_sel_gain
    RegSet16BitValue(0x1564, 0x0077);

    ///ADC: AFE Gain bypass
    RegSet16BitValue(0x1260, 0x1FFF);
}

static void _DrvMpTestItoOpenTestCalibrateMutualCsub(signed short nCSub)
{   
    unsigned char nChipVer;

    DBG("*** %s() ***\n", __func__);

    nChipVer = RegGetLByteValue(0x1ECE);
    DBG("*** Msg28xx Open Test# Chip ID = %d ***\n", nChipVer);    
    
    if (nChipVer != 0)
        RegSet16BitValue(0x10F0, 0x0004);//bit2
    
    _DrvMpTestItoOpenTestSetMutualCsubViaDBbus(nCSub);
    _DrvMpTestItoOpenTestAFEGainOne();    
}

static void _DrvMpTestItoTestDBBusReadDQMemStart(void)
{
    unsigned char nParCmdSelUseCfg = 0x7F;
    unsigned char nParCmdAdByteEn0 = 0x50;
    unsigned char nParCmdAdByteEn1 = 0x51;
    unsigned char nParCmdDaByteEn0 = 0x54;
    unsigned char nParCmdUSetSelB0 = 0x80;
    unsigned char nParCmdUSetSelB1 = 0x82;
    unsigned char nParCmdSetSelB2  = 0x85;
    unsigned char nParCmdIicUse    = 0x35;
    //unsigned char nParCmdWr        = 0x10;

    DBG("*** %s() ***\n", __func__);
    
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSelUseCfg, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn0, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn1, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdDaByteEn0, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdUSetSelB0, 1);        
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdUSetSelB1, 1); 
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSetSelB2,  1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdIicUse,    1);        
}

/*
static void _DrvMpTestItoTestDBBusReadDQMemStartAddr24(void)
{
    unsigned char nParCmdSelUseCfg = 0x7F;
//    unsigned char nParCmdAdByteEn0 = 0x50;
//    unsigned char nParCmdAdByteEn1 = 0x51;
    unsigned char nParCmdAdByteEn2 = 0x52;    
//    unsigned char nParCmdDaByteEn0 = 0x54;
    unsigned char nParCmdUSetSelB0 = 0x80;
    unsigned char nParCmdUSetSelB1 = 0x82;
    unsigned char nParCmdSetSelB2  = 0x85;
    unsigned char nParCmdIicUse    = 0x35;
    //unsigned char nParCmdWr        = 0x10;

    DBG("*** %s() ***\n", __func__);
    
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSelUseCfg, 1);
    //IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn0, 1);
    //IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn1, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn2, 1);    
    //IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdDaByteEn0, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdUSetSelB0, 1);        
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdUSetSelB1, 1); 
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSetSelB2,  1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdIicUse,    1);        
}
*/

static void _DrvMpTestItoTestDBBusReadDQMemEnd(void)
{
    unsigned char nParCmdNSelUseCfg = 0x7E;    
    
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdNSelUseCfg, 1);
}

/*
static void _DrvMpTestItoTestDBBusReadDQMemEndAddr24(void)
{
    unsigned char nParCmdSelUseCfg  = 0x7F;
    unsigned char nParCmdAdByteEn1  = 0x51;
    unsigned char nParCmdSetSelB0   = 0x81;
    unsigned char nParCmdSetSelB1   = 0x83;    
    unsigned char nParCmdNSetSelB2  = 0x84;
    unsigned char nParCmdIicUse     = 0x35;
    unsigned char nParCmdNSelUseCfg = 0x7E;
    unsigned char nParCmdNIicUse    = 0x34;

    DBG("*** %s() ***\n", __func__);
    RegSetLByteValue(0, 0);       

    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSelUseCfg, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn1, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSetSelB0, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSetSelB1, 1);    
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdNSetSelB2, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdIicUse, 1);        
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdNSelUseCfg, 1); 
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdNIicUse,  1);
}
*/

static void _DrvMpTestItoTestMsg28xxEnableAdcOneShot(void)
{
    RegSet16BitValueOn(0x100a, BIT0);

    return;
}

static signed int _DrvMpTestItoTestMsg28xxTriggerMutualOneShot(signed short * pResultData, unsigned short * pSenNum, unsigned short * pDrvNum)
{    
    unsigned short nAddr = 0x5000, nAddrNextSF = 0x1A4;
    unsigned short nSF = 0, nAfeOpening = 0, nDriOpening = 0;
    unsigned short nMaxDataNumOfOneSF = 0;
    unsigned short nDriMode = 0;
    int nDataShift = -1;
    unsigned short i, j, k;    
    unsigned char nRegData = 0;
    unsigned char nShotData[392] = {0};//13*15*2
    unsigned short nRegDataU16 = 0;
    signed short * pShotDataAll = NULL;
    unsigned char nParCmdIicUse    = 0x35;


    DBG("*** %s() ***\n", __func__);
        
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdIicUse, 1);        
    nRegData = RegGetLByteValue(0x130A);
    nSF = nRegData>>4;
    nAfeOpening = nRegData & 0x0f;
    
    if (nSF == 0)
    {
        return -1;
    }
    
    nRegData = RegGetLByteValue(0x100B);
    nDriMode = nRegData;
    
    nRegData = RegGetLByteValue(0x1312);
    nDriOpening = nRegData;

    DBG("*** Msg28xx MP Test# TriggerMutualOneShot nSF=%d, nAfeOpening=%d, nDriMode=%d, nDriOpening=%d. ***\n", nSF, nAfeOpening, nDriMode, nDriOpening);
    
    nMaxDataNumOfOneSF = nAfeOpening * nDriOpening;
    
    pShotDataAll = kzalloc(sizeof(signed short) * nSF * nMaxDataNumOfOneSF, GFP_KERNEL);

    RegSet16BitValueOff(0x3D08, BIT8);      ///FIQ_E_FRAME_READY_MASK
    
    ///polling frame-ready interrupt status
    _DrvMpTestItoTestMsg28xxEnableAdcOneShot();
    
    while (0x0000 == (nRegDataU16 & BIT8))
    {
        nRegDataU16 = RegGet16BitValue(0x3D18);
    }
    
    if (nDriMode == 2) // for short test
    {
        if (nAfeOpening % 2 == 0)
            nDataShift = -1;
        else
            nDataShift = 0;    //special case    
        //signed short nShortResultData[nSF][nAfeOpening];
        
        /// get ALL raw data
        for (i = 0; i < nSF; i++)
        {
            _DrvMpTestItoTestDBBusReadDQMemStart();
            RegGetXBitValue(nAddr + i * nAddrNextSF, nShotData, 28, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
            _DrvMpTestItoTestDBBusReadDQMemEnd();

            //_ItoTestDebugShowArray(nShotData, 26, 8, 16, 16);   
            for (j = 0; j < nAfeOpening; j++)
            {
                pResultData[i*MAX_CHANNEL_DRV+j] = (signed short)(nShotData[2 * j] | nShotData[2 * j + 1] << 8);

                if (nDataShift == 0 && (j == nAfeOpening-1))
                {
                    pResultData[i*MAX_CHANNEL_DRV+j] = (signed short)(nShotData[2 * (j + 1)] | nShotData[2 * (j + 1) + 1] << 8);                
                }
            }
        }

        *pSenNum = nSF;
        *pDrvNum = nAfeOpening;         
    }
    else // for open test
    {
        //signed short nOpenResultData[nSF * nAfeOpening][nDriOpening];
    
        if (nAfeOpening % 2 == 0 || nDriOpening % 2 == 0)
            nDataShift = -1;
        else
            nDataShift = 0;    //special case

        /// get ALL raw data, combine and handle datashift.
        for (i = 0; i < nSF; i++)
        {        
            _DrvMpTestItoTestDBBusReadDQMemStart();
            RegGetXBitValue(nAddr + i * nAddrNextSF, nShotData, 392, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
            _DrvMpTestItoTestDBBusReadDQMemEnd();
          
            //_ItoTestDebugShowArray(nShotData, 390, 8, 10, 16);  
            for (j = 0; j < nMaxDataNumOfOneSF; j++)
            {
                pShotDataAll[i*nMaxDataNumOfOneSF+j] = (signed short)(nShotData[2 * j] | nShotData[2 * j + 1] << 8);
    
                if (nDataShift == 0 && j == (nMaxDataNumOfOneSF - 1))
                    pShotDataAll[i*nMaxDataNumOfOneSF+j] = (signed short)(nShotData[2 * (j + 1)] | nShotData[2 * (j + 1) + 1] << 8);
            }
        }
        
        //problem here
        for (k = 0; k < nSF; k++)
        {
            for (i = k * nAfeOpening; i < nAfeOpening * (k + 1); i++) //Sen
            {
                for (j = 0; j < nDriOpening; j++) //Dri
                {
                    pResultData[i*MAX_CHANNEL_DRV+j] = pShotDataAll[k*nMaxDataNumOfOneSF + (j + (i - nAfeOpening * k) * nDriOpening)]; //resultData[Sen, Dri]
                }
            }
        }

        *pSenNum = nSF * nAfeOpening;
        *pDrvNum = nDriOpening;        
    }
    RegSet16BitValueOn(0x3D08, BIT8);      ///FIQ_E_FRAME_READY_MASK
    RegSet16BitValueOn(0x3D08, BIT4);      ///FIQ_E_TIMER0_MASK

    kfree(pShotDataAll);

    return 0;
}

static signed int _DrvMpTestItoTestMsg28xxGetMutualOneShotRawIIR(signed short * nResultData, unsigned short * pSenNum, unsigned short * pDrvNum)
{
    return _DrvMpTestItoTestMsg28xxTriggerMutualOneShot(nResultData, pSenNum, pDrvNum); 
}

static signed int _DrvMpTestItoTestMsg28xxGetDeltaC(signed int *pDeltaC)
{        
    signed short * pRawData = NULL;
    signed short nRawDataOverlapDone[SENSE_NUM][DRIVE_NUM];
    //signed short nDeltaC[MAX_MUTUAL_NUM] = {0};
    unsigned short nDrvPos = 0, nSenPos = 0, nShift = 0;    
    unsigned short nSenNumBak = 0;
    unsigned short nDrvNumBak = 0;    
    signed short i, j;
    
    DBG("*** %s() ***\n", __func__);

    pRawData = kzalloc(sizeof(signed short) * MAX_CHANNEL_SEN*2 * MAX_CHANNEL_DRV, GFP_KERNEL);

    if(_DrvMpTestItoTestMsg28xxGetMutualOneShotRawIIR(pRawData, &nSenNumBak, &nDrvNumBak) < 0)
    {
        DBG("*** Msg28xx Open Test# GetMutualOneShotRawIIR failed! ***\n");      
        return -1;
    }

    DBG("*** Msg28xx Open Test# nSenNumBak=%d nDrvNumBak=%d ***\n", nSenNumBak, nDrvNumBak); 

    for (i = 0; i < nSenNumBak; i++)
    {
        for (j = 0; j < nDrvNumBak; j++)
        {
            nShift = (unsigned short)(i * nDrvNumBak + j);
            
            if (_gTpType == TP_TYPE_X)
            {
                nDrvPos = g_MapVaMutual_X[nShift][1];            
                nSenPos = g_MapVaMutual_X[nShift][0];            
            }
            else if (_gTpType == TP_TYPE_Y)
            {
                nDrvPos = g_MapVaMutual_Y[nShift][1];            
                nSenPos = g_MapVaMutual_Y[nShift][0];            
            }
            
            if (nDrvPos != 0xFF && nSenPos != 0xFF)
            {
                nRawDataOverlapDone[nSenPos][nDrvPos] = pRawData[i*MAX_CHANNEL_DRV+j];
            }
        }
    }
  
    for (i = 0; i < _gSenseLineNum; i++)
    {
        for (j = 0; j < _gDriveLineNum; j++)
        {
            nShift = (unsigned short)(i * _gDriveLineNum + j);
            pDeltaC[nShift] = (signed int)nRawDataOverlapDone[i][j];
        }
    }
  
    DBG("*** Msg28xx Open Test# gDeltaC ***\n");
    _ItoTestDebugShowArray(pDeltaC, _gSenseLineNum * _gDriveLineNum, -32, 10, _gSenseLineNum);  

    kfree(pRawData);

    return 0;
}

static void _DrvMpTestItoTestMsg28xxAnaSwReset(void)
{
    DBG("*** %s() ***\n", __func__);
    
    /// reset ANA
    RegSet16BitValueOn(0x1002, (BIT0 | BIT1 | BIT2 | BIT3));     ///reg_tgen_soft_rst: 1 to reset
    RegSet16BitValueOff(0x1002, (BIT0 | BIT1 | BIT2 | BIT3));
    
    /// delay
    mdelay(20);
}

static signed int _DrvMpTestMsg28xxItoOpenTest(void)
{
    DBG("*** %s() ***\n", __func__);

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001);

    _DrvMpTestItoOpenTestCalibrateMutualCsub(CSUB_REF);
    RegSet16BitValue(0x156A, 0x000A); ///DAC com voltage
    _DrvMpTestItoTestMsg28xxAnaSwReset();
   

    if(_DrvMpTestItoTestMsg28xxGetDeltaC(_gDeltaC) < 0)
    {
        DBG("*** Msg28xx Open Test# GetDeltaC failed! ***\n");    
        return -1;
    } 
    
    return 0;
}

static unsigned char _DrvMpTestItoTestCheckValueInRange(signed int nValue, signed int nMax, signed int nMin)
{
   	if (nValue <= nMax && nValue >= nMin)
   	{
   	   	return 1;
   	}
   	else
   	{
   	   	return 0;
   	}
}

static signed int _DrvMpTestItoOpenTestMsg28xxOpenJudge(unsigned short nItemID, signed char pNormalTestResult[][2], unsigned short pNormalTestResultCheck[][13]/*, unsigned short nDriOpening*/)
{
    signed int nRetVal = 0;
    unsigned short nCSub = CSUB_REF;
    unsigned short nRowNum = 0, nColumnNum = 0;
    unsigned int nSum=0, nAvg=0, nDelta=0, nPrev=0;
    unsigned short i, j, k;

    DBG("*** %s() ***\n", __func__);
    
    for (i = 0; i < _gSenseLineNum * _gDriveLineNum; i++)
    {
        //deltaC_result[i] = deltaC[i];
        //_gResult[i] = 1673 * nCSub - _gDeltaC[i] * 2 / (nDriOpening + 1);
        if (_gDeltaC[i] > 31000)
        {
            return -1; 
        }
        
        _gResult[i] = 1673 * nCSub - _gDeltaC[i];
    
        // For mutual key, last column if not be used, show number "one".
        if ((MUTUAL_KEY == 1 || MUTUAL_KEY == 2) && (KEY_NUM != 0))
        {
            if ((_gSenseLineNum < _gDriveLineNum) && ((i + 1) % _gDriveLineNum == 0))
            {
                _gResult[i] = -32000;    
                for (k = 0; k < KEY_NUM; k++)
                    if ((i + 1) / _gDriveLineNum == KEYSEN[k])
                    {
                        //_gResult[i] = 1673 * nCSub - _gDeltaC[i] * 2 / (nDriOpening + 1);
                        _gResult[i] = 1673 * nCSub - _gDeltaC[i];
                    }    
            }
    
            if ((_gSenseLineNum > _gDriveLineNum) && (i > (_gSenseLineNum - 1) * _gDriveLineNum - 1))
            {
                _gResult[i] = -32000;        
                for (k = 0; k < KEY_NUM; k++)
                    if (((i + 1) - (_gSenseLineNum - 1) * _gDriveLineNum) == KEYSEN[k])
                    {
                        //_gResult[i] = 1673 * nCSub - _gDeltaC[i] * 2 / (nDriOpening + 1);
                        _gResult[i] = 1673 * nCSub - _gDeltaC[i];
                    }    
            }
        }
    }

    if(KEY_NUM > 0){
        if(_gDriveLineNum >= _gSenseLineNum){
            nRowNum = _gDriveLineNum-1;
            nColumnNum = _gSenseLineNum;
        }else{
            nRowNum = _gDriveLineNum;
            nColumnNum = _gSenseLineNum-1;
        }
    }else{
        nRowNum = _gDriveLineNum;
        nColumnNum = _gSenseLineNum;
    }

    DBG("*** Msg28xx Open Test# Show _gResult ***\n"); 
    //_ItoTestDebugShowArray(_gResult, nRowNum*nColumnNum, -32, 10, nColumnNum);
    for (j = 0; j < _gDriveLineNum; j ++)
    {
        for (i = 0; i < _gSenseLineNum; i ++)
        {
            DBG("%d  ", _gResult[i * _gDriveLineNum + j]);
        }
        DBG("\n");                
    } 

    for (j = 0; j < nRowNum; j ++)
    {
        nSum = 0;
        for (i = 0; i < nColumnNum; i++)
        {
             nSum = nSum + _gResult[i * _gDriveLineNum + j];                               
        } 
        
        nAvg = nSum / nColumnNum;             
        DBG("*** Msg28xx Open Test# OpenJudge average=%d ***\n", nAvg);        
        for (i = 0; i < nColumnNum; i ++)
        {
   	   	    if (0 == _DrvMpTestItoTestCheckValueInRange(_gResult[i * _gDriveLineNum + j], (signed int)(nAvg + nAvg * DC_RANGE/100), (signed int)(nAvg - nAvg * DC_RANGE/100)))
   	   	    {
                _gTestFailChannel[i * _gDriveLineNum + j] = 1;
                _gTestFailChannelCount ++; 
                nRetVal = -1;
            }
    
            if (i > 0)
            {
                nDelta = _gResult[i * _gDriveLineNum + j] > nPrev ? (_gResult[i * _gDriveLineNum + j] - nPrev) : (nPrev - _gResult[i * _gDriveLineNum + j]);
                if (nDelta > nPrev*FIR_RATIO/100)
                {
                    if (0 == _gTestFailChannel[i * _gDriveLineNum + j]) // for avoid _gTestFailChannelCount to be added twice
                    {
                        _gTestFailChannel[i * _gDriveLineNum + j] = 1;
                        _gTestFailChannelCount ++; 
                    }
                    nRetVal = -1;
                    DBG("\nSense%d, Drive%d, MAX_Ratio = %d,%d\t", i, j, nDelta, nPrev);
                }
            }
            nPrev = _gResult[i * _gDriveLineNum + j];
        }
    }  

    //DBG("*** Msg28xx Open Test# OpenJudge _gTestFailChannelCount=%d ***\n", _gTestFailChannelCount);    
    return nRetVal;
}

static signed int _DrvMpTestItoTestCheckSwitchStatus(void)
{
    unsigned int nRegData = 0;
    int nTimeOut = 280;
    int nT = 0;

    do
    {
        nRegData = RegGet16BitValue(0x1402);
        mdelay(20);
        nT++;
        if (nT > nTimeOut)
        {
            return -1;
        }

    } while (nRegData != 0x7447);

    return 0;
}

static signed int _DrvMpTestMsg28xxItoTestSwitchFwMode(unsigned char nFMode)
{
    DBG("*** %s() ***\n", __func__);

   
    RegSet16BitValue(0x0FE6, 0x0001);    //MCU_stop
    mdelay(100);    
    RegSet16BitValue(0X3C60, 0xAA55);    // disable watch dog

    RegSet16BitValue(0X3D08, 0xFFFF);
    RegSet16BitValue(0X3D18, 0xFFFF);
            
    RegSet16BitValue(0x1402, 0x7474);

    RegSet16BitValue(0x1E06, 0x0000);
    RegSet16BitValue(0x1E06, 0x0001);
    //RegSet16BitValue((uint)0x1E04, (uint)0x7D60);
    //RegSet16BitValue(0x1E04, 0x829F);
    RegSet16BitValue(0x0FE6, 0x0000);
    mdelay(150);

    if (_DrvMpTestItoTestCheckSwitchStatus()<0)
    {
        DBG("*** Msg28xx MP Test# CheckSwitchStatus failed! Enter MP mode failed ***\n");    
        return -1;
    }

    if (_gDeepStandby == 0)
    {
        //deep satndby mode
        RegSet16BitValue(0x1402, 0x6179);
        mdelay(600);

        DbBusEnterSerialDebugMode();
        DbBusWaitMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

        if (_DrvMpTestItoTestCheckSwitchStatus()<0)
        {
            _gDeepStandby = -1;
            DBG("*** Msg28xx MP Test# Deep standby fail, fw not support DEEP STANDBY ***\n");    
            return -1;
        }
    }

    switch (nFMode)
    {
        case MUTUAL:
            RegSet16BitValue(0x1402, 0x5705);
            break;

        case SELF:
            RegSet16BitValue(0x1402, 0x6278);
            break;

        case WATERPROOF:
            RegSet16BitValue(0x1402, 0x7992);
            break;

        case MUTUAL_SINGLE_DRIVE:
            RegSet16BitValue(0x1402, 0x0158);
            break;

        default:
            return -1;
    }
    if (_DrvMpTestItoTestCheckSwitchStatus()<0)
    {
        DBG("*** Msg28xx MP Test# CheckSwitchStatus failed! Enter FW mode failed  ***\n");     
        return -1;
    }

    RegSet16BitValue(0x0FE6, 0x0001);// stop mcu
    RegSet16BitValue(0x3D08, 0xFEFF);//open timer

    return 0;
}

unsigned short _DrvMpTestMsg28xxItoTestGetTpType(void)
{
    unsigned short nMajor = 0, nMinor = 0;
    unsigned char szDbBusTxData[3] = {0};
    unsigned char szDbBusRxData[4] = {0};

    DBG("*** %s() ***\n", __func__);

    szDbBusTxData[0] = 0x03;
    
    IicWriteData(SLAVE_I2C_ID_DWI2C, &szDbBusTxData[0], 1);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szDbBusRxData[0], 4);
    
    nMajor = (szDbBusRxData[1]<<8) + szDbBusRxData[0];
    nMinor = (szDbBusRxData[3]<<8) + szDbBusRxData[2];

    DBG("*** major = %d ***\n", nMajor);
    DBG("*** minor = %d ***\n", nMinor);

    return nMajor;
}

static unsigned short _DrvMpTestMsg28xxItoTestChooseTpType(void)
{
    //unsigned short nTpType = 0;
    unsigned int i = 0;

    DBG("*** %s() ***\n", __func__);

    SENSE_NUM = 0;
    DRIVE_NUM = 0;
    KEY_NUM = 0;
    KEY_LINE = 0;
    GR_NUM = 0;
    CSUB_REF = 0;
    SENSE_MUTUAL_SCAN_NUM = 0;
    MUTUAL_KEY = 0;
    PATTERN_TYPE = 0;
    
    SHORT_N1_TEST_NUMBER = 0;
    SHORT_N2_TEST_NUMBER = 0;
    SHORT_S1_TEST_NUMBER = 0;
    SHORT_S2_TEST_NUMBER = 0;
    SHORT_TEST_5_TYPE = 0;
    SHORT_X_TEST_NUMBER = 0;
                                                                                                                                        
    SHORT_N1_TEST_PIN = NULL;
    SHORT_N1_MUX_MEM_20_3E = NULL;
    SHORT_N2_TEST_PIN = NULL;
    SHORT_N2_MUX_MEM_20_3E = NULL;
    SHORT_S1_TEST_PIN = NULL;
    SHORT_S1_MUX_MEM_20_3E = NULL;
    SHORT_S2_TEST_PIN = NULL;
    SHORT_S2_MUX_MEM_20_3E = NULL;
    SHORT_X_TEST_PIN = NULL;
    SHORT_X_MUX_MEM_20_3E = NULL;                                                           
                                   
    PAD_TABLE_DRIVE = NULL;
    PAD_TABLE_SENSE = NULL;
    PAD_TABLE_GR = NULL;
                                   
    KEYSEN = NULL;
    KEYDRV = NULL;
                                                                
    //g_MapVaMutual = NULL;

    for (i = 0; i < 10; i ++)
    {
        _gTpType = _DrvMpTestMsg28xxItoTestGetTpType();
        DBG("TP Type = %d, i = %d\n", _gTpType, i);

        if (TP_TYPE_X == _gTpType || TP_TYPE_Y == _gTpType) // Modify.
        {
            break;
        }
        else if (i < 5)
        {
            mdelay(100);  
        }
        else
        {
            DrvPlatformLyrTouchDeviceResetHw();
        }
    }
    
    if (TP_TYPE_X == _gTpType) // Modify. 
    {
        DBG("*** Choose Tp Type X ***\n");        
        SENSE_NUM = SENSE_NUM_X;
        DRIVE_NUM = DRIVE_NUM_X;
        KEY_NUM = KEY_NUM_X;
        KEY_LINE = KEY_LINE_X;
        GR_NUM = GR_NUM_X;
        CSUB_REF = CSUB_REF_X;
        SENSE_MUTUAL_SCAN_NUM = SENSE_MUTUAL_SCAN_NUM_X;
        MUTUAL_KEY = MUTUAL_KEY_X;
        PATTERN_TYPE = PATTERN_TYPE_X;
    
        SHORT_N1_TEST_NUMBER = SHORT_N1_TEST_NUMBER_X;
        SHORT_N2_TEST_NUMBER = SHORT_N2_TEST_NUMBER_X;
        SHORT_S1_TEST_NUMBER = SHORT_S1_TEST_NUMBER_X;
        SHORT_S2_TEST_NUMBER = SHORT_S2_TEST_NUMBER_X;
        SHORT_TEST_5_TYPE = SHORT_TEST_5_TYPE_X;
        SHORT_X_TEST_NUMBER = SHORT_X_TEST_NUMBER_X;
                                                                                                                                            
        SHORT_N1_TEST_PIN = MSG28XX_SHORT_N1_TEST_PIN_X;
        SHORT_N1_MUX_MEM_20_3E = SHORT_N1_MUX_MEM_20_3E_X;
        SHORT_N2_TEST_PIN = MSG28XX_SHORT_N2_TEST_PIN_X;
        SHORT_N2_MUX_MEM_20_3E = SHORT_N2_MUX_MEM_20_3E_X;
        SHORT_S1_TEST_PIN = MSG28XX_SHORT_S1_TEST_PIN_X;
        SHORT_S1_MUX_MEM_20_3E = SHORT_S1_MUX_MEM_20_3E_X;
        SHORT_S2_TEST_PIN = MSG28XX_SHORT_S2_TEST_PIN_X;
        SHORT_S2_MUX_MEM_20_3E = SHORT_S2_MUX_MEM_20_3E_X;
        SHORT_X_TEST_PIN = MSG28XX_SHORT_X_TEST_PIN_X;
        SHORT_X_MUX_MEM_20_3E = SHORT_X_MUX_MEM_20_3E_X;                                                           
                                       
        PAD_TABLE_DRIVE = PAD_TABLE_DRIVE_X;
        PAD_TABLE_SENSE = PAD_TABLE_SENSE_X;
        PAD_TABLE_GR = PAD_TABLE_GR_X;
                                       
        KEYSEN = KEYSEN_X;
        KEYDRV = KEYDRV_X;
                                                                    
        //g_MapVaMutual = g_MapVaMutual_X;
    }
    else if (TP_TYPE_Y == _gTpType) // Modify. 
    {
        DBG("*** Choose Tp Type Y ***\n");        
        SENSE_NUM = SENSE_NUM_Y;
        DRIVE_NUM = DRIVE_NUM_Y;
        KEY_NUM = KEY_NUM_Y;
        KEY_LINE = KEY_LINE_Y;
        GR_NUM = GR_NUM_Y;
        CSUB_REF = CSUB_REF_Y;
        SENSE_MUTUAL_SCAN_NUM = SENSE_MUTUAL_SCAN_NUM_Y;
        MUTUAL_KEY = MUTUAL_KEY_Y;
        PATTERN_TYPE = PATTERN_TYPE_Y;
    
        SHORT_N1_TEST_NUMBER = SHORT_N1_TEST_NUMBER_Y;
        SHORT_N2_TEST_NUMBER = SHORT_N2_TEST_NUMBER_Y;
        SHORT_S1_TEST_NUMBER = SHORT_S1_TEST_NUMBER_Y;
        SHORT_S2_TEST_NUMBER = SHORT_S2_TEST_NUMBER_Y;
        SHORT_TEST_5_TYPE = SHORT_TEST_5_TYPE_Y;
        SHORT_X_TEST_NUMBER = SHORT_X_TEST_NUMBER_Y;
                                                                                                                                            
        SHORT_N1_TEST_PIN = MSG28XX_SHORT_N1_TEST_PIN_Y;
        SHORT_N1_MUX_MEM_20_3E = SHORT_N1_MUX_MEM_20_3E_Y;
        SHORT_N2_TEST_PIN = MSG28XX_SHORT_N2_TEST_PIN_Y;
        SHORT_N2_MUX_MEM_20_3E = SHORT_N2_MUX_MEM_20_3E_Y;
        SHORT_S1_TEST_PIN = MSG28XX_SHORT_S1_TEST_PIN_Y;
        SHORT_S1_MUX_MEM_20_3E = SHORT_S1_MUX_MEM_20_3E_Y;
        SHORT_S2_TEST_PIN = MSG28XX_SHORT_S2_TEST_PIN_Y;
        SHORT_S2_MUX_MEM_20_3E = SHORT_S2_MUX_MEM_20_3E_Y;
        SHORT_X_TEST_PIN = MSG28XX_SHORT_X_TEST_PIN_Y;
        SHORT_X_MUX_MEM_20_3E = SHORT_X_MUX_MEM_20_3E_Y;                                                           
                                       
        PAD_TABLE_DRIVE = PAD_TABLE_DRIVE_Y;
        PAD_TABLE_SENSE = PAD_TABLE_SENSE_Y;
        PAD_TABLE_GR = PAD_TABLE_GR_Y;
                                       
        KEYSEN = KEYSEN_Y;
        KEYDRV = KEYDRV_Y;
                                                                    
        //g_MapVaMutual = g_MapVaMutual_Y;
    }
    else
    {
        _gTpType = 0;
    }
    
    return _gTpType;
}

signed int _DrvMpTestMsg28xxItoOpenTestEntry(void)
{
    signed int nRetVal = 0;
//    unsigned char nDrvOpening = 0;
    //unsigned short nCheckState = 0;
    unsigned short nTime = 0;    
    signed char nNormalTestResult[8][2] = {{0}};    //0:golden    1:ratio
    unsigned short nNormalTestResultCheck[6][13] = {{0}};        //6:max subframe    13:max afe
    
    DBG("*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    _gDeepStandby = 0;
    DrvPlatformLyrDisableFingerTouchReport();
    DrvPlatformLyrTouchDeviceResetHw();

    if (!_DrvMpTestMsg28xxItoTestChooseTpType())
    {
        DBG("Choose Tp Type failed\n");
        nRetVal = -2;
        goto ITO_TEST_END;
    }

    _gSenseLineNum = SENSE_NUM;        
    _gDriveLineNum = DRIVE_NUM;

_retry_open:
    DrvPlatformLyrTouchDeviceResetHw();

    //reset_only
    DbBusResetSlave();
    DbBusEnterSerialDebugMode();
    //DbBusWaitMCU();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);    

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001);

    if(_DrvMpTestMsg28xxItoTestSwitchFwMode(MUTUAL) < 0)
    {
        nTime++;
        if(nTime < 5)
        {
            goto _retry_open;
        }    
        DBG("*** Msg28xx Open Test# Switch Fw Mode failed! ***\n");
        nRetVal = -1;
        goto ITO_TEST_END;
    }

    //nDrvOpening = RegGetLByteValue(0x1312);

    if(_DrvMpTestMsg28xxItoOpenTest() < 0)
    {
        DBG("*** Msg28xx Open Test# OpenTest failed! ***\n");
        nRetVal = -1;
        goto ITO_TEST_END;
    }
    
    mdelay(10);

    nRetVal = _DrvMpTestItoOpenTestMsg28xxOpenJudge(0, nNormalTestResult, nNormalTestResultCheck/*, nDrvOpening*/);
    DBG("*** Msg28xx Open Test# OpenTestOpenJudge return value = %d ***\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
    
    ITO_TEST_END:    

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);
    
    DrvPlatformLyrEnableFingerTouchReport();

    return nRetVal;
}

signed int _DrvMpTestItoOpenTest(void)
{
    signed int nRetVal = -1;    

    DBG("*** %s() ***\n", __func__);
    
    if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        nRetVal = _DrvMpTestMsg26xxmItoOpenTestEntry();
    }    
    else if (g_ChipType == CHIP_TYPE_MSG28XX)
    {
        nRetVal = _DrvMpTestMsg28xxItoOpenTestEntry();
    }

    return nRetVal;
}

static void _DrvMpTestItoTestSendDataIn(unsigned short nAddr, unsigned short nLength, unsigned short *data)
{
    unsigned char szDbBusTxData[256] = {0};
    int i = 0;

    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = (nAddr >> 8) & 0xFF;
    szDbBusTxData[2] = (nAddr & 0xFF);

    for (i = 0; i <= nLength ; i ++)
    {
        szDbBusTxData[3+2*i] = (data[i] & 0xFF);
        szDbBusTxData[4+2*i] = (data[i] >> 8) & 0xFF;
    }

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+nLength*2);
}

static void _DrvMpTestItoShortTestMsg26xxmSetPAD2GPO(unsigned char nItemID)
{
    unsigned short gpioSetting[MAX_CHANNEL_NUM] = {0};
    unsigned short gpioEnabling[MAX_CHANNEL_NUM] = {0};
    unsigned short gpioZero[MAX_CHANNEL_NUM] = {0};
    unsigned char 	gpioNum = 0;
    unsigned short *gpioPIN = NULL;
    int i = 0;
    int j = 0;

    DBG("*** %s() ***\n", __func__);

    if (nItemID == 1)
    {
        gpioNum = SHORT_N1_GPO_NUMBER_X;
        gpioPIN = kzalloc(sizeof(unsigned short) * gpioNum, GFP_KERNEL);

        for (i = 0; i <	gpioNum; i++)
        {
            gpioPIN[i] = SHORT_N1_GPO_PIN_X[i];
        }
    }
    else if (nItemID == 2)
    {
        gpioNum = SHORT_N2_GPO_NUMBER_X;
        gpioPIN = kzalloc(sizeof(unsigned short) * gpioNum, GFP_KERNEL);
    
        for (i = 0; i <	gpioNum; i++)
        {
            gpioPIN[i] = SHORT_N2_GPO_PIN_X[i];
        }
    }
    else if (nItemID == 3)
    {
        gpioNum = SHORT_S1_GPO_NUMBER_X;
        gpioPIN = kzalloc(sizeof(unsigned short) * gpioNum, GFP_KERNEL);
		
        for (i = 0; i <	gpioNum; i++)
        {
            gpioPIN[i] = SHORT_S1_GPO_PIN_X[i];
        }
    }
    else if (nItemID == 4)
    {
        gpioNum = SHORT_S2_GPO_NUMBER_X;
        gpioPIN = kzalloc(sizeof(unsigned short) * gpioNum, GFP_KERNEL);
		
        for (i = 0; i <	gpioNum; i++)
        {
            gpioPIN[i] = SHORT_S2_GPO_PIN_X[i];
        }
    }
    DBG("ItemID %d, gpioNum %d",nItemID, gpioNum);

    for (i = 0; i < MAX_CHANNEL_NUM; i++)
    {
        gpioEnabling[i] = 0xFFFF;
    }

    for (i = 0; i < gpioNum; i++)
    {
        gpioSetting[gpioPIN[i] / 16] |= (unsigned short)(1 << (gpioPIN[i] % 16));
        gpioEnabling[gpioPIN[i] / 16] &= (unsigned short)(~(1 << (gpioPIN[i] % 16)));
    }

    ///cts sw overwrite
    {
        _DrvMpTestItoTestSendDataIn(0x1E66, gpioNum, &gpioSetting[0]);   ///who -> will be controlled
        _DrvMpTestItoTestSendDataIn(0x1E6C, gpioNum, &gpioEnabling[0]);   ///who -> enable sw overwrite
        _DrvMpTestItoTestSendDataIn(0x1E72, gpioNum, &gpioZero[0]);       ///who -> set2GPO
        _DrvMpTestItoTestSendDataIn(0x1E78, gpioNum, &gpioZero[0]);       ///who -> GPO2GND
    }

    for (j = 0; j < gpioNum; j++)
    {
        if (PIN_GUARD_RING == gpioPIN[j])
        {
            unsigned short u16RegData;
            u16RegData = RegGet16BitValue(0x1E12);
            u16RegData = ((u16RegData & 0xFFF9) | BIT0);
            RegSet16BitValue(0x1E12, u16RegData);
        }
    }

    kfree(gpioPIN);
}

static void _DrvMpTestItoShortTestMsg26xxmChangeANASetting(unsigned char nItemID)
{
    unsigned short SHORT_MAP_ANA1[7] = {0};
    unsigned short SHORT_MAP_ANA2[1] = {0};
    unsigned short SHORT_MAP_ANA3[21] = {0};
    int i = 0;

    DBG("*** %s() ***\n", __func__);

    if (nItemID == 1)
    {
        for (i = 0; i <	7; i++)
        {
            SHORT_MAP_ANA1[i] = short_ANA1_N1_X[i];
        }
        
        for (i = 0; i < 21; i++)
        {
            SHORT_MAP_ANA3[i] = short_ANA3_N1_X[i];
        }

        SHORT_MAP_ANA2[0] = short_ANA2_N1_X[0];
    }
    else if (nItemID == 2)
    {
        for (i = 0; i <	7; i++)
        {
            SHORT_MAP_ANA1[i] = short_ANA1_N2_X[i];
        }
		
        for (i = 0; i < 21; i++)
        {
            SHORT_MAP_ANA3[i] = short_ANA3_N2_X[i];
        }

        SHORT_MAP_ANA2[0] = short_ANA2_N2_X[0];
    }
    else if (nItemID == 3)
    {
        for (i = 0; i <	7; i++)
        {
            SHORT_MAP_ANA1[i] = short_ANA1_S1_X[i];
        }
        
        for (i = 0; i < 21; i++)
        {
            SHORT_MAP_ANA3[i] = short_ANA3_S1_X[i];
        }

        SHORT_MAP_ANA2[0] = short_ANA2_S1_X[0];
    }
    else if (nItemID == 4)
    {
        for (i = 0; i <	7; i++)
        {
            SHORT_MAP_ANA1[i] = short_ANA1_S2_X[i];
        }
        
        for (i = 0; i < 21; i++)
        {
            SHORT_MAP_ANA3[i] = short_ANA3_S2_X[i];
        }

        SHORT_MAP_ANA2[0] = short_ANA2_S2_X[0];
    }

    ///change ANA setting
    {
        _DrvMpTestItoTestSendDataIn(0x1178, 7, &SHORT_MAP_ANA1[0]);		///ANA1_3C_42
        _DrvMpTestItoTestSendDataIn(0x1216, 1, &SHORT_MAP_ANA2[0]);   	///ANA2_0B
        _DrvMpTestItoTestSendDataIn(0x1006, 21, &SHORT_MAP_ANA3[0]);    ///ANA3_03_17
    }
}

static void _DrvMpTestItoShortTestMsg26xxmAnaFixPrs(unsigned short nOption)
{
    unsigned short nTemp = 0;

    DBG("*** %s() ***\n", __func__);

    nTemp = RegGet16BitValue(0x1208); //bank:ana2, addr:h000a
    nTemp &= 0x00F1;
    nTemp |= (unsigned short)((nOption << 1) & 0x000E);
    RegSet16BitValue(0x1208, nTemp);
}

static void _DrvMpTestItoShortTestMsg26xxmSetNoiseSensorMode(unsigned char nEnable)
{
    DBG("*** %s() ***\n", __func__);

    if (nEnable)
    {
        RegSet16BitValueOn(0x110E, BIT11);
        RegSet16BitValueOff(0x1116, BIT2);
    }
    else
    {
        RegSet16BitValueOff(0x110E, BIT11);
    }
}

static void _DrvMpTestItoShortTestMsg26xxmAndChangeCDtime(unsigned short nTime1, unsigned short nTime2)
{
    DBG("*** %s() ***\n", __func__);

    RegSet16BitValue(0x1224, nTime1);
    RegSet16BitValue(0x122A, nTime2);
}

static void _DrvMpTestMsg26xxmItoShortTest(unsigned char nItemID)
{
    int i;

    DBG("*** %s() ***\n", __func__);

    _DrvMpTestItoTestMsg26xxmMcuStop();
    _DrvMpTestItoShortTestMsg26xxmSetPAD2GPO(nItemID);
    _DrvMpTestItoShortTestMsg26xxmChangeANASetting(nItemID);
    _DrvMpTestItoTestMsg26xxmAnaSwitchToMutual();
    _DrvMpTestItoShortTestMsg26xxmAnaFixPrs(7);
    _DrvMpTestItoTestMsg26xxmDisableFilterNoiseDetect();
    _DrvMpTestItoShortTestMsg26xxmSetNoiseSensorMode(1);
    _DrvMpTestItoShortTestMsg26xxmAndChangeCDtime(SHORT_Charge_X, SHORT_Dump1_X);
    _DrvMpTestItoTestMsg26xxmAnaSwReset();
    _DrvMpTestItoTestMsg26xxmGetDeltaC(_gTempDeltaC);

    _DrvMpTestItoTestMsg26xxmMcuStop();
    _DrvMpTestItoShortTestMsg26xxmSetPAD2GPO(nItemID);
    _DrvMpTestItoShortTestMsg26xxmChangeANASetting(nItemID);
    _DrvMpTestItoTestMsg26xxmAnaSwitchToMutual();
    _DrvMpTestItoShortTestMsg26xxmAnaFixPrs(7);
    _DrvMpTestItoTestMsg26xxmDisableFilterNoiseDetect();
    _DrvMpTestItoShortTestMsg26xxmSetNoiseSensorMode(1);
    _DrvMpTestItoShortTestMsg26xxmAndChangeCDtime(SHORT_Charge_X, SHORT_Dump2_X);
    _DrvMpTestItoTestMsg26xxmAnaSwReset();
    _DrvMpTestItoTestMsg26xxmGetDeltaC(_gDeltaC);

    for (i = 0; i < MAX_MUTUAL_NUM ; i++)
    {
        if ((_gDeltaC[i] <= -(IIR_MAX)) || (_gTempDeltaC[i] <= -(IIR_MAX)) || (_gDeltaC[i] >= (IIR_MAX)) || (_gTempDeltaC[i] >= (IIR_MAX)))
        {
            _gDeltaC[i] = 0x7FFF;
        }
        else
        {
            _gDeltaC[i] = abs(_gDeltaC[i] - _gTempDeltaC[i]);
        }
        //DBG("ItemID%d, MUTUAL_NUM %d, _gDeltaC = %d\t", nItemID, i, _gDeltaC[i]);
    }
    DBG("\n");
}

static signed int _DrvMpTestItoShortTestCovertRValue(signed int nValue)
{
   	if (nValue == 0)
   	{
   	   	nValue = 1;
   	}   	   	   	

   	if (nValue >= IIR_MAX)
   	{
   	   	return 0;
   	}

   	return ((500*11398) / (nValue));
}

static ItoTestResult_e _DrvMpTestItoShortTestMsg26xxmJudge(unsigned char nItemID)
{
   	ItoTestResult_e nRetVal = ITO_TEST_OK;
   	unsigned char nTestPinLength = 0;
   	unsigned short i = 0;
   	unsigned char nGpioNum = 0;
   	unsigned char* pTestGpio = NULL;

    DBG("*** %s() ***\n", __func__);

   	if (nItemID == 1)
   	{
   	   	nGpioNum = SHORT_N1_TEST_NUMBER_X;
   	   	pTestGpio = kzalloc(sizeof(unsigned char) * nGpioNum, GFP_KERNEL);
   	   	
   	   	for (i = 0; i <	nGpioNum; i++)
   	   	{
   	   	   	pTestGpio[i] = SHORT_N1_TEST_PIN_X[i];
   	   	}
   	}
   	else if (nItemID == 2)
   	{
   	   	nGpioNum = SHORT_N2_TEST_NUMBER_X;
   	   	pTestGpio = kzalloc(sizeof(unsigned char) * nGpioNum, GFP_KERNEL);
   	   	
   	   	for (i = 0; i <	nGpioNum; i++)
   	   	{
   	   	   	pTestGpio[i] = SHORT_N2_TEST_PIN_X[i];
   	   	}
   	}
   	else if (nItemID == 3)
   	{
   	   	nGpioNum = SHORT_S1_TEST_NUMBER_X;
   	   	pTestGpio = kzalloc(sizeof(unsigned char) * nGpioNum, GFP_KERNEL);
   	   	
   	   	for (i = 0; i <	nGpioNum; i++)
   	   	{
   	   	   	pTestGpio[i] = SHORT_S1_TEST_PIN_X[i];
   	   	}
   	}
   	else if (nItemID == 4)
   	{
   	   	nGpioNum = SHORT_S2_TEST_NUMBER_X;
   	   	pTestGpio = kzalloc(sizeof(unsigned char) * nGpioNum, GFP_KERNEL);
   	   	
   	   	for (i = 0; i <	nGpioNum; i++)
   	   	{
   	   	   	pTestGpio[i] = SHORT_S2_TEST_PIN_X[i];
   	   	}
   	}

   	nTestPinLength = nGpioNum;

   	for (i = 0;i < nTestPinLength; i++)
   	{
   	   	_gShortTestChannel[i] = pTestGpio[i];

   	   	if (0 == _DrvMpTestItoTestCheckValueInRange(_gDeltaC[i], SHORT_VALUE, -SHORT_VALUE))
   	   	{
   	   	   	nRetVal = ITO_TEST_FAIL;
   	   	   	_gTestFailChannelCount++;
   	   	   	DBG("_gShortTestChannel i = %d, _gDeltaC = %d\t", i, _gDeltaC[i]);
   	   	}
   	}
   	
   	kfree(pTestGpio);

   	return nRetVal;
}

static ItoTestResult_e _DrvMpTestMsg26xxmItoShortTestEntry(void)
{
    ItoTestResult_e nRetVal1 = ITO_TEST_OK, nRetVal2 = ITO_TEST_OK, nRetVal3 = ITO_TEST_OK, nRetVal4 = ITO_TEST_OK, nRetVal5 = ITO_TEST_OK;
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned short nTestPinCount = 0;
    signed int nShortThreshold = 0;

    DBG("*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DrvPlatformLyrDisableFingerTouchReport();
    
    DrvPlatformLyrTouchDeviceResetHw();

	  //auto detetc fw version to decide switch fw mode
    if (_gTestAutoSwitchFlag!=0)
    {
        _DrvMpTestItoTestMsg26xxmGetSwitchFlag();
    }

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

  	_DrvMpTestItoTestMsg26xxmSwitchMode(_gTestSwitchMode, MUTUAL);

    for (i = 0; i < MAX_CHANNEL_NUM; i ++)
    {
        _gShortTestChannel[i] = 0;
    }

    for (i = 0; i < MAX_MUTUAL_NUM; i ++)
    {
        _gDeltaC[i] = 0;
    }

    _gSenseLineNum = _DrvMpTestItoTestAnaGetMutualChannelNum();
    _gDriveLineNum = _DrvMpTestItoTestAnaGetMutualSubFrameNum();

    for (i = 0; i < MAX_CHANNEL_NUM; i ++)
    {
        _gShortTestChannel[i] = 0xff;
    }

    _gTestFailChannelCount = 0;

    nTestPinCount = 0; // Reset nTestPinCount to 0 before test start

    //N1_ShortTest

    if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        _DrvMpTestMsg26xxmItoShortTest(1); 
    }

    nRetVal2 = _DrvMpTestItoShortTestMsg26xxmJudge(1);

    for (i = 0; i < MAX_CHANNEL_NUM; i++)
    {
        if (_gShortTestChannel[i] != 0)
        {
            nTestPinCount++;
        }
    }

    for (i = 0; i < nTestPinCount; i++)
    {
        for (j = 0; j < _gSenseLineNum; j++)
        {
            if (_gShortTestChannel[i] == SENSE_X[j])
            {
                _gSenseR[j] = _DrvMpTestItoShortTestCovertRValue(_gDeltaC[i]);

                DBG("_gSenseR[%d] = %d\t", j , _gSenseR[j]);
            }
        }
    }
    DBG("\n");

    //clear
    for (i = 0; i < MAX_CHANNEL_NUM; i ++)
    {
        _gShortTestChannel[i] = 0xff;
    }

    nTestPinCount = 0;

    //N2_ShortTest

    if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        _DrvMpTestMsg26xxmItoShortTest(2);
    }

    nRetVal3 = _DrvMpTestItoShortTestMsg26xxmJudge(2);

    for (i = 0; i < MAX_CHANNEL_NUM; i++)
    {
        if (_gShortTestChannel[i] != 0)
        {
            nTestPinCount++;
        }            
    }

    for (i = 0; i < nTestPinCount; i++)
    {
        for (j = 0; j < _gSenseLineNum; j++)
        {
            if (_gShortTestChannel[i] == SENSE_X[j])
            {
                _gSenseR[j] = _DrvMpTestItoShortTestCovertRValue(_gDeltaC[i]);

                DBG("_gSenseR[%d] = %d\t", j , _gSenseR[j]);
            }
        }
    }
    DBG("\n");

    for (i = 0; i < MAX_CHANNEL_NUM; i ++)
    {
        _gShortTestChannel[i] = 0xff;
    }
    
    nTestPinCount = 0;

    if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        _DrvMpTestMsg26xxmItoShortTest(3);
    }
    
    nRetVal4 = _DrvMpTestItoShortTestMsg26xxmJudge(3);

    for (i = 0; i < MAX_CHANNEL_NUM; i++)
    {
        if (_gShortTestChannel[i] != 0)
        {
            nTestPinCount++;
        }
    }

    for (i = 0; i < nTestPinCount; i++)
    {
        for (j = 0; j < _gDriveLineNum; j++)
        {
            if (_gShortTestChannel[i] == DRIVE_X[j])
            {
                _gDriveR[j] = _DrvMpTestItoShortTestCovertRValue(_gDeltaC[i]);
                DBG("_gDriveR[%d] = %d\t", j , _gDriveR[j]);
            }
        }
    }
    DBG("\n");
    
    for (i = 0; i < MAX_CHANNEL_NUM; i ++)
    {
        _gShortTestChannel[i] = 0xff;
    }
    
    nTestPinCount = 0;

    if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        _DrvMpTestMsg26xxmItoShortTest(4);
    }
    
    nRetVal4 = _DrvMpTestItoShortTestMsg26xxmJudge(4);

    for (i = 0; i < MAX_CHANNEL_NUM; i++)
    {
        if (_gShortTestChannel[i] != 0)
        {
            nTestPinCount++;
        }
    }

    for (i = 0; i < nTestPinCount; i++)
    {
        for (j = 0; j < _gDriveLineNum ; j++)
        {
            if (_gShortTestChannel[i] == DRIVE_X[j])
            {
                _gDriveR[j] = _DrvMpTestItoShortTestCovertRValue(_gDeltaC[i]);
                DBG("_gDriveR[%d] = %d\t", j , _gDriveR[j]);
            }
        }
    }
    DBG("\n");
    
    for (i = 0; i < MAX_CHANNEL_NUM; i ++)
    {
        _gShortTestChannel[i] = 0xff;
    }
    
    nTestPinCount = 0;
    nShortThreshold = _DrvMpTestItoShortTestCovertRValue(SHORT_VALUE);

    for (i = 0; i < _gSenseLineNum; i++)
    {
        _gResult[i] = _gSenseR[i];
    }

    for (i = 0; i < _gDriveLineNum; i++)
    {
        _gResult[i + _gSenseLineNum] = _gDriveR[i];
    }

    for (i = 0; i < (_gSenseLineNum + _gDriveLineNum); i++)
    {
        if (_gResult[i] < nShortThreshold)
        {
            _gTestFailChannel[i] = 1;
        }
        else
        {
            _gTestFailChannel[i] = 0;
        }
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    DrvPlatformLyrEnableFingerTouchReport();

    DBG("short test end\n");

    DBG("nRetVal1 = %d, nRetVal2 = %d, nRetVal3 = %d, nRetVal4 = %d, nRetVal5 = %d\n",nRetVal1,nRetVal2,nRetVal3,nRetVal4,nRetVal5);

    if ((nRetVal1 != ITO_TEST_OK) && (nRetVal2 == ITO_TEST_OK) && (nRetVal3 == ITO_TEST_OK) && (nRetVal4 == ITO_TEST_OK) && (nRetVal5 == ITO_TEST_OK))
    {
        return ITO_TEST_GET_TP_TYPE_ERROR;
    }
    else if ((nRetVal1 == ITO_TEST_OK) && ((nRetVal2 != ITO_TEST_OK) || (nRetVal3 != ITO_TEST_OK) || (nRetVal4 != ITO_TEST_OK) || (nRetVal5 != ITO_TEST_OK)))
    {
        return -1;
    }
    else
    {
        return ITO_TEST_OK;
    }
}

static void _DrvMpTestItoShortTestMsg28xxSetNoiseSensorMode(unsigned char nEnable)
{
    signed short j;

    DBG("*** %s() ***\n", __func__);    

    if (nEnable)
    {
        RegSet16BitValueOn(0x1546, BIT4);
        for (j = 0; j < 10; j++)
        {
            RegSet16BitValue(0x2148 + 2 * j, 0x0000);
        }
        RegSet16BitValue(0x215C, 0x1FFF);
    }
}

static void _DrvMpTestItoShortTestMsg28xxAnaFixPrs(unsigned short nOption)
{
    unsigned short nRegData = 0;

    DBG("*** %s() ***\n", __func__);

    RegSet16BitValue(0x0FE6, 0x0001);

    nRegData = RegGet16BitValue(0x1008);
    nRegData &= 0x00F1;
    nRegData |= (unsigned short)((nOption << 1) & 0x000E);
    RegSet16BitValue(0x1008, nRegData);
}

static void _DrvMpTestItoShortTestMsg28xxAndChangeCDtime(unsigned short nTime1, unsigned short nTime2)
{
    DBG("*** %s() ***\n", __func__);

    RegSet16BitValue(0x1026, nTime1);
    RegSet16BitValue(0x1030, nTime2);
}

static void _DrvMpTestItoShortTestMsg28xxChangeANASetting(void)
{
    int i, nMappingItem; 
    unsigned char nChipVer;


    DBG("*** %s() ***\n", __func__);

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001); //bank:mheg5, addr:h0073
    
    nChipVer = RegGetLByteValue(0x1ECE);
    //uint chip_ver = Convert.ToUInt16((uint)regdata[0]); //for U01 (SF shift).
    
    if (nChipVer != 0)
        RegSetLByteValue(0x131E, 0x01);
    
    for (nMappingItem = 0; nMappingItem < 6; nMappingItem++)
    {
        /// sensor mux sram read/write base address / write length
        RegSetLByteValue(0x2192, 0x00);
        RegSetLByteValue(0x2102, 0x01);
        RegSetLByteValue(0x2102, 0x00);
        RegSetLByteValue(0x2182, 0x08);
        RegSetLByteValue(0x2180, 0x08 * nMappingItem);
        RegSetLByteValue(0x2188, 0x01);
    
        for (i = 0; i < 8; i++)
        {
            if (nMappingItem == 0 && nChipVer == 0x0)
            {
                RegSet16BitValue(0x218A, _gMuxMem_20_3E_0_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMuxMem_20_3E_0_Settings[2 * i + 1]);
            }
            if ((nMappingItem == 1 && nChipVer == 0x0) || (nMappingItem == 0 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMuxMem_20_3E_1_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMuxMem_20_3E_1_Settings[2 * i + 1]);
            }
            if ((nMappingItem == 2 && nChipVer == 0x0) || (nMappingItem == 1 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMuxMem_20_3E_2_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMuxMem_20_3E_2_Settings[2 * i + 1]);
            }
            if ((nMappingItem == 3 && nChipVer == 0x0) || (nMappingItem == 2 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMuxMem_20_3E_3_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMuxMem_20_3E_3_Settings[2 * i + 1]);
            }
            if ((nMappingItem == 4 && nChipVer == 0x0) || (nMappingItem == 3 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMuxMem_20_3E_4_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMuxMem_20_3E_4_Settings[2 * i + 1]);
            }
            if ((nMappingItem == 5 && nChipVer == 0x0) || (nMappingItem == 4 && nChipVer != 0x0))
            {
                RegSet16BitValue(0x218A, _gMuxMem_20_3E_5_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMuxMem_20_3E_5_Settings[2 * i + 1]);
            }
            if (nMappingItem == 5 && nChipVer != 0x0)
            {
                RegSet16BitValue(0x218A, _gMuxMem_20_3E_6_Settings[2 * i]);
                RegSet16BitValue(0x218C, _gMuxMem_20_3E_6_Settings[2 * i + 1]);
            }
        }
    }
}

static void _DrvMpTestItoReadSetting(unsigned short * pPad2Sense, unsigned short * pPad2Drive, unsigned short * pPad2GR)
{
    DBG("*** %s() ***\n", __func__);

    memcpy(_gMuxMem_20_3E_1_Settings, SHORT_N1_MUX_MEM_20_3E, sizeof(unsigned short) * 16);
    memcpy(_gMuxMem_20_3E_2_Settings, SHORT_N2_MUX_MEM_20_3E, sizeof(unsigned short) * 16);
    memcpy(_gMuxMem_20_3E_3_Settings, SHORT_S1_MUX_MEM_20_3E, sizeof(unsigned short) * 16);
    memcpy(_gMuxMem_20_3E_4_Settings, SHORT_S2_MUX_MEM_20_3E, sizeof(unsigned short) * 16);

    if(SHORT_TEST_5_TYPE != 0)
    {
        memcpy(_gMuxMem_20_3E_5_Settings, SHORT_X_MUX_MEM_20_3E, sizeof(unsigned short) * 16);
    }

    memcpy(pPad2Sense, PAD_TABLE_SENSE, sizeof(unsigned short) * _gSenseLineNum);
    memcpy(pPad2Drive, PAD_TABLE_DRIVE, sizeof(unsigned short) * _gDriveLineNum);

    if (GR_NUM != 0)
    {
        memcpy(pPad2GR, PAD_TABLE_GR, sizeof(unsigned short) * GR_NUM);
    }
}

static signed int _DrvMpTestItoShortTestMsg28xxGetValueR(signed int * pTarget)
{    
    signed short * pRawData = NULL;
    unsigned short nSenNumBak = 0;
    unsigned short nDrvNumBak = 0;     
    unsigned short nShift = 0;
    signed short i, j;

    DBG("*** %s() ***\n", __func__);

    pRawData = kzalloc(sizeof(signed short) * MAX_CHANNEL_SEN*2 * MAX_CHANNEL_DRV, GFP_KERNEL);

    if (_DrvMpTestItoTestMsg28xxGetMutualOneShotRawIIR(pRawData, &nSenNumBak, &nDrvNumBak) < 0)
    {
        DBG("*** Msg28xx Short Test# GetMutualOneShotRawIIR failed! ***\n");                    
        return -1;
    }
    
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 13; j++)
        {
            nShift = (unsigned short)(j + 13 * i);
            pTarget[nShift] = pRawData[i*MAX_CHANNEL_DRV+j];
        }
    }

    kfree(pRawData);

    return 0;
}

static signed int _DrvMpTestMsg28xxItoShortTest(unsigned char nItemID)
{
    signed short i;
    unsigned char nRegData = 0;
    unsigned short nAfeCoef = 0;
    
    DBG("*** %s() ***\n", __func__);    

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001); //bank:mheg5, addr:h0073

    ///set Subframe = 6 ; Sensor = 13
    RegSetLByteValue(0x130A, 0x6D);
    RegSetLByteValue(0x1103, 0x06);
    RegSetLByteValue(0x1016, 0x0C);

    RegSetLByteValue(0x1104, 0x0C);
    RegSetLByteValue(0x100C, 0x0C);
    RegSetLByteValue(0x1B10, 0x0C);

    /// adc analog+digital pipe delay, 60= 13 AFE.
    RegSetLByteValue(0x102F, 0x60);

    ///trim: Fout 52M &  1.2V
    RegSet16BitValue(0x1420, 0xA55A);//password
    RegSet16BitValue(0x1428, 0xA55A);//password
    RegSet16BitValue(0x1422, 0xFC4C);//go

    _DrvMpTestItoShortTestMsg28xxSetNoiseSensorMode(1);    
    _DrvMpTestItoShortTestMsg28xxAnaFixPrs(3);    
    _DrvMpTestItoShortTestMsg28xxAndChangeCDtime(0x007E, 0x001F);    

    ///DAC overwrite
    RegSet16BitValue(0x150C, 0x80A2); //bit15 //AE:3.5v for test
    RegSet16BitValue(0x1520, 0xFFFF);//After DAC overwrite, output DC
    RegSet16BitValue(0x1522, 0xFFFF);
    RegSet16BitValue(0x1524, 0xFFFF);
    RegSet16BitValue(0x1526, 0xFFFF);

    /// all AFE Cfb use defalt (50p)
    RegSet16BitValue(0x1508, 0x1FFF);// all AFE Cfb: SW control
    RegSet16BitValue(0x1550, 0x0000);// all AFE Cfb use defalt (50p)

    /// reg_afe_icmp disenable
    RegSet16BitValue(0x1552, 0x0000);

    /// reg_hvbuf_sel_gain
    RegSet16BitValue(0x1564, 0x0077);

    ///ADC: AFE Gain bypass
    RegSet16BitValue(0x1260, 0x1FFF);

    ///reg_sel_ros disenable
    RegSet16BitValue(0x156A, 0x0000);

    ///reg_adc_desp_invert disenable
    RegSetLByteValue(0x1221, 0x00);


    ///AFE coef
    //protoHandle.MstarReadReg(mdkDevice, (uint)0x101A, ref regdata);
    nRegData = RegGetLByteValue(0x101A);
    nAfeCoef = 0x10000 / nRegData;
    //protoHandle.MstarWriteReg_16(mdkDevice, (uint)0x13D6, AFE_coef);
    RegSet16BitValue(0x13D6, nAfeCoef);
    
    /// AFE gain = 1X
    //RegSet16BitValue(0x1318, 0x4440);
    //RegSet16BitValue(0x131A, 0x4444);
    //RegSet16BitValue(0x13D6, 0x2000);

    _DrvMpTestItoShortTestMsg28xxChangeANASetting();
    _DrvMpTestItoTestMsg28xxAnaSwReset();

    if (_DrvMpTestItoShortTestMsg28xxGetValueR(_gDeltaC)<0)
    {
        DBG("*** Msg28xx Short Test# GetValueR failed! ***\n");      
        return -1;
    }
    _ItoTestDebugShowArray(_gDeltaC, 128, -32, 10, 8);

    for (i = 0; i < 65; i++) // 13 AFE * 5 subframe
    {
        if (_gDeltaC[i] <= -1000 || _gDeltaC[i] >= (IIR_MAX))
            _gDeltaC[i] = 0x7FFF;
        else
            _gDeltaC[i] = abs(_gDeltaC[i]);
    }
    return 0;
}

static signed int _DrvMpTestItoShortTestReadTestPins(unsigned char nItemID, unsigned short * pTestPins)
{    
    unsigned short nCount = 0;
    signed short i;

    DBG("*** %s() ***\n", __func__);
    
    switch (nItemID)
    {
        case 1:
        case 11:
            nCount = SHORT_N1_TEST_NUMBER;            
            memcpy(pTestPins, SHORT_N1_TEST_PIN, sizeof(unsigned short) * nCount);
            break;
        case 2:
        case 12:
            nCount = SHORT_N2_TEST_NUMBER;            
            memcpy(pTestPins, SHORT_N2_TEST_PIN, sizeof(unsigned short) * nCount);
            break;
        case 3:
        case 13:
            nCount = SHORT_S1_TEST_NUMBER;            
            memcpy(pTestPins, SHORT_S1_TEST_PIN, sizeof(unsigned short) * nCount);
            break;
        case 4:
        case 14:
            nCount = SHORT_S2_TEST_NUMBER;            
            memcpy(pTestPins, SHORT_S2_TEST_PIN, sizeof(unsigned short) * nCount);
            break;
    
        case 5:
        case 15:
            if(SHORT_TEST_5_TYPE != 0)
            {
                nCount = SHORT_X_TEST_NUMBER;            
                memcpy(pTestPins, SHORT_X_TEST_PIN, sizeof(unsigned short) * nCount);
            }    
            break;

        case 0:
        default:
            return 0;
    }

    for (i = nCount; i < MAX_CHANNEL_NUM; i++)
    {
        pTestPins[i] = 0xFFFF;    //PIN_NO_ERROR
    }
    
    return nCount;
}

static signed int _DrvMpTestItoShortTestMsg28xxJudge(unsigned char nItemID, /*signed char pNormalTestResult[][2],*/ unsigned short pTestPinMap[][13], unsigned short * pTestPinCount)
{
    signed int nRetVal = 0;
    unsigned short nTestPins[MAX_CHANNEL_NUM];
    signed short i; 

    DBG("*** %s() ***\n", __func__);

    *pTestPinCount = _DrvMpTestItoShortTestReadTestPins(nItemID, nTestPins);
    //_ItoTestDebugShowArray(nTestPins, *pTestPinCount, 16, 10, 8);    
    if (*pTestPinCount == 0)
    {
        if (nItemID == 5 && SHORT_TEST_5_TYPE == 0)
        {

        }
        else
        {
            DBG("*** Msg28xx Short Test# TestPinCount = 0 ***\n");       
            return -1;
        }
    }

  /*  
    unsigned short nCountTestPin = 0;    
    for (i = 0; i < testPins.Length; i++)
    {
        if (pTestPins[i] != 0xFFFF)
            nCountTestPin++;
    }
   */ 
    
    for (i = (nItemID - 1) * 13; i < (13 * nItemID); i++)
    {
        _gResult[i] = _gDeltaC[i];
    }
    
    for (i = 0; i < *pTestPinCount; i++)
    {
        pTestPinMap[nItemID][i] = nTestPins[i];    
   	   	if (0 == _DrvMpTestItoTestCheckValueInRange(_gResult[i + (nItemID - 1) * 13], SHORTVALUE, -1000))    //0: false   1: true
        {
            //pNormalTestResult[nItemID][0] = -1;    //-1: failed   0: success 
            //                         //0: golden   1: ratio
            DBG("*** Msg28xx Short Test# ShortTestMsg28xxJudge failed! ***\n");             
            nRetVal = -1;
        }
    }

    DBG("*** Msg28xx Short Test# nItemID = %d ***\n", nItemID);        
    //_ItoTestDebugShowArray(pTestPinMap[nItemID], *pTestPinCount, 16, 10, 8);
    
    return nRetVal;
}

static signed int _DrvMpTestItoShortTestMsg28xxCovertRValue(signed int nValue)
{
   	if (nValue >= IIR_MAX)
   	{
   	   	return 0;
   	}

    //return ((3.53 - 1.3) * 10 / (50 * (((float)nValue - 0 ) / 32768 * 1.1)));
    return 223 * 32768 / (nValue * 550);
}

static signed int _DrvMpTestMsg28xxItoShortTestEntry(void)
{
    //ItoTestResult_e nRetVal1 = ITO_TEST_OK, nRetVal2 = ITO_TEST_OK, nRetVal3 = ITO_TEST_OK, nRetVal4 = ITO_TEST_OK, nRetVal5 = ITO_TEST_OK;
    signed short i = 0, j = 0;
    //unsigned short nTestPinCount = 0;
    //signed int nShortThreshold = 0;
    unsigned short *pPad2Drive = NULL;
    unsigned short *pPad2Sense = NULL;
    unsigned short nTime = 0;    
    unsigned short nPad2GR[MAX_CHANNEL_NUM] = {0};
    signed int nResultTemp[(MAX_CHANNEL_SEN+MAX_CHANNEL_DRV)*2] = {0};

    ///short test1 to 5.
    //unsigned short nTestPinCount = 0;
    unsigned short nTestItemLoop = 6;
    unsigned short nTestItem = 0; 
    //signed char nNormalTestResult[8][2] = {0};    //0:golden    1:ratio
    unsigned short nTestPinMap[6][13] = {{0}};        //6:max subframe    13:max afe
    unsigned short nTestPinNum = 0;
//    signed int nThrs = 0;
    unsigned int nRetVal = 0;
        
    DBG("*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    _gDeepStandby = 0;
    DrvPlatformLyrDisableFingerTouchReport();
    DrvPlatformLyrTouchDeviceResetHw();

    if (!_DrvMpTestMsg28xxItoTestChooseTpType())
    {
        DBG("Choose Tp Type failed\n");
        DrvPlatformLyrTouchDeviceResetHw();    
        DrvPlatformLyrEnableFingerTouchReport();         
        return -2;
    }

    pPad2Drive = kzalloc(sizeof(signed short) * DRIVE_NUM, GFP_KERNEL);
    pPad2Sense = kzalloc(sizeof(signed short) * SENSE_NUM, GFP_KERNEL);
    _gSenseLineNum = SENSE_NUM;
    _gDriveLineNum = DRIVE_NUM;

_retry_short:
    DrvPlatformLyrTouchDeviceResetHw();

    //reset_only
    DbBusResetSlave();
    DbBusEnterSerialDebugMode();
    DbBusWaitMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    if(_DrvMpTestMsg28xxItoTestSwitchFwMode(MUTUAL_SINGLE_DRIVE) < 0)
    {
        nTime++;
        if(nTime < 5)
        {
            goto _retry_short;
        }    
        DBG("*** Msg28xx Short Test# Switch Fw Mode failed! ***\n");
        nRetVal = -1;

        goto ITO_TEST_END;
    }

    _DrvMpTestItoReadSetting(pPad2Sense, pPad2Drive, nPad2GR);


	//N1_ShortTest    
    if(_DrvMpTestMsg28xxItoShortTest(1) < 0)
    {
        DBG("*** Msg28xx Short Test# Get DeltaC failed! ***\n");
        nRetVal = -1;
        goto ITO_TEST_END;      
    }

    for (nTestItem = 1; nTestItem < nTestItemLoop; nTestItem++)
    {
        DBG("*** Short test item %d ***\n", nTestItem);
        if (_DrvMpTestItoShortTestMsg28xxJudge(nTestItem, /*nNormalTestResult,*/ nTestPinMap, &nTestPinNum) < 0)
        {
            DBG("*** Msg28xx Short Test# Item %d is failed! ***\n", nTestItem);
            nRetVal = -1;
            goto ITO_TEST_END;  
        }

        if (nTestItem == 1 || nTestItem == 2 || (nTestItem == 5 && SHORT_TEST_5_TYPE == 1))
        {
            for (i = 0; i < nTestPinNum; i++)
            {
                for (j = 0; j < _gSenseLineNum; j++)
                {
                    if (nTestPinMap[nTestItem][i] == pPad2Sense[j])
                    {
                        //_gSenseR[j] = _DrvMpTestItoShortTestMsg28xxCovertRValue(_gResult[i + (nTestItem - 1) * 13]);
                        _gSenseR[j] = _gResult[i + (nTestItem - 1) * 13];    //change comparison way because float computing in driver is prohibited
                    }
                }
            }
        }

        if (nTestItem == 3 || nTestItem == 4 || (nTestItem == 5 && SHORT_TEST_5_TYPE == 2))
        {
            for (i = 0; i < nTestPinNum; i++)
            {
                for (j = 0; j < _gDriveLineNum; j++)
                {
                    if (nTestPinMap[nTestItem][i] == pPad2Drive[j])
                    {
                        //_gDriveR[j] = _DrvMpTestItoShortTestMsg28xxCovertRValue(_gResult[i + (nTestItem - 1) * 13]);
                        _gDriveR[j] = _gResult[i + (nTestItem - 1) * 13];    //change comparison way because float computing in driver is prohibited
                    }
                }
            }
        }

        if (nTestItem == 5 && SHORT_TEST_5_TYPE == 3)
        {
            for (i = 0; i < nTestPinNum; i++)
            {
                for (j = 0; j < GR_NUM; j++)
                {
                    if (nTestPinMap[nTestItem][i] == nPad2GR[j])
                    {
                        //_gGRR[j] = _DrvMpTestItoShortTestMsg28xxCovertRValue(_gResult[i + (nTestItem - 1) * 13]);
                        _gGRR[j] = _gResult[i + (nTestItem - 1) * 13];    //change comparison way because float computing in driver is prohibited
                    }
                }
            }
        }
    }

    for (i = 0; i < _gSenseLineNum; i++)
    {
        nResultTemp[i] = _gSenseR[i];
    }

    //for (i = 0; i < _gDriveLineNum - 1; i++)
    for (i = 0; i < _gDriveLineNum; i++)
    {
        nResultTemp[i + _gSenseLineNum] = _gDriveR[i];
    }

    //nThrs = _DrvMpTestItoShortTestMsg28xxCovertRValue(SHORTVALUE);     
    for (i = 0; i < _gSenseLineNum + _gDriveLineNum; i++)
    {
        if(nResultTemp[i] == 0)
        {        
            _gResult[i] = _DrvMpTestItoShortTestMsg28xxCovertRValue(1);
        }
        else
        {
            _gResult[i] = _DrvMpTestItoShortTestMsg28xxCovertRValue(nResultTemp[i]);
        }
    }

    _ItoTestDebugShowArray(_gResult, _gSenseLineNum + _gDriveLineNum, -32, 10, 8);
    //for (i = 0; i < (_gSenseLineNum + _gDriveLineNum - 1); i++)
    for (i = 0; i < (_gSenseLineNum + _gDriveLineNum); i++)        
    {
        if (nResultTemp[i] > SHORTVALUE)    //change comparison way because float computing in driver is prohibited
        {
            _gTestFailChannel[i] = 1;
            _gTestFailChannelCount++;
            nRetVal = -1;
        }
        else
        {
            _gTestFailChannel[i] = 0;
        }
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    ITO_TEST_END:

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    DrvPlatformLyrEnableFingerTouchReport();
    kfree(pPad2Sense);
    kfree(pPad2Drive);    

    return nRetVal;
}

static signed int _DrvMpTestItoShortTest(void)
{
    signed int nRetVal = -1;    

    DBG("*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        return _DrvMpTestMsg26xxmItoShortTestEntry();
    }    
    else if(g_ChipType == CHIP_TYPE_MSG28XX)
    {
        return _DrvMpTestMsg28xxItoShortTestEntry();
    }    

    return nRetVal; 
}

static signed int _DrvMpTestItoTestMsg26xxmGetWaterProofOneShotRawIir(unsigned short wszResultData[])
{
    unsigned short nRegData;
    unsigned short i;
    unsigned short nTemp;
    unsigned char szDbBusTxData[3];
	unsigned int nGetdataNum = 12;
    unsigned char szShotData[24] = {0}; //Get 12 FIR data

    DBG("*** %s() ***\n", __func__);

    nTemp = RegGet16BitValue(0x3D08); //bank:intr_ctrl, addr:h0004
    nTemp &= (~(BIT8));
    RegSet16BitValue(0x3D08, nTemp);
    //RegSet16BitValueOff(0x3D08, BIT8);      ///FIQ_E_FRAME_READY_MASK

    _DrvMpTestItoTestMsg26xxmEnableAdcOneShot();

    nRegData = 0;
    while (0x0000 == (nRegData & BIT8))
    {
        nRegData = RegGet16BitValue(0x3D18); //bank:intr_ctrl, addr:h000c
    }

    for (i = 0; i < nGetdataNum * 2; i ++)
    {
        szShotData[i] = 0;
    }

    mdelay(200);
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x13; //bank:fir, addr:h0021
    szDbBusTxData[2] = 0x42;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &szShotData[0], 24); //12

    for (i = 0; i < nGetdataNum; i ++)
    {
    	nRegData = (unsigned short)(szShotData[i * 2] | szShotData[i * 2 + 1] << 8);
        wszResultData[i] = (short)nRegData;

		//DBG("wszResultData[%d] = %x\t", i  , wszResultData[i]);
    }

    nTemp = RegGet16BitValue(0x3D08); //bank:intr_ctrl, addr:h0004
    nTemp |= (BIT8 | BIT4);
    RegSet16BitValue(0x3D08, nTemp);

	return 0;
}

static signed int _DrvMpTestItoTestMsg26xxmGetWaterProofDeltaC(signed int *pTarget)
{
    unsigned short wszRawData[12];
    unsigned short i;

    DBG("*** %s() ***\n", __func__);

	if (_DrvMpTestItoTestMsg26xxmGetWaterProofOneShotRawIir(wszRawData) < 0)
    {
        DBG("*** Msg26xxm WaterProof Test# GetMutualOneShotRawIIR failed! ***\n");
        return -1;
    }

    for (i = 0; i < 12; i ++)
    {
        pTarget[i] = (signed short)wszRawData[i];
    }

	return 0;
}

static signed int _DrvMpTestMsg26xxmItoWaterProofTest(void)
{
    DBG("*** %s() ***\n", __func__);

    // Stop mcu
    //RegSet16BitValue(0x0FE6, 0x0001);

    _DrvMpTestItoTestMsg26xxmAnaSwReset();

    _DrvMpTestItoShortTestMsg26xxmAndChangeCDtime(WATERPROOF_Charge_X, WATERPROOF_Dump_X);

    if(_DrvMpTestItoTestMsg26xxmGetWaterProofDeltaC(_gDeltaC) < 0)
    {
        DBG("*** Msg26xxm WaterProof Test# GetWaterDeltaC failed! ***\n");
        return -1;
    }

    return 0;
}

static signed int _DrvMpTestMsg26xxmItoWaterProofTestJudge(void)
{
	unsigned short i;
	unsigned int nGetdataNum = 12;

    DBG("*** %s() ***\n", __func__);

	for (i = 0; i < nGetdataNum; i ++)
    {
        _gResultWater[i] = 0;
    }

    for (i = 0; i < nGetdataNum; i++)
    {
   		_gResultWater[i] = _gDeltaC[i];
    }

    return 0;
}

signed int _DrvMpTestMsg26xxmItoWaterProofTestEntry(void)
{
    signed int nRetVal = 0;
	unsigned int nRegData = 0;
    unsigned short i = 0;
	signed int nResultTemp[12] = {0};

    DBG("*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

	_gWaterProofNum = 12;

    DrvPlatformLyrDisableFingerTouchReport();

    DrvPlatformLyrTouchDeviceResetHw();

	  //auto detetc fw version to decide switch fw mode
    if (_gTestAutoSwitchFlag!=0)
    {
        _DrvMpTestItoTestMsg26xxmGetSwitchFlag();
    }

    DbBusResetSlave();
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    if(_DrvMpTestItoTestMsg26xxmSwitchMode(_gTestSwitchMode, WATERPROOF) < 0)
    {
        DBG("*** Msg26xxm WaterProof Test# Switch FW mode failed! ***\n");
        return -1;
    }

    _DrvMpTestItoTestMsg26xxmMcuStop();

	nRegData = RegGet16BitValue(0x3CE4);

	if (nRegData == 0x8bbd)//// if polling 0X8BBD, means the FW is NOT supporting WP.
	{
		DBG("*** Msg26xxm WaterProof Test# No supporting this function! ***\n");
		return -1;
	}

    mdelay(10);

    for (i = 0; i < MAX_MUTUAL_NUM; i ++)
    {
        _gTestFailChannel[i] = 0;
    }

    _gTestFailChannelCount = 0; // Reset _gTestFailChannelCount to 0 before test start

  	if(_DrvMpTestMsg26xxmItoWaterProofTest() < 0)
  	{
        DBG("*** Msg26xxm WaterProof Test# Get DeltaC failed! ***\n");
        return -1;
  	}

    _DrvMpTestMsg26xxmItoWaterProofTestJudge();

	for (i = 0; i < 12; i++)
	{
		nResultTemp[i] = _gResultWater[i];
	}

	_ItoTestDebugShowArray(_gResultWater, 12, -32, 10, 8);
	for (i = 0; i < 12; i++)
	{
		if (nResultTemp[i] < WATERPROOFVALUE)    //change comparison way because float computing in driver is prohibited
		{
			_gTestFailChannel[i] = 1;
			_gTestFailChannelCount++;
			nRetVal = -1;
		}
		else
		{
			_gTestFailChannel[i] = 0;
		}
	}


    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    DrvPlatformLyrEnableFingerTouchReport();

    return nRetVal;
}

static signed int _DrvMpTestItoWaterProofTestMsg28xxTriggerWaterProofOneShot(signed short * pResultData, unsigned int nDelay)
{
    unsigned short nAddr = 0x5000, nAddrNextSF = 0x1A4;
    unsigned short nSF = 0, nAfeOpening = 0, nDriOpening = 0;
    unsigned short nMaxDataNumOfOneSF = 0;
    unsigned short nDriMode = 0;
    unsigned short i;
    unsigned char nRegData = 0;
    unsigned char nShotData[390] = {0};//13*15*2
    unsigned short nRegDataU16 = 0;

    DBG("*** %s() ***\n", __func__);

    nRegData = RegGetLByteValue(0x130A);
    nSF = nRegData>>4;
    nAfeOpening = nRegData & 0x0f;

    if(nSF == 0)
    {
        return -1;
    }

    nRegData = RegGetLByteValue(0x100B);
    nDriMode = nRegData;

    nRegData = RegGetLByteValue(0x1312);
    nDriOpening = nRegData;

    DBG("*** Msg28xx WaterProof Test# TriggerWaterProofOneShot nSF=%d, nAfeOpening=%d, nDriMode=%d, nDriOpening=%d. ***\n", nSF, nAfeOpening, nDriMode, nDriOpening);

    nMaxDataNumOfOneSF = nAfeOpening * nDriOpening;

    RegSet16BitValueOff(0x3D08, BIT8);      ///FIQ_E_FRAME_READY_MASK

    ///polling frame-ready interrupt status
    _DrvMpTestItoTestMsg28xxEnableAdcOneShot();

    while (0x0000 == (nRegDataU16 & BIT8))
    {
        nRegDataU16 = RegGet16BitValue(0x3D18);
    }

    RegSet16BitValueOn(0x3D08, BIT8);      ///FIQ_E_FRAME_READY_MASK
    RegSet16BitValueOn(0x3D08, BIT4);      ///FIQ_E_TIMER0_MASK

    if (PATTERN_TYPE == 1) // for short test
    {
        //signed short nShortResultData[nSF][nAfeOpening];

        /// get ALL raw data
        _DrvMpTestItoTestDBBusReadDQMemStart();
        RegGetXBitValue(nAddr, nShotData, 16, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
        _DrvMpTestItoTestDBBusReadDQMemEnd();

        //_ItoTestDebugShowArray(nShotData, 26, 8, 16, 16);
        for (i = 0; i < 8; i++)
        {
            pResultData[i] = (signed short)(nShotData[2 * i] | nShotData[2 * i + 1] << 8);
        }
    }
    else if(PATTERN_TYPE == 3 || PATTERN_TYPE == 4)// for open test
    {
        //signed short nOpenResultData[nSF * nAfeOpening][nDriOpening];

        if(nSF >4)
            nSF = 4;

        /// get ALL raw data, combine and handle datashift.
        for (i = 0; i < nSF; i++)
        {
            _DrvMpTestItoTestDBBusReadDQMemStart();
            RegGetXBitValue(nAddr + i * nAddrNextSF, nShotData, 16, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
            _DrvMpTestItoTestDBBusReadDQMemEnd();

            //_ItoTestDebugShowArray(nShotData, 390, 8, 10, 16);
            pResultData[2 * i] = (signed short)(nShotData[4 * i] | nShotData[4 * i + 1] << 8);
            pResultData[2 * i + 1] = (signed short)(nShotData[4 * i + 2] | nShotData[4 * i + 3] << 8);
        }
    }
    else
    {
        return -1;
    }

    return 0;
}

static signed int _DrvMpTestItoWaterProofTesMsg28xxtGetWaterProofOneShotRawIIR(signed short * pRawDataWP, unsigned int nDelay)
{
    return _DrvMpTestItoWaterProofTestMsg28xxTriggerWaterProofOneShot(pRawDataWP, nDelay);
}

static signed int _DrvMpTestItoWaterProofTestMsg28xxGetDeltaCWP(signed int *pTarget, signed char nSwap, unsigned int nDelay)
{
    signed short nRawDataWP[12] = {0};
    signed short i;

    DBG("*** %s() ***\n", __func__);

    if(_DrvMpTestItoWaterProofTesMsg28xxtGetWaterProofOneShotRawIIR(nRawDataWP, nDelay) < 0)
    {
        DBG("*** Msg28xx Open Test# GetMutualOneShotRawIIR failed! ***\n");
        return -1;
    }

    for (i = 0; i < _gWaterProofNum; i++)
    {
        pTarget[i] = nRawDataWP[i];
    }

    return 0;
}

static signed int _DrvMpTestMsg28xxItoWaterProofTest(unsigned int nDelay)
{
    DBG("*** %s() ***\n", __func__);

    // Stop mcu
    RegSet16BitValue(0x0FE6, 0x0001); //bank:mheg5, addr:h0073
    _DrvMpTestItoTestMsg28xxAnaSwReset();

    if (_DrvMpTestItoWaterProofTestMsg28xxGetDeltaCWP(_gDeltaCWater, -1, nDelay)<0)
    {
        DBG("*** Msg28xx WaterProof Test# GetDeltaCWP failed! ***\n");
        return -1;
    }

    _ItoTestDebugShowArray(_gDeltaCWater, 12, -32, 10, 16);

    return 0;
}

static void _DrvMpTestMsg28xxItoWaterProofTestMsgJudge(void)
{
    int i;

    DBG("*** %s() ***\n", __func__);

    for (i = 0; i < _gWaterProofNum; i++)
    {
        _gResultWater[i] =  abs(_gDeltaCWater[i]);
    }
}

static signed int _DrvMpTestMsg28xxItoWaterProofTestEntry(void)
{
    signed short i = 0;
    unsigned int nRetVal = 0;
    unsigned short nRegDataWP = 0;
    unsigned int nDelay = 0;

    DBG("*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    _gWaterProofNum = 12;

    DrvPlatformLyrDisableFingerTouchReport();
    DrvPlatformLyrTouchDeviceResetHw();

    //reset_only
    DbBusResetSlave();
    DbBusEnterSerialDebugMode();
    DbBusWaitMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);

    if(_DrvMpTestMsg28xxItoTestSwitchFwMode(WATERPROOF) < 0)
    {
        DBG("*** Msg28xx WaterProof Test# Switch FW mode failed! ***\n");
        nRetVal =  -1;
        goto ITO_TEST_END;
    }

    nRegDataWP = RegGet16BitValue(0x1402);
    if(nRegDataWP == 0x8BBD)
    {
        DBG("*** Msg28xx WaterProof Test# FW don't support waterproof! ***\n");
        nRetVal = -1;
        goto ITO_TEST_END;         
    }

    if(_DrvMpTestMsg28xxItoWaterProofTest(nDelay) < 0)
    {
        DBG("*** Msg28xx WaterProof Test# Get DeltaC failed! ***\n");
        nRetVal = -1;
        goto ITO_TEST_END;        
    }

    _DrvMpTestMsg28xxItoWaterProofTestMsgJudge();

    for (i = 0; i < _gWaterProofNum; i++)
    {
        if (_gResultWater[i] > WATERVALUE)    //change comparison way because float computing in driver is prohibited
        {
            _gTestFailChannel[i] = 1;
            _gTestFailChannelCount++;
            nRetVal = -1;
        }
        else
        {
            _gTestFailChannel[i] = 0;
        }
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    ITO_TEST_END:

    DrvPlatformLyrTouchDeviceResetHw();
    mdelay(300);

    DrvPlatformLyrEnableFingerTouchReport();

    return nRetVal;
}

static signed int _DrvMpTestItoWaterProofTest(void)
{
    signed int nRetVal = -1;

    DBG("*** %s() ***\n", __func__);

    if(g_ChipType == CHIP_TYPE_MSG26XXM)
    {
        return _DrvMpTestMsg26xxmItoWaterProofTestEntry();
    }
    else if(g_ChipType == CHIP_TYPE_MSG28XX)
    {
        return _DrvMpTestMsg28xxItoWaterProofTestEntry();
    }

    return nRetVal;
}

static void _DrvMpTestItoTestDoWork(struct work_struct *pWork)
{
    signed int nRetVal = 0;
    
    DBG("*** %s() g_IsInMpTest = %d, _gTestRetryCount = %d ***\n", __func__, g_IsInMpTest, _gTestRetryCount);

    if (_gItoTestMode == ITO_TEST_MODE_OPEN_TEST)
    {
        nRetVal = _DrvMpTestItoOpenTest();
    }
    else if (_gItoTestMode == ITO_TEST_MODE_SHORT_TEST)
    {
        nRetVal = _DrvMpTestItoShortTest();
    }
    else if (_gItoTestMode == ITO_TEST_MODE_WATERPROOF_TEST)
    {
        nRetVal = _DrvMpTestItoWaterProofTest();
    }
    else
    {
        DBG("*** Undefined Mp Test Mode = %d ***\n", _gItoTestMode);
        return;
    }

    DBG("*** ctp mp test result = %d ***\n", nRetVal);
    
    if (nRetVal == 0)
    {
        _gCtpMpTestStatus = ITO_TEST_OK; //PASS
        g_IsInMpTest = 0;
        DBG("mp test success\n");
    }
    else
    {
        _gTestRetryCount --;
        if (_gTestRetryCount > 0)
        {
            DBG("_gTestRetryCount = %d\n", _gTestRetryCount);
            queue_work(_gCtpMpTestWorkQueue, &_gCtpItoTestWork);
        }
        else
        {
            if (nRetVal == -1)
            {
                _gCtpMpTestStatus = ITO_TEST_FAIL;
            }
            else if (nRetVal == -2)
            {
                _gCtpMpTestStatus = ITO_TEST_GET_TP_TYPE_ERROR;
            }
            else
            {
                _gCtpMpTestStatus = ITO_TEST_UNDEFINED_ERROR;
            }

            g_IsInMpTest = 0;
            DBG("mp test failed\n");
        }
    }
}

/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

signed int DrvMpTestGetTestResult(void)
{
    DBG("*** %s() ***\n", __func__);
    DBG("_gCtpMpTestStatus = %d\n", _gCtpMpTestStatus);

    return _gCtpMpTestStatus;
}

void DrvMpTestGetTestFailChannel(ItoTestMode_e eItoTestMode, unsigned char *pFailChannel, unsigned int *pFailChannelCount)
{
    unsigned int i;
    
    DBG("*** %s() ***\n", __func__);
    DBG("_gTestFailChannelCount = %d\n", _gTestFailChannelCount);
    
    for (i = 0; i < MAX_MUTUAL_NUM; i ++)
    {
    	  pFailChannel[i] = _gTestFailChannel[i];
    }
    
    *pFailChannelCount = MAX_MUTUAL_NUM; // Return the test result of all channels, APK will filter out the fail channels.
}

void DrvMpTestGetTestDataLog(ItoTestMode_e eItoTestMode, unsigned char *pDataLog, unsigned int *pLength)
{
    unsigned int i, j, k;
    
    DBG("*** %s() ***\n", __func__);

    if (eItoTestMode == ITO_TEST_MODE_OPEN_TEST)
    {
        k = 0;
        
        for (j = 0; j < _gDriveLineNum; j ++)
//        for (j = 0; j < (_gDriveLineNum-1); j ++)
        {
            for (i = 0; i < _gSenseLineNum; i ++)
            {
//                DBG("\nDrive%d, Sense%d, Value = %d\t", j, i, _gResult[i * _gDriveLineNum + j]); // add for debug

                if (_gResult[i * _gDriveLineNum + j] >= 0)
                {
                    pDataLog[k*5] = 0; // + : a positive number
                }
                else
                {
                    pDataLog[k*5] = 1; // - : a negative number
                }

                pDataLog[k*5+1] = (_gResult[i * _gDriveLineNum + j] >> 24) & 0xFF;
                pDataLog[k*5+2] = (_gResult[i * _gDriveLineNum + j] >> 16) & 0xFF;
                pDataLog[k*5+3] = (_gResult[i * _gDriveLineNum + j] >> 8) & 0xFF;
                pDataLog[k*5+4] = (_gResult[i * _gDriveLineNum + j]) & 0xFF;
                
                k ++;
            }
        }

        DBG("\nk = %d\n", k);

        *pLength = k*5;
    }
    else if (eItoTestMode == ITO_TEST_MODE_SHORT_TEST)
    {
        k = 0;
        
        for (i = 0; i < (_gDriveLineNum-1 + _gSenseLineNum); i++)
        {
            if (_gResult[i] >= 0)
            {
                pDataLog[k*5] = 0; // + : a positive number
            }
            else
            {
                pDataLog[k*5] = 1; // - : a negative number
            }

            pDataLog[k*5+1] = (_gResult[i] >> 24) & 0xFF;
            pDataLog[k*5+2] = (_gResult[i] >> 16) & 0xFF;
            pDataLog[k*5+3] = (_gResult[i] >> 8) & 0xFF;
            pDataLog[k*5+4] = (_gResult[i]) & 0xFF;
            k ++;
        }

        DBG("\nk = %d\n", k);

        *pLength = k*5;
    }
    else if (eItoTestMode == ITO_TEST_MODE_WATERPROOF_TEST)
    {
        k = 0;

        for (i = 0; i < _gWaterProofNum; i++)
        {
            if (_gResultWater[i] >= 0)
            {
                pDataLog[k*5] = 0; // + : a positive number
            }
            else
            {
                pDataLog[k*5] = 1; // - : a negative number
            }

            pDataLog[k*5+1] = (_gResultWater[i] >> 24) & 0xFF;
            pDataLog[k*5+2] = (_gResultWater[i] >> 16) & 0xFF;
            pDataLog[k*5+3] = (_gResultWater[i] >> 8) & 0xFF;
            pDataLog[k*5+4] = (_gResultWater[i]) & 0xFF;
            k ++;
        }

        DBG("\nk = %d\n", k);

        *pLength = k*5;
    }
    else 
    {
        DBG("*** Undefined MP Test Mode ***\n");
    }
}

void DrvMpTestGetTestScope(TestScopeInfo_t *pInfo)
{
    DBG("*** %s() ***\n", __func__);

    pInfo->nMy = _gDriveLineNum;
    pInfo->nMx = _gSenseLineNum;
    pInfo->nKeyNum = KEY_NUM;

    DBG("*** My = %d ***\n", pInfo->nMy);
    DBG("*** Mx = %d ***\n", pInfo->nMx);
    DBG("*** KeyNum = %d ***\n", pInfo->nKeyNum);    
}

void DrvMpTestScheduleMpTestWork(ItoTestMode_e eItoTestMode)
{
    DBG("*** %s() ***\n", __func__);

    if (g_IsInMpTest == 0)
    {
        DBG("ctp mp test start\n");
        
        _gItoTestMode = eItoTestMode;
        g_IsInMpTest = 1;
        _gTestRetryCount = CTP_MP_TEST_RETRY_COUNT;
        _gCtpMpTestStatus = ITO_TEST_UNDER_TESTING;
        
        queue_work(_gCtpMpTestWorkQueue, &_gCtpItoTestWork);
    }
}

void DrvMpTestCreateMpTestWorkQueue(void)
{
    DBG("*** %s() ***\n", __func__);

    _gCtpMpTestWorkQueue = create_singlethread_workqueue("ctp_mp_test");
    INIT_WORK(&_gCtpItoTestWork, _DrvMpTestItoTestDoWork);
}

#endif //CONFIG_ENABLE_ITO_MP_TEST
#endif //CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC