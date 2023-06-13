/*
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file transport_tls_esp32.c
 * @brief TLS transport interface implementations. This implementation uses
 * mbedTLS.
 */

/* Standard includes. */
#include "errno.h"

/* FreeRTOS includes. */
#include "freertos/FreeRTOS.h"

/* TLS transport header. */
#include "transport_tls_socket.h"

#include "esp_log.h"

/* TLS includes. */
#include "esp_transport_ssl.h"

#include "demo_config.h"

/* For using the ATECC608 secure element if support is configured */
#ifdef democonfigUSE_HSM
    #include "cryptoauthlib.h"
#endif

static const char *TAG = "tls_freertos";

#ifdef democonfigUSE_HSM

#define tlsesp32SERIAL_NUMBER_SIZE 9
#define tlsesp32REGISTRATION_ID_SIZE 21

#if defined(CONFIG_ATECC608A_TNG)
/**
 * @brief [Trust&GO] Dynamically generate and write the registration ID as a
 *  string into the passed pointer
 *
 * @param[in,out] ppcRegistrationId Input: Pointer to a null pointer, 
 *                      Output: Pointer to a null-terminated string
 * @param[in]     pucHsmData Pointer to a buffer holding data to be passed
 *                          (if any) to help generate the Registration ID
 * @param[in]     ulHsmDataLength Length of the buffer passed in the 
 *                          second parameter       
 * 
 * @return  0   if everything went through correctly 
 */
static uint32_t getRegistrationIdFromTNG( char **ppcRegistrationId,\
                                            uint8_t *pucHsmData,\
                                            uint32_t ulHsmDataLength ) {

        /* We don't check for NULL-ness of the input or the 
            ability to talk to the HSM - the getRegistrationId(...)
            function does that already
        */      
        
        *ppcRegistrationId = malloc(tlsesp32REGISTRATION_ID_SIZE);     
        if(*ppcRegistrationId == NULL) {
            return 3;
        }
        sprintf(*ppcRegistrationId,"sn%02X%02X%02X%02X%02X%02X%02X%02X%02X",pucHsmData[0],pucHsmData[1],\
        pucHsmData[2],pucHsmData[3],pucHsmData[4],pucHsmData[5],pucHsmData[6],pucHsmData[7],pucHsmData[8]);
        *(*ppcRegistrationId + tlsesp32REGISTRATION_ID_SIZE - 1) = '\0';

        return 0;
}

#elif defined(CONFIG_ATECC608A_TFLEX)
/**
 * @brief [TrustFLEX] Dynamically generate and write the registration ID as a
 *  string into the passed pointer
 *
 * @param[in,out] ppcRegistrationId Input: Pointer to a null pointer, 
 *                      Output: Pointer to a null-terminated string
 * @param[in]     pucHsmData Pointer to a buffer holding data to be passed
 *                          (if any) to help generate the Registration ID
 * @param[in]     ulHsmDataLength Length of the buffer passed in the 
 *                          second parameter       
 * 
 * @return  0   if everything went through correctly 
 */
static uint32_t getRegistrationIdFromTFLX( char **ppcRegistrationId,\
                                            uint8_t *pucHsmData,\
                                            uint32_t ulHsmDataLength ) {

        /* We don't check for NULL-ness of the input or the 
            ability to talk to the HSM - the getRegistrationId(...)
            function does that already
        */      
        /* TODO: Replace the below with your own implementation - the provided 
            implementation is applicable to TFLX-PROTO devices only
        */
        *ppcRegistrationId = malloc(tlsesp32REGISTRATION_ID_SIZE);     
        if(*ppcRegistrationId == NULL) {
            return 3;
        }
        sprintf(*ppcRegistrationId,"sn%02X%02X%02X%02X%02X%02X%02X%02X%02X",pucHsmData[0],pucHsmData[1],\
        pucHsmData[2],pucHsmData[3],pucHsmData[4],pucHsmData[5],pucHsmData[6],pucHsmData[7],pucHsmData[8]);
        *(*ppcRegistrationId + tlsesp32REGISTRATION_ID_SIZE - 1) = '\0';

        return 0;
}

#elif defined(CONFIG_ATECC608A_TCUSTOM)
/**
 * @brief [TrustCUSTOM] Dynamically generate and write the registration ID as a
 *  string into the passed pointer
 *
 * @param[in,out] ppcRegistrationId Input: Pointer to a null pointer, 
 *                      Output: Pointer to a null-terminated string
 * @param[in]     pucHsmData Pointer to a buffer holding data to be passed
 *                          (if any) to help generate the Registration ID
 * @param[in]     ulHsmDataLength Length of the buffer passed in the 
 *                          second parameter       
 * 
 * @return  0   if everything went through correctly 
 */
static uint32_t getRegistrationIdFromTCSM( char **ppcRegistrationId,\
                                            uint8_t *pucHsmData,\
                                            uint32_t ulHsmDataLength ) {

        /* We don't check for NULL-ness of the input or the 
            ability to talk to the HSM - the getRegistrationId(...)
            function does that already
        */      
        /* TODO: Replace the below with your own implementation - the provided 
            implementation is applicable to certs generated using esp-cryptoauth 
            tool only
        */
        *ppcRegistrationId = malloc(tlsesp32REGISTRATION_ID_SIZE - 2);     
        if(*ppcRegistrationId == NULL) {
            return 3;
        }

        sprintf(*ppcRegistrationId,"%02X%02X%02X%02X%02X%02X%02X%02X%02X",pucHsmData[0],pucHsmData[1],\
        pucHsmData[2],pucHsmData[3],pucHsmData[4],pucHsmData[5],pucHsmData[6],pucHsmData[7],pucHsmData[8]);
        *(*ppcRegistrationId + tlsesp32REGISTRATION_ID_SIZE - 3) = '\0';  
        return 0;
}
#endif

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
uint32_t getRegistrationId( char **ppcRegistrationId ) {

        if(*ppcRegistrationId != NULL) {
            return 1;
        }
        uint32_t ret = 0;
        uint8_t sernum[tlsesp32SERIAL_NUMBER_SIZE];
        ATCA_STATUS s;
        s = atcab_read_serial_number(sernum);
        if(s != ATCA_SUCCESS) {
            ESP_LOGE( TAG, "Failed to read serial number from ATECC608" );
            return 2;
        }

        #if defined(CONFIG_ATECC608A_TNG)
            ret = getRegistrationIdFromTNG(ppcRegistrationId,sernum,tlsesp32SERIAL_NUMBER_SIZE);
            if(ret != 0) {
                ESP_LOGE(TAG, "[TNG] Registration ID Gen Error!");
                return ret;
            }

        #elif defined(CONFIG_ATECC608A_TFLEX)
            ret = getRegistrationIdFromTFLX(ppcRegistrationId,sernum,tlsesp32SERIAL_NUMBER_SIZE);
            if(ret != 0) {
                ESP_LOGE(TAG, "[TFLX] Registration ID Gen Error!");
                return ret;
            }
        
        #elif defined(CONFIG_ATECC608A_TCUSTOM)
            ret = getRegistrationIdFromTCSM(ppcRegistrationId,sernum,tlsesp32SERIAL_NUMBER_SIZE);
            if(ret != 0) {
                ESP_LOGE(TAG, "[TCSM] Registration ID Gen Error!");
                return ret;
            }
 
        #endif

        ESP_LOGI( TAG, "Registration ID is %s", *ppcRegistrationId );  
        return 0;
    }

#endif /* democonfigUSE_HSM */


/**
 * @brief Definition of the network context for the transport interface
 * implementation that uses mbedTLS and FreeRTOS+TLS sockets.
 */
typedef struct EspTlsTransportParams
{
    esp_transport_handle_t xTransport;
    esp_transport_list_handle_t xTransportList;
    uint32_t ulReceiveTimeoutMs;
    uint32_t ulSendTimeoutMs;
} EspTlsTransportParams_t;

/* Each transport defines the same NetworkContext. The user then passes their respective transport */
/* as pParams for the transport which is defined in the transport header file */
/* (here it's TlsTransportParams_t) */
struct NetworkContext
{
   // TlsTransportParams_t
    void * pParams;
};

/*-----------------------------------------------------------*/

TlsTransportStatus_t TLS_Socket_Connect( NetworkContext_t * pNetworkContext,
                                         const char * pHostName,
                                         uint16_t usPort,
                                         const NetworkCredentials_t * pNetworkCredentials,
                                         uint32_t ulReceiveTimeoutMs,
                                         uint32_t ulSendTimeoutMs )
{
    TlsTransportStatus_t xReturnStatus = eTLSTransportSuccess;

    if( ( pNetworkContext == NULL ) ||
        ( pHostName == NULL ) ||
        ( pNetworkCredentials == NULL ) )
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                  "pHostName=%p, pNetworkCredentials=%p.",
                  pNetworkContext,
                  pHostName,
                  pNetworkCredentials );
        return eTLSTransportInvalidParameter;
    }

    TlsTransportParams_t * pxTlsParams = (TlsTransportParams_t*)pNetworkContext->pParams;

    if (( pxTlsParams == NULL ))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL" );
        return eTLSTransportInvalidParameter;
    }

    EspTlsTransportParams_t * pxEspTlsTransport;

    if ( pxTlsParams->xSSLContext != NULL )
    {
        pxEspTlsTransport = pxTlsParams->xSSLContext;
    }
    else
    {
        pxEspTlsTransport = (EspTlsTransportParams_t*) pvPortMalloc(sizeof(EspTlsTransportParams_t));

        if(pxEspTlsTransport == NULL)
        {
            return eTLSTransportInsufficientMemory;
        }

        // Create a transport list into which we put the transport.
        pxEspTlsTransport->xTransportList = esp_transport_list_init();
        pxEspTlsTransport->xTransport = esp_transport_ssl_init( );
        pxEspTlsTransport->ulReceiveTimeoutMs = ulReceiveTimeoutMs;
        pxEspTlsTransport->ulSendTimeoutMs = ulSendTimeoutMs;

        esp_transport_ssl_enable_global_ca_store(pxEspTlsTransport->xTransport);

        esp_transport_list_add(pxEspTlsTransport->xTransportList, pxEspTlsTransport->xTransport, "_ssl");

        pxTlsParams->xSSLContext = (void*)pxEspTlsTransport;

        if ( pNetworkCredentials->ppcAlpnProtos )
        {
            esp_transport_ssl_set_alpn_protocol( pxEspTlsTransport->xTransport, pNetworkCredentials->ppcAlpnProtos );
        }

        if ( pNetworkCredentials->xDisableSni )
        {
            esp_transport_ssl_skip_common_name_check( pxEspTlsTransport->xTransport );
        }

        if ( pNetworkCredentials->pucRootCa )
        {
            ESP_LOGI( TAG, "Setting CA store");
            esp_tls_set_global_ca_store( ( const unsigned char * ) pNetworkCredentials->pucRootCa, pNetworkCredentials->xRootCaSize );
        }
#ifdef democonfigUSE_HSM

        esp_transport_ssl_use_secure_element( pxEspTlsTransport->xTransport );

        #if defined(CONFIG_ATECC608A_TCUSTOM) || defined(CONFIG_ATECC608A_TFLEX)
            /*  This is TrustCUSTOM or TrustFLEX chip - the private key will be used from the ATECC608 device slot 0.
                We will plug in your custom device certificate here (should be in DER format).
            */
            if ( pNetworkCredentials->pucClientCert )
            {
                esp_transport_ssl_set_client_cert_data_der( pxEspTlsTransport->xTransport, ( const char *) pNetworkCredentials->pucClientCert, pNetworkCredentials->xClientCertSize );
            }

        
        #else
            /*  This is the Trust&GO chip - the private key will be used from ATECC608 device slot 0.
                We don't need to add certs to the network context as the esp-tls does that for us using cryptoauthlib API.
            */

        #endif

#else

        if ( pNetworkCredentials->pucClientCert )
        {
            esp_transport_ssl_set_client_cert_data_der( pxEspTlsTransport->xTransport, ( const char *) pNetworkCredentials->pucClientCert, pNetworkCredentials->xClientCertSize );
        }

        if ( pNetworkCredentials->pucPrivateKey )
        {
            esp_transport_ssl_set_client_key_data_der( pxEspTlsTransport->xTransport, (const char *) pNetworkCredentials->pucPrivateKey, pNetworkCredentials->xPrivateKeySize );
        }

#endif
    }

    if ( esp_transport_connect( pxEspTlsTransport->xTransport, pHostName, usPort, ulReceiveTimeoutMs ) < 0 )
    {
        ESP_LOGE( TAG, "Failed establishing TLS connection (esp_transport_connect failed)" );
        xReturnStatus = eTLSTransportConnectFailure;
    }
    else
    {
        xReturnStatus = eTLSTransportSuccess;
    }

    /* Clean up on failure. */
    if( xReturnStatus != eTLSTransportSuccess )
    {
        if( pxEspTlsTransport != NULL )
        {
            esp_transport_close( pxEspTlsTransport->xTransport );
            esp_tls_free_global_ca_store( );
            esp_transport_list_destroy(pxEspTlsTransport->xTransportList);
            vPortFree(pxEspTlsTransport);
            pxTlsParams->xSSLContext = NULL;
        }
    }
    else
    {
        ESP_LOGI( TAG, "(Network connection %p) Connection to %s established.",
                   pNetworkContext,
                   pHostName );
    }

    return xReturnStatus;
}
/*-----------------------------------------------------------*/

void TLS_Socket_Disconnect( NetworkContext_t * pNetworkContext )
{
    if (( pNetworkContext == NULL ))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p.", pNetworkContext );
        return;
    }

    TlsTransportParams_t * pxTlsParams = (TlsTransportParams_t*)pNetworkContext->pParams;

    if (( pxTlsParams == NULL ))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL." );
        return;
    }

    EspTlsTransportParams_t * pxEspTlsTransport = (EspTlsTransportParams_t *)pxTlsParams->xSSLContext;

    /* Attempting to terminate TLS connection. */
    esp_transport_close( pxEspTlsTransport->xTransport );

    /* Clear CA store. */
    esp_tls_free_global_ca_store( );

    /* Free TLS contexts. */
    esp_transport_list_destroy(pxEspTlsTransport->xTransportList);
    vPortFree(pxEspTlsTransport);
    pxTlsParams->xSSLContext = NULL;
}
/*-----------------------------------------------------------*/

int32_t TLS_Socket_Recv( NetworkContext_t * pNetworkContext,
                           void * pBuffer,
                           size_t xBytesToRecv )
{
    int32_t tlsStatus = 0;

    if (( pNetworkContext == NULL ) ||
        ( pBuffer == NULL) ||
        ( xBytesToRecv == 0) )
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                "pBuffer=%p, xBytesToRecv=%d.", pNetworkContext, pBuffer, xBytesToRecv );
        return eTLSTransportInvalidParameter;
    }

    TlsTransportParams_t * pxTlsParams = (TlsTransportParams_t*)pNetworkContext->pParams;

    if (( pxTlsParams == NULL ))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL." );
        return eTLSTransportInvalidParameter;
    }

    EspTlsTransportParams_t * pxEspTlsTransport = (EspTlsTransportParams_t *)pxTlsParams->xSSLContext;

    tlsStatus = esp_transport_read( pxEspTlsTransport->xTransport, pBuffer, xBytesToRecv, pxEspTlsTransport->ulReceiveTimeoutMs );
    if ( tlsStatus < 0 )
    {
        ESP_LOGE( TAG, "Reading failed, errno= %d", errno );
        return ESP_FAIL;
    }

    return tlsStatus;
}
/*-----------------------------------------------------------*/

int32_t TLS_Socket_Send( NetworkContext_t * pNetworkContext,
                           const void * pBuffer,
                           size_t xBytesToSend )
{
    int32_t tlsStatus = 0;

    if (( pNetworkContext == NULL ) ||
        ( pBuffer == NULL) ||
        ( xBytesToSend == 0) )
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                "pBuffer=%p, xBytesToSend=%d.", pNetworkContext, pBuffer, xBytesToSend );
        return eTLSTransportInvalidParameter;
    }

    TlsTransportParams_t * pxTlsParams = (TlsTransportParams_t*)pNetworkContext->pParams;

    if (( pxTlsParams == NULL ))
    {
        ESP_LOGE( TAG, "Invalid input parameter(s): Arguments cannot be NULL." );
        return eTLSTransportInvalidParameter;
    }

    EspTlsTransportParams_t * pxEspTlsTransport = (EspTlsTransportParams_t *)pxTlsParams->xSSLContext;

    tlsStatus = esp_transport_write( pxEspTlsTransport->xTransport, pBuffer, xBytesToSend, pxEspTlsTransport->ulSendTimeoutMs );
    if ( tlsStatus < 0 )
    {
        ESP_LOGE( TAG, "Writing failed, errno= %d", errno );
        return ESP_FAIL;
    }

    return tlsStatus;
}
/*-----------------------------------------------------------*/
