/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "jws.h"

#include "azure/az_core.h"
#include "azure/az_iot.h"

#include "azure_iot_result.h"
#include "azure_iot_json_reader.h"
#include "azure_iot_adu_client.h"

#include "mbedtls/base64.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/cipher.h"

/* For logging */
#include "demo_config.h"

/**
 * @brief Convenience macro to return if an operation failed.
 */
#define azureiotresultRETURN_IF_FAILED( exp )        \
    do                                               \
    {                                                \
        AzureIoTResult_t const _xAzResult = ( exp ); \
        if( _xAzResult != eAzureIoTSuccess )         \
        {                                            \
            return _xAzResult;                       \
        }                                            \
    } while( 0 )

#define jwsRSA3072_SIZE            384
#define jwsSHA256_SIZE             32
#define jwsPKCS7_PAYLOAD_OFFSET    19

#define jwsSHA256_JSON_VALUE       "sha256"
#define jwsSJWK_JSON_VALUE         "sjwk"
#define jwsKID_JSON_VALUE          "kid"
#define jwsN_JSON_VALUE            "n"
#define jwsE_JSON_VALUE            "e"
#define jwsALG_JSON_VALUE          "alg"

static unsigned char ucJWSHeader[ 1400 ];
static unsigned char ucJWSPayload[ 60 ];
static unsigned char ucJWSSignature[ 400 ];

static unsigned char ucJWKHeader[ 48 ];
static unsigned char ucJWKPayload[ 700 ];
static unsigned char ucJWKSignature[ 500 ];

static unsigned char ucSigningKeyN[ jwsRSA3072_SIZE ];
static unsigned char ucSigningKeyE[ 16 ];

static unsigned char ucManifestSHACalculation[ jwsSHA256_SIZE ];
static unsigned char ucParsedManifestSha[ jwsSHA256_SIZE ];

static unsigned char ucScratchCalculatationBuffer[ jwsRSA3072_SIZE + jwsSHA256_SIZE ];

static uint32_t prvSplitJWS( unsigned char * pucJWS,
                             uint32_t ulJWSLength,
                             unsigned char ** ppucHeader,
                             uint32_t * pulHeaderLength,
                             unsigned char ** ppucPayload,
                             uint32_t * pulPayloadLength,
                             unsigned char ** ppucSignature,
                             uint32_t * pulSignatureLength )
{
    unsigned char * pucFirstDot;
    unsigned char * pucSecondDot;
    uint32_t ulDotCount = 0;
    uint32_t ulIndex = 0;

    *ppucHeader = pucJWS;

    while( ulIndex < ulJWSLength )
    {
        if( *pucJWS == '.' )
        {
            ulDotCount++;

            if( ulDotCount == 1 )
            {
                pucFirstDot = pucJWS;
            }
            else if( ulDotCount == 2 )
            {
                pucSecondDot = pucJWS;
            }
            else if( ulDotCount > 2 )
            {
                return eAzureIoTErrorFailed;
            }
        }

        pucJWS++;
        ulIndex++;
    }

    if( ( ulDotCount != 2 ) || ( pucSecondDot >= ( *ppucHeader + ulJWSLength - 1 ) ) )
    {
        return eAzureIoTErrorFailed;
    }

    *pulHeaderLength = pucFirstDot - *ppucHeader;
    *ppucPayload = pucFirstDot + 1;
    *pulPayloadLength = pucSecondDot - *ppucPayload;
    *ppucSignature = pucSecondDot + 1;
    *pulSignatureLength = *ppucHeader + ulJWSLength - *ppucSignature;

    return 0;
}

static void prvSwapToUrlEncodingChars( unsigned char * pucSignature,
                                       uint32_t ulSignatureLength )
{
    uint32_t ulIndex = 0;

    while( ulIndex < ulSignatureLength )
    {
        if( *pucSignature == '-' )
        {
            *pucSignature = '+';
        }
        else if( *pucSignature == '_' )
        {
            *pucSignature = '/';
        }

        pucSignature++;
        ulIndex++;
    }
}

/**
 * @brief Calculate the SHA256 over a buffer of bytes
 *
 * @param pucInput The input buffer over which to calculate the SHA256.
 * @param ulInputLength The length of \p pucInput.
 * @param pucOutput The output buffer into which the SHA256. It must be 32 bytes in length.
 * @return uint32_t The result of the operation.
 * @retval 0 if successful.
 * @retval Non-0 if not successful.
 */
static uint32_t prvJWS_SHA256Calculate( const unsigned char * pucInput,
                                        uint32_t ulInputLength,
                                        unsigned char * pucOutput )
{
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init( &ctx );
    mbedtls_md_setup( &ctx, mbedtls_md_info_from_type( md_type ), 0 );
    mbedtls_md_starts( &ctx );
    mbedtls_md_update( &ctx, pucInput, ulInputLength );
    mbedtls_md_finish( &ctx, pucOutput );
    mbedtls_md_free( &ctx );

    return 0;
}

/**
 * @brief Verify the manifest via RS256 for the JWS.
 *
 * @param pucInput The input over which the RS256 will be verified.
 * @param ulInputLength The length of \p pucInput.
 * @param pucSignature The encrypted signature which will be decrypted by \p pucN and \p pucE.
 * @param ulSignatureLength The length of \p pucSignature.
 * @param pucN The key's modulus which is used to decrypt \p signature.
 * @param ulNLength The length of \p pucN.
 * @param pucE The exponent used for the key.
 * @param ulELength The length of \p pucE.
 * @param pucBuffer The buffer used as scratch space to make the calculations. It should be at least
 * `jwsRSA3072_SIZE` + `jwsSHA256_SIZE` in size.
 * @param ulBufferLength The length of \p pucBuffer.
 * @return uint32_t The result of the operation.
 * @retval 0 if successful.
 * @retval Non-0 if not successful.
 */
static uint32_t prvJWS_RS256Verify( unsigned char * pucInput,
                                    uint32_t ulInputLength,
                                    unsigned char * pucSignature,
                                    uint32_t ulSignatureLength,
                                    unsigned char * pucN,
                                    uint32_t ulNLength,
                                    unsigned char * pucE,
                                    uint32_t ulELength,
                                    unsigned char * pucBuffer,
                                    uint32_t ulBufferLength )
{
    AzureIoTResult_t xResult;
    int32_t lMbedTLSResult;

    if( ulBufferLength < jwsRSA3072_SIZE + jwsSHA256_SIZE )
    {
        LogInfo( ( "Buffer Not Large Enough\n" ) );
        return 1;
    }

    unsigned char * pucShaBuffer = pucBuffer + jwsRSA3072_SIZE;
    size_t ulDecryptedLength;

    /* The signature is encrypted using the input key. We need to decrypt the */
    /* signature which gives us the SHA256 inside a PKCS7 structure. We then compare */
    /* that to the SHA256 of the input. */
    mbedtls_rsa_context ctx;

    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, 0 );

    LogInfo( ( "--- Initializing Decryption ---\n" ) );

    lMbedTLSResult = mbedtls_rsa_import_raw( &ctx,
                                             pucN, ulNLength,
                                             NULL, 0,
                                             NULL, 0,
                                             NULL, 0,
                                             pucE, ulELength );

    if( lMbedTLSResult != 0 )
    {
        LogError( ( "mbedtls_rsa_import_raw res: %i\n", lMbedTLSResult ) );
        mbedtls_rsa_free( &ctx );
        return lMbedTLSResult;
    }

    lMbedTLSResult = mbedtls_rsa_complete( &ctx );

    if( lMbedTLSResult != 0 )
    {
        LogError( ( "mbedtls_rsa_complete res: %i\n", lMbedTLSResult ) );
        mbedtls_rsa_free( &ctx );
        return lMbedTLSResult;
    }

    lMbedTLSResult = mbedtls_rsa_check_pubkey( &ctx );

    if( lMbedTLSResult != 0 )
    {
        LogError( ( "mbedtls_rsa_check_pubkey res: %i\n", lMbedTLSResult ) );
        mbedtls_rsa_free( &ctx );
        return lMbedTLSResult;
    }

    LogInfo( ( "--- Decrypting ---\n" ) );

    /* RSA */
    lMbedTLSResult = mbedtls_rsa_pkcs1_decrypt( &ctx, NULL, NULL, MBEDTLS_RSA_PUBLIC, &ulDecryptedLength, pucSignature, pucBuffer, jwsRSA3072_SIZE );

    if( lMbedTLSResult != 0 )
    {
        LogError( ( "mbedtls_rsa_pkcs1_decrypt res: %i\n", lMbedTLSResult ) );
        mbedtls_rsa_free( &ctx );
        return lMbedTLSResult;
    }

    mbedtls_rsa_free( &ctx );

    LogInfo( ( "---- Calculating SHA256 over input ----\n" ) );
    xResult = prvJWS_SHA256Calculate( pucInput, ulInputLength,
                                      pucShaBuffer );

    LogInfo( ( "--- Checking if SHA256 of header+payload matches decrypted SHA256 ---\n" ) );

    /* TODO: remove this once we have a valid PKCS7 parser. */
    int doTheyMatch = memcmp( pucBuffer + jwsPKCS7_PAYLOAD_OFFSET, pucShaBuffer, jwsSHA256_SIZE );

    if( doTheyMatch == 0 )
    {
        LogInfo( ( "SHA of JWK matches\n" ) );
        xResult = 0;
    }
    else
    {
        LogError( ( "SHA of JWK does NOT match\n" ) );
        xResult = 1;
    }

    return xResult;
}


uint32_t JWS_ManifestAuthenticate( const char * pucManifest,
                                   uint32_t ulManifestLength,
                                   char * pucJWS,
                                   uint32_t ulJWSLength )
{
    uint32_t ulVerificationResult;

    int lMbedResult;
    unsigned char * pucBase64EncodedHeader;
    unsigned char * pucBase64EncodedPayload;
    unsigned char * pucBase64EncodedSignature;
    uint32_t ulBase64EncodedHeaderLength;
    uint32_t ulBase64EncodedPayloadLength;
    uint32_t ulBase64SignatureLength;
    AzureIoTJSONReader_t xJSONReader;

    LogInfo( ( "---------------------Begin Signature Validation --------------------\n\n" ) );

    /*------------------- Parse and Decode the Manifest Sig ------------------------*/

    AzureIoTResult_t xResult = prvSplitJWS( ( unsigned char * ) pucJWS, ulJWSLength,
                                            &pucBase64EncodedHeader, &ulBase64EncodedHeaderLength,
                                            &pucBase64EncodedPayload, &ulBase64EncodedPayloadLength,
                                            &pucBase64EncodedSignature, &ulBase64SignatureLength );
    prvSwapToUrlEncodingChars( pucBase64EncodedSignature, ulBase64SignatureLength );

    /* Note that we do not use mbedTLS to base64 decode values since we need the ability to assume padding characters. */
    /* mbedTLS will stop the decoding short and we would then need to hack in the remaining characters. */
    LogInfo( ( "---JWS Base64 Decode Header---\n" ) );
    int32_t outJWSHeaderLength;
    az_span xJWSBase64EncodedHeaderSpan = az_span_create( pucBase64EncodedHeader, ulBase64EncodedHeaderLength );
    az_span xJWSHeaderSpan = az_span_create( ucJWSHeader, sizeof( ucJWSHeader ) );
    az_result xCoreResult = az_base64_decode( xJWSHeaderSpan, xJWSBase64EncodedHeaderSpan, &outJWSHeaderLength );
    /* TODO: here and elsewhere, remove verbose logs */
    /* LogInfo(( "az_base64_decode return: 0x%x\n", xCoreResult)); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outJWSHeaderLength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outJWSHeaderLength, ( char * ) ucJWSHeader )); */

    LogInfo( ( "---JWS Base64 Decode Payload---\n" ) );
    int32_t outJWSPayloadLength;
    az_span xJWSBase64EncodedPayloadSpan = az_span_create( pucBase64EncodedPayload, ulBase64EncodedPayloadLength );
    az_span xJWSPayloadSpan = az_span_create( ucJWSPayload, sizeof( ucJWSPayload ) );
    xCoreResult = az_base64_decode( xJWSPayloadSpan, xJWSBase64EncodedPayloadSpan, &outJWSPayloadLength );
    /* LogInfo(( "az_base64_decode return: 0x%x\n", xCoreResult)); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outJWSPayloadLength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outJWSPayloadLength, ( char * ) ucJWSPayload )); */

    LogInfo( ( "---JWS Base64 Decode Signature---\n" ) );
    int32_t outJWSSignatureLength;
    az_span xJWSBase64EncodedSignatureSpan = az_span_create( pucBase64EncodedSignature, ulBase64SignatureLength );
    az_span xJWSSignatureSpan = az_span_create( ucJWSSignature, sizeof( ucJWSSignature ) );
    xCoreResult = az_base64_decode( xJWSSignatureSpan, xJWSBase64EncodedSignatureSpan, &outJWSSignatureLength );
    /* LogInfo(( "az_base64_decode return: 0x%x\n", xCoreResult)); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outJWSSignatureLength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outJWSSignatureLength, ( char * ) ucJWSSignature )); */


    /*------------------- Parse JSK JSON Payload ------------------------*/

    /* The "sjwk" is the signed signing public key */
    LogInfo( ( "---Parsing JWS JSON Payload---\n" ) );
    unsigned char * pucJWKManifest;
    az_span xJWKManifestSpan;

    AzureIoTJSONReader_Init( &xJSONReader, ( const uint8_t * ) az_span_ptr( xJWSHeaderSpan ), outJWSHeaderLength );
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, ( const uint8_t * ) jwsSJWK_JSON_VALUE, sizeof( jwsSJWK_JSON_VALUE ) - 1 ) )
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
            break;
        }
        else
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_SkipChildren( &xJSONReader ) );
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
    }

    if( ( xResult != eAzureIoTSuccess ) && ( xResult != eAzureIoTErrorJSONReaderDone ) )
    {
        LogError( ( "Parse JSK JSON Payload Error: 0x%08x\n", xResult ) );
        return xResult;
    }

    xJWKManifestSpan = xJSONReader._internal.xCoreReader.token.slice;

    pucJWKManifest = az_span_ptr( xJWKManifestSpan );
    uint32_t ulJWKManifestLength = az_span_size( xJWKManifestSpan );

    /*------------------- Base64 Decode the JWK Payload ------------------------*/

    unsigned char * pucJWKBase64EncodedHeader;
    unsigned char * pucJWKBase64EncodedPayload;
    unsigned char * pucJWKBase64EncodedSignature;
    uint32_t ulJWKBase64EncodedHeaderLength;
    uint32_t ulJWKBase64EncodedPayloadLength;
    uint32_t ulJWKBase64EncodedSignatureLength;

    LogInfo( ( "--- Base64 Decoding JWS Payload ---\n" ) );

    xResult = prvSplitJWS( pucJWKManifest, ulJWKManifestLength,
                           &pucJWKBase64EncodedHeader, &ulJWKBase64EncodedHeaderLength,
                           &pucJWKBase64EncodedPayload, &ulJWKBase64EncodedPayloadLength,
                           &pucJWKBase64EncodedSignature, &ulJWKBase64EncodedSignatureLength );
    prvSwapToUrlEncodingChars( pucJWKBase64EncodedSignature, ulJWKBase64EncodedSignatureLength );

    LogInfo( ( "--- JWK Base64 Decode Header ---\n" ) );
    int32_t outJWKHeaderLength;
    az_span xJWKBase64EncodedHeaderSpan = az_span_create( pucJWKBase64EncodedHeader, ulJWKBase64EncodedHeaderLength );
    az_span xJWKHeaderSpan = az_span_create( ucJWKHeader, sizeof( ucJWKHeader ) );
    xCoreResult = az_base64_decode( xJWKHeaderSpan, xJWKBase64EncodedHeaderSpan, &outJWKHeaderLength );
    /* LogInfo(( "az_base64_decode return: 0x%x\n", xCoreResult)); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outJWKHeaderLength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outJWKHeaderLength, ( char * ) ucJWKHeader )); */

    LogInfo( ( "--- JWK Base64 Decode Payload ---\n" ) );
    int32_t outJWKPayloadLength;
    az_span xJWKBase64EncodedPayloadSpan = az_span_create( pucJWKBase64EncodedPayload, ulJWKBase64EncodedPayloadLength );
    az_span xJWKPayloadSpan = az_span_create( ucJWKPayload, sizeof( ucJWKPayload ) );
    xCoreResult = az_base64_decode( xJWKPayloadSpan, xJWKBase64EncodedPayloadSpan, &outJWKPayloadLength );
    /* LogInfo(( "az_base64_decode return: 0x%x\n", xCoreResult)); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outJWKPayloadLength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outJWKPayloadLength, ( char * ) ucJWKPayload )); */

    LogInfo( ( "--- JWK Base64 Decode Signature ---\n" ) );
    int32_t outJWKSignatureLength;
    az_span xJWKBase64EncodedSignatureSpan = az_span_create( pucJWKBase64EncodedSignature, ulJWKBase64EncodedSignatureLength );
    az_span xJWKSignatureSpan = az_span_create( ucJWKSignature, sizeof( ucJWKSignature ) );
    xCoreResult = az_base64_decode( xJWKSignatureSpan, xJWKBase64EncodedSignatureSpan, &outJWKSignatureLength );
    /* LogInfo(( "az_base64_decode return: 0x%x\n", xCoreResult)); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outJWKSignatureLength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outJWKSignatureLength, ( char * ) ucJWKSignature )); */

    /*------------------- Parse id for root key ------------------------*/

    LogInfo( ( "--- Checking Root Key ---\n" ) );
    az_span kidSpan;
    AzureIoTJSONReader_Init( &xJSONReader, ( const uint8_t * ) ucJWKHeader, outJWKHeaderLength );
    /*Begin object */
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
    /*Property Name */
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, ( const uint8_t * ) jwsKID_JSON_VALUE, sizeof( jwsKID_JSON_VALUE ) - 1 ) )
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
            kidSpan = xJSONReader._internal.xCoreReader.token.slice;

            break;
        }
        else
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_SkipChildren( &xJSONReader ) );
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
    }

    if( ( xResult != eAzureIoTSuccess ) && ( xResult != eAzureIoTErrorJSONReaderDone ) )
    {
        LogError( ( "Parse Root Key Error: %i\n", xResult ) );
        return xResult;
    }

    az_span rootKeyIDSpan = az_span_create( ( uint8_t * ) AzureIoTADURootKeyId, sizeof( AzureIoTADURootKeyId ) - 1 );

    if( az_span_is_content_equal( rootKeyIDSpan, kidSpan ) )
    {
        LogInfo( ( "Using the correct root key\n" ) );
    }
    else
    {
        LogError( ( "Using the wrong root key\n" ) );

        return 1;
    }

    /*------------------- Parse necessary pieces for the verification ------------------------*/

    az_span xBase64EncodedNSpan = AZ_SPAN_EMPTY;
    az_span xBase64EncodedESpan = AZ_SPAN_EMPTY;
    az_span xAlgSpan = AZ_SPAN_EMPTY;
    LogInfo( ( "--- Parse Signing Key Payload ---\n" ) );

    AzureIoTJSONReader_Init( &xJSONReader, ( const uint8_t * ) ucJWKPayload, outJWKPayloadLength );
    /*Begin object */
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
    /*Property Name */
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );

    while( xResult == eAzureIoTSuccess && ( az_span_size( xBase64EncodedNSpan ) == 0 || az_span_size( xBase64EncodedESpan ) == 0 || az_span_size( xAlgSpan ) == 0 ) )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, ( const uint8_t * ) jwsN_JSON_VALUE, sizeof( jwsN_JSON_VALUE ) - 1 ) )
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
            xBase64EncodedNSpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
        else if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, ( const uint8_t * ) jwsE_JSON_VALUE, sizeof( jwsE_JSON_VALUE ) - 1 ) )
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
            xBase64EncodedESpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
        else if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, ( const uint8_t * ) jwsALG_JSON_VALUE, sizeof( jwsALG_JSON_VALUE ) - 1 ) )
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
            xAlgSpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
        else
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_SkipChildren( &xJSONReader ) );
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
    }

    if( ( xResult != eAzureIoTSuccess ) && ( xResult != eAzureIoTErrorJSONReaderDone ) )
    {
        LogError( ( "Parse Signing Key Payload Error: %i\n", xResult ) );
        return xResult;
    }

    /* LogInfo(( "--- Print Signing Key Parts ---\n" )); */
    /* LogInfo(( "\txBase64EncodedNSpan: %.*s\n", az_span_size( xBase64EncodedNSpan ), az_span_ptr( xBase64EncodedNSpan ) )); */
    /* LogInfo(( "\txBase64EncodedESpan: %.*s\n", az_span_size( xBase64EncodedESpan ), az_span_ptr( xBase64EncodedESpan ) )); */
    /* LogInfo(( "\txAlgSpan: %.*s\n", az_span_size( xAlgSpan ), az_span_ptr( xAlgSpan ) )); */

    /*------------------- Base64 decode the key ------------------------*/
    LogInfo( ( "--- Signing key base64 decoding N ---\n" ) );
    int32_t outSigningKeyNLength;
    az_span xNSpan = az_span_create( ucSigningKeyN, sizeof( ucSigningKeyN ) );
    xCoreResult = az_base64_decode( xNSpan, xBase64EncodedNSpan, &outSigningKeyNLength );
    /* LogInfo(( "az_base64_decode return: 0x%x\n", xCoreResult)); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outSigningKeyNLength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outSigningKeyNLength, ( char * ) ucSigningKeyN )); */

    LogInfo( ( "--- Signing key base64 decoding E ---\n" ) );
    int32_t outSigningKeyELength;
    az_span xESpan = az_span_create( ucSigningKeyE, sizeof( ucSigningKeyE ) );
    xCoreResult = az_base64_decode( xESpan, xBase64EncodedESpan, &outSigningKeyELength );
    /* LogInfo(( "az_base64_decode return: 0x%x\n", xCoreResult)); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outSigningKeyELength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outSigningKeyELength, ( char * ) ucSigningKeyE )); */


    /*------------------- Verify the signature ------------------------*/
    ulVerificationResult = prvJWS_RS256Verify( pucJWKBase64EncodedHeader, ulJWKBase64EncodedHeaderLength + ulJWKBase64EncodedPayloadLength + 1,
                                               ucJWKSignature, outJWKSignatureLength,
                                               ( unsigned char * ) AzureIoTADURootKeyN, sizeof( AzureIoTADURootKeyN ),
                                               ( unsigned char * ) AzureIoTADURootKeyE, sizeof( AzureIoTADURootKeyE ),
                                               ucScratchCalculatationBuffer, sizeof( ucScratchCalculatationBuffer ) );

    if( ulVerificationResult != 0 )
    {
        LogError( ( "Verification of signing key failed\n" ) );
        return ulVerificationResult;
    }

    /*------------------- Verify that the signature was signed by signing key ------------------------*/
    ulVerificationResult = prvJWS_RS256Verify( pucBase64EncodedHeader, ulBase64EncodedHeaderLength + ulBase64EncodedPayloadLength + 1,
                                               ucJWSSignature, outJWSSignatureLength,
                                               ucSigningKeyN, outSigningKeyNLength,
                                               ucSigningKeyE, outSigningKeyELength,
                                               ucScratchCalculatationBuffer, sizeof( ucScratchCalculatationBuffer ) );

    if( ulVerificationResult != 0 )
    {
        LogError( ( "Verification of signed manifest SHA failed\n" ) );
        return ulVerificationResult;
    }

    /*------------------- Verify that the SHAs match ------------------------*/
    ulVerificationResult = prvJWS_SHA256Calculate( ( const unsigned char * ) pucManifest,
                                                   ulManifestLength,
                                                   ucManifestSHACalculation );

    if( ulVerificationResult != 0 )
    {
        LogError( ( "SHA256 Calculation failed" ) );
        return ulVerificationResult;
    }

    AzureIoTJSONReader_Init( &xJSONReader, ( const uint8_t * ) ucJWSPayload, outJWSPayloadLength );
    /*Begin object */
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
    /*Property Name */
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );

    az_span sha256Span;

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, ( const uint8_t * ) jwsSHA256_JSON_VALUE, sizeof( jwsSHA256_JSON_VALUE ) - 1 ) )
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
            sha256Span = xJSONReader._internal.xCoreReader.token.slice;
            break;
        }
        else
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_SkipChildren( &xJSONReader ) );
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
    }

    if( ( xResult != eAzureIoTSuccess ) && ( xResult != eAzureIoTErrorJSONReaderDone ) )
    {
        LogError( ( "Parse SHA256 Error: %i\n", xResult ) );
        return xResult;
    }

    LogInfo( ( "Parsed SHA: %.*s\n", az_span_size( sha256Span ), ( char * ) az_span_ptr( sha256Span ) ) );

    int32_t outParsedManifestShaSize;
    az_span xParsedManifestSHA = az_span_create( ucParsedManifestSha, sizeof( ucParsedManifestSha ) );
    xCoreResult = az_base64_decode( xParsedManifestSHA, sha256Span, &outParsedManifestShaSize );

    if( outParsedManifestShaSize != jwsSHA256_SIZE )
    {
        LogError( ( "Base64 decoded SHA256 is not the correct length\n" ) );
        return 1;
    }

    ulVerificationResult = memcmp( ucManifestSHACalculation, ucParsedManifestSha, jwsSHA256_SIZE );

    if( ulVerificationResult != 0 )
    {
        LogError( ( "Calculated manifest SHA does not match SHA in payload\n" ) );
        return ulVerificationResult;
    }
    else
    {
        LogInfo( ( "Calculated manifest SHA matches parsed SHA\n" ) );
    }

    /*------------------- Done (Loop) ------------------------*/
    return ulVerificationResult;
}
