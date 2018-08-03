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
 * @file    mstar_drv_utility_adaption.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

////////////////////////////////////////////////////////////
/// Included Files
////////////////////////////////////////////////////////////
#include "mstar_drv_utility_adaption.h"

////////////////////////////////////////////////////////////
/// Data Types
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Constant
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Variables
////////////////////////////////////////////////////////////
extern unsigned int SLAVE_I2C_ID_DBBUS;
extern unsigned int SLAVE_I2C_ID_DWI2C;

extern struct i2c_client *g_I2cClient;
extern struct input_dev *g_InputDevice;
extern struct mutex g_Mutex;

extern unsigned char g_ChipType;
extern unsigned char g_IsUpdateFirmware;
extern unsigned char g_IsBypassHotknot;

////////////////////////////////////////////////////////////
/// Macro
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Function Prototypes
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Function Implementation
////////////////////////////////////////////////////////////


#ifdef CONFIG_ENABLE_DMA_IIC
#include <linux/dma-mapping.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/vmalloc.h>

static unsigned char *I2CDMABuf_va = NULL;
static dma_addr_t I2CDMABuf_pa = 0;

void DmaAlloc(void)
{
    if (NULL == I2CDMABuf_va)
    {
        g_InputDevice->dev.coherent_dma_mask = DMA_BIT_MASK(32);
        I2CDMABuf_va = (unsigned char *)dma_alloc_coherent(&g_InputDevice->dev, MAX_I2C_TRANSACTION_LENGTH_LIMIT, &I2CDMABuf_pa, GFP_KERNEL);
    }
    
    if (NULL == I2CDMABuf_va)
    {
        DBG("DmaAlloc FAILED!\n");
    }
    else
    {
        DBG("DmaAlloc SUCCESS!\n");
    }
}

void DmaReset(void)
{
    DBG("Dma memory reset!\n");

    memset(I2CDMABuf_va, 0, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
}

void DmaFree(void)
{
    if (NULL != I2CDMABuf_va)
    {
	      dma_free_coherent(&g_InputDevice->dev, MAX_I2C_TRANSACTION_LENGTH_LIMIT, I2CDMABuf_va, I2CDMABuf_pa);
	      I2CDMABuf_va = NULL;
	      I2CDMABuf_pa = 0;

        DBG("DmaFree SUCCESS!\n");
    }
}
#endif //CONFIG_ENABLE_DMA_IIC

//------------------------------------------------------------------------------//

unsigned short RegGet16BitValue(unsigned short nAddr)
{
    unsigned char tx_data[3] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF};
    unsigned char rx_data[2] = {0};

    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data[0], 2);

    return (rx_data[1] << 8 | rx_data[0]);
}

unsigned char RegGetLByteValue(unsigned short nAddr)
{
    unsigned char tx_data[3] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF};
    unsigned char rx_data = {0};

    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data, 1);

    return (rx_data);
}

unsigned char RegGetHByteValue(unsigned short nAddr)
{
    unsigned char tx_data[3] = {0x10, (nAddr >> 8) & 0xFF, (nAddr & 0xFF) + 1};
    unsigned char rx_data = {0};

    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data, 1);

    return (rx_data);
}

void RegGetXBitValue(unsigned short nAddr, unsigned char * pRxData, unsigned short nLength, unsigned short nMaxI2cLengthLimit)
{
    unsigned short nReadAddr = nAddr;    
    unsigned short nReadSize = 0;
    unsigned short nLeft = nLength;    
    unsigned short nOffset = 0; 
    unsigned char tx_data[3] = {0};

    tx_data[0] = 0x10;

    mutex_lock(&g_Mutex);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    while(nLeft > 0)
    {
        if(nLeft >= nMaxI2cLengthLimit)
        {
            nReadSize = nMaxI2cLengthLimit;
            //DBG("*** RegGetXBitValue# Length >= I2cMax   nReadAddr=%x, nReadSize=%d ***\n", nReadAddr, nReadSize);

            tx_data[1] = (nReadAddr >> 8) & 0xFF;
            tx_data[2] = nReadAddr & 0xFF;            
        
            IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
            IicReadData(SLAVE_I2C_ID_DBBUS, &pRxData[nOffset], nReadSize);
        
            nReadAddr = nReadAddr + nReadSize;    //set next read address
            nLeft = nLeft - nReadSize;
            nOffset = nOffset + nReadSize;
            //DBG("*** RegGetXBitValue# Length >= I2cMax   nLeft=%d, nOffset=%d ***\n", nLeft, nOffset);
        }
        else
        {
            nReadSize = nLeft;
            //DBG("*** RegGetXBitValue# Length < I2cMax   nReadAddr=%x, nReadSize=%d ***\n", nReadAddr, nReadSize);

            tx_data[1] = (nReadAddr >> 8) & 0xFF;
            tx_data[2] = nReadAddr & 0xFF;            
        
            IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
            IicReadData(SLAVE_I2C_ID_DBBUS, &pRxData[nOffset], nReadSize);
            
            nLeft = 0;
            nOffset = nOffset + nReadSize;            
            //DBG("*** RegGetXBitValue# Length < I2cMax   nLeft=%d, nOffset=%d ***\n", nLeft, nOffset);
        }
    }

    mutex_unlock(&g_Mutex);    
}

void RegSet16BitValue(unsigned short nAddr, unsigned short nData)
{
    unsigned char tx_data[5] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF, nData & 0xFF, nData >> 8};
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 5);
}

void RegSetLByteValue(unsigned short nAddr, unsigned char nData)
{
    unsigned char tx_data[4] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF, nData};
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 4);
}

void RegSetHByteValue(unsigned short nAddr, unsigned char nData)
{
    unsigned char tx_data[4] = {0x10, (nAddr >> 8) & 0xFF, (nAddr & 0xFF) + 1, nData};
    IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 4);
}

void RegSet16BitValueOn(unsigned short nAddr, unsigned short nData) //Set bit on nData from 0 to 1
{
    unsigned short rData = RegGet16BitValue(nAddr);
    rData |= nData;
    RegSet16BitValue(nAddr, rData);
}

void RegSet16BitValueOff(unsigned short nAddr, unsigned short nData) //Set bit on nData from 1 to 0
{
    unsigned short rData = RegGet16BitValue(nAddr);
    rData &= (~nData);
    RegSet16BitValue(nAddr, rData);
}

unsigned short RegGet16BitValueByAddressMode(unsigned short nAddr, AddressMode_e eAddressMode)
{
    unsigned short nData = 0;
    
    if (eAddressMode == ADDRESS_MODE_16BIT)
    {
        nAddr = nAddr - (nAddr & 0xFF) + ((nAddr & 0xFF) << 1);
    }
    
    nData = RegGet16BitValue(nAddr);
    
    return nData;
}
	
void RegSet16BitValueByAddressMode(unsigned short nAddr, unsigned short nData, AddressMode_e eAddressMode)
{
    if (eAddressMode == ADDRESS_MODE_16BIT)
    {
        nAddr = nAddr - (nAddr & 0xFF) + ((nAddr & 0xFF) << 1);
    }
    
    RegSet16BitValue(nAddr, nData);
}

void RegMask16BitValue(unsigned short nAddr, unsigned short nMask, unsigned short nData, AddressMode_e eAddressMode) 
{
    unsigned short nTmpData = 0;
    
    if (nData > nMask)
    {
        return;
    }

    nTmpData = RegGet16BitValueByAddressMode(nAddr, eAddressMode);
    nTmpData = (nTmpData & (~nMask));
    nTmpData = (nTmpData | nData);
    RegSet16BitValueByAddressMode(nAddr, nTmpData, eAddressMode);
}

signed int DbBusEnterSerialDebugMode(void)
{
    signed int rc = 0;
    unsigned char data[5];

    // Enter the Serial Debug Mode
    data[0] = 0x53;
    data[1] = 0x45;
    data[2] = 0x52;
    data[3] = 0x44;
    data[4] = 0x42;

    rc = IicWriteData(SLAVE_I2C_ID_DBBUS, data, 5);
    
    return rc;
}

void DbBusExitSerialDebugMode(void)
{
    unsigned char data[1];

    // Exit the Serial Debug Mode
    data[0] = 0x45;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);

    // Delay some interval to guard the next transaction
//    udelay(200);        // delay about 0.2ms
}

void DbBusIICUseBus(void)
{
    unsigned char data[1];

    // IIC Use Bus
    data[0] = 0x35;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusIICNotUseBus(void)
{
    unsigned char data[1];

    // IIC Not Use Bus
    data[0] = 0x34;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusIICReshape(void)
{
    unsigned char data[1];

    // IIC Re-shape
    data[0] = 0x71;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusStopMCU(void)
{
    unsigned char data[1];

    // Stop the MCU
    data[0] = 0x37;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusNotStopMCU(void)
{
    unsigned char data[1];

    // Not Stop the MCU
    data[0] = 0x36;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusResetSlave(void)
{
    unsigned char data[1];

    // IIC Reset Slave
    data[0] = 0x00;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusWaitMCU(void)
{
    unsigned char data[1];

    // Stop the MCU
    data[0] = 0x37;
    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);

    data[0] = 0x61;
    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);    
}

signed int IicWriteData(unsigned char nSlaveId, unsigned char* pBuf, unsigned short nSize)
{
    signed int rc = 0;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    struct i2c_msg msgs[] =
    {
        {
            .addr = nSlaveId,
            .flags = 0, // if read flag is undefined, then it means write flag.
            .len = nSize,
            .buf = pBuf,
        },
    };

    /* If everything went ok (i.e. 1 msg transmitted), return #bytes
       transmitted, else error code. */
    if (g_I2cClient != NULL)
    {
        if (g_ChipType == CHIP_TYPE_MSG28XX && nSlaveId == SLAVE_I2C_ID_DWI2C && (g_IsUpdateFirmware != 0 || g_IsBypassHotknot != 0))
        {
            PRINTF_ERR("Not allow to execute SmBus command while update firmware.\n");
        }
        else
        {
            rc = i2c_transfer(g_I2cClient->adapter, msgs, 1);

            if (rc == 1)
            {
                rc = nSize;
            }
            else // rc < 0
            {
                PRINTF_ERR("IicWriteData() error %d\n", rc);
            }
        }
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    if (g_I2cClient != NULL)
    {
        if (g_ChipType == CHIP_TYPE_MSG28XX && nSlaveId == SLAVE_I2C_ID_DWI2C && (g_IsUpdateFirmware != 0 || g_IsBypassHotknot != 0))
        {
            PRINTF_ERR("Not allow to execute SmBus command while update firmware.\n");
        }
        else
        {
            unsigned char nAddrBefore = g_I2cClient->addr;
            g_I2cClient->addr = nSlaveId;

#ifdef CONFIG_ENABLE_DMA_IIC
            if (nSize > 8 && NULL != I2CDMABuf_va)
            {
                signed int i = 0;
	          
                for (i = 0; i < nSize; i ++)
                {
                    I2CDMABuf_va[i] = pBuf[i];
                }
                g_I2cClient->ext_flag = g_I2cClient->ext_flag | I2C_DMA_FLAG;
                rc = i2c_master_send(g_I2cClient, (unsigned char *)I2CDMABuf_pa, nSize);
            }
            else
            {
                g_I2cClient->ext_flag = g_I2cClient->ext_flag & (~I2C_DMA_FLAG);	
                rc = i2c_master_send(g_I2cClient, pBuf, nSize);
            }
#else
            rc = i2c_master_send(g_I2cClient, pBuf, nSize);
#endif //CONFIG_ENABLE_DMA_IIC
            g_I2cClient->addr = nAddrBefore;

            if (rc < 0)
            {
                PRINTF_ERR("IicWriteData() error %d, nSlaveId=%d, nSize=%d\n", rc, nSlaveId, nSize);
            }
        }            
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#endif
    
    return rc;
}

signed int IicReadData(unsigned char nSlaveId, unsigned char* pBuf, unsigned short nSize)
{
    signed int rc = 0;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    struct i2c_msg msgs[] =
    {
        {
            .addr = nSlaveId,
            .flags = I2C_M_RD, // read flag
            .len = nSize,
            .buf = pBuf,
        },
    };

    /* If everything went ok (i.e. 1 msg transmitted), return #bytes
       transmitted, else error code. */
    if (g_I2cClient != NULL)
    {
        if (g_ChipType == CHIP_TYPE_MSG28XX && nSlaveId == SLAVE_I2C_ID_DWI2C && (g_IsUpdateFirmware != 0 || g_IsBypassHotknot != 0))
        {
            PRINTF_ERR("Not allow to execute SmBus command while update firmware.\n");
        }
        else
        {
            rc = i2c_transfer(g_I2cClient->adapter, msgs, 1);
        
            if (rc == 1)
            {
                rc = nSize;
            }
            else // rc < 0
            {
                PRINTF_ERR("IicReadData() error %d\n", rc);
            }
        }
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    if (g_I2cClient != NULL)
    {
        if (g_ChipType == CHIP_TYPE_MSG28XX && nSlaveId == SLAVE_I2C_ID_DWI2C && (g_IsUpdateFirmware != 0 || g_IsBypassHotknot != 0))
        {
            PRINTF_ERR("Not allow to execute SmBus command while update firmware.\n");
        }
        else
        {
            unsigned char nAddrBefore = g_I2cClient->addr;
            g_I2cClient->addr = nSlaveId;

#ifdef CONFIG_ENABLE_DMA_IIC
            if (nSize > 8 && NULL != I2CDMABuf_va)
            {
                signed int i = 0;
        
                g_I2cClient->ext_flag = g_I2cClient->ext_flag | I2C_DMA_FLAG;
                rc = i2c_master_recv(g_I2cClient, (unsigned char *)I2CDMABuf_pa, nSize);
        
                for (i = 0; i < nSize; i ++)
                {
                    pBuf[i] = I2CDMABuf_va[i];
                }
            }
            else
            {
                g_I2cClient->ext_flag = g_I2cClient->ext_flag & (~I2C_DMA_FLAG);	
                rc = i2c_master_recv(g_I2cClient, pBuf, nSize);
            }
#else
            rc = i2c_master_recv(g_I2cClient, pBuf, nSize);
#endif //CONFIG_ENABLE_DMA_IIC
            g_I2cClient->addr = nAddrBefore;

            if (rc < 0)
            {
                PRINTF_ERR("IicReadData() error %d, nSlaveId=%d, nSize=%d\n", rc, nSlaveId, nSize);
            }
        }            
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#endif
    
    return rc;
}

signed int IicSegmentReadDataByDbBus(unsigned char nRegBank, unsigned char nRegAddr, unsigned char* pBuf, unsigned short nSize, unsigned short nMaxI2cLengthLimit)
{
    signed int rc = 0;
    unsigned short nLeft = nSize;
    unsigned short nOffset = 0;
    unsigned short nSegmentLength = 0;
    unsigned short nReadSize = 0;
    unsigned short nOver = 0;
    unsigned char  szWriteBuf[3] = {0};
    unsigned char  nNextRegBank = nRegBank;
    unsigned char  nNextRegAddr = nRegAddr;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    struct i2c_msg msgs[2] =
    {
        {
            .addr = SLAVE_I2C_ID_DBBUS,
            .flags = 0, // write flag
            .len = 3,
            .buf = szWriteBuf,
        },
        {
            .addr = SLAVE_I2C_ID_DBBUS,
            .flags =  I2C_M_RD, // read flag
        },
    };

    // If everything went ok (i.e. 1 msg transmitted), return #bytes transmitted, else error code. 
    if (g_I2cClient != NULL)
    {
        if (nMaxI2cLengthLimit >= 256)
        {
            nSegmentLength = 256;
        }
        else
        {
            nSegmentLength = 128;
        }
        
        PRINTF_ERR("nSegmentLength = %d\n", nSegmentLength);	// add for debug

        while (nLeft > 0)
        {
            szWriteBuf[0] = 0x10;
            nRegBank = nNextRegBank;
            szWriteBuf[1] = nRegBank;
            nRegAddr = nNextRegAddr;
            szWriteBuf[2] = nRegAddr;

            PRINTF_ERR("nRegBank = 0x%x\n", nRegBank);	// add for debug
            PRINTF_ERR("nRegAddr = 0x%x\n", nRegAddr);	// add for debug

            msgs[1].buf = &pBuf[nOffset];

            if (nLeft > nSegmentLength)
            {
                if ((nRegAddr + nSegmentLength) < MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = nRegAddr + nSegmentLength; 
                    
                    PRINTF_ERR("nNextRegAddr = 0x%x\n", nNextRegAddr);	// add for debug

                    msgs[1].len = nSegmentLength;
                    nLeft -= nSegmentLength;
                    nOffset += msgs[1].len;
                }
                else if ((nRegAddr + nSegmentLength) == MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00; 		
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug

                    msgs[1].len = nSegmentLength;
                    nLeft -= nSegmentLength;
                    nOffset += msgs[1].len;
                }
                else // ((nRegAddr + nSegmentLength) > MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00;
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_INFO("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug
                    
                    nOver = (nRegAddr + nSegmentLength) - MAX_TOUCH_IC_REGISTER_BANK_SIZE;

                    PRINTF_ERR("nOver = 0x%x\n", nOver);	// add for debug

                    msgs[1].len = nSegmentLength - nOver; 		
                    nLeft -= msgs[1].len;
                    nOffset += msgs[1].len;
                }
            }
            else
            {
                if ((nRegAddr + nLeft) < MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = nRegAddr + nLeft; 
                    
                    PRINTF_ERR("nNextRegAddr = 0x%x\n", nNextRegAddr);	// add for debug

                    msgs[1].len = nLeft;
                    nLeft = 0;
//                    nOffset += msgs[1].len;
                }
                else if ((nRegAddr + nLeft) == MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00; 		
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug

                    msgs[1].len = nLeft;
                    nLeft = 0;
//                    nOffset += msgs[1].len;
                }
                else // ((nRegAddr + nLeft) > MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00;
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug
                    
                    nOver = (nRegAddr + nLeft) - MAX_TOUCH_IC_REGISTER_BANK_SIZE;

                    PRINTF_ERR("nOver = 0x%x\n", nOver);	// add for debug

                    msgs[1].len = nLeft - nOver; 		
                    nLeft -= msgs[1].len;
                    nOffset += msgs[1].len;
                }
            }

            rc = i2c_transfer(g_I2cClient->adapter, &msgs[0], 2);
            if (rc == 2)
            {
                nReadSize = nReadSize + msgs[1].len;
            }
            else // rc < 0
            {
                PRINTF_ERR("IicSegmentReadDataByDbBus() -> i2c_transfer() error %d\n", rc);
                
                return rc;
            }
        }
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    if (g_I2cClient != NULL)
    {
#ifndef CONFIG_ENABLE_DMA_IIC
        unsigned char *pReadBuf = NULL;
#endif //CONFIG_ENABLE_DMA_IIC
        unsigned short nLength = 0;
        unsigned char nAddrBefore = g_I2cClient->addr;
        
        g_I2cClient->addr = SLAVE_I2C_ID_DBBUS;

        if (nMaxI2cLengthLimit >= 256)
        {
            nSegmentLength = 256;
        }
        else
        {
            nSegmentLength = 128;
        }

        PRINTF_ERR("nSegmentLength = %d\n", nSegmentLength);	// add for debug

#ifdef CONFIG_ENABLE_DMA_IIC
        if (NULL != I2CDMABuf_va)
        {
            signed int i = 0;

            while (nLeft > 0)
            {
                szWriteBuf[0] = 0x10;
                nRegBank = nNextRegBank;
                szWriteBuf[1] = nRegBank;
                nRegAddr = nNextRegAddr;
                szWriteBuf[2] = nRegAddr;

                PRINTF_ERR("nRegBank = 0x%x\n", nRegBank);	// add for debug
                PRINTF_ERR("nRegAddr = 0x%x\n", nRegAddr);	// add for debug

                if (nLeft > nSegmentLength)
                {
                    if ((nRegAddr + nSegmentLength) < MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                    {
                        nNextRegAddr = nRegAddr + nSegmentLength; 
                    
                        PRINTF_ERR("nNextRegAddr = 0x%x\n", nNextRegAddr);	// add for debug

                        nLength = nSegmentLength;
                        nLeft -= nSegmentLength;
                    }
                    else if ((nRegAddr + nSegmentLength) == MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                    {
                        nNextRegAddr = 0x00; 		
                        nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                        PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug

                        nLength = nSegmentLength;
                        nLeft -= nSegmentLength;
                    }
                    else // ((nRegAddr + nSegmentLength) > MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                    {
                        nNextRegAddr = 0x00;
                        nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                        PRINTF_INFO("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug
                    
                        nOver = (nRegAddr + nSegmentLength) - MAX_TOUCH_IC_REGISTER_BANK_SIZE;

                        PRINTF_ERR("nOver = 0x%x\n", nOver);	// add for debug

                        nLength = nSegmentLength - nOver; 		
                        nLeft -= nLength;
                    }
                }
                else
                {
                    if ((nRegAddr + nLeft) < MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                    {
                        nNextRegAddr = nRegAddr + nLeft; 
                    
                        PRINTF_ERR("nNextRegAddr = 0x%x\n", nNextRegAddr);	// add for debug

                        nLength = nLeft;
                        nLeft = 0;
                    }
                    else if ((nRegAddr + nLeft) == MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                    {
                        nNextRegAddr = 0x00; 		
                        nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                        PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug

                        nLength = nLeft;
                        nLeft = 0;
                    }
                    else // ((nRegAddr + nLeft) > MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                    {
                        nNextRegAddr = 0x00;
                        nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                        PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug
                    
                        nOver = (nRegAddr + nLeft) - MAX_TOUCH_IC_REGISTER_BANK_SIZE;

                        PRINTF_ERR("nOver = 0x%x\n", nOver);	// add for debug

                        nLength = nLeft - nOver; 		
                        nLeft -= nLength;
                    }
                }

                g_I2cClient->ext_flag = g_I2cClient->ext_flag & (~I2C_DMA_FLAG);
                rc = i2c_master_send(g_I2cClient, &szWriteBuf[0], 3);
                if (rc < 0)
                {
                    PRINTF_ERR("IicSegmentReadDataByDbBus() -> i2c_master_send() error %d\n", rc);

                    return rc;
                }

                g_I2cClient->ext_flag = g_I2cClient->ext_flag | I2C_DMA_FLAG;
                rc = i2c_master_recv(g_I2cClient, (unsigned char *)I2CDMABuf_pa, nLength);
                if (rc < 0)
                {
                    PRINTF_ERR("IicSegmentReadDataByDbBus() -> i2c_master_recv() error %d\n", rc);
                    
                    return rc;
                }
                else
                {
                    for (i = 0; i < nLength; i ++)
                    {
                        pBuf[i+nOffset] = I2CDMABuf_va[i];
                    }
                    nOffset += nLength;

                    nReadSize = nReadSize + nLength;
                }
            }
        }
        else
        {
            PRINTF_ERR("IicSegmentReadDataByDbBus() -> I2CDMABuf_va is NULL\n");
        }
#else
        while (nLeft > 0)
        {
            szWriteBuf[0] = 0x10;
            nRegBank = nNextRegBank;
            szWriteBuf[1] = nRegBank;
            nRegAddr = nNextRegAddr;
            szWriteBuf[2] = nRegAddr;

            PRINTF_ERR("nRegBank = 0x%x\n", nRegBank);	// add for debug
            PRINTF_ERR("nRegAddr = 0x%x\n", nRegAddr);	// add for debug

            pReadBuf = &pBuf[nOffset];

            if (nLeft > nSegmentLength)
            {
                if ((nRegAddr + nSegmentLength) < MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = nRegAddr + nSegmentLength; 
                    
                    PRINTF_ERR("nNextRegAddr = 0x%x\n", nNextRegAddr);	// add for debug

                    nLength = nSegmentLength;
                    nLeft -= nSegmentLength;
                    nOffset += nLength;
                }
                else if ((nRegAddr + nSegmentLength) == MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00; 		
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug

                    nLength = nSegmentLength;
                    nLeft -= nSegmentLength;
                    nOffset += nLength;
                }
                else // ((nRegAddr + nSegmentLength) > MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00;
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_INFO("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug
                    
                    nOver = (nRegAddr + nSegmentLength) - MAX_TOUCH_IC_REGISTER_BANK_SIZE;

                    PRINTF_ERR("nOver = 0x%x\n", nOver);	// add for debug

                    nLength = nSegmentLength - nOver; 		
                    nLeft -= nLength;
                    nOffset += nLength;
                }
            }
            else
            {
                if ((nRegAddr + nLeft) < MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = nRegAddr + nLeft; 
                    
                    PRINTF_ERR("nNextRegAddr = 0x%x\n", nNextRegAddr);	// add for debug

                    nLength = nLeft;
                    nLeft = 0;
//                    nOffset += nLength;
                }
                else if ((nRegAddr + nLeft) == MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00; 		
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug

                    nLength = nLeft;
                    nLeft = 0;
//                    nOffset += nLength;
                }
                else // ((nRegAddr + nLeft) > MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00;
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug
                    
                    nOver = (nRegAddr + nLeft) - MAX_TOUCH_IC_REGISTER_BANK_SIZE;

                    PRINTF_ERR("nOver = 0x%x\n", nOver);	// add for debug

                    nLength = nLeft - nOver; 		
                    nLeft -= nLength;
                    nOffset += nLength;
                }
            }

            rc = i2c_master_send(g_I2cClient, &szWriteBuf[0], 3);
            if (rc < 0)
            {
                PRINTF_ERR("IicSegmentReadDataByDbBus() -> i2c_master_send() error %d\n", rc);

                return rc;
            }

            rc = i2c_master_recv(g_I2cClient, pReadBuf, nLength);
            if (rc < 0)
            {
                PRINTF_ERR("IicSegmentReadDataByDbBus() -> i2c_master_recv() error %d\n", rc);

                return rc;
            }
            else
            {
                nReadSize = nReadSize + nLength;
            }
        }
#endif //CONFIG_ENABLE_DMA_IIC
        g_I2cClient->addr = nAddrBefore;
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#endif
    
    return nReadSize;
}

signed int IicSegmentReadDataBySmBus(unsigned short nAddr, unsigned char* pBuf, unsigned short nSize, unsigned short nMaxI2cLengthLimit)
{
    signed int rc = 0;
    unsigned short nLeft = nSize;
    unsigned short nOffset = 0;
    unsigned short nReadSize = 0;
    unsigned char  szWriteBuf[3] = {0};

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    struct i2c_msg msgs[2] =
    {
        {
            .addr = SLAVE_I2C_ID_DWI2C,
            .flags = 0, // write flag
            .len = 3,
            .buf = szWriteBuf,
        },
        {
            .addr = SLAVE_I2C_ID_DWI2C,
            .flags =  I2C_M_RD, // read flag
        },
    };

    // If everything went ok (i.e. 1 msg transmitted), return #bytes transmitted, else error code. 
    if (g_I2cClient != NULL)
    {
        while (nLeft > 0)
        {
            szWriteBuf[0] = 0x53;
            szWriteBuf[1] = ((nAddr + nOffset) >> 8) & 0xFF;
            szWriteBuf[2] = (nAddr + nOffset) & 0xFF;

            msgs[1].buf = &pBuf[nOffset];

            if (nLeft > nMaxI2cLengthLimit)
            {
                msgs[1].len = nMaxI2cLengthLimit;
                nLeft -= nMaxI2cLengthLimit;
                nOffset += msgs[1].len;
            }
            else
            {
                msgs[1].len = nLeft;
                nLeft = 0;
//                nOffset += msgs[1].len;
            }

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM
            mdelay(I2C_SMBUS_WRITE_COMMAND_DELAY_FOR_QCOM_PLATFORM); // TODO : add for testing
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM

            rc = i2c_transfer(g_I2cClient->adapter, &msgs[0], 2);
            if (rc == 2)
            {
                nReadSize = nReadSize + msgs[1].len;
            }
            else // rc < 0
            {
                PRINTF_ERR("IicSegmentReadDataBySmBus() -> i2c_transfer() error %d\n", rc);
                
                return rc;
            }
        }
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    if (g_I2cClient != NULL)
    {
#ifndef CONFIG_ENABLE_DMA_IIC
        unsigned char *pReadBuf = NULL;
#endif //CONFIG_ENABLE_DMA_IIC
        unsigned short nLength = 0;
        unsigned char nAddrBefore = g_I2cClient->addr;
        
        g_I2cClient->addr = SLAVE_I2C_ID_DWI2C;

#ifdef CONFIG_ENABLE_DMA_IIC
        while (nLeft > 0)
        {
            signed int i = 0;

            szWriteBuf[0] = 0x53;
            szWriteBuf[1] = ((nAddr + nOffset) >> 8) & 0xFF;
            szWriteBuf[2] = (nAddr + nOffset) & 0xFF;

            if (nLeft > nMaxI2cLengthLimit)
            {
                nLength = nMaxI2cLengthLimit;
                nLeft -= nMaxI2cLengthLimit;
            }
            else
            {
                nLength = nLeft;
                nLeft = 0;
            }
            
            g_I2cClient->ext_flag = g_I2cClient->ext_flag & (~I2C_DMA_FLAG);
            rc = i2c_master_send(g_I2cClient, &szWriteBuf[0], 3);
            if (rc < 0)
            {
                PRINTF_ERR("IicSegmentReadDataBySmBus() -> i2c_master_send() error %d\n", rc);

                return rc;
            }

            g_I2cClient->ext_flag = g_I2cClient->ext_flag | I2C_DMA_FLAG;
            rc = i2c_master_recv(g_I2cClient, (unsigned char *)I2CDMABuf_pa, nLength);
            if (rc < 0)
            {
                PRINTF_ERR("IicSegmentReadDataBySmBus() -> i2c_master_recv() error %d\n", rc);

                return rc;
            }
            else
            {
                for (i = 0; i < nLength; i ++)
                {
                    pBuf[i+nOffset] = I2CDMABuf_va[i];
                }
                nOffset += nLength;

                nReadSize = nReadSize + nLength;
            }
        }
#else
        while (nLeft > 0)
        {
            szWriteBuf[0] = 0x53;
            szWriteBuf[1] = ((nAddr + nOffset) >> 8) & 0xFF;
            szWriteBuf[2] = (nAddr + nOffset) & 0xFF;

            pReadBuf = &pBuf[nOffset];

            if (nLeft > nMaxI2cLengthLimit)
            {
                nLength = nMaxI2cLengthLimit;
                nLeft -= nMaxI2cLengthLimit;
                nOffset += nLength;
            }
            else
            {
                nLength = nLeft;
                nLeft = 0;
//                nOffset += nLength;
            }
            
            rc = i2c_master_send(g_I2cClient, &szWriteBuf[0], 3);
            if (rc < 0)
            {
                PRINTF_ERR("IicSegmentReadDataBySmBus() -> i2c_master_send() error %d\n", rc);

                return rc;
            }

            rc = i2c_master_recv(g_I2cClient, pReadBuf, nLength);
            if (rc < 0)
            {
                PRINTF_ERR("IicSegmentReadDataBySmBus() -> i2c_master_recv() error %d\n", rc);

                return rc;
            }
            else
            {
                nReadSize = nReadSize + nLength;
            }
        }
#endif //CONFIG_ENABLE_DMA_IIC
        g_I2cClient->addr = nAddrBefore;
   }
   else
   {
       PRINTF_ERR("i2c client is NULL\n");
   }
#endif
	
   return nReadSize;
}

void mstpMemSet(void *pDst, signed char nVal, unsigned int nSize)
{
    memset(pDst, nVal, nSize);
}

void mstpMemCopy(void *pDst, void *pSource, unsigned int nSize)
{
    memcpy(pDst, pSource, nSize);
}

void mstpDelay(unsigned int nTime)
{
    mdelay(nTime);
}

//------------------------------------------------------------------------------//
