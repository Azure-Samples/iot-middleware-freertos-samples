/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/*
 * THESE FUNCTIONS AND VALUES ARE NEEDED FOR COMPILATIONS OF THE UNIT TESTS
 */

/* Standard includes. */
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <strings.h>
#include <unistd.h>
#include <assert.h>

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"

/* TCP/IP stack includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

#include "mbedtls/entropy.h"

#define mainHOST_NAME           "RTOSDemo"
#define mainDEVICE_NICK_NAME    "linux_demo"

/*
 * Just seeds the simple pseudo random number generator.
 *
 * !!! NOTE !!!
 * This is not a secure method of generating random numbers and production
 * devices should use a true random number generator (TRNG).
 */
static void prvSRand( UBaseType_t ulSeed );

/* Needed for compilation */
const uint8_t ucMACAddress[ 6 ] = { configMAC_ADDR0, configMAC_ADDR1, configMAC_ADDR2, configMAC_ADDR3, configMAC_ADDR4, configMAC_ADDR5 };

/* Use by the pseudo random number generator. */
static UBaseType_t ulNextRand;

/* Needed for compilation */
void vLoggingPrintf( const char * pcFormat,
                     ... )
{
    va_list arg;

    va_start( arg, pcFormat );
    vprintf( pcFormat, arg );
    va_end( arg );
}

/* Needed for compilation */
extern uint32_t ulApplicationGetNextSequenceNumber( uint32_t ulSourceAddress,
                                                    uint16_t usSourcePort,
                                                    uint32_t ulDestinationAddress,
                                                    uint16_t usDestinationPort )
{
    ( void ) ulSourceAddress;
    ( void ) usSourcePort;
    ( void ) ulDestinationAddress;
    ( void ) usDestinationPort;

    return ( uint32_t ) configRAND32();
}
/*-----------------------------------------------------------*/

BaseType_t xApplicationGetRandomNumber( uint32_t * pulNumber )
{
    *pulNumber = ( uint32_t ) configRAND32();
    return pdTRUE;
}
/*-----------------------------------------------------------*/

/* Needed for compilation */
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize )
{
    ( void ) ppxIdleTaskStackBuffer;
    ( void ) ppxIdleTaskStackBuffer;
    ( void ) pulIdleTaskStackSize;
}
/*-----------------------------------------------------------*/

/* Needed for compilation */
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize )
{
    ( void ) ppxTimerTaskStackBuffer;
    ( void ) ppxTimerTaskStackBuffer;
    ( void ) pulTimerTaskStackSize;
}
/*-----------------------------------------------------------*/

uint64_t ullGetUnixTime( void )
{
    return ( uint64_t ) time( NULL );
}
/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

/* Needed for compilation */
int iMainRand32( void )
{
    return 0;
}
/*-----------------------------------------------------------*/

/* Called by FreeRTOS+TCP when the network connects or disconnects.  Disconnect
 * events are only received if implemented in the MAC driver. */
void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
{
    ( void ) eNetworkEvent;
}
/*-----------------------------------------------------------*/

#if ( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 )

    BaseType_t xApplicationDNSQueryHook( const char * pcName )
    {
        ( void ) pcName;
    }

#endif /* if ( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 ) */
/*-----------------------------------------------------------*/
