/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "transport_socket.h"

#include "sockets_wrapper.h"

typedef struct TlsTransportParams
{
    SocketHandle xTCPSocket;
    SSLContextHandle xSSLContext;
} TlsTransportParams_t;

struct NetworkContext
{
    TlsTransportParams_t * pParams;
};

int32_t Azure_Socket_Connect( NetworkContext_t * pxNetworkContext,
                                        const char * pHostName,
                                         uint16_t usPort,
                                         uint32_t ulReceiveTimeoutMs,
                                         uint32_t ulSendTimeoutMs )
{
    BaseType_t xSocketStatus;

    if( ( pxHTTPTransport->pxNetworkContext->pParams->xTCPSocket = Sockets_Open() ) == SOCKETS_INVALID_SOCKET )
    {
        LogError( ( "Failed to open socket." ) );
        ulStatus = eTLSTransportConnectFailure;
    }
    else if( ( xSocketStatus = Sockets_SetSockOpt( pxHTTPTransport->pxNetworkContext->pParams->xTCPSocket,
                                                   SOCKETS_SO_RCVTIMEO,
                                                   &xRecvTimeout,
                                                   sizeof( xRecvTimeout ) ) != 0 ) )
    {
        LogError( ( "Failed to set receive timeout on socket %d.", xSocketStatus ) );
        ulStatus = eTLSTransportInternalError;
    }
    else if( ( xSocketStatus = Sockets_SetSockOpt( pxHTTPTransport->pxNetworkContext->pParams->xTCPSocket,
                                                   SOCKETS_SO_SNDTIMEO,
                                                   &xSendTimeout,
                                                   sizeof( xSendTimeout ) ) != 0 ) )
    {
        LogError( ( "Failed to set send timeout on socket %d.", xSocketStatus ) );
        ulStatus = eTLSTransportInternalError;
    }
    else if( ( xSocketStatus = Sockets_Connect( pxHTTPTransport->pxNetworkContext->pParams->xTCPSocket,
                                                pucURL,
                                                80 ) ) != 0 )
    {
        LogError( ( "Failed to connect to %s with error %d.",
                    pucURL,
                    xSocketStatus ) );
        ulStatus = eTLSTransportConnectFailure;
    }

    return xSocketStatus;
}

int32_t Azure_Socket_Send( NetworkContext_t * pxNetworkContext,
                         const void * pvBuffer,
                         size_t xBytesToSend )
{
    return Sockets_Send( pxNetworkContext->pParams->xTCPSocket, pvBuffer, xBytesToSend );
}

int32_t Azure_Socket_Recv( NetworkContext_t * pxNetworkContext,
                         void * pvBuffer,
                         size_t xBytesToRecv )
{
    return Sockets_Recv( pxNetworkContext->pParams->xTCPSocket,
                         pvBuffer,
                         xBytesToRecv );
}
