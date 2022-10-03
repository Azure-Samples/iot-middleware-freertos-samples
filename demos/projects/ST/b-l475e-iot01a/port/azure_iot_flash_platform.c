/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <string.h>

#include "azure_iot_flash_platform.h"

#include "azure_iot_flash_platform_port.h"

#include "stm32l4xx_hal.h"
/* Logging */
#include "azure_iot.h"

#include "azure/core/az_base64.h"

#include "mbedtls/md.h"

#define azureiotflashSHA_256_SIZE    32

static uint8_t ucPartitionReadBuffer[ 32 ];
static uint8_t ucDecodedManifestHash[ azureiotflashSHA_256_SIZE ];
static uint8_t ucCalculatedHash[ azureiotflashSHA_256_SIZE ];

static AzureIoTResult_t prvBase64Decode( uint8_t * base64Encoded,
                                         size_t ulBase64EncodedLength,
                                         uint8_t * pucOutputBuffer,
                                         size_t bufferLen,
                                         size_t * outputSize )
{
    az_result xCoreResult;

    az_span encodedSpan = az_span_create( base64Encoded, ulBase64EncodedLength );

    az_span outputSpan = az_span_create( pucOutputBuffer, bufferLen );

    if( az_result_failed( xCoreResult = az_base64_decode( outputSpan, encodedSpan, ( int32_t * ) outputSize ) ) )
    {
        AZLogError( ( "az_base64_decode failed: core error=0x%08x", xCoreResult ) );
        return eAzureIoTErrorFailed;
    }

    LogInfo( ( "Unencoded the base64 encoding\r\n" ) );

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_Init( AzureADUImage_t * const pxAduImage )
{
    (void) pxAduImage;

    // if BFB2 is on, clear bank 1, otherwise clear bank 2
    // 1) unlock flash CR register
    // 2) unlock option bytes
    // 3) program option byte with toggled BFB2 state
    // 4) set OBL_LAUNCH bit and wait for it to clear
    // 5) reset
   
    // Using HAL calls this is:
    // HAL_FLASH_Unlock();
    // HAL_FLASH_OB_Unlock();
    // HAL_FLASHEx_OBProgram(&optionProgramInit);
    // HAL_FLASH_OB_Launch();
    // __NVIC_SystemReset();

    // LogInfo( ( OB_USER_BFB2) );
    // LogInfo( ( "BFB2 ^^\r\n" ) );


    static FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PAGEError;
    uint32_t FlashError = 0;
    // int sofar=0;

    FLASH_OBProgramInitTypeDef  optionBytes;

    // EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    // EraseInitStruct.Banks   = FLASH_BANK_1;
    // EraseInitStruct.Page = 252;
    // EraseInitStruct.NbPages     = 1;
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_MASSERASE;
    // EraseInitStruct.Banks   = FLASH_BANK_2;
    
    /* Clear OPTVERR bit set on virgin samples. */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
    HAL_FLASHEx_OBGetConfig(&optionBytes); // Get current optionbytes configuration
    LogInfo( ( "get current option bytes\r\n" ) );

    
    // HAL_FLASH_OB_Unlock();

    // LogInfo( ( "option bytes unlocked\r\n" ) );
    
    // If BFB2 set, we will reset it
    if ((optionBytes.USERConfig & OB_BFB2_ENABLE) == OB_BFB2_ENABLE)
    {
        EraseInitStruct.Banks   = FLASH_BANK_1;
        pxAduImage->xUpdatePartition = (FLASH_BASE + FLASH_BANK_SIZE);
        LogInfo( ( "erasing bank 1\r\n" ) );
    }
    else
    {
        EraseInitStruct.Banks   = FLASH_BANK_2;
        pxAduImage->xUpdatePartition = (FLASH_BASE + FLASH_BANK_SIZE);
        LogInfo( ( "erasing bank 2\r\n" ) );
    }
    // HAL_FLASH_OB_Lock();

    HAL_FLASH_Unlock();
    // erase non-boot bank
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) == HAL_OK){
        LogInfo( ( "flash erased\r\n" ) );
    }
    else
    {
        /*Error occurred while page erase.*/
        FlashError = HAL_FLASH_GetError();
        HAL_FLASH_Lock();
        LogInfo( ( FlashError ) );
        LogInfo( ( "Error erasing ^^\r\n" ) );
        return FlashError;
    }

    HAL_FLASH_Lock();

    // while (sofar<numberofwords)
    // {
        // if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, 0x08080010, OB_USER_BFB2) == HAL_OK)
        // {
        //     LogInfo( ( "we did it joe\r\n" ) );
        // }
        // else
        // {
        // /* Error occurred while writing data in Flash memory*/
        //     FlashError = HAL_FLASH_GetError();
        //     LogInfo( ( FlashError ) );
        //     LogInfo( ( "Error writing ^^\r\n" ) );
        //     if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, 0x08080060, FlashError) == HAL_OK)
        //     {
        //         LogInfo( ( "we did it joe\r\n" ) );
        //     }
        //     else
        //     {
        //     /* Error occurred while writing data in Flash memory*/
        //         FlashError = HAL_FLASH_GetError();
        //         LogInfo( ( FlashError ) );
        //         LogInfo( ( "Error writing ^^\r\n" ) );
        //         return FlashError;
        //     }
        // }
        
    // }
    // HAL_FLASH_Lock();




    // LogInfo( ( "BOOT MODE:\r\n" ) );
    // LogInfo( (__HAL_SYSCFG_GET_BOOT_MODE()) );
    // LogInfo( ( "\r\n" ) );

    LogInfo( ( "AzureIoTPlatform_Init()\r\n" ) );

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_WriteBlock( AzureADUImage_t * const pxAduImage,
                                              uint32_t ulOffset,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize )
{
    // (void)pxAduImage;
    uint8_t * nextWriteAddr = pxAduImage->xUpdatePartition + ulOffset;
    LogInfo( ( "AzureIoTPlatform_WriteBlock(): start write address %04x\r\n", pxAduImage->xUpdatePartition ) );
    LogInfo( ( "AzureIoTPlatform_WriteBlock(): next write address %04x\r\n", nextWriteAddr ) );
    LogInfo( ( "AzureIoTPlatform_WriteBlock(): offset %04x\r\n", ulOffset ) );
    LogInfo( ( "AzureIoTPlatform_WriteBlock(): should be same as next write address %04x\r\n", pxAduImage->xUpdatePartition + ulOffset ) );
    // int sofar = 0;
    uint8_t * nextReadAddr = pData;
    
    //if last block
        // write 1..n-1 packets in loop
        // write final n packet
    // else
        // write 1-n packets in loop

    //write 1..n-1 packets in loop
    //if last block
        // write final n packet
    // else
        // write n packet
    
    
    // if (ulOffset > 28671) {
    //     LogInfo( ( "AzureIoTPlatform_WriteBlock(): offset %i\r\n", ulOffset ) );
    //     LogInfo( ( "AzureIoTPlatform_WriteBlock(): ulBlockSize %i\r\n", ulBlockSize ) );
    //     LogInfo( ( "AzureIoTPlatform_WriteBlock(): write address %04x\r\n", 0x08080000 + ulOffset ) );
    //     return eAzureIoTSuccess;
    // }
    // if (ulOffset > 24575) {
    //     if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FAST_AND_LAST, 0x08080000 + ulOffset, pData) == HAL_OK)
    //     {
    //         LogInfo( ( "LAST ONE we did it joe\r\n" ) );
    //     }
    //     else
    //     {
    //     /* Error occurred while writing data in Flash memory*/
    //         LogInfo( ( HAL_FLASH_GetError() ) );
    //         LogInfo( ( "Error writing ^^\r\n" ) );
    //         return HAL_FLASH_GetError();
    //     }
    //     LogInfo( ( "AzureIoTPlatform_WriteBlock(): offset %i\r\n", ulOffset ) );
    //     return eAzureIoTSuccess;
    // }
    HAL_FLASH_Unlock();

    // fast programming - for packets 1...n on blocks 1...(n-1) and packet 1...(n-1) on block n
    // write sections 1...n-1 of block
    while (nextWriteAddr < pxAduImage->xUpdatePartition + ulOffset + ulBlockSize - 256) {
        // FLASH_TYPEPROGRAM_FAST 256 bytes, so need to do this 16 times per chunk
        //         uint64_t data_to_send = pData[sofar+7] | (pData[sofar+6]<<8) | (pData[sofar+5]<<16) | (pData[sofar+4]<<24) | pData[sofar+3]<<32 | (pData[sofar+2]<<40) | (pData[sofar+1]<<48) | (pData[sofar]<<56);
        // uint64_t data_to_send = pData[sofar] | (pData[sofar+1]<<8) | (pData[sofar+2]<<16) | (pData[sofar+3]<<24);
        // LogInfo( ( "AzureIoTPlatform_WriteBlock(): data %04x\r\n", data_to_send ) );
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FAST, nextWriteAddr, nextReadAddr) != HAL_OK)
        // if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FAST, nextWriteAddr,  (pData + sofar)) == HAL_OK)
        {
            /* Error occurred while writing data in Flash memory*/
            LogInfo( ( "Error Writing regular %04x\r\n", HAL_FLASH_GetError() ) );
            // LogInfo( ( "Error Writing regular %"PRIu32"\r\n", HAL_FLASH_GetError() ) );
            HAL_FLASH_Lock();
            return HAL_FLASH_GetError();
            // LogInfo( ( "AzureIoTPlatform_WriteBlock(): wrote to addr %04x\r\n", nextWriteAddr ) );
            // LogInfo( ( "AzureIoTPlatform_WriteBlock(): read addr %04x\r\n", (pData + sofar) ) );
            // LogInfo( ( "AzureIoTPlatform_WriteBlock(): read addr %04x\r\n", nextReadAddr ) );
            // LogInfo( ( "AzureIoTPlatform_WriteBlock(): sofar %04x\r\n", sofar ) );
            // LogInfo( ( "AzureIoTPlatform_WriteBlock(): data %04x\r\n", data_to_send ) );
            // LogInfo( ( "we did it joe\r\n" ) );
        }
        // LogInfo( ( "Wrote to %04x\r\n", nextWriteAddr ) );
        nextWriteAddr += 256;
        nextReadAddr += 256;
    }

    // if last block and last packet of that block
    if (pxAduImage->ulImageFileSize - ulOffset <= ulBlockSize) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FAST_AND_LAST, nextWriteAddr, nextReadAddr) != HAL_OK)
        {
            /* Error occurred while writing data in Flash memory*/
            LogInfo( ( "Error Writing last last %i\r\n", HAL_FLASH_GetError() ) );
            HAL_FLASH_Lock();
            return HAL_FLASH_GetError();
        }
        // LogInfo( ( "AzureIoTPlatform_WriteBlock(): offset %i\r\n", ulOffset ) );
        // return eAzureIoTSuccess;
    }
    else {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FAST, nextWriteAddr, nextReadAddr) != HAL_OK)
        {
            /* Error occurred while writing data in Flash memory*/
            LogInfo( ( "Error Writing last of section %i\r\n", HAL_FLASH_GetError() ) );
            HAL_FLASH_Lock();
            return HAL_FLASH_GetError();
        }
    }
    

    // LogInfo( ( "AzureIoTPlatform_WriteBlock(): offset %04x\r\n", ulOffset ) );
    // LogInfo( ( "AzureIoTPlatform_WriteBlock(): write address %04x\r\n", pxAduImage->xUpdatePartition + ulOffset ) );
    // LogInfo( ( "AzureIoTPlatform_WriteBlock(): ulBlockSize %i\r\n", ulBlockSize ) );
    // 4096 bytes
    HAL_FLASH_Lock();
    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_VerifyImage( AzureADUImage_t * const pxAduImage,
                                               uint8_t * pucSHA256Hash,
                                               uint32_t ulSHA256HashLength )
{
    (void)pxAduImage;
    (void)pucSHA256Hash;
    (void)ulSHA256HashLength;

    // TODO: Fill in to verify bytes written to flash bank match the SHA256 given by pucSHA256Hash.

    LogInfo( ( "AzureIoTPlatform_VerifyImage()\r\n" ) );

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_EnableImage( AzureADUImage_t * const pxAduImage )
{
    (void)pxAduImage;

    // TODO: Fill in to program board to decide which memory bank to use on reboot.
    
    // 2) unlock option bytes
    // 3) program option byte with toggled BFB2 state
    // 4) set OBL_LAUNCH bit and wait for it to clear
    // 5) reset
   
    FLASH_OBProgramInitTypeDef  optionBytes;
    // uint8_t setOrReset;

    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    LogInfo( ( "option bytes unlocked\r\n" ) );
    HAL_FLASHEx_OBGetConfig(&optionBytes); // Get current optionbytes configuration
    LogInfo( ( "get current option bytes\r\n" ) );
    optionBytes.OptionType = OPTIONBYTE_USER;
    optionBytes.USERType = OB_USER_BFB2;
    // If BFB2 set, we will reset it
    if ((optionBytes.USERConfig & OB_BFB2_ENABLE) == OB_BFB2_ENABLE)
    {
        LogInfo( ( "bfb2 already enabled\r\n" ) );
        // setOrReset = 0;
        optionBytes.USERConfig = OB_BFB2_DISABLE;
        LogInfo( ( "disable bfb2\r\n" ) );
    }
    else
    {
        LogInfo( ( "bfb2 not enabled\r\n" ) );
        // setOrReset = 1;
        optionBytes.USERConfig = OB_BFB2_ENABLE;
        LogInfo( ( "enable bfb2\r\n" ) );
    }
   
    
    // if (setOrReset)// Set the bfb2 option bit as needed
    // {
    //     optionBytes.USERConfig = OB_BFB2_ENABLE;
    //     LogInfo( ( "enable bfb2\r\n" ) );
    // }
    // else
    // {
    //     optionBytes.USERConfig = OB_BFB2_DISABLE;
    //     LogInfo( ( "disable bfb2\r\n" ) );
    // }

    HAL_FLASHEx_OBProgram(&optionBytes);
    // if ( optionBytes.USERConfig & OB_BFB2_ENABLE )
    // {
    //     LogInfo( ( "bfb2 now enabled\r\n" ) );
    //     setOrReset = 0;
    // }
    // else
    // {
    //     LogInfo( ( "bfb2 now disabled\r\n" ) );
    //     setOrReset = 1;
    // }
    LogInfo( ( "programmed option bytes\r\n" ) );
    
    // HAL_FLASH_OB_Launch();
    // while (waitForReset != HAL_OK) {
    //     LogInfo( ( "waiting for reset\r\n" ) );
    // }
    
    LogInfo( ( "ob launch return value %02x\r\n", HAL_FLASH_OB_Launch() ) );
    LogInfo( ( "set obl launch bit\r\n" ) );
    // HAL_FLASH_OB_Lock();
    // LogInfo( ( "ob lock\r\n" ) );
    // HAL_FLASH_Lock();
    // HAL_NVIC_SystemReset();
    // __NVIC_SystemReset();

    LogInfo( ( "AzureIoTPlatform_EnableImage()\r\n" ) );

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_ResetDevice( AzureADUImage_t * const pxAduImage )
{
    LogInfo( ( "AzureIoTPlatform_ResetDevice()\r\n" ) );

    HAL_NVIC_SystemReset();
    return eAzureIoTSuccess;
}
