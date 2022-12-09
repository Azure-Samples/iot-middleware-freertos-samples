/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdint.h>
#include <stdio.h>

#include "azure_sample_ca_recovery.h"

#define TEST_HUB_HOSTNAME               "ericwol-cert.azure-devices.net"
#define TEST_SIGNATURE                  "LpJ1ROhCUvn2qaKeDODzD2hbcgJdJHA1uygyxC5ywUBtOzKB3e+kT0B+Z/VjENbPMrBZJSncHBfuW+95yVoHpBib2kH3YOD3ZvsdTtYpIPG2HbBPtrxlzaBlek54gD9uZw+Fp3nImdUfMs/L8Hx8NDNr0HLKZVLMqnk/Vh91U1skUYLcWGQDzPpWHlKx2JakNk3BEBBR5pGCu8IVMmfo3le1ztZUcSfiOrgXndmCgngVwmWj0EiLc6MvtUVCjVJvpsOR59mXkxn3tN3ijjWEeDkOBGg4oofWXeD1vSolHYNMQ7aLHJFJ5yDU2t+bQqi87qcSAJ7jtJz7COPXIxS3MQ=="
#define TEST_BUNDLE_JSON_OBJECT_TEXT    "{\"version\":\"1.0\",\"expiryTime\":\"12/9/2022 3:51:18 PM\",\"certs\":\"-----BEGIN CERTIFICATE-----\\r\\nMIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\\r\\nRTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\\r\\nVQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\\r\\nDTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\\r\\nZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\\r\\nVHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\\r\\nmD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\\r\\nIZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\\r\\nmpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\\r\\nXmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\\r\\ndc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\\r\\njl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\\r\\nBE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\\r\\nDQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\\r\\n9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\\r\\njkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\\r\\nEpn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\\r\\nksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\\r\\nR9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\\r\\n-----END CERTIFICATE-----\\r\\n\"}"
#define TEST_BUNDLE_VERSION             "1.0"
#define TEST_BUNDLE_EXPIRY              "12/9/2022 3:51:18 PM"
#define TEST_BUNDLE_CERT                "-----BEGIN CERTIFICATE-----\\r\\nMIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\\r\\nRTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\\r\\nVQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\\r\\nDTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\\r\\nZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\\r\\nVHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\\r\\nmD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\\r\\nIZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\\r\\nmpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\\r\\nXmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\\r\\ndc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\\r\\njl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\\r\\nBE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\\r\\nDQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\\r\\n9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\\r\\njkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\\r\\nEpn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\\r\\nksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\\r\\nR9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\\r\\n-----END CERTIFICATE-----\\r\\n"

static char * ucTestPayload = "{\"iotHubHostName\":\"ericwol-cert.azure-devices.net\",\"payload\":{\"signature\":\"LpJ1ROhCUvn2qaKeDODzD2hbcgJdJHA1uygyxC5ywUBtOzKB3e+kT0B+Z/VjENbPMrBZJSncHBfuW+95yVoHpBib2kH3YOD3ZvsdTtYpIPG2HbBPtrxlzaBlek54gD9uZw+Fp3nImdUfMs/L8Hx8NDNr0HLKZVLMqnk/Vh91U1skUYLcWGQDzPpWHlKx2JakNk3BEBBR5pGCu8IVMmfo3le1ztZUcSfiOrgXndmCgngVwmWj0EiLc6MvtUVCjVJvpsOR59mXkxn3tN3ijjWEeDkOBGg4oofWXeD1vSolHYNMQ7aLHJFJ5yDU2t+bQqi87qcSAJ7jtJz7COPXIxS3MQ==\",\"certTrustBundle\":\"{\\\"version\\\":\\\"1.0\\\",\\\"expiryTime\\\":\\\"12/9/2022 3:51:18 PM\\\",\\\"certs\\\":\\\"-----BEGIN CERTIFICATE-----\\\\r\\\\nMIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\\\\r\\\\nRTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\\\\r\\\\nVQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\\\\r\\\\nDTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\\\\r\\\\nZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\\\\r\\\\nVHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\\\\r\\\\nmD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\\\\r\\\\nIZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\\\\r\\\\nmpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\\\\r\\\\nXmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\\\\r\\\\ndc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\\\\r\\\\njl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\\\\r\\\\nBE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\\\\r\\\\nDQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\\\\r\\\\n9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\\\\r\\\\njkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\\\\r\\\\nEpn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\\\\r\\\\nksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\\\\r\\\\nR9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\\\\r\\\\n-----END CERTIFICATE-----\\\\r\\\\n\\\"}\"}}";

static char ucPayloadBuffer[ 2048 ];

void prvCopyBuffer()
{
    memcpy( ucPayloadBuffer, ucTestPayload, strlen( ucTestPayload ) );
}

int vStartTestTask( void )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONReader_t xReader;
    AzureIoTCARecovery_RecoveryPayload xPayload;

    prvCopyBuffer();

    AzureIoTJSONReader_Init( &xReader, ucPayloadBuffer, strlen( ucTestPayload ) );

    printf( "Parsing Recovery Payload\n" );
    xResult = AzureIoTCARecovery_ParseRecoveryPayload( &xReader, &xPayload );

    if( xResult != eAzureIoTSuccess )
    {
        printf( "\tFailed!\n" );
        return 1;
    }

    printf( "Checking all struct values\n" );

    if( strncmp( xPayload.pucIoTHubHostname, TEST_HUB_HOSTNAME, sizeof( TEST_HUB_HOSTNAME ) - 1 ) != 0 )
    {
        printf( "\tHub Name Failed!\n" );
        return 1;
    }
    else if( strncmp( xPayload.pucPayloadSignature, TEST_SIGNATURE, sizeof( TEST_SIGNATURE ) - 1 ) != 0 )
    {
        printf( "\tSignature Failed!\n" );
        return 1;
    }
    else if( strncmp( xPayload.pucTrustBundleJSONObjectText, TEST_BUNDLE_JSON_OBJECT_TEXT, sizeof( TEST_BUNDLE_JSON_OBJECT_TEXT ) - 1 ) != 0 )
    {
        printf( "\tTrust Bundle Object Text Failed!\n" );
        printf( "\tReceived: %.*s", xPayload.ulTrustBundleJSONObjectTextLength, xPayload.pucTrustBundleJSONObjectText );
        return 1;
    }
    else if( strncmp( xPayload.xTrustBundle.pucVersion, TEST_BUNDLE_VERSION, sizeof( TEST_BUNDLE_VERSION ) - 1 ) != 0 )
    {
        printf( "\tBundle Version Failed!\n" );
        return 1;
    }
    else if( strncmp( xPayload.xTrustBundle.pucExpiryTime, TEST_BUNDLE_EXPIRY, sizeof( TEST_BUNDLE_EXPIRY ) - 1 ) != 0 )
    {
        printf( "\tBundle Expiry Failed!\n" );
        return 1;
    }
    else if( strncmp( xPayload.xTrustBundle.pucCertificates, TEST_BUNDLE_CERT, sizeof( TEST_BUNDLE_CERT ) - 1 ) != 0 )
    {
        printf( "\tBundle Certs Failed!\n" );
        return 1;
    }

    return 1;
}
