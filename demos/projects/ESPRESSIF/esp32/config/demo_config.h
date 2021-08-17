/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

#ifndef DEMO_CONFIG_H
#define DEMO_CONFIG_H

#include <stdlib.h>

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
    #define LIBRARY_LOG_NAME    "AzureIoTDemo"
#endif

#define SINGLE_PARENTHESIS_LOGE( x, ... ) ESP_LOGE( LIBRARY_LOG_NAME, x, ##__VA_ARGS__ )
#define LogError( message )               SINGLE_PARENTHESIS_LOGE message

#define SINGLE_PARENTHESIS_LOGI( x, ... ) ESP_LOGI( LIBRARY_LOG_NAME, x, ##__VA_ARGS__ )
#define LogInfo( message )                SINGLE_PARENTHESIS_LOGI message

#define SINGLE_PARENTHESIS_LOGW( x, ... ) ESP_LOGW( LIBRARY_LOG_NAME, x, ##__VA_ARGS__ )
#define LogWarn( message )                SINGLE_PARENTHESIS_LOGW message

#define SINGLE_PARENTHESIS_LOGD( x, ... ) ESP_LOGD( LIBRARY_LOG_NAME, x, ##__VA_ARGS__ )
#define LogDebug( message )               SINGLE_PARENTHESIS_LOGD message

/************ End of logging configuration ****************/

/**
 * @brief Enable Device Provisioning
 *
 * @note To disable Device Provisioning undef this macro
 *
 */

#ifdef CONFIG_ENABLE_DPS_SAMPLE
    #define democonfigENABLE_DPS_SAMPLE
#endif

#ifdef democonfigENABLE_DPS_SAMPLE

/**
 * @brief Provisioning service endpoint.
 *
 * @note https://docs.microsoft.com/en-us/azure/iot-dps/concepts-service#service-operations-endpoint
 *
 */
#define democonfigENDPOINT "global.azure-devices-provisioning.net"

/**
 * @brief Id scope of provisioning service.
 *
 * @note https://docs.microsoft.com/en-us/azure/iot-dps/concepts-service#id-scope
 *
 */
#define democonfigID_SCOPE CONFIG_AZURE_DPS_ID_SCOPE

/**
 * @brief Registration Id of provisioning service
 *
 * @warning If using X509 authentication, this MUST match the Common Name of the cert.
 *
 *  @note https://docs.microsoft.com/en-us/azure/iot-dps/concepts-service#registration-id
 */
#define democonfigREGISTRATION_ID CONFIG_AZURE_DPS_REGISTRATION_ID


#endif // democonfigENABLE_DPS_SAMPLE

/**
 * @brief IoTHub device Id.
 *
 */
#define democonfigDEVICE_ID CONFIG_AZURE_IOT_DEVICE_ID

/**
 * @brief IoTHub module Id.
 *
 * @note This is optional argument for IoTHub
 */
#define democonfigMODULE_ID CONFIG_AZURE_IOT_MODULE_ID

/**
 * @brief IoTHub hostname.
 *
 */
#define democonfigHOSTNAME CONFIG_AZURE_IOT_HUB_FQDN

/**
 * @brief Device symmetric key
 *
 */
#define democonfigDEVICE_SYMMETRIC_KEY CONFIG_AZURE_IOT_DEVICE_SYMMETRIC_KEY

/**
 * @brief Client's X509 Certificate.
 *
 */
// #define democonfigCLIENT_CERTIFICATE_PEM    "<YOUR DEVICE CERT HERE>"

/**
 * @brief Client's private key.
 *
 */
// #define democonfigCLIENT_PRIVATE_KEY_PEM    "<YOUR DEVICE PRIVATE KEY HERE>"

/**
 * @brief Baltimore Trusted Root CA.
 *
 */
#define democonfigROOT_CA_PEM "-----BEGIN CERTIFICATE-----\r\n" \
"MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\r\n" \
"RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\r\n" \
"VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\r\n" \
"DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\r\n" \
"ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\r\n" \
"VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\r\n" \
"mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\r\n" \
"IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\r\n" \
"mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\r\n" \
"XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\r\n" \
"dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\r\n" \
"jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\r\n" \
"BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\r\n" \
"DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\r\n" \
"9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\r\n" \
"jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\r\n" \
"Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\r\n" \
"ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\r\n" \
"R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\r\n" \
"-----END CERTIFICATE-----\r\n"

/**
 * @brief Set the stack size of the main demo task.
 *
 */
#define democonfigDEMO_STACKSIZE CONFIG_AZURE_TASK_STACKSIZE

/**
 * @brief Size of the network buffer for MQTT packets.
 */
#define democonfigNETWORK_BUFFER_SIZE CONFIG_NETWORK_BUFFER_SIZE

/**
 * @brief IoTHub endpoint port.
 */
#define democonfigIOTHUB_PORT 8883

/**
 * @brief Defines configRAND32, used by the common sample modules.
 */
#define configRAND32() (rand()/RAND_MAX)

#endif /* DEMO_CONFIG_H */
