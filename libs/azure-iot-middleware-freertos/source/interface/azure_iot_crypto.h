/**
 * @file azure_iot_crypto.h
 *
 * @brief The port file for crypto APIs
 *
 * Used for verifying the ADU image payload.
 *
 */

#include <stdint.h>

uint32_t ulAzureIoTCrypto_SHA256Calculate( const char * pucMetadataPtr,
                                           uint32_t ulMetadataSize,
                                           const char * pucInputPtr,
                                           uint64_t ulInputSize,
                                           const char * pucOutputPtr,
                                           uint64_t ulOutputSize );

uint32_t ulAzureIoTCrypto_RS256Verify( const char * pucInputPtr,
                                       uint64_t ulInputSize,
                                       const char * pucSignaturePtr,
                                       uint64_t ulSignatureSize,
                                       const char * pucN,
                                       uint64_t ullNSize,
                                       const char * pucE,
                                       uint64_t ullESize,
                                       const char * pucBufferPtr,
                                       uint32_t ulBufferSize );
