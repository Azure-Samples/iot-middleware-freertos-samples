/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <string.h>

#include "azure_iot_flash_platform.h"

#include "azure_iot_flash_platform_port.h"

#include "esp_ota_ops.h"
#include "esp_system.h"
#include "mbedtls/md.h"

#define azureiotflashSHA_256_SIZE 32

static uint8_t ucPartitionReadBuffer[32];
static uint8_t ucCalculatedHash[azureiotflashSHA_256_SIZE];

static AzureIoTResult_t prvBase64Decode(char* base64Encoded, uint8_t * pucOutputBuffer, size_t bufferLen, size_t * outputSize)
{
  az_result xCoreResult;
  az_span encodedSpan = az_span_create_from_str(base64Encoded);

  az_span outputSpan = az_span_create(pucOutputBuffer, bufferLen);

  if( az_result_failed( xCoreResult = az_base64_decode( outputSpan, encodedSpan, (int32_t *)outputSize ) ) )
  {
      AZLogError( ( "az_base64_decode failed: core error=0x%08x", xCoreResult ) );
      return eAzureIoTErrorFailed;
  }

  LogInfo(("Unencoded the base64 encoding\r\n"));

  return eAzureIoTSuccess;
}

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
  int xResult;
  esp_err_t espErr;

  uint32_t ulOutputSize;
  uint32_t ulReadSize;

  prvBase64Decode(pucFileHash, pucSHA256Hash, azureiotflashSHA_256_SIZE, (size_t *)&ulOutputSize);

  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);

  LogInfo(("Starting the mbedtls calculation\r\n"));
  for ( size_t ulOffset = 0; ulOffset < ulFlashedFileSize; ulOffset += sizeof(ucPartitionReadBuffer) )
  {
      ulReadSize = ulFlashedFileSize - ulOffset < sizeof(ucPartitionReadBuffer) ? ulFlashedFileSize - ulOffset : sizeof(ucPartitionReadBuffer);

      espErr = esp_partition_read_raw(pxAduClient->xImage.xUpdatePartition,
        ulOffset,
        ucPartitionReadBuffer,
        ulReadSize);
      (void)espErr;

      mbedtls_md_update(&ctx, (const unsigned char *) ucPartitionReadBuffer, ulReadSize);

      printf(".");
      if(i % 32 == 0 && i != 0)
      {
        printf("\r\n");
      }
  }
  
  LogInfo(("Done\r\n"));

  mbedtls_md_finish(&ctx, ucCalculatedHash);
  mbedtls_md_free(&ctx);

  if (memcmp(pucSHA256Hash, ucCalculatedHash, sizeof(pucSHA256Hash)) == 0)
  {
    LogInfo( ( "SHA's match\r\n" ) );
    xResult = eAzureIoTSuccess;
  }
  else
  {
    LogInfo(("SHA's do not match\r\n"));
    LogInfo(("Wanted: "));
    for(int i = 0; i < 32; i++)
    {
      printf("%x", pucSHA256Hash[i]);
    }

    LogInfo(("\r\n"));
    LogInfo(("Calculated: "));

    for(int i = 0; i < 32; i++)
    {
      printf("%x", ucCalculatedHash[i]);
    }

    xResult = eAzureIoTErrorFailed;
  }

  return xResult;
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
