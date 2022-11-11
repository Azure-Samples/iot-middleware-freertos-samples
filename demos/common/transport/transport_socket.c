/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "transport_socket.h"

/* Include header that defines log levels. */
#include "logging_levels.h"

/* Logging configuration for the Sockets. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME     "SocketTransport"
#endif
#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_ERROR
#endif

/* Prototype for the function used to print to console on Windows simulator
 * of FreeRTOS.
 * The function prints to the console before the network is connected;
 * then a UDP port after the network has connected. */
extern void vLoggingPrintf( const char * pcFormatString,
                            ... );

/* Map the SdkLog macro to the logging function to enable logging
 * on Windows simulator. */
#ifndef SdkLog
    #define SdkLog( message )    vLoggingPrintf message
#endif

#include "logging_stack.h"

/************ End of logging configuration ****************/

#include "sockets_wrapper.h"

/* Each transport defines the same NetworkContext. The user then passes their respective transport */
/* as pParams for the transport which is defined in the transport header file */
/* (here it's SocketTransportParams_t) */
struct NetworkContext
{
    /* SocketTransportParams_t */
    void * pParams;
};

SocketTransportStatus_t Azure_Socket_Connect( NetworkContext_t * pxNetworkContext,
                                              const char * pHostName,
                                              uint16_t usPort,
                                              uint32_t ulReceiveTimeoutMs,
                                              uint32_t ulSendTimeoutMs )
{
    SocketTransportStatus_t xSocketStatus;
    SocketTransportParams_t * pxSocketParams = ( SocketTransportParams_t * ) pxNetworkContext->pParams;

    TickType_t xRecvTimeout = pdMS_TO_TICKS( ulReceiveTimeoutMs );
    TickType_t xSendTimeout = pdMS_TO_TICKS( ulSendTimeoutMs );

    if( ( pxSocketParams->xTCPSocket = Sockets_Open() ) == SOCKETS_INVALID_SOCKET )
    {
        LogError( ( "Failed to open socket." ) );
        xSocketStatus = eSocketTransportConnectFailure;
    }
    else if( ( xSocketStatus = Sockets_SetSockOpt( pxSocketParams->xTCPSocket,
                                                   SOCKETS_SO_RCVTIMEO,
                                                   &xRecvTimeout,
                                                   sizeof( xRecvTimeout ) ) != 0 ) )
    {
        LogError( ( "Failed to set receive timeout on socket %d.", xSocketStatus ) );
        xSocketStatus = eSocketTransportInternalError;
    }
    else if( ( xSocketStatus = Sockets_SetSockOpt( pxSocketParams->xTCPSocket,
                                                   SOCKETS_SO_SNDTIMEO,
                                                   &xSendTimeout,
                                                   sizeof( xSendTimeout ) ) != 0 ) )
    {
        LogError( ( "Failed to set send timeout on socket %d.", xSocketStatus ) );
        xSocketStatus = eSocketTransportInternalError;
    }
    else if( ( xSocketStatus = Sockets_Connect( pxSocketParams->xTCPSocket,
                                                pHostName,
                                                usPort ) ) != 0 )
    {
        LogError( ( "Failed to connect to %s with error %d.",
                    pHostName,
                    xSocketStatus ) );
        xSocketStatus = eSocketTransportConnectFailure;
    }
    else
    {
        xSocketStatus = eSocketTransportSuccess;
    }

    return xSocketStatus;
}

void Azure_Socket_Close( NetworkContext_t * pNetworkContext )
{
    ( void ) pNetworkContext;
}

int32_t Azure_Socket_Send( NetworkContext_t * pxNetworkContext,
                           const void * pvBuffer,
                           size_t xBytesToSend )
{
    SocketTransportParams_t * pxSocketParams = ( SocketTransportParams_t * ) pxNetworkContext->pParams;

    return Sockets_Send( pxSocketParams->xTCPSocket, pvBuffer, xBytesToSend );
}

int32_t Azure_Socket_Recv( NetworkContext_t * pxNetworkContext,
                           void * pvBuffer,
                           size_t xBytesToRecv )
{
    SocketTransportParams_t * pxSocketParams = ( SocketTransportParams_t * ) pxNetworkContext->pParams;

    return Sockets_Recv( pxSocketParams->xTCPSocket,
                         pvBuffer,
                         xBytesToRecv );
}
