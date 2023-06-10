/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @brief Defines an interface to be used by samples when interacting with azure sample base modules.
 */

#ifndef AZURE_SAMPLE_CONNECTION_H
#define AZURE_SAMPLE_CONNECTION_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Indicates if the sample is connected to the internet or not.
 *
 * @return A #bool result where true indicates the sample is connected to the internet, and false indicates it is not.
 */
bool xAzureSample_IsConnectedToInternet( void );

#endif /* AZURE_SAMPLE_CONNECTION_H */
