/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <string.h>

#include "azure_iot_flash_platform.h"

#include "azure_iot_flash_platform_port.h"

#include "esp_ota_ops.h"
#include "esp_system.h"
#include "mbedtls/md.h"

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

/*
 * Note for this API:
 *    - The SHA256 that this return is the one which is appended at the end of the image by the ESPIDF
 *    - The hash is then verified over the partition memory address [0 : IMAGE-SIZE - 32] since the last 32 bytes
 *      are the SHA256 hash. This means that the sah256 which is create by the ADU service will be different, as it
 *      will be over the memory address [0 : IMAGE-SIZE] AKA including the appended SHA256 hash
 *    - Appended hash can be viewed by running `esptool.py --chip esp32 image_info .\azure_iot_freertos_esp32.bin`
 *  https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/spi_flash.html#_CPPv424esp_partition_get_sha256PK15esp_partition_tP7uint8_t */
AzureIoTResult_t AzureIoTPlatform_VerifyImage( AzureADUImage_t * const pxAduImage,
                                                uint8_t * pucSHA256Hash)
{
  int ret;

  uint8_t imageSHA256[32];

  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  
  const size_t payloadLength = strlen("test");
  
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char *) imageSHA256, sizeof(imageSHA256));
  mbedtls_md_finish(&ctx, imageSHA256);
  mbedtls_md_free(&ctx);

  /*
  Might have to use 
  
  esp_err_t esp_partition_read_raw(const esp_partition_t* partition,
        size_t src_offset, void* dst, size_t size)
  */

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
