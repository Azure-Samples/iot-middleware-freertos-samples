#include "azure_iot_http.h"

#include "core_http_client.h"



uint32_t AzureIoTHTTP_Init( AzureIoTHTTPHandle_t xHTTPHandle,
                            AzureIoTTransportInterface_t * pxHTTPTransport,
                            const char * pucURL,
                            uint32_t ulURLLength )
{
    ( void ) xHTTPHandle;
    ( void ) pucURL;

    if( xHTTPHandle == NULL )
    {
        return 1;
    }

    xHTTPHandle->xRequestInfo.pHost = pucURL;
    xHTTPHandle->xRequestInfo.hostLen = ulURLLength;
    xHTTPHandle->pxHTTPTransport = pxHTTPTransport;

    return 0;
}

uint32_t AzureIoTHTTP_Request( AzureIoTHTTPHandle_t xHTTPHandle )
{
    ( void ) xHTTPHandle;

    return 0;
}

uint32_t ulAzureIoTHTTP_Deinit( AzureIoTHTTPHandle_t xHTTPHandle )
{
    ( void ) xHTTPHandle;

    return 0;
}
