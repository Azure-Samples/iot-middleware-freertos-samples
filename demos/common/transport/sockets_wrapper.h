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

/**
 * @file sockets_wrapper.h
 * @brief Wrap platform specific socket.
 */

#ifndef SOCKETS_WRAPPER_H
#define SOCKETS_WRAPPER_H

#include "FreeRTOS.h"

typedef void * SocketHandle;

#ifndef SOCKETS_MAX_HOST_NAME_LENGTH
    #define SOCKETS_MAX_HOST_NAME_LENGTH    ( 64 )
#endif

/**
 * @brief Error codes
 * 
 */
#define SOCKETS_ERROR_NONE                  ( 0 )     /*!< No error. */
#define SOCKETS_SOCKET_ERROR                ( -1 )    /*!< Catch-all sockets error code. */
#define SOCKETS_EWOULDBLOCK                 ( -11 )   /*!< A resource is temporarily unavailable. */
#define SOCKETS_ENOMEM                      ( -12 )   /*!< Memory allocation failed. */
#define SOCKETS_EINVAL                      ( -22 )   /*!< Invalid argument. */
#define SOCKETS_ENOPROTOOPT                 ( -109 )  /*!< A bad option was specified. */
#define SOCKETS_ENOTCONN                    ( -126 )  /*!< The supplied socket is not connected. */
#define SOCKETS_EISCONN                     ( -127 )  /*!< The supplied socket is already connected. */
#define SOCKETS_ECLOSED                     ( -128 )  /*!< The supplied socket has already been closed. */
#define SOCKETS_PERIPHERAL_RESET            ( -1006 ) /*!< Communications peripheral has been reset. */
/**@} */

#define SOCKETS_INVALID_SOCKET              ( ( SocketHandle ) ~0U )

/**
 * @brief Option Names supported via Sockets_SetSockOpt API.
 * 
 */
#define SOCKETS_SO_RCVTIMEO                 ( 0 )  /**< Set the receive timeout. */
#define SOCKETS_SO_SNDTIMEO                 ( 1 )  /**< Set the send timeout. */

/**
 * @brief Initialize the sockets
 * 
 * @return A #BaseType_t with the result of the operation.
 *        - On success returns SOCKETS_ERROR_NONE
 */
BaseType_t Sockets_Init();

/**
 * @brief DeInitialize the sockets.
 * 
 * @return A #BaseType_t with the result of the operation.
 *        - On success returns SOCKETS_ERROR_NONE
 */
BaseType_t Sockets_DeInit();

/**
 * @brief Open a socket and return socket handle.
 * 
 * @return A #SocketHandle SocketHandle
 *         - On failure it returns SOCKETS_INVALID_SOCKET
 */
SocketHandle Sockets_Open();

/**
 * @brief Closes socket handle.
 * 
 * @param[in] xSocket The #SocketHandle used for this call.
 * @return A #BaseType_t with the result of the operation.
 *        - On success returns SOCKETS_ERROR_NONE
 */
BaseType_t Sockets_Close( SocketHandle xSocket );

/**
 * @brief Connect the socket to hostname and port.
 * 
 * @param[in] xSocket The #SocketHandle used for this call.
 * @param[in] pcHostName `NULL` terminated hostname
 * @param[in] usPort Connecting port.
 * @return A #BaseType_t with the result of the operation.
 *        - On success returns SOCKETS_ERROR_NONE
 */
BaseType_t Sockets_Connect( SocketHandle xSocket,
                            const char * pcHostName,
                            uint16_t usPort );

/**
 * @brief Disconnect socket handle.
 * 
 * @param[in] xSocket The #SocketHandle used for this call.
 */
void Sockets_Disconnect( SocketHandle xSocket );

/**
 * @brief Receive data from socket handle.
 * 
 * @param[in] xSocket The #SocketHandle used for this call.
 * @param[in,out] pucReceiveBuffer Buffer used for receiving data.
 * @param[in] xReceiveBufferLength Length of the buffer.
 * @return A #BaseType_t with the result of the operation.
 *        - On success returns number of bytes copied.
 *        - On failure return neagtive error code.
 */
BaseType_t Sockets_Recv( SocketHandle xSocket,
                         uint8_t * pucReceiveBuffer,
                         size_t xReceiveBufferLength );

/**
 * @brief Send data to socket handle.
 * 
 * @param[in] xSocket The #SocketHandle used for this call.
 * @param[in] pucData Buffer that contains data to be sent.
 * @param[in] xDataLength Length of the data to be sent.
 * @return A #BaseType_t with the result of the operation.
 *        - On success returns number of bytes sent.
 */
BaseType_t Sockets_Send( SocketHandle xSocket,
                         const uint8_t * pucData,
                         size_t xDataLength );

/**
 * @brief Set option for socket handle.
 * 
 * @param[in] xSocket The #SocketHandle used for this call.
 * @param[in] lOptionName Option name.
 * @param[in] pvOptionValue Pointer to option value.
 * @param[in] xOptionLength Lenght of option value.
* @return A #BaseType_t with the result of the operation.
 *        - On success returns SOCKETS_ERROR_NONE
 */
BaseType_t Sockets_SetSockOpt( SocketHandle xSocket,
                               int32_t lOptionName,
                               const void * pvOptionValue,
                               size_t xOptionLength );

#endif /* SOCKETS_WRAPPER_H */
