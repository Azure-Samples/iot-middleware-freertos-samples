/**
 * @file azure_iot_http.h
 *
 * @brief The port file for http APIs.
 *
 * Used in ADU.
 *
 */

#include <stdint.h>

#include "azure_iot_http_port.h"
#include "azure_iot_transport_interface.h"

typedef AzureIoTHTTP_t       * AzureIoTHTTPHandle_t;

uint32_t AzureIoTHTTP_Init( AzureIoTHTTPHandle_t xHTTPHandle,
                              const char * pucURL,
                              uint32_t ulURLLength );

uint32_t AzureIoTHTTP_Request( AzureIoTHTTPHandle_t xHTTPHandle );

uint32_t ulAzureIoTHTTP_Deinit( AzureIoTHTTPHandle_t xHTTPHandle );
