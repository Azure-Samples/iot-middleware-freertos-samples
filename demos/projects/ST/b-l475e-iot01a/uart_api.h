/*
 * uart_api.h
 *
 *  Created on: 24.11.2023
 *      Author: Wiktor Komorowski
 */
#ifndef _UART_API_
#define _UART_API_

#ifdef __cplusplus
 extern "C" {
#endif
//========================================================================================================== INCLUDES
#include "errors.h"
#include "uart_data_struct.h"

//========================================================================================================== DEFINITIONS AND MACROS

//========================================================================================================== VARIABLES

//========================================================================================================== FUNCTIONS DECLARATIONS
void uart_api_init(void);
error_t uart_send_data(uint8_t* data, uint16_t data_size, tx_callback_t pre_trans_callback, tx_callback_t post_trans_callback, void* callback_args);

#ifdef __cplusplus
}
#endif

#endif //_UART_API_