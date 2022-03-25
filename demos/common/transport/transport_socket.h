/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @brief Socket transport for plaintext writing.
 *
 */

#ifndef TRANSPORT_SOCKET_H
#define TRANSPORT_SOCKET_H

#include "azure_iot_transport_interface.h"

#include "sockets_wrapper.h"

#include "transport_abstraction.h"

int32_t Foo_Socket_Send( NetworkContext_t * pxNetworkContext,
                         const void * pvBuffer,
                         size_t xBytesToSend );

int32_t Foo_Socket_Recv( NetworkContext_t * pxNetworkContext,
                         void * pvBuffer,
                         size_t xBytesToRecv );

#endif /* TRANSPORT_SOCKET_H */
