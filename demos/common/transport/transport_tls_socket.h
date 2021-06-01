/*
 * FreeRTOS V202011.00
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
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include "azure_iot_transport_interface.h"

#include "sockets_wrapper.h"

typedef struct NetworkContext NetworkContext_t;

/* SSL Context Handle */
typedef void * SSLContextHandle;

typedef struct TlsTransportParams
{
    SocketHandle xTCPSocket;
    SSLContextHandle xSSLContext;
} TlsTransportParams_t;

/**
 * @brief Contains the credentials necessary for tls connection setup.
 */
typedef struct NetworkCredentials
{
    /**
     * @brief To use ALPN, set this to a NULL-terminated list of supported
     * protocols in decreasing order of preference.
     */
    const char ** ppcAlpnProtos;

    /**
     * @brief Disable server name indication (SNI) for a TLS session.
     */
    BaseType_t xDisableSni;

    const uint8_t * pucRootCa;      /**< @brief String representing a trusted server root certificate. */
    size_t xRootCaSize;             /**< @brief Size associated with #NetworkCredentials.pRootCa. */
    const uint8_t * pucClientCert;  /**< @brief String representing the client certificate. */
    size_t xClientCertSize;         /**< @brief Size associated with #NetworkCredentials.pClientCert. */
    const uint8_t * pucPrivateKey;  /**< @brief String representing the client certificate's private key. */
    size_t xPrivateKeySize;         /**< @brief Size associated with #NetworkCredentials.pPrivateKey. */
} NetworkCredentials_t;

/**
 * @brief TLS Connect / Disconnect return status.
 */
typedef enum TlsTransportStatus
{
    TLS_TRANSPORT_SUCCESS = 0,         /**< Function successfully completed. */
    TLS_TRANSPORT_INVALID_PARAMETER,   /**< At least one parameter was invalid. */
    TLS_TRANSPORT_INSUFFICIENT_MEMORY, /**< Insufficient memory required to establish connection. */
    TLS_TRANSPORT_INVALID_CREDENTIALS, /**< Provided credentials were invalid. */
    TLS_TRANSPORT_HANDSHAKE_FAILED,    /**< Performing TLS handshake with server failed. */
    TLS_TRANSPORT_INTERNAL_ERROR,      /**< A call to a system API resulted in an internal error. */
    TLS_TRANSPORT_CONNECT_FAILURE      /**< Initial connection to the server failed. */
} TlsTransportStatus_t;

/**
 * @brief Connect to tls endpoint
 * 
 * @param[in] pxNetworkContext Pointer to the Network context.
 * @param[in] pcHostName Pointer to NULL terminated hostname.
 * @param[in] usPort Port to connect to.
 * @param[in] pxNetworkCredentials Pointer to network credentials. 
 * @param[in] ulReceiveTimeoutMs Receive timeout.
 * @param[in] ulSendTimeoutMs Send timeout.
 * @return A #TlsTransportStatus_t with the result of the operation.
 */
TlsTransportStatus_t TLS_Socket_Connect( NetworkContext_t * pxNetworkContext,
                                         const char * pcHostName, uint16_t usPort,
                                         const NetworkCredentials_t * pxNetworkCredentials,
                                         uint32_t ulReceiveTimeoutMs, uint32_t ulSendTimeoutMs );

/**
 * @brief Disconnect the tls connection
 * 
 * @param[in] pxNetworkContext Pointer to the Network context.
 */
void TLS_Socket_Disconnect( NetworkContext_t * pxNetworkContext );

/**
 * @brief Receive data from tls.
 * 
 * @param pxNetworkContext Pointer to the Network context.
 * @param pvBuffer Buffer used for receiving data. 
 * @param xBytesToRecv Sizze of the buffer.
 * @return An #int32_t number of bytes copied.
 */
int32_t TLS_Socket_Recv( NetworkContext_t * pxNetworkContext,
                         void * pvBuffer,
                         size_t xBytesToRecv );

/**
 * @brief Send data using tls. 
 * 
 * @param pxNetworkContext Pointer to the Network context.
 * @param pvBuffer Buffer that contains data to be sent. 
 * @param xBytesToSend Length of the data to be sent.
 * @return An #int32_t number of bytes successfully sent. 
 */
int32_t TLS_Socket_Send( NetworkContext_t * pxNetworkContext,
                         const void * pvBuffer,
                         size_t xBytesToSend );
