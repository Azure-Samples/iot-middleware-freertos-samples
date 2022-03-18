
#include "azure_iot_http.h"

#include "core_http_client.h"


uint32_t ulAzureIoTHTTP_Init( AzureIoTHTTPHandle_t xHTTPHandle,
                              const char * pucURL )
{
  (void)xHTTPHandle;
  (void)pucURL;

  return 0;
}

uint32_t ulAzureIoTHTTP_Request( AzureIoTHTTPHandle_t xHTTPHandle )
{
  (void)xHTTPHandle;

  return 0;
}

uint32_t ulAzureIoTHTTP_Deinit( AzureIoTHTTPHandle_t xHTTPHandle )
{
  (void)xHTTPHandle;

  return 0;
}
