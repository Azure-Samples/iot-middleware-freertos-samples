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

typedef void * Socket_t;

#define SOCKETS_ERROR_NONE               ( 0 )     /*!< No error. */
#define SOCKETS_SOCKET_ERROR             ( -1 )    /*!< Catch-all sockets error code. */
#define SOCKETS_EWOULDBLOCK              ( -11 )   /*!< A resource is temporarily unavailable. */
#define SOCKETS_ENOMEM                   ( -12 )   /*!< Memory allocation failed. */
#define SOCKETS_EINVAL                   ( -22 )   /*!< Invalid argument. */
#define SOCKETS_ENOPROTOOPT              ( -109 )  /*!< A bad option was specified . */
#define SOCKETS_ENOTCONN                 ( -126 )  /*!< The supplied socket is not connected. */
#define SOCKETS_EISCONN                  ( -127 )  /*!< The supplied socket is already connected. */
#define SOCKETS_ECLOSED                  ( -128 )  /*!< The supplied socket has already been closed. */
#define SOCKETS_PERIPHERAL_RESET         ( -1006 ) /*!< Communications peripheral has been reset. */
/**@} */

#define SOCKETS_INVALID_SOCKET    ( ( Socket_t ) ~0U )

#define SOCKETS_SO_RCVTIMEO              ( 0 )  /**< Set the receive timeout. */
#define SOCKETS_SO_SNDTIMEO              ( 1 )  /**< Set the send timeout. */

Socket_t Sockets_Open();

BaseType_t Sockets_Close( Socket_t tcpSocket );

BaseType_t Sockets_Connect( Socket_t tcpSocket,
                            const char * pHostName,
                            uint16_t port );

void Sockets_Disconnect( Socket_t tcpSocket );

BaseType_t Sockets_Recv( Socket_t tcpSocket,
                         unsigned char * pucReceiveBuffer,
                         size_t xReceiveBufferLength );

BaseType_t Sockets_Send( Socket_t tcpSocket,
                         const unsigned char * pucData,
                         size_t xDataLength );

int32_t SOCKETS_SetSockOpt( Socket_t tcpSocket,
                            int32_t lOptionName,
                            const void * pvOptionValue,
                            size_t xOptionLength );

BaseType_t SOCKETS_Init();

#endif /* SOCKETS_WRAPPER_H */