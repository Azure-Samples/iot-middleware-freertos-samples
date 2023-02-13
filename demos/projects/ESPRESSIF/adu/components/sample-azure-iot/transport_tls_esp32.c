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
 * @file transport_tls_esp32.c
 * @brief TLS transport interface implementations. This implementation uses
 * mbedTLS.
 */

/* Standard includes. */
#include "errno.h"

/* FreeRTOS includes. */
#include "freertos/FreeRTOS.h"

/* TLS transport header. */
#include "transport_tls_socket.h"

#include "esp_log.h"

/* TLS includes. */
#include "esp_transport_ssl.h"


// We will malloc this and put it in TlsTransportParams_t.xSSLContext
typedef struct EspTlsTransportParams
{
    esp_transport_handle_t xTransport;
    esp_transport_list_handle_t xTransportList;
    uint32_t ulReceiveTimeoutMs;
    uint32_t ulSendTimeoutMs;
} EspTlsTransportParams_t;

/* Each transport defines the same NetworkContext. The user then passes their respective transport */
/* as pParams for the transport which is defined in the transport header file */
/* (here it's TlsTransportParams_t) */
struct NetworkContext
{
   // TlsTransportParams_t
    void * pParams;
};

static const char *TAG = "esp_tls";
/*-----------------------------------------------------------*/

TlsTransportStatus_t TLS_Socket_Connect( NetworkContext_t * pNetworkContext,
                                         const char * pHostName,
                                         uint16_t usPort,
                                         const NetworkCredentials_t * pNetworkCredentials,
                                         uint32_t ulReceiveTimeoutMs,
                                         uint32_t ulSendTimeoutMs )
{
    TlsTransportStatus_t xReturnStatus = eTLSTransportSuccess;

    if( ( pNetworkContext == NULL ) ||
        ( pHostName == NULL ) ||
        ( pNetworkCredentials == NULL ) )
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                  "pHostName=%p, pNetworkCredentials=%p.",
                  pNetworkContext,
                  pHostName,
                  pNetworkCredentials );
        return eTLSTransportInvalidParameter;
    }

    TlsTransportParams_t * pxTlsParams = (TlsTransportParams_t*)pNetworkContext->pParams;

    if (( pxTlsParams == NULL ))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL." );
        return eTLSTransportInvalidParameter;
    }

    EspTlsTransportParams_t * pxEspTlsTransport = (EspTlsTransportParams_t*) pvPortMalloc(sizeof(EspTlsTransportParams_t));

    if(pxEspTlsTransport == NULL)
    {
      return eTLSTransportInsufficientMemory;
    }

    // Create a transport list into which we put the transport.
    pxEspTlsTransport->xTransportList = esp_transport_list_init();
    pxEspTlsTransport->xTransport = esp_transport_ssl_init( );
    pxEspTlsTransport->ulReceiveTimeoutMs = ulReceiveTimeoutMs;
    pxEspTlsTransport->ulSendTimeoutMs = ulSendTimeoutMs;

    esp_transport_ssl_enable_global_ca_store(pxEspTlsTransport->xTransport);

    esp_transport_list_add(pxEspTlsTransport->xTransportList, pxEspTlsTransport->xTransport, "_ssl");

    pxTlsParams->xSSLContext = (void*)pxEspTlsTransport;

    if ( pNetworkCredentials->ppcAlpnProtos )
    {
        esp_transport_ssl_set_alpn_protocol( pxEspTlsTransport->xTransport, pNetworkCredentials->ppcAlpnProtos );
    }

    if ( pNetworkCredentials->xDisableSni )
    {
        esp_transport_ssl_skip_common_name_check( pxEspTlsTransport->xTransport );
    }

    if ( pNetworkCredentials->pucRootCa )
    {
        esp_tls_set_global_ca_store( ( const unsigned char * ) pNetworkCredentials->pucRootCa, pNetworkCredentials->xRootCaSize );
    }

    if ( pNetworkCredentials->pucClientCert )
    {
        esp_transport_ssl_set_client_cert_data_der( pxEspTlsTransport->xTransport, ( const char *) pNetworkCredentials->pucClientCert, pNetworkCredentials->xClientCertSize );
    }

    if ( pNetworkCredentials->pucPrivateKey )
    {
        esp_transport_ssl_set_client_key_data_der( pxEspTlsTransport->xTransport, (const char *) pNetworkCredentials->pucPrivateKey, pNetworkCredentials->xPrivateKeySize );
    }

    if ( esp_transport_connect( pxEspTlsTransport->xTransport, pHostName, usPort, ulReceiveTimeoutMs ) < 0 )
    {
        ESP_LOGE( TAG, "Failed establishing TLS connection (esp_transport_connect failed)" );
        xReturnStatus = eTLSTransportConnectFailure;
    }
    else
    {
        xReturnStatus = eTLSTransportSuccess;
    }

    /* Clean up on failure. */
    if( xReturnStatus != eTLSTransportSuccess )
    {
        if( pxEspTlsTransport != NULL )
        {
            esp_transport_close( pxEspTlsTransport->xTransport );
            esp_transport_list_destroy(pxEspTlsTransport->xTransportList);
            vPortFree(pxEspTlsTransport);
            pxTlsParams->xSSLContext = NULL;
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

void TLS_Socket_Disconnect( NetworkContext_t * pNetworkContext )
{
    if (( pNetworkContext == NULL ))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p.", pNetworkContext );
        return;
    }

    TlsTransportParams_t * pxTlsParams = (TlsTransportParams_t*)pNetworkContext->pParams;

    if (( pxTlsParams == NULL ))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL." );
        return;
    }

    EspTlsTransportParams_t * pxEspTlsTransport = (EspTlsTransportParams_t *)pxTlsParams->xSSLContext;

    if((pxEspTlsTransport == NULL))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pxTlsParams->xSSLContext=%p.", pxEspTlsTransport );
        return;
    }

    /* Attempting to terminate TLS connection. */
    esp_transport_close( pxEspTlsTransport->xTransport );

    /* Destroy list of transports */
    esp_transport_list_destroy(pxEspTlsTransport->xTransportList);
    vPortFree(pxEspTlsTransport);
    pxTlsParams->xSSLContext = NULL;
}
/*-----------------------------------------------------------*/

int32_t TLS_Socket_Recv( NetworkContext_t * pNetworkContext,
                           void * pBuffer,
                           size_t xBytesToRecv )
{
    int32_t tlsStatus = 0;

    if (( pNetworkContext == NULL ) ||
        ( pBuffer == NULL) ||
        ( xBytesToRecv == 0) )
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                "pBuffer=%p, xBytesToRecv=%d.", pNetworkContext, pBuffer, xBytesToRecv );
        return eTLSTransportInvalidParameter;
    }

    TlsTransportParams_t * pxTlsParams = (TlsTransportParams_t*)pNetworkContext->pParams;

    if (( pxTlsParams == NULL ))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL." );
        return eTLSTransportInvalidParameter;
    }

    EspTlsTransportParams_t * pxEspTlsTransport = (EspTlsTransportParams_t *)pxTlsParams->xSSLContext;

    if((pxEspTlsTransport == NULL))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pxTlsParams->xSSLContext=%p.", pxEspTlsTransport );
        return eTLSTransportInvalidParameter;
    }

    tlsStatus = esp_transport_read( pxEspTlsTransport->xTransport, pBuffer, xBytesToRecv, pxEspTlsTransport->ulReceiveTimeoutMs );
    if ( tlsStatus < 0 )
    {
        ESP_LOGE( TAG, "Reading failed, errno= %d", errno );
        return ESP_FAIL;
    }

    return tlsStatus;
}
/*-----------------------------------------------------------*/

int32_t TLS_Socket_Send( NetworkContext_t * pNetworkContext,
                           const void * pBuffer,
                           size_t xBytesToSend )
{
    int32_t tlsStatus = 0;

    if (( pNetworkContext == NULL ) ||
        ( pBuffer == NULL) ||
        ( xBytesToSend == 0) )
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                "pBuffer=%p, xBytesToSend=%d.", pNetworkContext, pBuffer, xBytesToSend );
        return eTLSTransportInvalidParameter;
    }

    TlsTransportParams_t * pxTlsParams = (TlsTransportParams_t*)pNetworkContext->pParams;

    if (( pxTlsParams == NULL ))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL." );
        return eTLSTransportInvalidParameter;
    }

    EspTlsTransportParams_t * pxEspTlsTransport = (EspTlsTransportParams_t *)pxTlsParams->xSSLContext;

    if((pxEspTlsTransport == NULL))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pxTlsParams->xSSLContext=%p.", pxEspTlsTransport );
        return eTLSTransportInvalidParameter;
    }

    tlsStatus = esp_transport_write( pxEspTlsTransport->xTransport, pBuffer, xBytesToSend, pxEspTlsTransport->ulSendTimeoutMs );
    if ( tlsStatus < 0 )
    {
        ESP_LOGE( TAG, "Writing failed, errno= %d", errno );
        return ESP_FAIL;
    }

    return tlsStatus;
}
/*-----------------------------------------------------------*/
