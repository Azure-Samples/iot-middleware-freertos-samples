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
 * @file tls_freertos.c
 * @brief TLS transport interface implementations. This implementation uses
 * mbedTLS.
 */

/* Standard includes. */
#include <string.h>

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
    esp_transport_handle_t transport;
    uint32_t receiveTimeoutMs;
    uint32_t sendTimeoutMs;
};

static const char *TAG = "tls_freertos";
/*-----------------------------------------------------------*/

TlsTransportStatus_t TLS_Socket_Connect( NetworkContext_t * pNetworkContext,
                                           const char * pHostName,
                                           uint16_t port,
                                           const NetworkCredentials_t * pNetworkCredentials,
                                           uint32_t receiveTimeoutMs,
                                           uint32_t sendTimeoutMs )
{
    TlsTransportStatus_t returnStatus = eTLSTransportSuccess;

    if( ( pNetworkContext == NULL ) ||
        ( pHostName == NULL ) ||
        ( pNetworkCredentials == NULL ) )
    {
        ESP_LOGE(TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                    "pHostName=%p, pNetworkCredentials=%p.",
                    pNetworkContext,
                    pHostName,
                    pNetworkCredentials);
        return eTLSTransportInvalidParameter;
    }
    
    pNetworkContext->transport = esp_transport_ssl_init();
    pNetworkContext->receiveTimeoutMs = receiveTimeoutMs;
    pNetworkContext->sendTimeoutMs = sendTimeoutMs;
    if (pNetworkCredentials->ppcAlpnProtos) {
        esp_transport_ssl_set_alpn_protocol(pNetworkContext->transport, pNetworkCredentials->ppcAlpnProtos);
    }

    if (pNetworkCredentials->xDisableSni) {
        esp_transport_ssl_skip_common_name_check(pNetworkContext->transport);
    }

    if (pNetworkCredentials->pucRootCa) {
        esp_transport_ssl_set_cert_data_der(pNetworkContext->transport, (const char *)pNetworkCredentials->pucRootCa, pNetworkCredentials->xRootCaSize);
    }

    if (pNetworkCredentials->pucClientCert) {
        esp_transport_ssl_set_client_cert_data_der(pNetworkContext->transport, (const char *)pNetworkCredentials->pucClientCert, pNetworkCredentials->xClientCertSize);
    }

    if (pNetworkCredentials->pucPrivateKey) {
        esp_transport_ssl_set_client_key_data_der(pNetworkContext->transport, (const char *)pNetworkCredentials->pucPrivateKey, pNetworkCredentials->xPrivateKeySize);
    }

    if (esp_transport_connect(pNetworkContext->transport, pHostName, port, receiveTimeoutMs) < 0) {
        returnStatus = eTLSTransportConnectFailure;
    } else {
        returnStatus = eTLSTransportSuccess;
    }

    /* Clean up on failure. */
    if( returnStatus != eTLSTransportSuccess )
    {
        if( pNetworkContext != NULL )
        {
            esp_transport_close( pNetworkContext->transport );
            esp_transport_destroy( pNetworkContext->transport );
        }
    }
    else
    {
        ESP_LOGI(TAG, "(Network connection %p) Connection to %s established.",
                   pNetworkContext,
                   pHostName);
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

void TLS_Socket_Disconnect( NetworkContext_t * pNetworkContext )
{
    if (( pNetworkContext == NULL ) ) {
        ESP_LOGE(TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p.", pNetworkContext);
        return;
    }

    /* Attempting to terminate TLS connection. */
    esp_transport_close( pNetworkContext->transport );

    /* Free TLS contexts. */
    esp_transport_destroy( pNetworkContext->transport );
}
/*-----------------------------------------------------------*/

int32_t TLS_Socket_Recv( NetworkContext_t * pNetworkContext,
                           void * pBuffer,
                           size_t bytesToRecv )
{
    int32_t tlsStatus = 0;

    if (( pNetworkContext == NULL ) || 
        ( pBuffer == NULL) || 
        ( bytesToRecv == 0) ) {
        ESP_LOGE(TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                "pBuffer=%p, bytesToRecv=%d.", pNetworkContext, pBuffer, bytesToRecv );
        return eTLSTransportInvalidParameter;
    }

    tlsStatus = esp_transport_read(pNetworkContext->transport, pBuffer, bytesToRecv, pNetworkContext->receiveTimeoutMs);
    if (tlsStatus < 0) {
        ESP_LOGE(TAG, "Reading failed, errno= %d", errno);
        return ESP_FAIL;
    }

    return tlsStatus;
}
/*-----------------------------------------------------------*/

int32_t TLS_Socket_Send( NetworkContext_t * pNetworkContext,
                           const void * pBuffer,
                           size_t bytesToSend )
{
    int32_t tlsStatus = 0;

    if (( pNetworkContext == NULL ) || 
        ( pBuffer == NULL) || 
        ( bytesToSend == 0) ) {
        ESP_LOGE(TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                "pBuffer=%p, bytesToSend=%d.", pNetworkContext, pBuffer, bytesToSend );
        return eTLSTransportInvalidParameter;
    }

    tlsStatus = esp_transport_write(pNetworkContext->transport, pBuffer, bytesToSend, pNetworkContext->sendTimeoutMs);
    if (tlsStatus < 0) {
        ESP_LOGE(TAG, "Writing failed, errno= %d", errno);
        return ESP_FAIL;
    }

    return tlsStatus;
}
/*-----------------------------------------------------------*/

