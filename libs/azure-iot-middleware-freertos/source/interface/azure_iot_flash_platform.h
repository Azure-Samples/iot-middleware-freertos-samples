/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file azure_iot_flash_platform.h
 *
 * @brief Defines the flash platform interface for devices enabling ADU.
 */
#ifndef AZURE_IOT_FLASH_PLATFORM_H
#define AZURE_IOT_FLASH_PLATFORM_H

#include <stdint.h>

#include "azure_iot_result.h"

typedef struct AzureADUImage
{
    void * unused;
} AzureADUImage_t;

AzureIoTResult_t AzureIoTPlatform_WriteBlock( AzureADUImage_t * const pxFileContext,
                                              uint32_t ulOffset,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize );

AzureIoTResult_t AzureIoTPlatform_EnableImage();

AzureIoTResult_t AzureIoTPlatform_ResetDevice();

#endif /* AZURE_IOT_FLASH_PLATFORM_H */
