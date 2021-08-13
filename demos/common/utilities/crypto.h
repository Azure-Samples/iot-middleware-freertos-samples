/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdint.h>

/**
 * @brief Initialize crypto
 *
 * @return An #uint32_t with result of operation.
 */
uint32_t Crypto_Init();

/**
 * @brief Compute HMAC SHA256
 *
 * @param[in] pucKey Pointer to key.
 * @param[in] ulKeyLength Length of Key.
 * @param[in] pucData Pointer to data for HMAC
 * @param[in] ulDataLength Length of data.
 * @param[in,out] pucOutput Buffer to place computed HMAC.
 * @param[out] ulOutputLength Length of output buffer.
 * @param[in] pulBytesCopied Number of bytes copied to out buffer.
 * @return An #uint32_t with result of operation.
 */
uint32_t Crypto_HMAC( const uint8_t * pucKey,
                      uint32_t ulKeyLength,
                      const uint8_t * pucData,
                      uint32_t ulDataLength,
                      uint8_t * pucOutput,
                      uint32_t ulOutputLength,
                      uint32_t * pulBytesCopied );
