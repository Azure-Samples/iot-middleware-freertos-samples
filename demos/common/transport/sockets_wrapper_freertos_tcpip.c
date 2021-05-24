/*
 * FreeRTOS V202104.00
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

/**
 * @file sockets_wrapper.c
 * @brief FreeRTOS Sockets connect and disconnect wrapper implementation.
 */

#include "sockets_wrapper.h"

/* Standard includes. */
#include <string.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_DNS.h"
/*-----------------------------------------------------------*/

/* Maximum number of times to call FreeRTOS_recv when initiating a graceful shutdown. */
#ifndef FREERTOS_SOCKETS_WRAPPER_SHUTDOWN_LOOPS
    #define FREERTOS_SOCKETS_WRAPPER_SHUTDOWN_LOOPS    ( 3 )
#endif

/* A negative error code indicating a network failure. */
#define FREERTOS_SOCKETS_WRAPPER_NETWORK_ERROR    ( -1 )

/*-----------------------------------------------------------*/

SocketHandle Sockets_Open()
{
    Socket_t ulSocketNumber = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
    SocketHandle xSocket;

    if( ulSocketNumber == FREERTOS_INVALID_SOCKET )
    {
        xSocket = ( SocketHandle ) SOCKETS_INVALID_SOCKET;
    }
    else
    {
        xSocket = ( SocketHandle ) ulSocketNumber;
    }

    return xSocket;
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Close( SocketHandle xSocket )
{
    return ( BaseType_t ) FreeRTOS_closesocket( ( Socket_t ) xSocket );
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Connect( SocketHandle xSocket,
                            const char * pHostName,
                            uint16_t port )
{
    Socket_t tcpSocket = ( Socket_t ) xSocket;
    BaseType_t lRetVal = 0;
    struct freertos_sockaddr serverAddress = { 0 };
    uint32_t ulIPAddres;

    /* Check for errors from DNS lookup. */
    if( ( ulIPAddres = ( uint32_t ) FreeRTOS_gethostbyname( pHostName ) ) == 0 )
    {
        lRetVal = SOCKETS_SOCKET_ERROR;
    }
    else
    {
        /* Connection parameters. */
        serverAddress.sin_family = FREERTOS_AF_INET;
        serverAddress.sin_port = FreeRTOS_htons( port );
        serverAddress.sin_addr = ulIPAddres;
        serverAddress.sin_len = ( uint8_t ) sizeof( serverAddress );

        if( FreeRTOS_connect( tcpSocket, &serverAddress, sizeof( serverAddress ) ) != 0 )
        {
            lRetVal = SOCKETS_SOCKET_ERROR;
        }
    }

    return lRetVal;
}
/*-----------------------------------------------------------*/

void Sockets_Disconnect( SocketHandle xSocket )
{
    BaseType_t waitForShutdownLoopCount = 0;
    uint8_t pDummyBuffer[ 2 ];
    Socket_t tcpSocket = ( Socket_t ) xSocket;

    if( tcpSocket != FREERTOS_INVALID_SOCKET )
    {
        /* Initiate graceful shutdown. */
        ( void ) FreeRTOS_shutdown( tcpSocket, FREERTOS_SHUT_RDWR );

        /* Wait for the socket to disconnect gracefully (indicated by FreeRTOS_recv()
         * returning a FREERTOS_EINVAL error) before closing the socket. */
        while( FreeRTOS_recv( tcpSocket, pDummyBuffer, sizeof( pDummyBuffer ), 0 ) >= 0 )
        {
            /* We don't need to delay since FreeRTOS_recv should already have a timeout. */

            if( ++waitForShutdownLoopCount >= FREERTOS_SOCKETS_WRAPPER_SHUTDOWN_LOOPS )
            {
                break;
            }
        }

        ( void ) FreeRTOS_closesocket( tcpSocket );
    }
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Send( SocketHandle xSocket,
                         const unsigned char * pucData,
                         size_t xDataLength )
{
    return ( BaseType_t ) FreeRTOS_send( ( Socket_t ) xSocket,
                                         pucData, xDataLength, 0 ) ;
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Recv( SocketHandle xSocket,
                         unsigned char * pucReceiveBuffer,
                         size_t xReceiveBufferLength )
{
    return ( BaseType_t ) FreeRTOS_recv( ( Socket_t ) xSocket,
                                         pucReceiveBuffer, xReceiveBufferLength, 0 ) ;
}
/*-----------------------------------------------------------*/

BaseType_t SOCKETS_SetSockOpt( SocketHandle xSocket,
                               int32_t lOptionName,
                               const void * pvOptionValue,
                               size_t xOptionLength )
{
    Socket_t tcpSocket = ( Socket_t ) xSocket;
    BaseType_t xRetVal;
    int ulRet = 0;
    TickType_t xTimeout;
    
    switch( lOptionName )
    {
        case SOCKETS_SO_RCVTIMEO :
        case SOCKETS_SO_SNDTIMEO:
        {
            /* Comply with Berkeley standard - a 0 timeout is wait forever. */
            xTimeout = *( ( const TickType_t * ) pvOptionValue );

            if( xTimeout == 0U )
            {
                xTimeout = portMAX_DELAY;
            }

            ulRet = FreeRTOS_setsockopt( tcpSocket,
                                         0,
                                         lOptionName,
                                         &xTimeout,
                                         xOptionLength );
            
            if( ulRet != 0 )
            {
                xRetVal = SOCKETS_EINVAL;
            }
            else
            {
                xRetVal = SOCKETS_ERROR_NONE;
            }
            
            break;
        }
        break;

        default :
        {
            xRetVal = SOCKETS_ENOPROTOOPT;
        }
        break;
    }

    return xRetVal;
}
/*-----------------------------------------------------------*/
