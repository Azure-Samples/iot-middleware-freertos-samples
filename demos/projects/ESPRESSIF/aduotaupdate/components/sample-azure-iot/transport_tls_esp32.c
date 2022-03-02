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

/* FreeRTOS includes. */
#include "freertos/FreeRTOS.h"

/* TLS transport header. */
#include "transport_tls_socket.h"

#include "esp_log.h"

/* TLS includes. */
#include "esp_transport_ssl.h"

/**
 * @brief Definition of the network context for the transport interface
 * implementation that uses mbedTLS and FreeRTOS+TLS sockets.
 */
struct NetworkContext
{
    esp_transport_handle_t xTransport;
    uint32_t ulReceiveTimeoutMs;
    uint32_t ulSendTimeoutMs;
};

static const char *TAG = "tls_freertos";
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

    pNetworkContext->xTransport = esp_transport_ssl_init( );
    pNetworkContext->ulReceiveTimeoutMs = ulReceiveTimeoutMs;
    pNetworkContext->ulSendTimeoutMs = ulSendTimeoutMs;
    if ( pNetworkCredentials->ppcAlpnProtos )
    {
        esp_transport_ssl_set_alpn_protocol( pNetworkContext->xTransport, pNetworkCredentials->ppcAlpnProtos );
    }

    if ( pNetworkCredentials->xDisableSni )
    {
        esp_transport_ssl_skip_common_name_check( pNetworkContext->xTransport );
    }

    if ( pNetworkCredentials->pucRootCa )
    {
        esp_transport_ssl_set_cert_data_der( pNetworkContext->xTransport, ( const char * ) pNetworkCredentials->pucRootCa, pNetworkCredentials->xRootCaSize );
    }

    if ( pNetworkCredentials->pucClientCert )
    {
        esp_transport_ssl_set_client_cert_data_der( pNetworkContext->xTransport, ( const char *) pNetworkCredentials->pucClientCert, pNetworkCredentials->xClientCertSize );
    }

    if ( pNetworkCredentials->pucPrivateKey )
    {
        esp_transport_ssl_set_client_key_data_der( pNetworkContext->xTransport, (const char *) pNetworkCredentials->pucPrivateKey, pNetworkCredentials->xPrivateKeySize );
    }

    if ( esp_transport_connect( pNetworkContext->xTransport, pHostName, usPort, ulReceiveTimeoutMs ) < 0 )
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
        if( pNetworkContext != NULL )
        {
            esp_transport_close( pNetworkContext->xTransport );
            esp_transport_destroy( pNetworkContext->xTransport );
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

    /* Attempting to terminate TLS connection. */
    esp_transport_close( pNetworkContext->xTransport );

    /* Free TLS contexts. */
    esp_transport_destroy( pNetworkContext->xTransport );
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

    tlsStatus = esp_transport_read( pNetworkContext->xTransport, pBuffer, xBytesToRecv, pNetworkContext->ulReceiveTimeoutMs );
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

    tlsStatus = esp_transport_write( pNetworkContext->xTransport, pBuffer, xBytesToSend, pNetworkContext->ulSendTimeoutMs );
    if ( tlsStatus < 0 )
    {
        ESP_LOGE( TAG, "Writing failed, errno= %d", errno );
        return ESP_FAIL;
    }

    return tlsStatus;
}
/*-----------------------------------------------------------*/
