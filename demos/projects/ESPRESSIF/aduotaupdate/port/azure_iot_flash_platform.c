/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_iot_flash_platform.h"

#include "azure_iot_flash_platform_port.h"

#include "esp_ota_ops.h"

AzureIoTResult_t AzureIoTPlatform_Init( AzureADUImage_t * const pxAduImage )
{
  pxAduImage->pucBufferToWrite = NULL;
  pxAduImage->ulBytesToWriteLength = 0;
  pxAduImage->ulCurrentOffset = 0;
  pxAduImage->ulImageFileSize = 0;
  pxAduImage->xUpdatePartition = esp_ota_get_next_update_partition( esp_ota_get_running_partition() );

  return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_WriteBlock( AzureADUImage_t * const pxAduImage,
                                              uint32_t ulOffset,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize )
{
  (void)pxAduImage;
  (void)ulOffset;
  (void)pData;
  (void)ulBlockSize;

  return eAzureIoTSuccess;
}


AzureIoTResult_t AzureIoTPlatform_EnableImage(AzureADUImage_t * const pxAduImage)
{
  esp_err_t err = esp_ota_set_boot_partition(pxAduImage->xUpdatePartition);
  (void)err;
  return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_ResetDevice(AzureADUImage_t * const pxAduImage)
{
  return eAzureIoTSuccess;
}
