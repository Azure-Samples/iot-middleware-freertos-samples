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
