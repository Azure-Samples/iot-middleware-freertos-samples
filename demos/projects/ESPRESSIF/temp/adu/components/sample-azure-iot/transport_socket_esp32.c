/*
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file transport_socket_esp32.c
 * @brief Socket transport interface implementations.
 */

/* Standard includes. */
#include <errno.h>

/* FreeRTOS includes. */
#include "freertos/FreeRTOS.h"

/* Socket transport header. */
#include "transport_socket.h"

#include "esp_log.h"

/* Socket includes. */
#include "esp_transport.h"
#include "esp_transport_tcp.h"

// We will malloc this and put it in SocketTransportParams_t.xSocketContext
typedef struct EspSocketTransportParams
{
    esp_transport_handle_t xTransport;
    esp_transport_list_handle_t xTransportList;
    uint32_t ulReceiveTimeoutMs;
    uint32_t ulSendTimeoutMs;
} EspSocketTransportParams_t;

/* Each transport defines the same NetworkContext. The user then passes their respective transport */
/* as pParams for the transport which is defined in the transport header file */
/* (here it's SocketTransportParams_t) */
struct NetworkContext
{
   // SocketTransportParams_t
    void * pParams;
};

static const char *TAG = "esp_sockets";
/*-----------------------------------------------------------*/

SocketTransportStatus_t Azure_Socket_Connect( NetworkContext_t * pNetworkContext,
                                         const char * pHostName,
                                         uint16_t usPort,
                                         uint32_t ulReceiveTimeoutMs,
                                         uint32_t ulSendTimeoutMs )
{
    SocketTransportStatus_t xReturnStatus = eSocketTransportSuccess;

    if( ( pNetworkContext == NULL ) ||
        ( pHostName == NULL ) )
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                  "pHostName=%p.",
                  pNetworkContext,
                  pHostName );
        return eSocketTransportInvalidParameter;
    }

    SocketTransportParams_t * pxSocketTransport = (SocketTransportParams_t *)pNetworkContext->pParams;

    if((pxSocketTransport == NULL))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL." );
        return eSocketTransportInvalidParameter;
    }

    EspSocketTransportParams_t * pxEspSocketTransport = (EspSocketTransportParams_t*) pvPortMalloc(sizeof(EspSocketTransportParams_t));
    if(pxEspSocketTransport == NULL)
    {
      return eSocketTransportInsufficientMemory;
    }

    pxEspSocketTransport->xTransport = esp_transport_tcp_init( );
    pxEspSocketTransport->xTransportList = esp_transport_list_init();
    pxEspSocketTransport->ulReceiveTimeoutMs = ulReceiveTimeoutMs;
    pxEspSocketTransport->ulSendTimeoutMs = ulSendTimeoutMs;

    esp_transport_list_add(pxEspSocketTransport->xTransportList, pxEspSocketTransport->xTransport, "_tcp");

    pxSocketTransport->xSocketContext = (void*)pxEspSocketTransport;

    if ( esp_transport_connect( pxEspSocketTransport->xTransport, pHostName, usPort, ulReceiveTimeoutMs ) < 0 )
    {
        ESP_LOGE( TAG, "Failed establishing socket connection (esp_transport_connect failed)" );
        xReturnStatus = eSocketTransportConnectFailure;
    }
    else
    {
        xReturnStatus = eSocketTransportSuccess;
    }

    /* Clean up on failure. */
    if( xReturnStatus != eSocketTransportSuccess )
    {
        if( pNetworkContext != NULL )
        {
            esp_transport_close( pxEspSocketTransport->xTransport );
            esp_transport_list_destroy(pxEspSocketTransport->xTransportList);
            vPortFree(pxEspSocketTransport);
        }
    }
    else
    {
        ESP_LOGI( TAG, "(Network connection %p) Connection to %s established.",
                   pNetworkContext,
                   pHostName );
    }

    return xReturnStatus;
}
/*-----------------------------------------------------------*/

void Azure_Socket_Close( NetworkContext_t * pNetworkContext )
{
    if (( pNetworkContext == NULL ))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p.", pNetworkContext );
        return;
    }

    SocketTransportParams_t * pxSocketTransport = (SocketTransportParams_t *)pNetworkContext->pParams;

    if((pxSocketTransport == NULL))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL." );
        return;
    }

    EspSocketTransportParams_t * pxEspSocketTransport = (EspSocketTransportParams_t*)pxSocketTransport->xSocketContext;

    /* Attempting to terminate socket connection. */
    esp_transport_close( pxEspSocketTransport->xTransport );

    /* Destroy list of transports */
    esp_transport_list_destroy(pxEspSocketTransport->xTransportList);
    vPortFree( pxEspSocketTransport );
}
/*-----------------------------------------------------------*/

int32_t Azure_Socket_Recv( NetworkContext_t * pNetworkContext,
                           void * pBuffer,
                           size_t xBytesToRecv )
{
    int32_t lsocketStatus = 0;

    if (( pNetworkContext == NULL ) ||
        ( pBuffer == NULL) ||
        ( xBytesToRecv == 0) )
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                "pBuffer=%p, xBytesToRecv=%d.", pNetworkContext, pBuffer, xBytesToRecv );
        return eSocketTransportInvalidParameter;
    }

    SocketTransportParams_t * pxSocketTransport = (SocketTransportParams_t *)pNetworkContext->pParams;

    if((pxSocketTransport == NULL))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL." );
        return eSocketTransportInvalidParameter;
    }

    EspSocketTransportParams_t * pxEspSocketTransport = (EspSocketTransportParams_t*)pxSocketTransport->xSocketContext;

    if((pxEspSocketTransport == NULL))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pxSocketTransport->xSocketContext=%p.", pxEspSocketTransport );
        return eSocketTransportInvalidParameter;
    }

    lsocketStatus = esp_transport_read( pxEspSocketTransport->xTransport, pBuffer, xBytesToRecv, pxEspSocketTransport->ulReceiveTimeoutMs );
    if ( lsocketStatus < 0 )
    {
        ESP_LOGE( TAG, "Reading failed, errno= %d", errno );
        return ESP_FAIL;
    }

    return lsocketStatus;
}
/*-----------------------------------------------------------*/

int32_t Azure_Socket_Send( NetworkContext_t * pNetworkContext,
                           const void * pBuffer,
                           size_t xBytesToSend )
{
    int32_t lsocketStatus = 0;

    if (( pNetworkContext == NULL ) ||
        ( pBuffer == NULL) ||
        ( xBytesToSend == 0) )
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                "pBuffer=%p, xBytesToSend=%d.", pNetworkContext, pBuffer, xBytesToSend );
        return eSocketTransportInvalidParameter;
    }

    SocketTransportParams_t * pxSocketTransport = (SocketTransportParams_t *)pNetworkContext->pParams;

    if((pxSocketTransport == NULL))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL." );
        return eSocketTransportInvalidParameter;
    }

    EspSocketTransportParams_t * pxEspSocketTransport = (EspSocketTransportParams_t*)pxSocketTransport->xSocketContext;

    if((pxEspSocketTransport == NULL))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pxSocketTransport->xSocketContext=%p.", pxEspSocketTransport );
        return eSocketTransportInvalidParameter;
    }

    lsocketStatus = esp_transport_write( pxEspSocketTransport->xTransport, pBuffer, xBytesToSend, pxEspSocketTransport->ulSendTimeoutMs );
    if ( lsocketStatus < 0 )
    {
        ESP_LOGE( TAG, "Writing failed, errno= %d", errno );
        return ESP_FAIL;
    }

    return lsocketStatus;
}
/*-----------------------------------------------------------*/
