
#include "azure_iot_flash_platform.h"

AzureIoTResult_t AzureIoTPlatform_Init( AzureADUImage_t * const pxAduImage )
{
  (void)pxAduImage;

  return eAzureIoTSuccess;
}

int64_t AzureIoTPlatform_GetSingleFlashBootBankSize()
{    
  return INT64_MAX;
}

AzureIoTResult_t AzureIoTPlatform_WriteBlock( AzureADUImage_t * const pxFileContext,
                                              uint32_t ulOffset,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize )
{
  (void)pxFileContext;
  (void)ulOffset;
  (void)pData;
  (void)ulBlockSize;

  return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_VerifyImage( AzureADUImage_t * const pxAduImage,
                                               uint8_t * pucSHA256Hash,
                                               uint32_t ulSHA256HashLength )
{
    (void)pxAduImage;
    (void)pucSHA256Hash;
    (void)ulSHA256HashLength;

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_EnableImage( AzureADUImage_t * const pxAduImage )
{
  (void)pxAduImage;

  return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_ResetDevice( AzureADUImage_t * const pxAduImage )
{
  (void)pxAduImage;

  return eAzureIoTSuccess;
}
