/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

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

#define azureiotRSA3072_SIZE    384
#define azureiotSHA256_SIZE     32

char ucBase64DecodedHeader[ 1400 ];
char ucBase64DecodedPayload[ 60 ];
char ucBase64DecodedSignature[ 400 ];

char ucBase64DecodedJWKHeader[ 48 ];
char ucBase64DecodedJWKPayload[ 700 ];
char ucBase64DecodedJWKSignature[ 500 ];

char ucBase64DecodedSigningKeyN[ azureiotRSA3072_SIZE ];
char ucBase64DecodedSigningKeyE[ 16 ];

char ucEscapedManifestSHACalculation[ azureiotSHA256_SIZE ];
char parsedSha[ azureiotSHA256_SIZE ];

char ucCalculatationBuffer[ azureiotRSA3072_SIZE + azureiotSHA256_SIZE ];

static uint32_t prvSplitJWS( char * pucJWS,
                             uint32_t ulJWSLength,
                             char ** ppucHeader,
                             uint32_t * pulHeaderLength,
                             char ** ppucPayload,
                             uint32_t * pulPayloadLength,
                             char ** ppucSignature,
                             uint32_t * pulSignatureLength )
{
    char * pucFirstDot;
    char * pucSecondDot;
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

static void prvSwapToUrlEncodingChars( char * pucSignature,
                                       uint32_t ulSignatureLength )
{
    uint32_t ulIndex = 0;

    char * hold = pucSignature;

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

uint32_t AzureIoT_SHA256Calculate( const char * input,
                                   uint32_t inputLength,
                                   char * output )
{
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init( &ctx );
    mbedtls_md_setup( &ctx, mbedtls_md_info_from_type( md_type ), 0 );
    mbedtls_md_starts( &ctx );
    mbedtls_md_update( &ctx, ( const unsigned char * ) input, inputLength );
    mbedtls_md_finish( &ctx, output );
    mbedtls_md_free( &ctx );

    return 0;
}

uint32_t AzureIoT_RS256Verify( char * input,
                               uint32_t inputLength,
                               char * signature,
                               uint32_t signatureLength,
                               unsigned char * n,
                               uint32_t nLength,
                               unsigned char * e,
                               uint32_t eLength,
                               char * buffer,
                               uint32_t bufferLength )
{
    AzureIoTResult_t xResult;
    int mbedTLSResult;

    if( bufferLength < azureiotRSA3072_SIZE + azureiotSHA256_SIZE )
    {
        printf( "Buffer Not Large Enough\n" );
        return 1;
    }

    char * shaBuffer = buffer + azureiotRSA3072_SIZE;
    char * metadata;
    uint32_t metadataLength;

    char * decryptedPtr = buffer;
    size_t decryptedLength;

    printf( "RS256 Verify Input:\n" );
    printf( "%.*s", inputLength, input );
    printf( "\n" );

    /* The signature is encrypted using the input key. We need to decrypt the */
    /* signature which gives us the SHA256 inside a PKCS7 structure. We then compare
    /* that to the SHA256 of the input. */
    mbedtls_rsa_context ctx;

    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, 0 );

    printf( "---- Initializing Decryption ----\n" );

    mbedTLSResult = mbedtls_rsa_import_raw( &ctx,
                                            n, nLength,
                                            NULL, 0,
                                            NULL, 0,
                                            NULL, 0,
                                            e, eLength );
    printf( "\tN Length: %i | E Length: %i\n", nLength, eLength );

    if( mbedTLSResult != 0 )
    {
        printf( "mbedtls res: %i\n", mbedTLSResult );
    }

    mbedTLSResult = mbedtls_rsa_complete( &ctx );

    if( mbedTLSResult != 0 )
    {
        printf( "mbedtls res: %i\n", mbedTLSResult );
    }

    mbedTLSResult = mbedtls_rsa_check_pubkey( &ctx );

    if( mbedTLSResult != 0 )
    {
        printf( "mbedtls res: %i\n", mbedTLSResult );
    }

    printf( "---- Decrypting ----\n" );

    /* RSA */
    mbedTLSResult = mbedtls_rsa_pkcs1_decrypt( &ctx, NULL, NULL, MBEDTLS_RSA_PUBLIC, &decryptedLength, signature, buffer, azureiotRSA3072_SIZE );

    if( mbedTLSResult != 0 )
    {
        printf( "mbedtls res: %i | %x\n", mbedTLSResult, -mbedTLSResult );
    }

    printf( "\tDecrypted text length: %li\n", decryptedLength );

    printf( "\tDecrypted text:\n" );
    int i = 0;

    while( i < decryptedLength )
    {
        printf( "0x%.2x ", ( unsigned char ) *( buffer + i ) );
        i++;
    }

    printf( "\n" );

    printf( "---- Calculating SHA256 over input ----\n" );
    xResult = AzureIoT_SHA256Calculate( input, inputLength,
                                        shaBuffer );

    printf( "\tCalculated: " );

    i = 0;

    while( i < azureiotSHA256_SIZE )
    {
        printf( "0x%.2x ", ( unsigned char ) *( shaBuffer + i ) );
        i++;
    }

    printf( "\n" );

    printf( "--- Checking for if SHA256 of header+payload matches decrypted SHA256 ---\n" );

    int doTheyMatch = memcmp( buffer + 19, shaBuffer, azureiotSHA256_SIZE );

    if( doTheyMatch == 0 )
    {
        printf( "\tSHA of JWK matches\n" );
        xResult = 0;
    }
    else
    {
        printf( "\tThey don't match\n" );
        xResult = 1;
    }

    return xResult;
}



uint32_t JWS_Verify( const char * pucEscapedManifest,
                     uint32_t ulEscapedManifestLength,
                     char * pucJWS,
                     uint32_t ulJWSLength )
{
    uint32_t ulVerificationResult;

    int mbedtResult;
    char * pucHeader;
    char * pucPayload;
    char * pucSignature;
    uint32_t ulHeaderLength;
    uint32_t ulPayloadLength;
    uint32_t ulSignatureLength;
    AzureIoTJSONReader_t xJSONReader;

    printf( "---------------------Begin Signature Validation --------------------\n\n" );

    /*------------------- Parse and Decode the Manifest Sig ------------------------*/

    AzureIoTResult_t xResult = prvSplitJWS( pucJWS, ulJWSLength,
                                            &pucHeader, &ulHeaderLength,
                                            &pucPayload, &ulPayloadLength,
                                            &pucSignature, &ulSignatureLength );
    prvSwapToUrlEncodingChars( pucSignature, ulSignatureLength );

    printf( "---JWS Base64 Decode Header---\n" );
    int32_t outJWSDecodedHeaderLength;
    mbedtResult = mbedtls_base64_decode( ucBase64DecodedHeader, sizeof( ucBase64DecodedHeader ), ( size_t * ) &outJWSDecodedHeaderLength, pucHeader, ulHeaderLength );
    printf( "\tmbedTLS Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outJWSDecodedHeaderLength );
    printf( "\t%.*s\n\n", ( int ) outJWSDecodedHeaderLength, ucBase64DecodedHeader );

    printf( "---JWS Base64 Decode Payload---\n" );
    int32_t outJWSDecodedLength;
    mbedtResult = mbedtls_base64_decode( ucBase64DecodedPayload, sizeof( ucBase64DecodedPayload ), ( size_t * ) &outJWSDecodedLength, pucPayload, ulPayloadLength );
    printf( "\tmbedTLS Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outJWSDecodedLength );
    printf( "\t%.*s\n\n", ( int ) outJWSDecodedLength, ucBase64DecodedPayload );

    printf( "---JWS Base64 Decode Signature---\n" );
    int32_t outJWSDecodedSignatureLength;
    mbedtResult = mbedtls_base64_decode( ucBase64DecodedSignature, sizeof( ucBase64DecodedSignature ), ( size_t * ) &outJWSDecodedSignatureLength, pucSignature, ulSignatureLength );
    printf( "\tmbedTLS Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outJWSDecodedSignatureLength );
    printf( "\t%.*s\n\n", ( int ) outJWSDecodedSignatureLength, ucBase64DecodedSignature );


    /*------------------- Parse JSK JSON Payload ------------------------*/

    /* The "sjwk" is the signed signing public key */
    printf( "---Parsing JWS JSON Payload---\n" );

    /*TODO: REMOVE THIS HACK */
    ucBase64DecodedHeader[ outJWSDecodedHeaderLength ] = '"';
    ucBase64DecodedHeader[ outJWSDecodedHeaderLength + 1 ] = '}';
    outJWSDecodedHeaderLength = outJWSDecodedHeaderLength + 2;
    AzureIoTJSONReader_Init( &xJSONReader, ucBase64DecodedHeader, outJWSDecodedHeaderLength );
    xResult = AzureIoTJSONReader_NextToken( &xJSONReader );

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "sjwk", strlen( "sjwk" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            printf( "Coreresult: %i\n", xResult );
            break;
        }
        else
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            xResult = AzureIoTJSONReader_SkipChildren( &xJSONReader );
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
    }

    az_span xJWKManifestSpan = xJSONReader._internal.xCoreReader.token.slice;

    char * pucJWKManifest = az_span_ptr( xJWKManifestSpan );
    uint32_t ulJWKManifestLength = az_span_size( xJWKManifestSpan );
    printf( "JWKManifest Length: %i\n", ulJWKManifestLength );

    /*------------------- Base64 Decode the JWK Payload ------------------------*/

    char * pucJWKHeader;
    char * pucJWKPayload;
    char * pucJWKSignature;
    uint32_t ulJWKHeaderLength;
    uint32_t ulJWKPayloadLength;
    uint32_t ulJWKSignatureLength;

    printf( "--- Base64 Decoding JWS Payload ---\n" );

    xResult = prvSplitJWS( pucJWKManifest, ulJWKManifestLength,
                           &pucJWKHeader, &ulJWKHeaderLength,
                           &pucJWKPayload, &ulJWKPayloadLength,
                           &pucJWKSignature, &ulJWKSignatureLength );
    prvSwapToUrlEncodingChars( pucJWKSignature, ulJWKSignatureLength );

    printf( "--- JWK Base64 Decode Header ---\n" );
    int32_t outDecodedJWKSizeOne;
    mbedtls_base64_decode( ucBase64DecodedJWKHeader, sizeof( ucBase64DecodedJWKHeader ), ( size_t * ) &outDecodedJWKSizeOne, pucJWKHeader, ulJWKHeaderLength );
    printf( "\tCore Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outDecodedJWKSizeOne );
    printf( "\t%.*s\n\n", ( int ) outDecodedJWKSizeOne, ucBase64DecodedJWKHeader );

    printf( "--- JWK Base64 Decode Payload ---\n" );
    int32_t outDecodedJWKSizeTwo;
    mbedtResult = mbedtls_base64_decode( ucBase64DecodedJWKPayload,
                                         sizeof( ucBase64DecodedJWKPayload ),
                                         ( size_t * ) &outDecodedJWKSizeTwo,
                                         pucJWKPayload,
                                         ulJWKPayloadLength );
    printf( "\tCore Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outDecodedJWKSizeTwo );
    printf( "\t%.*s\n\n", ( int ) outDecodedJWKSizeTwo, ucBase64DecodedJWKPayload );

    printf( "--- JWK Base64 Decode Signature ---\n" );
    int32_t outDecodedJWKSizeThree;
    mbedtResult = mbedtls_base64_decode( ucBase64DecodedJWKSignature, sizeof( ucBase64DecodedJWKSignature ), ( size_t * ) &outDecodedJWKSizeThree, pucJWKSignature, ulJWKSignatureLength );
    printf( "\tCore Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outDecodedJWKSizeThree );
    printf( "\t%.*s\n\n", ( int ) outDecodedJWKSizeThree, ucBase64DecodedJWKSignature );

    /*------------------- Parse id for root key ------------------------*/

    printf( "--- Checking Root Key ---\n" );
    az_span kidSpan;
    AzureIoTJSONReader_Init( &xJSONReader, ucBase64DecodedJWKHeader, outDecodedJWKSizeOne );
    xResult = AzureIoTJSONReader_NextToken( &xJSONReader );

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "kid", strlen( "kid" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            kidSpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
        else
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            xResult = AzureIoTJSONReader_SkipChildren( &xJSONReader );
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
    }

    az_span rootKeyIDSpan = az_span_create( ( uint8_t * ) AzureIoTADURootKeyId, sizeof( AzureIoTADURootKeyId ) - 1 );

    if( az_span_is_content_equal( rootKeyIDSpan, kidSpan ) )
    {
        printf( "\tUsing the correct root key\n" );
    }
    else
    {
        printf( "\tUsing the wrong root key\n" );

        while( 1 )
        {
        }
    }

    /*------------------- Parse necessary pieces for the verification ------------------------*/

    az_span nSpan;
    az_span eSpan;
    az_span algSpan;
    printf( "--- Parse Signing Key Payload ---\n" );

    AzureIoTJSONReader_Init( &xJSONReader, ucBase64DecodedJWKPayload, outDecodedJWKSizeTwo );
    xResult = AzureIoTJSONReader_NextToken( &xJSONReader );

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "n", strlen( "n" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            nSpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
        else if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "e", strlen( "e" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            eSpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
        else if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "alg", strlen( "alg" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            algSpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
        else
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            xResult = AzureIoTJSONReader_SkipChildren( &xJSONReader );
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
    }

    printf( "--- Print Signing Key Parts ---\n" );
    printf( "\tnSpan: %.*s\n", az_span_size( nSpan ), az_span_ptr( nSpan ) );
    printf( "\teSpan: %.*s\n", az_span_size( eSpan ), az_span_ptr( eSpan ) );
    printf( "\talgSpan: %.*s\n", az_span_size( algSpan ), az_span_ptr( algSpan ) );

    /*------------------- Base64 decode the key ------------------------*/
    printf( "--- Signing key base64 decoding N ---\n" );
    int32_t outDecodedSigningKeyN;
    mbedtResult = mbedtls_base64_decode( ucBase64DecodedSigningKeyN, sizeof( ucBase64DecodedSigningKeyN ), ( size_t * ) &outDecodedSigningKeyN, az_span_ptr( nSpan ), az_span_size( nSpan ) );
    printf( "\tmbedtResult Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outDecodedSigningKeyN );
    printf( "\t%.*s\n\n", ( int ) outDecodedSigningKeyN, ucBase64DecodedSigningKeyN );

    printf( "--- Signing key base64 decoding E ---\n" );
    int32_t outDecodedSigningKeyE;
    mbedtResult = mbedtls_base64_decode( ucBase64DecodedSigningKeyE, sizeof( ucBase64DecodedSigningKeyE ), ( size_t * ) &outDecodedSigningKeyE, az_span_ptr( eSpan ), az_span_size( eSpan ) );
    printf( "\tmbedtResult Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outDecodedSigningKeyE );
    printf( "\t%.*s\n\n", ( int ) outDecodedSigningKeyE, ucBase64DecodedSigningKeyE );


    /*------------------- Verify the signature ------------------------*/
    ulVerificationResult = AzureIoT_RS256Verify( pucJWKHeader, ulJWKHeaderLength + ulJWKPayloadLength + 1,
                                                 ucBase64DecodedJWKSignature, outDecodedJWKSizeThree,
                                                 ( unsigned char * ) AzureIoTADURootKeyN, sizeof( AzureIoTADURootKeyN ),
                                                 ( unsigned char * ) AzureIoTADURootKeyE, sizeof( AzureIoTADURootKeyE ),
                                                 ucCalculatationBuffer, sizeof( ucCalculatationBuffer ) );

    if( ulVerificationResult != 0 )
    {
        printf( "Verification of signing key failed\n" );
        return ulVerificationResult;
    }

    /*------------------- Verify that the signature was signed by signing key ------------------------*/
    ulVerificationResult = AzureIoT_RS256Verify( pucHeader, ulHeaderLength + ulPayloadLength + 1,
                                                 ucBase64DecodedSignature, outJWSDecodedSignatureLength,
                                                 ucBase64DecodedSigningKeyN, outDecodedSigningKeyN,
                                                 ucBase64DecodedSigningKeyE, outDecodedSigningKeyE,
                                                 ucCalculatationBuffer, sizeof( ucCalculatationBuffer ) );

    if( ulVerificationResult != 0 )
    {
        printf( "Verification of signed manifest SHA failed\n" );
        return ulVerificationResult;
    }

    /*------------------- Verify that the SHAs match ------------------------*/
    /* decodedSpanHeader */
    ulVerificationResult = AzureIoT_SHA256Calculate( pucEscapedManifest,
                                                     ulEscapedManifestLength,
                                                     ucEscapedManifestSHACalculation );

    xResult = AzureIoTJSONReader_Init( &xJSONReader, ucBase64DecodedPayload, outJWSDecodedLength );
    /*Begin object */
    xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
    /*Property Name */
    xResult = AzureIoTJSONReader_NextToken( &xJSONReader );

    az_span sha256Span;

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "sha256", strlen( "sha256" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            sha256Span = xJSONReader._internal.xCoreReader.token.slice;
            break;
        }
        else
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            xResult = AzureIoTJSONReader_SkipChildren( &xJSONReader );
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
    }

    printf( "Parsed SHA: %.*s\n", az_span_size( sha256Span ), az_span_ptr( sha256Span ) );

    int32_t outParseSha;
    mbedtResult = mbedtls_base64_decode( parsedSha, sizeof( parsedSha ), ( size_t * ) &outParseSha, az_span_ptr( sha256Span ), az_span_size( sha256Span ) );

    ulVerificationResult = memcmp( ucEscapedManifestSHACalculation, parsedSha, azureiotSHA256_SIZE );

    if( ulVerificationResult != 0 )
    {
        printf( "Calculated manifest SHA does not match SHA in payload\n" );
        return ulVerificationResult;
    }
    else
    {
        printf( "Calculated manifest SHA matches parsed SHA\n" );
    }

    /*------------------- Done (Loop) ------------------------*/
    return ulVerificationResult;
}
