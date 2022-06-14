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

/* The default IP and MAC address used by the demo.  The address configuration
 * defined here will be used if ipconfigUSE_DHCP is 0, or if ipconfigUSE_DHCP is
 * 1 but a DHCP server could not be contacted.  See the online documentation for
 * more information. */
static const uint8_t ucIPAddress[ 4 ] = { configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3 };
static const uint8_t ucNetMask[ 4 ] = { configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3 };
static const uint8_t ucGatewayAddress[ 4 ] = { configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3 };
static const uint8_t ucDNSServerAddress[ 4 ] = { configDNS_SERVER_ADDR0, configDNS_SERVER_ADDR1, configDNS_SERVER_ADDR2, configDNS_SERVER_ADDR3 };

/* Set the following constant to pdTRUE to log using the method indicated by the
 * name of the constant, or pdFALSE to not log using the method indicated by the
 * name of the constant.  Options include to standard out (xLogToStdout), to a disk
 * file (xLogToFile), and to a UDP port (xLogToUDP).  If xLogToUDP is set to pdTRUE
 * then UDP messages are sent to the IP address configured as the UDP logging server
 * address (see the configUDP_LOGGING_ADDR0 definitions in FreeRTOSConfig.h) and
 * the port number set by configPRINT_PORT in FreeRTOSConfig.h. */
const BaseType_t xLogToStdout = pdTRUE, xLogToFile = pdFALSE, xLogToUDP = pdFALSE;

/* Default MAC address configuration.  The demo creates a virtual network
 * connection that uses this MAC address by accessing the raw Ethernet data
 * to and from a real network connection on the host PC.  See the
 * configNETWORK_INTERFACE_TO_USE definition for information on how to configure
 * the real network connection to use. */
const uint8_t ucMACAddress[ 6 ] = { configMAC_ADDR0, configMAC_ADDR1, configMAC_ADDR2, configMAC_ADDR3, configMAC_ADDR4, configMAC_ADDR5 };

/* Use by the pseudo random number generator. */
static UBaseType_t ulNextRand;

void vLoggingInit( BaseType_t xLogToStdout,
                   BaseType_t xLogToFile,
                   BaseType_t xLogToUDP,
                   uint32_t ulRemoteIPAddress,
                   uint16_t usRemotePort )
{
    /* Can only be called before the scheduler has started. */
    configASSERT( xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED );

    /* FreeRTOSIPConfig is set such that no print messages will be output.
     *              * Avoid compiler warnings about unused parameters. */
    ( void ) xLogToStdout;
    ( void ) xLogToFile;
    ( void ) xLogToUDP;
    ( void ) usRemotePort;
    ( void ) ulRemoteIPAddress;
}

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

/* Needed for compilation */
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

static void prvSRand( UBaseType_t ulSeed )
{
    /* Utility function to seed the pseudo random number generator. */
    ulNextRand = ulSeed;
}
/*-----------------------------------------------------------*/

UBaseType_t uxRand( void )
{
    const uint32_t ulMultiplier = 0x015a4e35UL, ulIncrement = 1UL;

    /* Utility function to generate a pseudo random number. */

    ulNextRand = ( ulMultiplier * ulNextRand ) + ulIncrement;
    return( ( int ) ( ulNextRand >> 16UL ) & 0x7fffUL );
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

/* Needed for compilation */
int mbedtls_platform_entropy_poll( void * data,
                                   unsigned char * output,
                                   size_t len,
                                   size_t * olen )
{
    FILE * file;
    size_t read_len;

    ( ( void ) data );

    *olen = 0;

    file = fopen( "/dev/urandom", "rb" );

    if( file == NULL )
    {
        return( MBEDTLS_ERR_ENTROPY_SOURCE_FAILED );
    }

    read_len = fread( output, 1, len, file );

    if( read_len != len )
    {
        fclose( file );
        return( MBEDTLS_ERR_ENTROPY_SOURCE_FAILED );
    }

    fclose( file );
    *olen = len;

    return( 0 );
}
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

void vAssertCalled( const char * pcFile,
                    uint32_t ulLine )
{
    volatile uint32_t ulBlockVariable = 0UL;
    volatile char * pcFileName = ( volatile char * ) pcFile;
    volatile uint32_t ulLineNumber = ulLine;

    ( void ) pcFileName;
    ( void ) ulLineNumber;

    printf( "vAssertCalled( %s, %u\n", pcFile, ulLine );

    /* Setting ulBlockVariable to a non-zero value in the debugger will allow
     * this function to be exited. */
    taskDISABLE_INTERRUPTS();
    {
        while( ulBlockVariable == 0UL )
        {
            assert( false );
        }
    }
    taskENABLE_INTERRUPTS();
}
/*-----------------------------------------------------------*/


/*-----------------------------------------------------------*/

#if ( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 ) || ( ipconfigDHCP_REGISTER_HOSTNAME == 1 )

    const char * pcApplicationHostnameHook( void )
    {
        /* This function will be called during the DHCP: the machine will be
         * registered with an IP address plus this name. */
        return mainHOST_NAME;
    }

#endif
/*-----------------------------------------------------------*/

#if ( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 )

    BaseType_t xApplicationDNSQueryHook( const char * pcName )
    {
        ( void ) pcName;
    }

#endif /* if ( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 ) */
/*-----------------------------------------------------------*/
