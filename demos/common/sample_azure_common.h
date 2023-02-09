/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @brief Contains helpers that are used in multiple samples
 */

#ifndef SAMPLE_AZURE_COMMON_IF_H
#define SAMPLE_AZURE_COMMON_IF_H

/* Demo Specific configs. */
#include "demo_config.h"

#if NDEBUG
    #define panic_handler()                                                                           \
    vLoggingPrintf( "[ERROR] [%s:%s:%d]\r\nRestarting device...\r\n", __FILE__, __func__, __LINE__ ); \
    configRestartDevice()
#else
    #define panic_handler()                                                   \
    vLoggingPrintf( "[ERROR] [%s:%s:%d]\r\n", __FILE__, __func__, __LINE__ ); \
    vLoggingPrintf( "Looping to enable attaching a debugger\r\n" );           \
    taskDISABLE_INTERRUPTS();                                                 \
    for( ; ; ) {; }
#endif

#endif /* ifndef SAMPLE_AZURE_COMMON_IF_H */
