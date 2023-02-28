/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#ifndef CORE_MQTT_CONFIG_H
#define CORE_MQTT_CONFIG_H

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
    #define LIBRARY_LOG_NAME    "MQTT"
#endif

#define SINGLE_PARENTHESIS_LOGE( x, ... )    ESP_LOGE( LIBRARY_LOG_NAME, x, ## __VA_ARGS__ )
#define LogError( message )                  SINGLE_PARENTHESIS_LOGE message

#define SINGLE_PARENTHESIS_LOGI( x, ... )    ESP_LOGI( LIBRARY_LOG_NAME, x, ## __VA_ARGS__ )
#define LogInfo( message )                   SINGLE_PARENTHESIS_LOGI message

#define SINGLE_PARENTHESIS_LOGW( x, ... )    ESP_LOGW( LIBRARY_LOG_NAME, x, ## __VA_ARGS__ )
#define LogWarn( message )                   SINGLE_PARENTHESIS_LOGW message

#define SINGLE_PARENTHESIS_LOGD( x, ... )    ESP_LOGD( LIBRARY_LOG_NAME, x, ## __VA_ARGS__ )
#define LogDebug( message )                  SINGLE_PARENTHESIS_LOGD message

/************ End of logging configuration ****************/

/**
 * @brief The maximum number of MQTT PUBLISH messages that may be pending
 * acknowledgement at any time.
 *
 * QoS 1 and 2 MQTT PUBLISHes require acknowledgment from the server before
 * they can be completed. While they are awaiting the acknowledgment, the
 * client must maintain information about their state. The value of this
 * macro sets the limit on how many simultaneous PUBLISH states an MQTT
 * context maintains.
 */
#define MQTT_STATE_ARRAY_MAX_COUNT    10U

#endif /* ifndef CORE_MQTT_CONFIG_H */
