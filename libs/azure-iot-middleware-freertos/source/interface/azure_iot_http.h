/**
 * @file azure_iot_http.h
 *
 * @brief The port file for http APIs.
 *
 * Used in ADU.
 *
 */

#include <stdint.h>

/* Maps MQTTContext directly to AzureIoTHTTP */
/* Defined in the .c for the port */
typedef struct HTTPContext   AzureIoTHTTP_t;

typedef AzureIoTHTTP_t       * AzureIoTHTTPHandle_t;

uint32_t ulAzureIoTHTTP_Init( AzureIoTHTTPHandle_t xHTTPHandle,
                              const char * pucURL );

uint32_t ulAzureIoTHTTP_Request( AzureIoTHTTPHandle_t xHTTPHandle );

uint32_t ulAzureIoTHTTP_Deinit( AzureIoTHTTPHandle_t xHTTPHandle );
