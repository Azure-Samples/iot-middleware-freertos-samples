/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

// Write Baltimore CA cert or Digicert CA cert
#define WRITE_BALTIMORE
// #define WRITE_DIGICERT

#define CA_CERT_NAMESPACE "root-ca-cert"

#define AZURE_TRUST_BUNDLE_NAME "az-tb"
#define AZURE_TRUST_BUNDLE_VERSION_NAME "az-tb-ver"

#if defined(WRITE_BALTIMORE) && defined(WRITE_DIGICERT)
#error "Only define one trust bundle to write."
#endif

#ifdef WRITE_BALTIMORE
static uint8_t trust_bundle[] =
/* Baltimore */
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\r\n"
"RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\r\n"
"VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\r\n"
"DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\r\n"
"ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\r\n"
"VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\r\n"
"mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\r\n"
"IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\r\n"
"mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\r\n"
"XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\r\n"
"dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\r\n"
"jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\r\n"
"BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\r\n"
"DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\r\n"
"9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\r\n"
"jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\r\n"
"Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\r\n"
"ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\r\n"
"R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\r\n"
"-----END CERTIFICATE-----\r\n";
static uint32_t trust_bundle_size = sizeof(trust_bundle) - 1;
static uint32_t trust_bundle_version = 1;
#elif defined(WRITE_DIGICERT)
static uint8_t trust_bundle[] =
/* Digicert */
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\r\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\r\n"
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\r\n"
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\r\n"
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\r\n"
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\r\n"
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\r\n"
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\r\n"
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\r\n"
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\r\n"
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\r\n"
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\r\n"
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\r\n"
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\r\n"
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\r\n"
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\r\n"
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\r\n"
"MrY=\r\n"
"-----END CERTIFICATE-----\r\n"
/* MSFT RSA */
"-----BEGIN CERTIFICATE-----\r\n"
"MIIFqDCCA5CgAwIBAgIQHtOXCV/YtLNHcB6qvn9FszANBgkqhkiG9w0BAQwFADBl\r\n"
"MQswCQYDVQQGEwJVUzEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMTYw\r\n"
"NAYDVQQDEy1NaWNyb3NvZnQgUlNBIFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9yaXR5\r\n"
"IDIwMTcwHhcNMTkxMjE4MjI1MTIyWhcNNDIwNzE4MjMwMDIzWjBlMQswCQYDVQQG\r\n"
"EwJVUzEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMTYwNAYDVQQDEy1N\r\n"
"aWNyb3NvZnQgUlNBIFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9yaXR5IDIwMTcwggIi\r\n"
"MA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQDKW76UM4wplZEWCpW9R2LBifOZ\r\n"
"Nt9GkMml7Xhqb0eRaPgnZ1AzHaGm++DlQ6OEAlcBXZxIQIJTELy/xztokLaCLeX0\r\n"
"ZdDMbRnMlfl7rEqUrQ7eS0MdhweSE5CAg2Q1OQT85elss7YfUJQ4ZVBcF0a5toW1\r\n"
"HLUX6NZFndiyJrDKxHBKrmCk3bPZ7Pw71VdyvD/IybLeS2v4I2wDwAW9lcfNcztm\r\n"
"gGTjGqwu+UcF8ga2m3P1eDNbx6H7JyqhtJqRjJHTOoI+dkC0zVJhUXAoP8XFWvLJ\r\n"
"jEm7FFtNyP9nTUwSlq31/niol4fX/V4ggNyhSyL71Imtus5Hl0dVe49FyGcohJUc\r\n"
"aDDv70ngNXtk55iwlNpNhTs+VcQor1fznhPbRiefHqJeRIOkpcrVE7NLP8TjwuaG\r\n"
"YaRSMLl6IE9vDzhTyzMMEyuP1pq9KsgtsRx9S1HKR9FIJ3Jdh+vVReZIZZ2vUpC6\r\n"
"W6IYZVcSn2i51BVrlMRpIpj0M+Dt+VGOQVDJNE92kKz8OMHY4Xu54+OU4UZpyw4K\r\n"
"UGsTuqwPN1q3ErWQgR5WrlcihtnJ0tHXUeOrO8ZV/R4O03QK0dqq6mm4lyiPSMQH\r\n"
"+FJDOvTKVTUssKZqwJz58oHhEmrARdlns87/I6KJClTUFLkqqNfs+avNJVgyeY+Q\r\n"
"W5g5xAgGwax/Dj0ApQIDAQABo1QwUjAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/\r\n"
"BAUwAwEB/zAdBgNVHQ4EFgQUCctZf4aycI8awznjwNnpv7tNsiMwEAYJKwYBBAGC\r\n"
"NxUBBAMCAQAwDQYJKoZIhvcNAQEMBQADggIBAKyvPl3CEZaJjqPnktaXFbgToqZC\r\n"
"LgLNFgVZJ8og6Lq46BrsTaiXVq5lQ7GPAJtSzVXNUzltYkyLDVt8LkS/gxCP81OC\r\n"
"gMNPOsduET/m4xaRhPtthH80dK2Jp86519efhGSSvpWhrQlTM93uCupKUY5vVau6\r\n"
"tZRGrox/2KJQJWVggEbbMwSubLWYdFQl3JPk+ONVFT24bcMKpBLBaYVu32TxU5nh\r\n"
"SnUgnZUP5NbcA/FZGOhHibJXWpS2qdgXKxdJ5XbLwVaZOjex/2kskZGT4d9Mozd2\r\n"
"TaGf+G0eHdP67Pv0RR0Tbc/3WeUiJ3IrhvNXuzDtJE3cfVa7o7P4NHmJweDyAmH3\r\n"
"pvwPuxwXC65B2Xy9J6P9LjrRk5Sxcx0ki69bIImtt2dmefU6xqaWM/5TkshGsRGR\r\n"
"xpl/j8nWZjEgQRCHLQzWwa80mMpkg/sTV9HB8Dx6jKXB/ZUhoHHBk2dxEuqPiApp\r\n"
"GWSZI1b7rCoucL5mxAyE7+WL85MB+GqQk2dLsmijtWKP6T+MejteD+eMuMZ87zf9\r\n"
"dOLITzNy4ZQ5bb0Sr74MTnB8G2+NszKTc0QWbej09+CVgI+WXTik9KveCjCHk9hN\r\n"
"AHFiRSdLOkKEW39lt2c0Ui2cFmuqqNh7o0JMcccMyj6D5KbvtwEwXlGjefVwaaZB\r\n"
"RA+GsCyRxj3qrg+E\r\n"
"-----END CERTIFICATE-----\r\n";
static uint32_t trust_bundle_size = sizeof(trust_bundle) - 1;
static uint32_t trust_bundle_version = 2;
#else
#error "Please define whether you wish to write Baltimore (WRITE_BALTIMORE) or Digicert (WRITE_DIGICERT)"
#endif

/* Read from NVS and print restart counter
   and the table with run times.
   Return an error if anything goes wrong
   during this process.
 */
esp_err_t print_what_saved(void)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(CA_CERT_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
      return err;
    }

    // Read AZURE_TRUST_BUNDLE_VERSION_NAME
    int32_t trust_bundle_version = 0; // value will default to 0, if not set yet in NVS
    err = nvs_get_i32(my_handle, AZURE_TRUST_BUNDLE_VERSION_NAME, &trust_bundle_version);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
      return err;
    }
    printf("Trust bundle version = %d\n", trust_bundle_version);

    // Read run time blob
    size_t trust_bundle_read_size = 0;  // value will default to 0, if not set yet in NVS
    // obtain required memory space to store blob being read from NVS
    err = nvs_get_blob(my_handle, AZURE_TRUST_BUNDLE_NAME, NULL, &trust_bundle_read_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
      return err;
    }

    if (trust_bundle_read_size == 0)
    {
        printf("Nothing saved yet!\n");
    }
    else
    {
        uint8_t* trust_bundle_read = malloc(trust_bundle_read_size);
        err = nvs_get_blob(my_handle, AZURE_TRUST_BUNDLE_NAME, trust_bundle_read, &trust_bundle_read_size);

        if (err != ESP_OK)
        {
            free(trust_bundle_read);
            return err;
        }

        printf("Stored trust bundle:\n");
        printf("%.*s\n", trust_bundle_read_size, trust_bundle_read);

        free(trust_bundle_read);
    }

    // Close
    nvs_close(my_handle);

    return ESP_OK;
}

/* Save new run time value in NVS
   by first reading a table of previously saved values
   and then adding the new value at the end of the table.
   Return an error if anything goes wrong
   during this process.
 */
esp_err_t save_trust_bundle(void)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open CA Cert namespace
    err = nvs_open(CA_CERT_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
      return err;
    }

    // Read the current trust bundle version
    int32_t read_trust_bundle_version = 0;  // value will default to 0, if not set yet in NVS
    err = nvs_get_i32(my_handle, AZURE_TRUST_BUNDLE_VERSION_NAME, &read_trust_bundle_version);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
      return err;
    }

    // If version matches, do not write to not overuse NVS
    if (read_trust_bundle_version == trust_bundle_version)
    {
      printf("Trust bundle version in NVS matches bundle version to write.\n");
      return ESP_OK;
    }

    // Set new trust bundle version
    printf("Writing trust bundle version\n");
    err = nvs_set_i32(my_handle, AZURE_TRUST_BUNDLE_VERSION_NAME, trust_bundle_version);

    if(err != ESP_OK)
    {
      return err;
    }

    // Write value including previously saved blob if available
    printf("Writing trust bundle\n");
    err = nvs_set_blob(my_handle, AZURE_TRUST_BUNDLE_NAME, trust_bundle, trust_bundle_size);

    if (err != ESP_OK)
    {
      return err;
    }

    // Commit
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
    {
      return err;
    }

    // Close
    nvs_close(my_handle);

    printf("Printing what was just written\n");

    err = print_what_saved();
    if (err != ESP_OK)
    {
      printf("Error (%s) reading data from NVS!\n", esp_err_to_name(err));
    }

    return ESP_OK;
}

void app_main(void)
{
    // Delay to give time to open UART console
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    printf("||| Printing what is currently saved |||\n");
    err = print_what_saved();
    if (err != ESP_OK)
    {
      printf("Error (%s) reading data from NVS!\n", esp_err_to_name(err));
    }

    printf("||| Check if bundle needs to be written. Write if needed. |||\n");
    err = save_trust_bundle();
    if (err != ESP_OK)
    {
      printf("Error (%s) saving restart counter to NVS!\n", esp_err_to_name(err));
    }

    while (1)
    {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
