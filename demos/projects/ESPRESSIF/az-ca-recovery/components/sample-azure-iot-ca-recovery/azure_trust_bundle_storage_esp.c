/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <string.h>

#include "azure_trust_bundle_storage.h"

#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#define CA_CERT_NAMESPACE                  "root-ca-cert"
#define AZURE_TRUST_BUNDLE_NAME            "az-tb"
#define AZURE_TRUST_BUNDLE_VERSION_NAME    "az-tb-ver"

AzureIoTResult_t AzureIoTCAStorage_ReadTrustBundle( const uint8_t * pucTrustBundle,
                                                    uint32_t ulTrustBundleLength,
                                                    uint32_t * pulOutTrustBundleLength,
                                                    const uint8_t * pucTrustBundleVersion,
                                                    uint32_t ulTrustBundleVersionLength,
                                                    uint32_t * pulOutTrustBundleVersionLength )
{
    nvs_handle_t xNVSHandle;
    esp_err_t err;
    size_t ulTrustBundleReadSize = 0; /* value will default to 0, if not set yet in NVS */
    size_t ulTrustBundleVersionReadSize = 0;

    /* Open CA Cert namespace */
    err = nvs_open( CA_CERT_NAMESPACE, NVS_READWRITE, &xNVSHandle );

    if( err != ESP_OK )
    {
        printf( "Error (%s) opening NVS!\n", esp_err_to_name( err ) );
        return err;
    }

    err = nvs_get_blob( xNVSHandle, AZURE_TRUST_BUNDLE_VERSION_NAME, NULL, &ulTrustBundleVersionReadSize );

    if( err != ESP_OK )
    {
        printf( "Error (%s) getting AZURE_TRUST_BUNDLE_VERSION_NAME from NVS!\n", esp_err_to_name( err ) );
        nvs_close( xNVSHandle );
        return err;
    }

    if( ulTrustBundleVersionReadSize > ulTrustBundleVersionLength )
    {
        printf( "Not enough size to read version\n" );
        nvs_close( xNVSHandle );
        return eAzureIoTErrorOutOfMemory;
    }

    *pulOutTrustBundleVersionLength = ulTrustBundleVersionReadSize;

    /* Read the current trust bundle version */
    err = nvs_get_blob( xNVSHandle, AZURE_TRUST_BUNDLE_VERSION_NAME, &pucTrustBundleVersion, pulOutTrustBundleVersionLength );

    if( err != ESP_OK )
    {
        nvs_close( xNVSHandle );
        return err;
    }

    /* Get size of cert */
    err = nvs_get_blob( xNVSHandle, AZURE_TRUST_BUNDLE_NAME, NULL, &ulTrustBundleReadSize );

    if( ( err != ESP_OK ) && ( err != ESP_ERR_NVS_NOT_FOUND ) )
    {
        nvs_close( xNVSHandle );
        return err;
    }

    if( ulTrustBundleReadSize == 0 )
    {
        printf( "Nothing saved yet!\n" );
    }
    else if( ulTrustBundleReadSize > ulTrustBundleLength )
    {
        printf( "Not enough space to read trust bundle" );
    }
    else
    {
        *pulOutTrustBundleLength = ulTrustBundleReadSize;
        err = nvs_get_blob( xNVSHandle, AZURE_TRUST_BUNDLE_NAME, ( void * ) pucTrustBundle, pulOutTrustBundleLength );

        if( err != ESP_OK )
        {
            nvs_close( xNVSHandle );
            return err;
        }
    }

    return err;
}

AzureIoTResult_t AzureIoTCAStorage_WriteTrustBundle( const uint8_t * pucTrustBundle,
                                                     uint32_t ulTrustBundleLength,
                                                     const uint8_t * pucTrustBundleVersion,
                                                     uint32_t ulTrustBundleVersionLength )
{
    nvs_handle_t xNVSHandle;
    esp_err_t err;
    size_t ulTrustBundleVersionReadSize = 0;

    /* Open CA Cert namespace */
    err = nvs_open( CA_CERT_NAMESPACE, NVS_READWRITE, &xNVSHandle );

    if( err != ESP_OK )
    {
        printf( "Error (%s) opening NVS!\n", esp_err_to_name( err ) );
        return err;
    }

    uint8_t ucReadTrustBundleVersion[ 8 ] = { 0 }; /* value will default to 0, if not set yet in NVS */

    err = nvs_get_blob( xNVSHandle, AZURE_TRUST_BUNDLE_VERSION_NAME, NULL, &ulTrustBundleVersionReadSize );

    if( err != ESP_OK )
    {
        printf( "Error (%s) getting AZURE_TRUST_BUNDLE_VERSION_NAME from NVS!\n", esp_err_to_name( err ) );
        nvs_close( xNVSHandle );
        return err;
    }

    if( ulTrustBundleVersionReadSize > 8 )
    {
        printf( "Not enough size to read version\n" );
        nvs_close( xNVSHandle );
        return eAzureIoTErrorOutOfMemory;
    }

    /* Read the current trust bundle version */
    err = nvs_get_blob( xNVSHandle, AZURE_TRUST_BUNDLE_VERSION_NAME, &ucReadTrustBundleVersion, &ulTrustBundleVersionReadSize );

    if( err != ESP_OK )
    {
        printf( "Error (%s) getting AZURE_TRUST_BUNDLE_VERSION_NAME from NVS!\n", esp_err_to_name( err ) );
        nvs_close( xNVSHandle );
        return err;
    }

    /* If version matches, do not write to not overuse NVS */
    if( memcmp( ucReadTrustBundleVersion, pucTrustBundleVersion, ulTrustBundleVersionReadSize ) == 0 )
    {
        printf( "Trust bundle version in NVS matches bundle version to write.\n" );
        nvs_close( xNVSHandle );
        return ESP_OK;
    }

    /* Write value */
    printf( "Writing trust bundle:\r\n%.*s\n", ulTrustBundleLength, pucTrustBundle );
    err = nvs_set_blob( xNVSHandle, AZURE_TRUST_BUNDLE_NAME, pucTrustBundle, ulTrustBundleLength );

    if( err != ESP_OK )
    {
        printf( "Error (%s) getting AZURE_TRUST_BUNDLE_VERSION_NAME from NVS!\n", esp_err_to_name( err ) );
        nvs_close( xNVSHandle );
        return err;
    }

    /* Set new trust bundle version */
    printf( "Writing trust bundle version: %.*s\n", ulTrustBundleVersionLength, pucTrustBundleVersion );
    err = nvs_set_blob( xNVSHandle, AZURE_TRUST_BUNDLE_VERSION_NAME, pucTrustBundleVersion, ulTrustBundleVersionLength );

    if( err != ESP_OK )
    {
        printf( "Error (%s) getting AZURE_TRUST_BUNDLE_VERSION_NAME from NVS!\n", esp_err_to_name( err ) );
        nvs_close( xNVSHandle );
        return err;
    }

    /* Commit */
    err = nvs_commit( xNVSHandle );

    if( err != ESP_OK )
    {
        printf( "Error (%s) getting AZURE_TRUST_BUNDLE_VERSION_NAME from NVS!\n", esp_err_to_name( err ) );
        nvs_close( xNVSHandle );
        return err;
    }

    nvs_close( xNVSHandle );

    return err;
}
