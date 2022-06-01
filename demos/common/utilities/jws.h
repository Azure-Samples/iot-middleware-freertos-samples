/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdint.h>

uint32_t AzureIoT_SHA256Calculate( const char * input,
                                   uint32_t inputLength,
                                   char * output );

uint32_t AzureIoT_RS256Verify( char * input,
                               uint32_t inputLength,
                               char * signature,
                               uint32_t signatureLength,
                               unsigned char * n,
                               uint32_t nLength,
                               unsigned char * e,
                               uint32_t eLength,
                               char * buffer,
                               uint32_t bufferLength );

uint32_t JWS_Verify( const char * pucEscapedManifest,
                     uint32_t ulEscapedManifestLength,
                     char * pucJWS,
                     uint32_t ulJWSLength );
