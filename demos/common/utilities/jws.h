/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file
 *
 * @brief APIs to authenticate an ADU manifest.
 *
 */

#ifndef JWS_H
#define JWS_H

#include <stdint.h>

/**
 * @brief Authenticate the manifest from ADU.
 *
 * @param pucManifest The escaped manifest from the ADU twin property.
 * @param ulManifestLength The length of \p pucManifest.
 * @param pucJWS The JWS used to authenticate \p pucManifest.
 * @param ulJWSLength The length of \p pucJWS.
 * @return uint32_t The return value of this function.
 * @retval 0 if successful.
 * @retval Otherwise if failed.
 */
uint32_t JWS_ManifestAuthenticate( const char * pucManifest,
                                   uint32_t ulManifestLength,
                                   char * pucJWS,
                                   uint32_t ulJWSLength );

#endif /* JWS_H */
