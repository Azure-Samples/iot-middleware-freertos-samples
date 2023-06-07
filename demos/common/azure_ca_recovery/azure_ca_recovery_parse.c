/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_ca_recovery_parse.h"

#include <stdio.h>

#include "azure/core/internal/az_result_internal.h"

#define RETURN_IF_JSON_TOKEN_NOT_TYPE( jr_ptr, json_token_type ) \
    if( jr_ptr->token.kind != json_token_type )                  \
    {                                                            \
        return AZ_ERROR_JSON_INVALID_STATE;                      \
    }

/* BEGIN EMBEDDED SDK CODE SECTION */

#define AZ_IOT_CA_RECOVERY_HUB_HOSTNAME_NAME         "iotHubHostName"
#define AZ_IOT_CA_RECOVERY_PAYLOAD_NAME              "payload"
#define AZ_IOT_CA_RECOVERY_SIGNATURE_NAME            "signature"
#define AZ_IOT_CA_RECOVERY_CERT_TRUST_BUNDLE_NAME    "certTrustBundle"
#define AZ_IOT_CA_RECOVERY_VERSION_NAME              "version"
#define AZ_IOT_CA_RECOVERY_EXPIRY_TIME_NAME          "expiryTime"
#define AZ_IOT_CA_RECOVERY_CERTS_NAME                "certs"

typedef struct azure_iot_ca_recovery_trust_bundle
{
    uint32_t version;

    uint64_t expiry_time;

    az_span certificates;
} azure_iot_ca_recovery_trust_bundle;

typedef struct azure_iot_ca_recovery_recovery_payload
{
    az_span payload_signature;

    az_span trust_bundle_json_object_text;

    azure_iot_ca_recovery_trust_bundle trust_bundle;
} azure_iot_ca_recovery_payload;

static az_result az_iot_ca_recovery_parse_trust_bundle( az_json_reader * ref_json_reader,
                                                        azure_iot_ca_recovery_trust_bundle * trust_bundle )
{
    _az_RETURN_IF_FAILED( az_json_reader_next_token( ref_json_reader ) );
    RETURN_IF_JSON_TOKEN_NOT_TYPE( ref_json_reader, AZ_JSON_TOKEN_BEGIN_OBJECT );
    _az_RETURN_IF_FAILED( az_json_reader_next_token( ref_json_reader ) );

    trust_bundle->certificates = AZ_SPAN_EMPTY;
    trust_bundle->expiry_time = 0;
    trust_bundle->version = 0;

    while( ref_json_reader->token.kind != AZ_JSON_TOKEN_END_OBJECT )
    {
        if( az_json_token_is_text_equal(
                &ref_json_reader->token,
                AZ_SPAN_FROM_STR( AZ_IOT_CA_RECOVERY_VERSION_NAME ) ) )
        {
            _az_RETURN_IF_FAILED( az_json_reader_next_token( ref_json_reader ) );
            RETURN_IF_JSON_TOKEN_NOT_TYPE( ref_json_reader, AZ_JSON_TOKEN_NUMBER );
            _az_RETURN_IF_FAILED( az_json_token_get_uint32( &ref_json_reader->token, &trust_bundle->version ) );
        }
        else if( az_json_token_is_text_equal(
                     &ref_json_reader->token,
                     AZ_SPAN_FROM_STR( AZ_IOT_CA_RECOVERY_EXPIRY_TIME_NAME ) ) )
        {
            _az_RETURN_IF_FAILED( az_json_reader_next_token( ref_json_reader ) );
            RETURN_IF_JSON_TOKEN_NOT_TYPE( ref_json_reader, AZ_JSON_TOKEN_NUMBER );

            _az_RETURN_IF_FAILED( az_json_token_get_uint64( &ref_json_reader->token, &trust_bundle->expiry_time ) );
        }
        else if( az_json_token_is_text_equal(
                     &ref_json_reader->token,
                     AZ_SPAN_FROM_STR( AZ_IOT_CA_RECOVERY_CERTS_NAME ) ) )
        {
            _az_RETURN_IF_FAILED( az_json_reader_next_token( ref_json_reader ) );
            RETURN_IF_JSON_TOKEN_NOT_TYPE( ref_json_reader, AZ_JSON_TOKEN_STRING );

            if( ref_json_reader->token.kind != AZ_JSON_TOKEN_NULL )
            {
                trust_bundle->certificates = ref_json_reader->token.slice;
            }
        }

        _az_RETURN_IF_FAILED( az_json_reader_next_token( ref_json_reader ) );
    }

    return AZ_OK;
}

static az_result az_iot_ca_recovery_parse_recovery_payload( az_json_reader * ref_json_reader,
                                                            azure_iot_ca_recovery_payload * recovery_payload )
{
    /* _az_PRECONDITION_NOT_NULL(ref_json_reader); */
    /* _az_PRECONDITION_NOT_NULL(recovery_payload); */

    _az_RETURN_IF_FAILED( az_json_reader_next_token( ref_json_reader ) );
    RETURN_IF_JSON_TOKEN_NOT_TYPE( ref_json_reader, AZ_JSON_TOKEN_BEGIN_OBJECT );
    _az_RETURN_IF_FAILED( az_json_reader_next_token( ref_json_reader ) );

    recovery_payload->payload_signature = AZ_SPAN_EMPTY;
    recovery_payload->trust_bundle_json_object_text = AZ_SPAN_EMPTY;

    while( ref_json_reader->token.kind != AZ_JSON_TOKEN_END_OBJECT )
    {
        RETURN_IF_JSON_TOKEN_NOT_TYPE( ref_json_reader, AZ_JSON_TOKEN_PROPERTY_NAME );

        if( az_json_token_is_text_equal(
                &ref_json_reader->token,
                AZ_SPAN_FROM_STR( AZ_IOT_CA_RECOVERY_SIGNATURE_NAME ) ) )
        {
            _az_RETURN_IF_FAILED( az_json_reader_next_token( ref_json_reader ) );
            RETURN_IF_JSON_TOKEN_NOT_TYPE( ref_json_reader, AZ_JSON_TOKEN_STRING );

            if( ref_json_reader->token.kind != AZ_JSON_TOKEN_NULL )
            {
                recovery_payload->payload_signature = ref_json_reader->token.slice;
            }
        }
        else if( az_json_token_is_text_equal(
                     &ref_json_reader->token,
                     AZ_SPAN_FROM_STR( AZ_IOT_CA_RECOVERY_CERT_TRUST_BUNDLE_NAME ) ) )
        {
            _az_RETURN_IF_FAILED( az_json_reader_next_token( ref_json_reader ) );
            RETURN_IF_JSON_TOKEN_NOT_TYPE( ref_json_reader, AZ_JSON_TOKEN_STRING );

            if( ref_json_reader->token.kind != AZ_JSON_TOKEN_NULL )
            {
                recovery_payload->trust_bundle_json_object_text = ref_json_reader->token.slice;
            }
        }

        _az_RETURN_IF_FAILED( az_json_reader_next_token( ref_json_reader ) );
    }

    return AZ_OK;
}

/* END EMBEDDED SDK CODE SECTION */

static void prvCastCoreToMiddleware( AzureIoTCARecovery_RecoveryPayload * pxRecoveryPayload,
                                     azure_iot_ca_recovery_payload * recovery_payload )
{
    pxRecoveryPayload->pucPayloadSignature = az_span_ptr( recovery_payload->payload_signature );
    pxRecoveryPayload->ulPayloadSignatureLength = az_span_size( recovery_payload->payload_signature );
    pxRecoveryPayload->pucTrustBundleJSONObjectText = az_span_ptr( recovery_payload->trust_bundle_json_object_text );
    pxRecoveryPayload->ulTrustBundleJSONObjectTextLength = az_span_size( recovery_payload->trust_bundle_json_object_text );

    pxRecoveryPayload->xTrustBundle.pucCertificates = az_span_ptr( recovery_payload->trust_bundle.certificates );
    pxRecoveryPayload->xTrustBundle.ulCertificatesLength = az_span_size( recovery_payload->trust_bundle.certificates );
    pxRecoveryPayload->xTrustBundle.ulVersion = recovery_payload->trust_bundle.version;
    pxRecoveryPayload->xTrustBundle.ullExpiryTimeSecs = recovery_payload->trust_bundle.expiry_time;
}

AzureIoTResult_t AzureIoTCARecovery_ParseRecoveryPayload( AzureIoTJSONReader_t * pxReader,
                                                          AzureIoTCARecovery_RecoveryPayload * pxRecoveryPayload )
{
    az_result xCoreResult;
    azure_iot_ca_recovery_payload recovery_payload;
    az_json_reader jr;

    xCoreResult = az_iot_ca_recovery_parse_recovery_payload( &pxReader->_internal.xCoreReader, &recovery_payload );

    if( az_result_failed( xCoreResult ) )
    {
        /* TODO: Uncomment once this is a real API (is a private func) */
        /* return AzureIoT_TranslateCoreError(xCoreResult); */

        printf( "az_iot_ca_recovery_parse_recovery_payload Error: 0x%08x\n", ( unsigned int ) xCoreResult );

        return eAzureIoTErrorFailed;
    }

    recovery_payload.trust_bundle_json_object_text = az_json_string_unescape( recovery_payload.trust_bundle_json_object_text, recovery_payload.trust_bundle_json_object_text );

    xCoreResult = az_json_reader_init( &jr, recovery_payload.trust_bundle_json_object_text, NULL );

    if( az_result_failed( xCoreResult ) )
    {
        /* TODO: Uncomment once this is a real API (is a private func) */
        /* return AzureIoT_TranslateCoreError(xCoreResult); */

        printf( "az_json_reader_initError: 0x%08x\n", ( unsigned int ) xCoreResult );

        return eAzureIoTErrorFailed;
    }

    xCoreResult = az_iot_ca_recovery_parse_trust_bundle( &jr, &recovery_payload.trust_bundle );

    if( az_result_failed( xCoreResult ) )
    {
        /* TODO: Uncomment once this is a real API (is a private func) */
        /* return AzureIoT_TranslateCoreError(xCoreResult); */

        printf( "az_iot_ca_recovery_parse_trust_bundle Error: 0x%08x\n", ( unsigned int ) xCoreResult );

        return eAzureIoTErrorFailed;
    }

    prvCastCoreToMiddleware( pxRecoveryPayload, &recovery_payload );

    return eAzureIoTSuccess;
}
