/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_sample_ca_recovery.h"

// BEGIN EMBEDDED SDK CODE SECTION

#define AZ_IOT_CA_RECOVERY_HUB_HOSTNAME_NAME "iotHubHostName"
#define AZ_IOT_CA_RECOVERY_PAYLOAD_NAME "payload"
#define AZ_IOT_CA_RECOVERY_SIGNATURE_NAME "signature"
#define AZ_IOT_CA_RECOVERY_CERT_TRUST_BUNDLE_NAME "certTrustBundle"
#define AZ_IOT_CA_RECOVERY_VERSION_NAME "version"
#define AZ_IOT_CA_RECOVERY_EXPIRY_TIME_NAME "expiryTime"
#define AZ_IOT_CA_RECOVERY_CERTS_NAME "certs"

typedef struct azure_iot_ca_recovery_trust_bundle {
  az_span version;

  az_span expiry_time;

  az_span certificates;
} azure_iot_ca_recovery_trust_bundle;

typedef struct azure_iot_ca_recovery_recovery_payload {
  az_span iothub_hostname;

  az_span payload_signature;

  az_span trust_bundle_json_object_text;

  azure_iot_ca_recovery_trust_bundle trust_bundle;
} azure_iot_ca_recovery_payload;

static az_result az_iot_ca_recovery_parse_recovery_payload(
    az_json_reader* ref_json_reader,
    azure_iot_ca_recovery_payload* recovery_payload)
{
  _az_PRECONDITION_NOT_NULL(ref_json_reader);
  _az_PRECONDITION_NOT_NULL(recovery_payload);

  RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_PROPERTY_NAME);
  RETURN_IF_JSON_TOKEN_NOT_TEXT(ref_json_reader, AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_SERVICE);

  _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
  RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_BEGIN_OBJECT);
  _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

  recovery_payload->iothub_hostname = AZ_SPAN_EMPTY;
  recovery_payload->payload_signature = AZ_SPAN_EMPTY;
  recovery_payload->trust_bundle_json_object_text = AZ_SPAN_EMPTY;
  recovery_payload->trust_bundle = { 0 };

  while (ref_json_reader->token.kind != AZ_JSON_TOKEN_END_OBJECT)
  {
    RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_PROPERTY_NAME);

    if (az_json_token_is_text_equal(
            &ref_json_reader->token,
            AZ_SPAN_FROM_STR(AZ_IOT_CA_RECOVERY_HUB_HOSTNAME_NAME)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_STRING);

      if (ref_json_reader->token.kind != AZ_JSON_TOKEN_NULL)
      {
        recovery_payload->iothub_hostname = ref_json_reader->token.slice;
      }
    }
    else if (az_json_token_is_text_equal(
                 &ref_json_reader->token,
                 AZ_SPAN_FROM_STR(AZ_IOT_CA_RECOVERY_PAYLOAD_NAME)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

      while (ref_json_reader->token.kind != AZ_JSON_TOKEN_END_OBJECT)
      {
        if (az_json_token_is_text_equal(
                    &ref_json_reader->token,
                    AZ_SPAN_FROM_STR(AZ_IOT_CA_RECOVERY_SIGNATURE_NAME)))
        {
          _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
          RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_STRING);

          if (ref_json_reader->token.kind != AZ_JSON_TOKEN_NULL)
          {
            recovery_payload->payload_signature = ref_json_reader->token.slice;
          }
        }
        else if (az_json_token_is_text_equal(
                    &ref_json_reader->token,
                    AZ_SPAN_FROM_STR(AZ_IOT_CA_RECOVERY_CERT_TRUST_BUNDLE_NAME)))
        {
          _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

          while (ref_json_reader->token.kind != AZ_JSON_TOKEN_END_OBJECT)
          {
            if (az_json_token_is_text_equal(
                    &ref_json_reader->token,
                    AZ_SPAN_FROM_STR(AZ_IOT_CA_RECOVERY_VERSION_NAME)))
            {
              _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
              RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_STRING);

              if (ref_json_reader->token.kind != AZ_JSON_TOKEN_NULL)
              {
                recovery_payload->trust_bundle.version = ref_json_reader->token.slice;
              }
            }
            else if (az_json_token_is_text_equal(
                    &ref_json_reader->token,
                    AZ_SPAN_FROM_STR(AZ_IOT_CA_RECOVERY_EXPIRY_TIME_NAME)))
            {
              _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
              RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_STRING);

              if (ref_json_reader->token.kind != AZ_JSON_TOKEN_NULL)
              {
                recovery_payload->trust_bundle.expiry_time = ref_json_reader->token.slice;
              }
            }
            else if (az_json_token_is_text_equal(
                    &ref_json_reader->token,
                    AZ_SPAN_FROM_STR(AZ_IOT_CA_RECOVERY_CERTS_NAME)))
            {
              _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
              RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_STRING);

              if (ref_json_reader->token.kind != AZ_JSON_TOKEN_NULL)
              {
                recovery_payload->trust_bundle.certificates = ref_json_reader->token.slice;
              }
            }
          }
        }
      }
    }

    _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
  }

  return AZ_OK;
}

// END EMBEDDED SDK CODE SECTION

AzureIoTResult_t AzureIoTCARecovery_ParseRecoveryPayload(AzureIoTJSONReader_t * pxReader,
                                                AzureIoTCARecovery_RecoveryPayload * pxRecoveryPayload)
{
  
}
