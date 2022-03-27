/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file azure_iot_http.h
 *
 * @brief The port file for http APIs.
 *
 * Used in ADU.
 *
 */
#ifndef AZURE_IOT_HTTP_H
#define AZURE_IOT_HTTP_H

#include <stdint.h>

#include "azure_iot_http_port.h"
#include "azure_iot_transport_interface.h"

#define azureiothttpHEADER_BUFFER_SIZE            256
#define azureiothttpCHUNK_DOWNLOAD_BUFFER_SIZE    512

typedef AzureIoTHTTP_t * AzureIoTHTTPHandle_t;

typedef enum AzureIoTHTTPResult
{
    eAzureIoTHTTPSuccess = 0,      /** Function completed successfully. */
    eAzureIoTHTTPBadParameter,     /** At least one parameter was invalid. */
    eAzureIoTHTTPNoMemory,         /** A provided buffer was too small. */
    eAzureIoTHTTPSendFailed,       /** The transport send function failed. */
    eAzureIoTHTTPRecvFailed,       /** The transport receive function failed. */
    eAzureIoTHTTPBadResponse,      /** An invalid packet was received from the server. */
    eAzureIoTHTTPServerRefused,    /** The server refused a CONNECT or SUBSCRIBE. */
    eAzureIoTHTTPNoDataAvailable,  /** No data available from the transport interface. */
    eAzureIoTHTTPIllegalState,     /** An illegal state in the state record. */
    eAzureIoTHTTPStateCollision,   /** A collision with an existing state record entry. */
    eAzureIoTHTTPKeepAliveTimeout, /** Timeout while waiting for PINGRESP. */
    eAzureIoTHTTPFailed            /** Function failed with Unknown Error. */
} AzureIoTHTTPResult_t;

AzureIoTHTTPResult_t AzureIoTHTTP_Init( AzureIoTHTTPHandle_t xHTTPHandle,
                            AzureIoTTransportInterface_t * pxHTTPTransport,
                            const char * pucURL,
                            uint32_t ulURLLength,
                            const char * pucPath,
                            uint32_t ulPathLength );

AzureIoTHTTPResult_t AzureIoTHTTP_Request( AzureIoTHTTPHandle_t xHTTPHandle );

AzureIoTHTTPResult_t ulAzureIoTHTTP_Deinit( AzureIoTHTTPHandle_t xHTTPHandle );

#endif /* AZURE_IOT_HTTP_H */
