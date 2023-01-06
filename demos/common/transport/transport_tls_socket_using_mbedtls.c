/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file transport_tls_socket_using_mbedtls.c
 * @brief TLS transport interface implementations. This implementation uses
 * mbedTLS.
 */

/* Standard includes. */
#include <string.h>

/* Include header that defines log levels. */
#include "logging_levels.h"

/* Logging configuration for the Sockets. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME     "TlsTransport"
#endif
#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_ERROR
#endif

/* Prototype for the function used to print to console on Windows simulator
 * of FreeRTOS.
 * The function prints to the console before the network is connected;
 * then a UDP port after the network has connected. */
extern void vLoggingPrintf( const char * pcFormatString,
                            ... );

/* Map the SdkLog macro to the logging function to enable logging
 * on Windows simulator. */
#ifndef SdkLog
    #define SdkLog( message )    vLoggingPrintf message
#endif

#include "logging_stack.h"

/************ End of logging configuration ****************/

/* FreeRTOS includes. */
#include "FreeRTOS.h"

/* TLS transport header. */
#include "transport_tls_socket.h"

/* FreeRTOS Socket wrapper include. */
#include "sockets_wrapper.h"

/* mbedTLS util includes. */
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ssl.h"
#include "mbedtls/threading.h"
#include "mbedtls/x509.h"
#include "mbedtls/error.h"

/*-----------------------------------------------------------*/

/* Each transport defines the same NetworkContext. The user then passes their respective transport */
/* as pParams for the transport which is defined in the transport header file */
/* (here it's TlsTransportParams_t) */
struct NetworkContext
{
    /* TlsTransportParams_t */
    void * pParams;
};

/**
 * @brief Secured connection context.
 */
typedef struct MbedSSLContext
{
    mbedtls_ssl_config config;               /**< @brief SSL connection configuration. */
    mbedtls_ssl_context context;             /**< @brief SSL connection context */
    mbedtls_x509_crt_profile certProfile;    /**< @brief Certificate security profile for this connection. */
    mbedtls_x509_crt rootCa;                 /**< @brief Root CA certificate context. */
    mbedtls_x509_crt clientCert;             /**< @brief Client certificate context. */
    mbedtls_pk_context privKey;              /**< @brief Client private key context. */
    mbedtls_entropy_context entropyContext;  /**< @brief Entropy context for random number generation. */
    mbedtls_ctr_drbg_context ctrDrgbContext; /**< @brief CTR DRBG context for random number generation. */
} MbedSSLContext_t;

/*-----------------------------------------------------------*/

/**
 * @brief Represents string to be logged when mbedTLS returned error
 * does not contain a high-level code.
 */
static const char * pcNoHighLevelMbedTlsCodeStr = "<No-High-Level-Code>";

/**
 * @brief Represents string to be logged when mbedTLS returned error
 * does not contain a low-level code.
 */
static const char * pcNoLowLevelMbedTlsCodeStr = "<No-Low-Level-Code>";

/**
 * @brief Utility for converting the high-level code in an mbedTLS error to string,
 * if the code-contains a high-level code; otherwise, using a default string.
 */
#define mbedtlsHighLevelCodeOrDefault( mbedTlsCode )       \
    ( mbedtls_high_level_strerr( mbedTlsCode ) != NULL ) ? \
    mbedtls_high_level_strerr( mbedTlsCode ) : pcNoHighLevelMbedTlsCodeStr

/**
 * @brief Utility for converting the level-level code in an mbedTLS error to string,
 * if the code-contains a level-level code; otherwise, using a default string.
 */
#define mbedtlsLowLevelCodeOrDefault( mbedTlsCode )       \
    ( mbedtls_low_level_strerr( mbedTlsCode ) != NULL ) ? \
    mbedtls_low_level_strerr( mbedTlsCode ) : pcNoLowLevelMbedTlsCodeStr

/*-----------------------------------------------------------*/

/**
 * @brief Initialize the mbed TLS structures in a network connection.
 *
 * @param[in] pxSslContext The SSL context to initialize.
 */
static void sslContextInit( MbedSSLContext_t * pxSslContext );

/**
 * @brief Free the mbed TLS structures in a network connection.
 *
 * @param[in] pxSslContext The SSL context to free.
 */
static void sslContextFree( MbedSSLContext_t * pxSslContext );

/**
 * @brief Add X509 certificate to the trusted list of root certificates.
 *
 * OpenSSL does not provide a single function for reading and loading certificates
 * from files into stores, so the file API must be called. Start with the
 * root certificate.
 *
 * @param[out] pxSslContext SSL context to which the trusted server root CA is to be added.
 * @param[in] pucRootCa PEM-encoded string of the trusted server root CA.
 * @param[in] xRootCaSize Size of the trusted server root CA.
 *
 * @return 0 on success; otherwise, failure;
 */
static int32_t setRootCa( MbedSSLContext_t * pxSslContext,
                          const uint8_t * pucRootCa,
                          size_t xRootCaSize );

/**
 * @brief Set X509 certificate as client certificate for the server to authenticate.
 *
 * @param[out] pxSslContext SSL context to which the client certificate is to be set.
 * @param[in] pucClientCert PEM-encoded string of the client certificate.
 * @param[in] xClientCertSize Size of the client certificate.
 *
 * @return 0 on success; otherwise, failure;
 */
static int32_t setClientCertificate( MbedSSLContext_t * pxSslContext,
                                     const uint8_t * pucClientCert,
                                     size_t xClientCertSize );

/**
 * @brief Set private key for the client's certificate.
 *
 * @param[out] pxSslContext SSL context to which the private key is to be set.
 * @param[in] pucPrivateKey PEM-encoded string of the client private key.
 * @param[in] xPrivateKeySize Size of the client private key.
 *
 * @return 0 on success; otherwise, failure;
 */
static int32_t setPrivateKey( MbedSSLContext_t * pxSslContext,
                              const uint8_t * pucPrivateKey,
                              size_t xPrivateKeySize );

/**
 * @brief Passes TLS credentials to the OpenSSL library.
 *
 * Provides the root CA certificate, client certificate, and private key to the
 * OpenSSL library. If the client certificate or private key is not NULL, mutual
 * authentication is used when performing the TLS handshake.
 *
 * @param[out] pxSslContext SSL context to which the credentials are to be imported.
 * @param[in] pxNetworkCredentials TLS credentials to be imported.
 *
 * @return 0 on success; otherwise, failure;
 */
static int32_t setCredentials( MbedSSLContext_t * pxSslContext,
                               const NetworkCredentials_t * pxNetworkCredentials );

/**
 * @brief Set optional configurations for the TLS connection.
 *
 * This function is used to set SNI and ALPN protocols.
 *
 * @param[in] pxSslContext SSL context to which the optional configurations are to be set.
 * @param[in] pcHostName Remote host name, used for server name indication.
 * @param[in] pxNetworkCredentials TLS setup parameters.
 */
static void setOptionalConfigurations( MbedSSLContext_t * pxSslContext,
                                       const char * pcHostName,
                                       const NetworkCredentials_t * pxNetworkCredentials );

/**
 * @brief Setup TLS by initializing contexts and setting configurations.
 *
 * @param[in] pxNetworkContext Network context.
 * @param[in] pcHostName Remote host name, used for server name indication.
 * @param[in] pxNetworkCredentials TLS setup parameters.
 *
 * @return #eTLSTransportSuccess, #eTLSTransportInsufficientMemory, #eTLSTransportInvalidCredentials,
 * or #eTLSTransportInternalError.
 */
static TlsTransportStatus_t tlsSetup( NetworkContext_t * pxNetworkContext,
                                      const char * pcHostName,
                                      const NetworkCredentials_t * pxNetworkCredentials );

/**
 * @brief Perform the TLS handshake on a TCP connection.
 *
 * @param[in] pxNetworkContext Network context.
 * @param[in] pxNetworkCredentials TLS setup parameters.
 *
 * @return #eTLSTransportSuccess, #eTLSTransportHandshakeFailed, or #eTLSTransportInternalError.
 */
static TlsTransportStatus_t tlsHandshake( NetworkContext_t * pxNetworkContext,
                                          const NetworkCredentials_t * pxNetworkCredentials );

/**
 * @brief Initialize mbedTLS.
 *
 * @param[out] entropyContext mbed TLS entropy context for generation of random numbers.
 * @param[out] ctrDrgbContext mbed TLS CTR DRBG context for generation of random numbers.
 *
 * @return #eTLSTransportSuccess, or #eTLSTransportInternalError.
 */
static TlsTransportStatus_t initMbedtls( mbedtls_entropy_context * pxEntropyContext,
                                         mbedtls_ctr_drbg_context * pxCtrDrgbContext );

/*-----------------------------------------------------------*/

static void sslContextInit( MbedSSLContext_t * pxSslContext )
{
    configASSERT( pxSslContext != NULL );

    mbedtls_ssl_config_init( &( pxSslContext->config ) );
    mbedtls_x509_crt_init( &( pxSslContext->rootCa ) );
    mbedtls_pk_init( &( pxSslContext->privKey ) );
    mbedtls_x509_crt_init( &( pxSslContext->clientCert ) );
    mbedtls_ssl_init( &( pxSslContext->context ) );
}
/*-----------------------------------------------------------*/

static void sslContextFree( MbedSSLContext_t * pxSslContext )
{
    configASSERT( pxSslContext != NULL );

    mbedtls_ssl_free( &( pxSslContext->context ) );
    mbedtls_x509_crt_free( &( pxSslContext->rootCa ) );
    mbedtls_x509_crt_free( &( pxSslContext->clientCert ) );
    mbedtls_pk_free( &( pxSslContext->privKey ) );
    mbedtls_entropy_free( &( pxSslContext->entropyContext ) );
    mbedtls_ctr_drbg_free( &( pxSslContext->ctrDrgbContext ) );
    mbedtls_ssl_config_free( &( pxSslContext->config ) );
}
/*-----------------------------------------------------------*/

static int32_t setRootCa( MbedSSLContext_t * pxSslContext,
                          const uint8_t * pucRootCa,
                          size_t xRootCaSize )
{
    int32_t lMbedtlsError = -1;

    configASSERT( pxSslContext != NULL );
    configASSERT( pucRootCa != NULL );

    /* Parse the server root CA certificate into the SSL context. */
    lMbedtlsError = mbedtls_x509_crt_parse( &( pxSslContext->rootCa ),
                                            pucRootCa,
                                            xRootCaSize );

    if( lMbedtlsError != 0 )
    {
        LogError( ( "Failed to parse server root CA certificate: lMbedtlsError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
    }
    else
    {
        mbedtls_ssl_conf_ca_chain( &( pxSslContext->config ),
                                   &( pxSslContext->rootCa ),
                                   NULL );
    }

    return lMbedtlsError;
}
/*-----------------------------------------------------------*/

static int32_t setClientCertificate( MbedSSLContext_t * pxSslContext,
                                     const uint8_t * pucClientCert,
                                     size_t xClientCertSize )
{
    int32_t lMbedtlsError = -1;

    configASSERT( pxSslContext != NULL );
    configASSERT( pucClientCert != NULL );

    /* Setup the client certificate. */
    lMbedtlsError = mbedtls_x509_crt_parse( &( pxSslContext->clientCert ),
                                            pucClientCert,
                                            xClientCertSize );

    if( lMbedtlsError != 0 )
    {
        LogError( ( "Failed to parse the client certificate: lMbedtlsError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
    }

    return lMbedtlsError;
}
/*-----------------------------------------------------------*/

static int32_t setPrivateKey( MbedSSLContext_t * pxSslContext,
                              const uint8_t * pucPrivateKey,
                              size_t xPrivateKeySize )
{
    int32_t lMbedtlsError = -1;

    configASSERT( pxSslContext != NULL );
    configASSERT( pucPrivateKey != NULL );

    /* Setup the client private key. */
    lMbedtlsError = mbedtls_pk_parse_key( &( pxSslContext->privKey ),
                                          pucPrivateKey,
                                          xPrivateKeySize,
                                          NULL,
                                          0 );

    if( lMbedtlsError != 0 )
    {
        LogError( ( "Failed to parse the client key: lMbedtlsError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
    }

    return lMbedtlsError;
}
/*-----------------------------------------------------------*/

static int32_t setCredentials( MbedSSLContext_t * pxSslContext,
                               const NetworkCredentials_t * pxNetworkCredentials )
{
    int32_t lMbedtlsError = -1;

    configASSERT( pxSslContext != NULL );
    configASSERT( pxNetworkCredentials != NULL );

    /* Set up the certificate security profile, starting from the default value. */
    pxSslContext->certProfile = mbedtls_x509_crt_profile_default;

    /* Set SSL authmode and the RNG context. */
    mbedtls_ssl_conf_authmode( &( pxSslContext->config ),
                               MBEDTLS_SSL_VERIFY_REQUIRED );
    mbedtls_ssl_conf_rng( &( pxSslContext->config ),
                          mbedtls_ctr_drbg_random,
                          &( pxSslContext->ctrDrgbContext ) );
    mbedtls_ssl_conf_cert_profile( &( pxSslContext->config ),
                                   &( pxSslContext->certProfile ) );

    lMbedtlsError = setRootCa( pxSslContext,
                               pxNetworkCredentials->pucRootCa,
                               pxNetworkCredentials->xRootCaSize );

    if( ( pxNetworkCredentials->pucClientCert != NULL ) &&
        ( pxNetworkCredentials->pucPrivateKey != NULL ) )
    {
        if( lMbedtlsError == 0 )
        {
            lMbedtlsError = setClientCertificate( pxSslContext,
                                                  pxNetworkCredentials->pucClientCert,
                                                  pxNetworkCredentials->xClientCertSize );
        }

        if( lMbedtlsError == 0 )
        {
            lMbedtlsError = setPrivateKey( pxSslContext,
                                           pxNetworkCredentials->pucPrivateKey,
                                           pxNetworkCredentials->xPrivateKeySize );
        }

        if( lMbedtlsError == 0 )
        {
            lMbedtlsError = mbedtls_ssl_conf_own_cert( &( pxSslContext->config ),
                                                       &( pxSslContext->clientCert ),
                                                       &( pxSslContext->privKey ) );
        }
    }

    return lMbedtlsError;
}
/*-----------------------------------------------------------*/

static void setOptionalConfigurations( MbedSSLContext_t * pxSslContext,
                                       const char * pcHostName,
                                       const NetworkCredentials_t * pxNetworkCredentials )
{
    int32_t lMbedtlsError = -1;

    configASSERT( pxSslContext != NULL );
    configASSERT( pcHostName != NULL );
    configASSERT( pxNetworkCredentials != NULL );

    if( pxNetworkCredentials->ppcAlpnProtos != NULL )
    {
        /* Include an application protocol list in the TLS ClientHello
         * message. */
        lMbedtlsError = mbedtls_ssl_conf_alpn_protocols( &( pxSslContext->config ),
                                                         pxNetworkCredentials->ppcAlpnProtos );

        if( lMbedtlsError != 0 )
        {
            LogError( ( "Failed to configure ALPN protocol in mbed TLS: lMbedtlsError[%d]= %s : %s.",
                        lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
        }
    }

    /* Enable SNI if requested. */
    if( pxNetworkCredentials->xDisableSni == pdFALSE )
    {
        lMbedtlsError = mbedtls_ssl_set_hostname( &( pxSslContext->context ),
                                                  pcHostName );

        if( lMbedtlsError != 0 )
        {
            LogError( ( "Failed to set server name: lMbedtlsError[%d]= %s : %s.",
                        lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
        }
    }

    /* Set Maximum Fragment Length if enabled. */
    #ifdef MBEDTLS_SSL_MAX_FRAGMENT_LENGTH

        /* Enable the max fragment extension. 4096 bytes is currently the largest fragment size permitted.
         * See RFC 8449 https://tools.ietf.org/html/rfc8449 for more information.
         *
         * Smaller values can be found in "mbedtls/include/ssl.h".
         */
        lMbedtlsError = mbedtls_ssl_conf_max_frag_len( &( pxSslContext->config ), MBEDTLS_SSL_MAX_FRAG_LEN_4096 );

        if( lMbedtlsError != 0 )
        {
            LogError( ( "Failed to maximum fragment length extension: lMbedtlsError[%d]= %s : %s.",
                        lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
        }
    #endif /* ifdef MBEDTLS_SSL_MAX_FRAGMENT_LENGTH */
}
/*-----------------------------------------------------------*/

static TlsTransportStatus_t tlsSetup( NetworkContext_t * pxNetworkContext,
                                      const char * pcHostName,
                                      const NetworkCredentials_t * pxNetworkCredentials )
{
    TlsTransportParams_t * pxTlsTransportParams = NULL;
    TlsTransportStatus_t xRetVal = eTLSTransportSuccess;
    int32_t lMbedtlsError = 0;
    MbedSSLContext_t * pxSSLContext = NULL;

    configASSERT( pxNetworkContext != NULL );
    configASSERT( pxNetworkContext->pParams != NULL );
    configASSERT( pcHostName != NULL );
    configASSERT( pxNetworkCredentials != NULL );
    configASSERT( pxNetworkCredentials->pucRootCa != NULL );

    pxTlsTransportParams = ( TlsTransportParams_t * ) pxNetworkContext->pParams;
    configASSERT( pxTlsTransportParams->xSSLContext != NULL );

    pxSSLContext = ( MbedSSLContext_t * ) pxTlsTransportParams->xSSLContext;

    /* Initialize the mbed TLS context structures. */
    sslContextInit( pxSSLContext );

    lMbedtlsError = mbedtls_ssl_config_defaults( &( pxSSLContext->config ),
                                                 MBEDTLS_SSL_IS_CLIENT,
                                                 MBEDTLS_SSL_TRANSPORT_STREAM,
                                                 MBEDTLS_SSL_PRESET_DEFAULT );

    if( lMbedtlsError != 0 )
    {
        LogError( ( "Failed to set default SSL configuration: lMbedtlsError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );

        /* Per mbed TLS docs, mbedtls_ssl_config_defaults only fails on memory allocation. */
        xRetVal = eTLSTransportInsufficientMemory;
    }

    if( xRetVal == eTLSTransportSuccess )
    {
        lMbedtlsError = setCredentials( pxSSLContext,
                                        pxNetworkCredentials );

        if( lMbedtlsError != 0 )
        {
            xRetVal = eTLSTransportInvalidCredentials;
        }
        else
        {
            /* Optionally set SNI and ALPN protocols. */
            setOptionalConfigurations( pxSSLContext,
                                       pcHostName,
                                       pxNetworkCredentials );
        }
    }

    return xRetVal;
}
/*-----------------------------------------------------------*/

static TlsTransportStatus_t tlsHandshake( NetworkContext_t * pxNetworkContext,
                                          const NetworkCredentials_t * pxNetworkCredentials )
{
    TlsTransportParams_t * pxTlsTransportParams = NULL;
    TlsTransportStatus_t xRetVal = eTLSTransportSuccess;
    int32_t lMbedtlsError = 0;
    MbedSSLContext_t * pxSSLContext = NULL;

    configASSERT( pxNetworkContext != NULL );
    configASSERT( pxNetworkContext->pParams != NULL );
    configASSERT( pxNetworkCredentials != NULL );

    pxTlsTransportParams = ( TlsTransportParams_t * ) pxNetworkContext->pParams;
    configASSERT( pxTlsTransportParams->xSSLContext != NULL );

    pxSSLContext = ( MbedSSLContext_t * ) pxTlsTransportParams->xSSLContext;

    /* Initialize the mbed TLS secured connection context. */
    lMbedtlsError = mbedtls_ssl_setup( &( pxSSLContext->context ),
                                       &( pxSSLContext->config ) );

    if( lMbedtlsError != 0 )
    {
        LogError( ( "Failed to set up mbed TLS SSL context: lMbedtlsError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );

        xRetVal = eTLSTransportInternalError;
    }
    else
    {
        /* Set the underlying IO for the TLS connection. */

        /* MISRA Rule 11.2 flags the following line for casting the second
         * parameter to void *. This rule is suppressed because
         * #mbedtls_ssl_set_bio requires the second parameter as void *.
         */
        /* coverity[misra_c_2012_rule_11_2_violation] */
        mbedtls_ssl_set_bio( &( pxSSLContext->context ),
                             ( void * ) pxTlsTransportParams->xTCPSocket,
                             mbedtls_platform_send,
                             mbedtls_platform_recv,
                             NULL );
    }

    if( xRetVal == eTLSTransportSuccess )
    {
        /* Perform the TLS handshake. */
        do
        {
            lMbedtlsError = mbedtls_ssl_handshake( &( pxSSLContext->context ) );
        } while( ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_READ ) ||
                 ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_WRITE ) );

        if( lMbedtlsError != 0 )
        {
            LogError( ( "Failed to perform TLS handshake: lMbedtlsError[%d]= %s : %s.",
                        lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );

            if( lMbedtlsError == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED )
            {
                xRetVal = eTLSTransportCAVerifyFailed;
            }
            else
            {
                xRetVal = eTLSTransportHandshakeFailed;
            }
        }
        else
        {
            LogInfo( ( "(Network connection %p) TLS handshake successful.",
                       pxNetworkContext ) );
        }
    }

    return xRetVal;
}
/*-----------------------------------------------------------*/

static TlsTransportStatus_t initMbedtls( mbedtls_entropy_context * pxEntropyContext,
                                         mbedtls_ctr_drbg_context * pxCtrDrgbContext )
{
    TlsTransportStatus_t xRetVal = eTLSTransportSuccess;
    int32_t lMbedtlsError = 0;

    /* Set the mutex functions for mbed TLS thread safety. */
    mbedtls_threading_set_alt( mbedtls_platform_mutex_init,
                               mbedtls_platform_mutex_free,
                               mbedtls_platform_mutex_lock,
                               mbedtls_platform_mutex_unlock );

    /* Initialize contexts for random number generation. */
    mbedtls_entropy_init( pxEntropyContext );
    mbedtls_ctr_drbg_init( pxCtrDrgbContext );

    /* Add a strong entropy source. At least one is required. */
    lMbedtlsError = mbedtls_entropy_add_source( pxEntropyContext,
                                                mbedtls_platform_entropy_poll,
                                                NULL,
                                                32,
                                                MBEDTLS_ENTROPY_SOURCE_STRONG );

    if( lMbedtlsError != 0 )
    {
        LogError( ( "Failed to add entropy source: lMbedtlsError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
        xRetVal = eTLSTransportInternalError;
    }

    if( xRetVal == eTLSTransportSuccess )
    {
        /* Seed the random number generator. */
        lMbedtlsError = mbedtls_ctr_drbg_seed( pxCtrDrgbContext,
                                               mbedtls_entropy_func,
                                               pxEntropyContext,
                                               NULL,
                                               0 );

        if( lMbedtlsError != 0 )
        {
            LogError( ( "Failed to seed PRNG: lMbedtlsError[%d]= %s : %s.",
                        lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
            xRetVal = eTLSTransportInternalError;
        }
    }

    if( xRetVal == eTLSTransportSuccess )
    {
        LogDebug( ( "Successfully initialized mbedTLS." ) );
    }

    return xRetVal;
}
/*-----------------------------------------------------------*/

TlsTransportStatus_t TLS_Socket_Connect( NetworkContext_t * pxNetworkContext,
                                         const char * pcHostName,
                                         uint16_t usPort,
                                         const NetworkCredentials_t * pxNetworkCredentials,
                                         uint32_t ulReceiveTimeoutMs,
                                         uint32_t ulSendTimeoutMs )
{
    TlsTransportParams_t * pxTlsTransportParams = NULL;
    TlsTransportStatus_t xRetVal = eTLSTransportSuccess;
    BaseType_t xSocketStatus = 0;
    MbedSSLContext_t * pxSSLContext;
    TickType_t xRecvTimeout = pdMS_TO_TICKS( ulReceiveTimeoutMs );
    TickType_t xSendTimeout = pdMS_TO_TICKS( ulSendTimeoutMs );

    if( ( pxNetworkContext == NULL ) ||
        ( pxNetworkContext->pParams == NULL ) ||
        ( pcHostName == NULL ) ||
        ( pxNetworkCredentials == NULL ) )
    {
        LogError( ( "Invalid input parameter(s): Arguments cannot be NULL. pxNetworkContext=%p, "
                    "pcHostName=%p, pxNetworkCredentials=%p.",
                    pxNetworkCredentials,
                    pcHostName,
                    pxNetworkCredentials ) );
        xRetVal = eTLSTransportInvalidParameter;
    }
    else if( ( pxNetworkCredentials->pucRootCa == NULL ) )
    {
        LogError( ( "pucRootCa cannot be NULL." ) );
        xRetVal = eTLSTransportInvalidParameter;
    }
    else if( ( pxSSLContext = pvPortMalloc( sizeof( MbedSSLContext_t ) ) ) == NULL )
    {
        LogError( ( "Failed to allocate mbed ssl context memmory ." ) );
        xRetVal = eTLSTransportInsufficientMemory;
    }
    else
    {
        pxTlsTransportParams = pxNetworkContext->pParams;
        pxTlsTransportParams->xSSLContext = ( SSLContextHandle ) pxSSLContext;

        if( ( pxTlsTransportParams->xTCPSocket = Sockets_Open() ) == SOCKETS_INVALID_SOCKET )
        {
            LogError( ( "Failed to open socket." ) );
            xRetVal = eTLSTransportConnectFailure;
        }
        else if( ( xSocketStatus = Sockets_SetSockOpt( pxTlsTransportParams->xTCPSocket,
                                                       SOCKETS_SO_RCVTIMEO,
                                                       &xRecvTimeout,
                                                       sizeof( xRecvTimeout ) ) != 0 ) )
        {
            LogError( ( "Failed to set receive timeout on socket %d.", xSocketStatus ) );
            xRetVal = eTLSTransportInternalError;
        }
        else if( ( xSocketStatus = Sockets_SetSockOpt( pxTlsTransportParams->xTCPSocket,
                                                       SOCKETS_SO_SNDTIMEO,
                                                       &xSendTimeout,
                                                       sizeof( xSendTimeout ) ) != 0 ) )
        {
            LogError( ( "Failed to set send timeout on socket %d.", xSocketStatus ) );
            xRetVal = eTLSTransportInternalError;
        }
        else if( ( xSocketStatus = Sockets_Connect( pxTlsTransportParams->xTCPSocket,
                                                    pcHostName,
                                                    usPort ) ) != 0 )
        {
            LogError( ( "Failed to connect to %s with error %d.",
                        pcHostName,
                        xSocketStatus ) );
            xRetVal = eTLSTransportConnectFailure;
        }
        else if( ( xRetVal = initMbedtls( &( pxSSLContext->entropyContext ),
                                          &( pxSSLContext->ctrDrgbContext ) ) ) != eTLSTransportSuccess )
        {
            LogError( ( "Failed to initialize Mbedtls %d.", xRetVal ) );
        }
        else if( ( xRetVal = tlsSetup( pxNetworkContext, pcHostName,
                                       pxNetworkCredentials ) ) != eTLSTransportSuccess )
        {
            LogError( ( "Failed to setup Mbedtls %d.", xRetVal ) );
        }
        else if( ( xRetVal = tlsHandshake( pxNetworkContext, pxNetworkCredentials ) ) != eTLSTransportSuccess )
        {
            LogError( ( "Failed to do TLS handshake %d.", xRetVal ) );
        }
        else
        {
            LogInfo( ( "(Network connection %p) Connection to %s established.",
                       pxNetworkContext,
                       pcHostName ) );
        }

        /* Clean up on failure. */
        if( xRetVal != eTLSTransportSuccess )
        {
            if( ( pxNetworkContext != NULL ) && ( pxNetworkContext->pParams != NULL ) )
            {
                sslContextFree( pxSSLContext );
                vPortFree( pxSSLContext );
                pxTlsTransportParams->xSSLContext = NULL;

                if( pxTlsTransportParams->xTCPSocket != SOCKETS_INVALID_SOCKET )
                {
                    ( void ) Sockets_Disconnect( pxTlsTransportParams->xTCPSocket );
                    ( void ) Sockets_Close( pxTlsTransportParams->xTCPSocket );
                }
            }
        }
    }

    return xRetVal;
}
/*-----------------------------------------------------------*/

void TLS_Socket_Disconnect( NetworkContext_t * pxNetworkContext )
{
    TlsTransportParams_t * pxTlsTransportParams = NULL;
    int32_t lMbedtlsError = 0;
    MbedSSLContext_t * pxSSLContext;

    if( ( pxNetworkContext == NULL ) || ( pxNetworkContext->pParams != NULL ) )
    {
        /* WANT_READ and WANT_WRITE can be ignored. Logging for debugging purposes. */
        LogInfo( ( "(Network connection %p) TLS close-notify sent; ",
                   "received %s as the TLS status can be ignored for close-notify."
                   ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_READ ) ? "WANT_READ" : "WANT_WRITE",
                   pxNetworkContext ) );
    }

    pxTlsTransportParams = ( TlsTransportParams_t * ) pxNetworkContext->pParams;

    if( pxTlsTransportParams->xSSLContext == NULL )
    {
        {
            /* WANT_READ and WANT_WRITE can be ignored. Logging for debugging purposes. */
            LogInfo( ( "(Network connection %p) TLS close-notify sent; ",
                       "received %s as the TLS status can be ignored for close-notify."
                       ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_READ ) ? "WANT_READ" : "WANT_WRITE",
                       pxNetworkContext ) );
        }
    }

    pxSSLContext = ( MbedSSLContext_t * ) pxTlsTransportParams->xSSLContext;

    /* Attempting to terminate TLS connection. */
    lMbedtlsError = mbedtls_ssl_close_notify( &( pxSSLContext->context ) );

    /* Ignore the WANT_READ and WANT_WRITE return values. */
    if( ( lMbedtlsError != MBEDTLS_ERR_SSL_WANT_READ ) &&
        ( lMbedtlsError != MBEDTLS_ERR_SSL_WANT_WRITE ) )
    {
        if( lMbedtlsError == 0 )
        {
            LogInfo( ( "(Network connection %p) TLS close-notify sent.",
                       pxNetworkContext ) );
        }
        else
        {
            LogError( ( "(Network connection %p) Failed to send TLS close-notify: mbedTLSError[%d]= %s : %s.",
                        pxNetworkContext, lMbedtlsError,
                        mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
        }
    }
    else
    {
        /* WANT_READ and WANT_WRITE can be ignored. Logging for debugging purposes. */
        LogInfo( ( "(Network connection %p) TLS close-notify sent; ",
                   "received %s as the TLS status can be ignored for close-notify."
                   ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_READ ) ? "WANT_READ" : "WANT_WRITE",
                   pxNetworkContext ) );
    }

    /* Call socket shutdown function to close connection. */
    Sockets_Disconnect( pxTlsTransportParams->xTCPSocket );
    Sockets_Close( pxTlsTransportParams->xTCPSocket );

    /* Free mbed TLS contexts. */
    sslContextFree( pxSSLContext );
    vPortFree( pxSSLContext );

    /* Clear the mutex functions for mbed TLS thread safety. */
    mbedtls_threading_free_alt();
}
/*-----------------------------------------------------------*/

int32_t TLS_Socket_Recv( NetworkContext_t * pxNetworkContext,
                         void * pvBuffer,
                         size_t xBytesToRecv )
{
    int32_t lMbedtlsError = 0;
    MbedSSLContext_t * pxSSLContext;
    TlsTransportParams_t * pxTlsTransportParams = NULL;

    configASSERT( ( pxNetworkContext != NULL ) &&
                  ( pxNetworkContext->pParams != NULL ) );

    pxTlsTransportParams = ( TlsTransportParams_t * ) pxNetworkContext->pParams;

    configASSERT( pxTlsTransportParams->xSSLContext != NULL );

    pxSSLContext = ( MbedSSLContext_t * ) pxTlsTransportParams->xSSLContext;
    lMbedtlsError = ( int32_t ) mbedtls_ssl_read( &( pxSSLContext->context ),
                                                  pvBuffer,
                                                  xBytesToRecv );

    if( ( lMbedtlsError == MBEDTLS_ERR_SSL_TIMEOUT ) ||
        ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_READ ) ||
        ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_WRITE ) )
    {
        LogDebug( ( "Failed to read data. However, a read can be retried on this error. "
                    "mbedTLSError[%d]= %s : %s.", lMbedtlsError,
                    mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );

        /* Mark these set of errors as a timeout. The libraries may retry read
         * on these errors. */
        lMbedtlsError = 0;
    }
    else if( lMbedtlsError < 0 )
    {
        LogError( ( "Failed to read data: mbedTLSError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
    }
    else
    {
        /* Empty else marker. */
    }

    return lMbedtlsError;
}
/*-----------------------------------------------------------*/

int32_t TLS_Socket_Send( NetworkContext_t * pxNetworkContext,
                         const void * pvBuffer,
                         size_t xBytesToSend )
{
    int32_t lMbedtlsError = 0;
    MbedSSLContext_t * pxSSLContext;
    TlsTransportParams_t * pxTlsTransportParams = NULL;

    configASSERT( ( pxNetworkContext != NULL ) &&
                  ( pxNetworkContext->pParams != NULL ) );

    pxTlsTransportParams = ( TlsTransportParams_t * ) pxNetworkContext->pParams;

    configASSERT( pxTlsTransportParams->xSSLContext != NULL );

    pxSSLContext = ( MbedSSLContext_t * ) pxTlsTransportParams->xSSLContext;
    lMbedtlsError = ( int32_t ) mbedtls_ssl_write( &( pxSSLContext->context ),
                                                   pvBuffer,
                                                   xBytesToSend );

    if( ( lMbedtlsError == MBEDTLS_ERR_SSL_TIMEOUT ) ||
        ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_READ ) ||
        ( lMbedtlsError == MBEDTLS_ERR_SSL_WANT_WRITE ) )
    {
        LogDebug( ( "Failed to send data. However, send can be retried on this error. "
                    "mbedTLSError[%d]= %s : %s.", lMbedtlsError,
                    mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );

        /* Mark these set of errors as a timeout. The libraries may retry send
         * on these errors. */
        lMbedtlsError = 0;
    }
    else if( lMbedtlsError < 0 )
    {
        LogError( ( "Failed to send data:  mbedTLSError[%d]= %s : %s.",
                    lMbedtlsError, mbedtlsHighLevelCodeOrDefault( lMbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( lMbedtlsError ) ) );
    }
    else
    {
        /* Empty else marker. */
    }

    return lMbedtlsError;
}
/*-----------------------------------------------------------*/
