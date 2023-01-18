/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

#ifndef DEMO_CONFIG_H
#define DEMO_CONFIG_H

#include <stdlib.h>

/*
 * This Model ID is tightly tied to the code implementation in `sample_azure_iot_pnp_simulated_device.c`
 * If you intend to test a different Model ID, please provide the implementation of the model on your application.
 */
#define sampleazureiotMODEL_ID                                "dtmi:com:example:Thermostat;1"


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
 * @note https://docs.microsoft.com/azure/iot-dps/concepts-service#service-operations-endpoint
 *
 */
#define democonfigENDPOINT "global.azure-devices-provisioning.net"

/**
 * @brief Id scope of provisioning service.
 *
 * @note https://docs.microsoft.com/azure/iot-dps/concepts-service#id-scope
 *
 */
#define democonfigID_SCOPE CONFIG_AZURE_DPS_ID_SCOPE

/**
 * @brief Registration Id of provisioning service
 *
 * @warning If using X509 authentication, this MUST match the Common Name of the cert.
 *
 *  @note https://docs.microsoft.com/azure/iot-dps/concepts-service#registration-id
 */
#define democonfigREGISTRATION_ID CONFIG_AZURE_DPS_REGISTRATION_ID

/**
 * @brief CA recovery Id scope of provisioning service.
 *
 * @note https://docs.microsoft.com/azure/iot-dps/concepts-service#id-scope
 *
 */
#define democonfigRECOVERY_ID_SCOPE CONFIG_AZURE_DPS_RECOVERY_ID_SCOPE

/**
 * @brief CA recovery Registration Id of provisioning service
 *
 * @warning If using X509 authentication, this MUST match the Common Name of the cert.
 *
 *  @note https://docs.microsoft.com/azure/iot-dps/concepts-service#registration-id
 */
#define democonfigRECOVERY_REGISTRATION_ID CONFIG_AZURE_DPS_RECOVERY_REGISTRATION_ID


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
#ifdef CONFIG_AZURE_IOT_DEVICE_SYMMETRIC_KEY
#define democonfigDEVICE_SYMMETRIC_KEY CONFIG_AZURE_IOT_DEVICE_SYMMETRIC_KEY
#endif

/**
 * @brief CA Recovery Device symmetric key
 *
 */
#ifdef CONFIG_AZURE_IOT_DEVICE_RECOVERY_SYMMETRIC_KEY
#define democonfigDEVICE_RECOVERY_SYMMETRIC_KEY CONFIG_AZURE_IOT_DEVICE_RECOVERY_SYMMETRIC_KEY
#endif

/**
 * @brief CA Recovery signing key N value (modulus)
 * 
 */
static uint8_t ucAzureIoTRecoveryRootKeyN[] =
{
    
};

#define democonfigRECOVERY_SIGNING_KEY_N ucAzureIoTRecoveryRootKeyN


/**
 * @brief CA Recovery signing key N value (modulus)
 * 
 */
static uint8_t ucAzureIoTRecoveryRootKeyE[] = { 0x01, 0x00, 0x01 };

#define democonfigRECOVERY_SIGNING_KEY_E ucAzureIoTRecoveryRootKeyE


/**
 * @brief Client's X509 Certificate.
 *
 */
#ifdef CONFIG_AZURE_IOT_DEVICE_CLIENT_CERTIFICATE
#define democonfigCLIENT_CERTIFICATE_PEM CONFIG_AZURE_IOT_DEVICE_CLIENT_CERTIFICATE
#endif

/**
 * @brief Client's private key.
 *
 */
#ifdef CONFIG_AZURE_IOT_DEVICE_CLIENT_CERTIFICATE_PRIVATE_KEY
#define democonfigCLIENT_PRIVATE_KEY_PEM    CONFIG_AZURE_IOT_DEVICE_CLIENT_CERTIFICATE_PRIVATE_KEY
#endif

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

/**
 * @brief Defines the macro for HSM usage depending on whether 
 * the support for ATECC608 is enabled in the kconfig menu
 */
#ifdef CONFIG_ESP_TLS_USE_SECURE_ELEMENT
    #if CONFIG_MBEDTLS_ATCA_HW_ECDSA_SIGN & CONFIG_MBEDTLS_ATCA_HW_ECDSA_VERIFY
        #define democonfigUSE_HSM

        /**
         * @brief Dynamically generate and write the registration ID as a
         *  string into the passed pointer
         *
         * @param[in,out] ppcRegistrationId Input: Pointer to a null pointer, 
         *                      Output: Pointer to a null-terminated string
         * 
         * @return  1  if the input is not a pointer to a NULL pointer,
         *          2  if we are not able to talk to the HSM
         *          3  if something else went wrong (eg: memory allocation failed)
         *          0   if everything went through correctly 
         */
    
        uint32_t getRegistrationId( char **ppcRegistrationId ); 
    #endif
#endif /* CONFIG_ESP_TLS_USE_SECURE_ELEMENT */

#endif /* DEMO_CONFIG_H */
