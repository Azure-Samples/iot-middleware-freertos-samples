/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdint.h>

uint32_t Crypto_Init();

uint32_t Crypto_HMAC( const uint8_t * pucKey, uint32_t ulKeyLength,
                      const uint8_t * pucData, uint32_t ulDataLength,
                      uint8_t * pucOutput, uint32_t ulOutputLength,
                      uint32_t * pulBytesCopied ); 
