/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#ifndef AZURE_IOT_CONFIG_H
#define AZURE_IOT_CONFIG_H

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/*
 * Include logging header files and define logging macros in the following order:
 * 1. Include the header file "esp_log.h".
 * 2. Define the LIBRARY_LOG_NAME and LIBRARY_LOG_LEVEL macros depending on
 * the logging configuration for DEMO.
 * 3. Define macros to replace module logging functions by esp logging functions.
 */

#include "esp_log.h"

#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME    "AZ IOT"
#endif

#define SINGLE_PARENTHESIS_LOGE( x, ... )    ESP_LOGE( LIBRARY_LOG_NAME, x, ## __VA_ARGS__ )
#define AZLogError( message )                SINGLE_PARENTHESIS_LOGE message

#define SINGLE_PARENTHESIS_LOGI( x, ... )    ESP_LOGI( LIBRARY_LOG_NAME, x, ## __VA_ARGS__ )
#define AZLogInfo( message )                 SINGLE_PARENTHESIS_LOGI message

#define SINGLE_PARENTHESIS_LOGW( x, ... )    ESP_LOGW( LIBRARY_LOG_NAME, x, ## __VA_ARGS__ )
#define AZLogWarn( message )                 SINGLE_PARENTHESIS_LOGW message

#define SINGLE_PARENTHESIS_LOGD( x, ... )    ESP_LOGD( LIBRARY_LOG_NAME, x, ## __VA_ARGS__ )
#define AZLogDebug( message )                SINGLE_PARENTHESIS_LOGD message

/************ End of logging configuration ****************/

#endif /* AZURE_IOT_CONFIG_H */
