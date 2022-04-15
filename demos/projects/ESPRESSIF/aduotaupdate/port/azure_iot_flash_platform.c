/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_iot_flash_platform.h"

#include "azure_iot_flash_platform_port.h"

#include "esp_ota_ops.h"
#include "esp_system.h"

AzureIoTResult_t AzureIoTPlatform_Init( AzureADUImage_t * const pxAduImage )
{
  const esp_partition_t * pxCurrentPartition = esp_ota_get_running_partition();

  if ( pxCurrentPartition == NULL )
  {
    printf( ( "esp_ota_get_running_partition failed" ) );
    return eAzureIoTErrorFailed;
  }

  pxAduImage->pucBufferToWrite = NULL;
  pxAduImage->ulBytesToWriteLength = 0;
  pxAduImage->ulCurrentOffset = 0;
  pxAduImage->ulImageFileSize = 0;
  pxAduImage->xUpdatePartition = esp_ota_get_next_update_partition( pxCurrentPartition );

  if ( pxAduImage->xUpdatePartition == NULL )
  {
    printf( ( "esp_ota_get_next_update_partition failed" ) );
    return eAzureIoTErrorFailed;
  }

  printf("[Platform] Size: %d | Address: %d\r\n",
      pxAduImage->xUpdatePartition->size,
      pxAduImage->xUpdatePartition->address);

  esp_partition_erase_range( pxAduImage->xUpdatePartition, 0, pxAduImage->xUpdatePartition->size );

  return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_WriteBlock( AzureADUImage_t * const pxAduImage,
                                              uint32_t ulOffset,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize )
{
  int ret;

  ret = esp_partition_write(pxAduImage->xUpdatePartition, ulOffset, pData, ulBlockSize);
  if (ret != ESP_OK) {
      return ret;
  }

  return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_VerifyImage( AzureADUImage_t * const pxAduImage,
                                                uint8_t * pucSHA256Hash)
{
  int ret;

  uint8_t imageSHA256[32];

  ret = esp_partition_get_sha256(pxAduImage->xUpdatePartition, imageSHA256);

  if (memcmp(pucSHA256Hash, imageSHA256, 32) == 0)
  {
    return eAzureIoTSuccess;
  }
  else
  {
    return eAzureIoTErrorFailed;
  }
}

AzureIoTResult_t AzureIoTPlatform_EnableImage(AzureADUImage_t * const pxAduImage)
{
  esp_err_t err = esp_ota_set_boot_partition(pxAduImage->xUpdatePartition);
  (void)err;
  return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_ResetDevice(AzureADUImage_t * const pxAduImage)
{
  esp_restart();
  return eAzureIoTSuccess;
}
