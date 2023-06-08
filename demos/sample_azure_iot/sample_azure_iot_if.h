/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @brief Defines an interface to be used by samples when interacting with sample_azure_iot_pnp.c module.
 *        This interface allows the module implementing the specific plug-and-play model to exchange data
 *        with the module responsible for communicating with Azure IoT Hub.
 */

#ifndef SAMPLE_AZURE_IOT_IF_H
#define SAMPLE_AZURE_IOT_IF_H

#include <stdbool.h>
#include <stdint.h>

bool xIsSampleConnectedToInternet( void );

#endif /* SAMPLE_AZURE_IOT_IF_H */
