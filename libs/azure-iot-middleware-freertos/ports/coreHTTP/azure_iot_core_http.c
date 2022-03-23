#include "azure_iot_http.h"

#include <string.h>

#include "azure_iot.h"

#include "core_http_client.h"
/* Kernel includes. */
#include "FreeRTOS.h"

static uint8_t pucHeaderBuffer[200];
static uint8_t pucResponseBuffer[512];


uint32_t AzureIoTHTTP_Init( AzureIoTHTTPHandle_t xHTTPHandle,
                            AzureIoTTransportInterface_t * pxHTTPTransport,
                            const char * pucURL,
                            uint32_t ulURLLength,
                            const char * pucPath,
                            uint32_t ulPathLength )
{
    ( void ) xHTTPHandle;
    ( void ) pucURL;

    if( xHTTPHandle == NULL )
    {
        return 1;
    }

    ( void ) memset( &xHTTPHandle->xRequestInfo, 0, sizeof( xHTTPHandle->xRequestInfo ) );
    ( void ) memset( &xHTTPHandle->xRequestHeaders, 0, sizeof( xHTTPHandle->xRequestHeaders ) );

    xHTTPHandle->xRequestHeaders.pBuffer = pucHeaderBuffer;
    xHTTPHandle->xRequestHeaders.bufferLen = sizeof(pucHeaderBuffer);

    xHTTPHandle->xRequestInfo.pHost = pucURL;
    xHTTPHandle->xRequestInfo.hostLen = ulURLLength;
    xHTTPHandle->xRequestInfo.pPath = pucPath;
    xHTTPHandle->xRequestInfo.pathLen = ulPathLength;
    xHTTPHandle->xRequestInfo.pMethod = "GET";
    xHTTPHandle->xRequestInfo.methodLen = sizeof("GET") - 1;

    xHTTPHandle->pxHTTPTransport = pxHTTPTransport;

    printf( ( "[HTTP] Initialize Request Headers.\r\n" ) );
    HTTPClient_InitializeRequestHeaders( &xHTTPHandle->xRequestHeaders, &xHTTPHandle->xRequestInfo );

    return 0;
}

uint32_t AzureIoTHTTP_Request( AzureIoTHTTPHandle_t xHTTPHandle )
{
    ( void ) xHTTPHandle;

    HTTPStatus_t httpLibraryStatus = HTTPSuccess;

    xHTTPHandle->xResponse.pBuffer = pucResponseBuffer;
    xHTTPHandle->xResponse.bufferLen = sizeof(pucResponseBuffer);

    httpLibraryStatus = HTTPClient_Send( ( TransportInterface_t * ) xHTTPHandle->pxHTTPTransport, &xHTTPHandle->xRequestHeaders, NULL, 0, &xHTTPHandle->xResponse, 0 );

    if( httpLibraryStatus == HTTPSuccess )
    {
        if( xHTTPHandle->xResponse.statusCode == 200 )
        {
            /* Handle a response Status-Code of 200 OK. */
            printf( ( "[HTTP] Success 200.\r\n" ) );
            printf( "[HTTP] Payload: %.*s\r\n", (int)xHTTPHandle->xResponse.bodyLen, (char*)xHTTPHandle->xResponse.pBody);
        }
        else
        {
            /* Handle an error */
            printf( "[HTTP] Failed %d.\r\n", xHTTPHandle->xResponse.statusCode );
        }
    }

    return 0;
}

uint32_t ulAzureIoTHTTP_Deinit( AzureIoTHTTPHandle_t xHTTPHandle )
{
    ( void ) xHTTPHandle;

    return 0;
}
