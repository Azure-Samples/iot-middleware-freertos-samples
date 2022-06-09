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
    uint32_t ulSignatureLength;
    AzureIoTJSONReader_t xJSONReader;

    LogInfo( ( "---------------------Begin Signature Validation --------------------\n\n" ) );

    /*------------------- Parse and Decode the Manifest Sig ------------------------*/

    AzureIoTResult_t xResult = prvSplitJWS( ( unsigned char * ) pucJWS, ulJWSLength,
                                            &pucBase64EncodedHeader, &ulBase64EncodedHeaderLength,
                                            &pucBase64EncodedPayload, &ulBase64EncodedPayloadLength,
                                            &pucBase64EncodedSignature, &ulSignatureLength );
    prvSwapToUrlEncodingChars( pucBase64EncodedSignature, ulSignatureLength );

    LogInfo( ( "---JWS Base64 Decode Header---\n" ) );
    int32_t outJWSHeaderLength;
    lMbedResult = mbedtls_base64_decode( ( unsigned char * ) ucJWSHeader, sizeof( ucJWSHeader ), ( size_t * ) &outJWSHeaderLength, ( const unsigned char * ) pucBase64EncodedHeader, ulBase64EncodedHeaderLength );
    /* TODO: here and elsewhere, remove verbose logs */
    /* LogInfo(( "\tmbedTLS Return: 0x%x\n", lMbedResult )); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outJWSHeaderLength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outJWSHeaderLength, ( char * ) ucJWSHeader )); */

    LogInfo( ( "---JWS Base64 Decode Payload---\n" ) );
    int32_t outJWSPayloadLength;
    lMbedResult = mbedtls_base64_decode( ( unsigned char * ) ucJWSPayload, sizeof( ucJWSPayload ), ( size_t * ) &outJWSPayloadLength, ( const unsigned char * ) pucBase64EncodedPayload, ulBase64EncodedPayloadLength );
    /* LogInfo(( "\tmbedTLS Return: 0x%x\n", lMbedResult )); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outJWSPayloadLength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outJWSPayloadLength, ( char * ) ucJWSPayload )); */

    LogInfo( ( "---JWS Base64 Decode Signature---\n" ) );
    int32_t outJWSSignatureLength;
    lMbedResult = mbedtls_base64_decode( ( unsigned char * ) ucJWSSignature, sizeof( ucJWSSignature ), ( size_t * ) &outJWSSignatureLength, ( const unsigned char * ) pucBase64EncodedSignature, ulSignatureLength );
    /* LogInfo(( "\tmbedTLS Return: 0x%x\n", lMbedResult )); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outJWSSignatureLength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outJWSSignatureLength, ( char * ) ucJWSSignature )); */


    /*------------------- Parse JSK JSON Payload ------------------------*/

    /* The "sjwk" is the signed signing public key */
    LogInfo( ( "---Parsing JWS JSON Payload---\n" ) );
    unsigned char * pucJWKManifest;
    az_span xJWKManifestSpan;

    /*TODO: REMOVE THIS HACK */
    /* The JWK standard explicitly prohibits the padding characters from base64 encoded sections. */
    /* The mbedtls base64 decoder does not assume the padding characters, and therefore cuts the */
    /* decoding short. We hardcode the remaining characters in for now. */
    ucJWSHeader[ outJWSHeaderLength ] = '"';
    ucJWSHeader[ outJWSHeaderLength + 1 ] = '}';
    outJWSHeaderLength = outJWSHeaderLength + 2;
    AzureIoTJSONReader_Init( &xJSONReader, ( const uint8_t * ) ucJWSHeader, outJWSHeaderLength );
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
        LogError( ( "Parse JSK JSON Payload Error: %i\n", xResult ) );
        return xResult;
    }

    xJWKManifestSpan = xJSONReader._internal.xCoreReader.token.slice;

    pucJWKManifest = az_span_ptr( xJWKManifestSpan );
    uint32_t ulJWKManifestLength = az_span_size( xJWKManifestSpan );

    /*------------------- Base64 Decode the JWK Payload ------------------------*/

    unsigned char * pucJWKHeader;
    unsigned char * pucJWKPayload;
    unsigned char * pucJWKSignature;
    uint32_t ulJWKHeaderLength;
    uint32_t ulJWKPayloadLength;
    uint32_t ulJWKSignatureLength;

    LogInfo( ( "--- Base64 Decoding JWS Payload ---\n" ) );

    xResult = prvSplitJWS( pucJWKManifest, ulJWKManifestLength,
                           &pucJWKHeader, &ulJWKHeaderLength,
                           &pucJWKPayload, &ulJWKPayloadLength,
                           &pucJWKSignature, &ulJWKSignatureLength );
    prvSwapToUrlEncodingChars( pucJWKSignature, ulJWKSignatureLength );

    LogInfo( ( "--- JWK Base64 Decode Header ---\n" ) );
    int32_t outJWKHeaderLength;
    lMbedResult = mbedtls_base64_decode( ( unsigned char * ) ucJWKHeader, sizeof( ucJWKHeader ), ( size_t * ) &outJWKHeaderLength, pucJWKHeader, ulJWKHeaderLength );
    /* LogInfo(( "\tmbedTLS Return: 0x%x\n", lMbedResult )); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outJWKHeaderLength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outJWKHeaderLength, ( char * ) ucJWKHeader )); */

    LogInfo( ( "--- JWK Base64 Decode Payload ---\n" ) );
    int32_t outJWKPayloadLength;
    lMbedResult = mbedtls_base64_decode( ( unsigned char * ) ucJWKPayload,
                                         sizeof( ucJWKPayload ),
                                         ( size_t * ) &outJWKPayloadLength,
                                         pucJWKPayload,
                                         ulJWKPayloadLength );
    /* LogInfo(( "\tmbedTLS Return: 0x%x\n", lMbedResult )); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outJWKPayloadLength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outJWKPayloadLength, ( char * ) ucJWKPayload )); */

    LogInfo( ( "--- JWK Base64 Decode Signature ---\n" ) );
    int32_t outJWKSignatureLength;
    lMbedResult = mbedtls_base64_decode( ( unsigned char * ) ucJWKSignature, sizeof( ucJWKSignature ), ( size_t * ) &outJWKSignatureLength, pucJWKSignature, ulJWKSignatureLength );
    /* LogInfo(( "\tmbedTLS Return: 0x%x\n", lMbedResult )); */
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

    az_span nSpan = AZ_SPAN_EMPTY;
    az_span eSpan = AZ_SPAN_EMPTY;
    az_span algSpan = AZ_SPAN_EMPTY;
    LogInfo( ( "--- Parse Signing Key Payload ---\n" ) );

    AzureIoTJSONReader_Init( &xJSONReader, ( const uint8_t * ) ucJWKPayload, outJWKPayloadLength );
    /*Begin object */
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
    /*Property Name */
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );

    while( xResult == eAzureIoTSuccess && ( az_span_size( nSpan ) == 0 || az_span_size( eSpan ) == 0 || az_span_size( algSpan ) == 0 ) )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, ( const uint8_t * ) jwsN_JSON_VALUE, sizeof( jwsN_JSON_VALUE ) - 1 ) )
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
            nSpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
        else if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, ( const uint8_t * ) jwsE_JSON_VALUE, sizeof( jwsE_JSON_VALUE ) - 1 ) )
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
            eSpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
        else if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, ( const uint8_t * ) jwsALG_JSON_VALUE, sizeof( jwsALG_JSON_VALUE ) - 1 ) )
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( &xJSONReader ) );
            algSpan = xJSONReader._internal.xCoreReader.token.slice;

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
    /* LogInfo(( "\tnSpan: %.*s\n", az_span_size( nSpan ), az_span_ptr( nSpan ) )); */
    /* LogInfo(( "\teSpan: %.*s\n", az_span_size( eSpan ), az_span_ptr( eSpan ) )); */
    /* LogInfo(( "\talgSpan: %.*s\n", az_span_size( algSpan ), az_span_ptr( algSpan ) )); */

    /*------------------- Base64 decode the key ------------------------*/
    LogInfo( ( "--- Signing key base64 decoding N ---\n" ) );
    int32_t outSigningKeyNLength;
    lMbedResult = mbedtls_base64_decode( ( unsigned char * ) ucSigningKeyN, sizeof( ucSigningKeyN ), ( size_t * ) &outSigningKeyNLength, az_span_ptr( nSpan ), az_span_size( nSpan ) );
    /* LogInfo(( "\tlMbedResult Return: 0x%x\n", lMbedResult )); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outSigningKeyNLength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outSigningKeyNLength, ( char * ) ucSigningKeyN )); */

    LogInfo( ( "--- Signing key base64 decoding E ---\n" ) );
    int32_t outSigningKeyELength;
    lMbedResult = mbedtls_base64_decode( ( unsigned char * ) ucSigningKeyE, sizeof( ucSigningKeyE ), ( size_t * ) &outSigningKeyELength, az_span_ptr( eSpan ), az_span_size( eSpan ) );
    /* LogInfo(( "\tlMbedResult Return: 0x%x\n", lMbedResult )); */
    /* LogInfo(( "\tOut Decoded Size: %i\n", outSigningKeyELength )); */
    /* LogInfo(( "\t%.*s\n\n", ( int ) outSigningKeyELength, ( char * ) ucSigningKeyE )); */


    /*------------------- Verify the signature ------------------------*/
    ulVerificationResult = prvJWS_RS256Verify( pucJWKHeader, ulJWKHeaderLength + ulJWKPayloadLength + 1,
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
    lMbedResult = mbedtls_base64_decode( ( unsigned char * ) ucParsedManifestSha, sizeof( ucParsedManifestSha ), ( size_t * ) &outParsedManifestShaSize, az_span_ptr( sha256Span ), az_span_size( sha256Span ) );
    ( void ) lMbedResult;

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
