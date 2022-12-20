/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_ca_storage.h"

#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#define CA_CERT_NAMESPACE                  "root-ca-cert"
#define AZURE_TRUST_BUNDLE_NAME            "az-tb"
#define AZURE_TRUST_BUNDLE_VERSION_NAME    "az-tb-ver"

AzureIoTResult_t AzureIoTCAStorage_ReadTrustBundle( const uint8_t * pucTrustBundle,
                                                    uint32_t ulTrustBundleLength,
                                                    uint32_t * pulOutTrustBundleLength,
                                                    uint32_t * ulTrustBundleVersion )
{
    nvs_handle_t xNVSHandle;
    esp_err_t err;

    /* Open CA Cert namespace */
    err = nvs_open( CA_CERT_NAMESPACE, NVS_READWRITE, &xNVSHandle );

    /* Read AZURE_TRUST_BUNDLE_VERSION_NAME */
    int32_t ulTrustBundleVersion = 0; /* value will default to 0, if not set yet in NVS */

    err = nvs_get_i32( xNVSHandle, AZURE_TRUST_BUNDLE_VERSION_NAME, &ulTrustBundleVersion );

    if( ( err != ESP_OK ) && ( err != ESP_ERR_NVS_NOT_FOUND ) )
    {
        return err;
    }

    /* Get size of cert */
    size_t ulTrustBundleReadSize = 0; /* value will default to 0, if not set yet in NVS */
    err = nvs_get_blob( xNVSHandle, AZURE_TRUST_BUNDLE_NAME, NULL, &ulTrustBundleReadSize );

    if( ( err != ESP_OK ) && ( err != ESP_ERR_NVS_NOT_FOUND ) )
    {
        return err;
    }

    if( ulTrustBundleReadSize == 0 )
    {
        printf( "Nothing saved yet!\n" );
    }
    else
    {
        err = nvs_get_blob( xNVSHandle, AZURE_TRUST_BUNDLE_NAME, pucTrustBundle, &ulTrustBundleReadSize );

        if( err != ESP_OK )
        {
            return err;
        }
    }

    return err;
}

AzureIoTResult_t AzureIoTCAStorage_WriteTrustBundle( const uint8_t * pucTrustBundle,
                                                     uint32_t ulTrustBundleLength,
                                                     uint32_t ulTrustBundleVersion )
{
      nvs_handle_t xNVSHandle;
    esp_err_t err;

    /* Open CA Cert namespace */
    err = nvs_open( CA_CERT_NAMESPACE, NVS_READWRITE, &xNVSHandle );

    int32_t read_trust_bundle_version = 0; /* value will default to 0, if not set yet in NVS */

    /* Read the current trust bundle version */
    err = nvs_get_i32( xNVSHandle, AZURE_TRUST_BUNDLE_VERSION_NAME, &read_trust_bundle_version );

    if( err != ESP_OK )
    {
        printf( "Error (%s) getting AZURE_TRUST_BUNDLE_VERSION_NAME from NVS!\n", esp_err_to_name( err ) );
        return err;
    }

    /* If version matches, do not write to not overuse NVS */
    if( read_trust_bundle_version == trust_bundle_version )
    {
        printf( "Trust bundle version in NVS matches bundle version to write.\n" );
        return ESP_OK;
    }

    /* Write value including previously saved blob if available */
    printf( "Writing trust bundle\n" );
    err = nvs_set_blob( xNVSHandle, AZURE_TRUST_BUNDLE_NAME, pucTrustBundle, ulTrustBundleLength );

    if( err != ESP_OK )
    {
        printf( "Error (%s) getting AZURE_TRUST_BUNDLE_VERSION_NAME from NVS!\n", esp_err_to_name( err ) );
        return err;
    }

    /* Set new trust bundle version */
    printf( "Writing trust bundle version\n" );
    err = nvs_set_i32( xNVSHandle, AZURE_TRUST_BUNDLE_VERSION_NAME, ulTrustBundleVersion );

    if( err != ESP_OK )
    {
        printf( "Error (%s) getting AZURE_TRUST_BUNDLE_VERSION_NAME from NVS!\n", esp_err_to_name( err ) );
        return err;
    }

    /* Commit */
    err = nvs_commit( xNVSHandle );

    if( err != ESP_OK )
    {
        printf( "Error (%s) getting AZURE_TRUST_BUNDLE_VERSION_NAME from NVS!\n", esp_err_to_name( err ) );
        return err;
    }

    printf( "Printing what was just written\n" );
    err = read_saved_bundle( xNVSHandle );

    return err;
}
