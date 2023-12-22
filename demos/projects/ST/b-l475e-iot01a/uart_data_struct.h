/*
 * uart_data_struct.h
 *
 *  Created on: 23 lut 2023
 *      Author: Wiktor Komorowski
 */

#ifndef SHARED_COMPONENTS_UART_INC_UART_DATA_STRUCT_H_
#define SHARED_COMPONENTS_UART_INC_UART_DATA_STRUCT_H_

#ifdef __cplusplus
 extern "C" {
#endif

//========================================================================================================== INCLUDES
#include "stdint.h"

//========================================================================================================== DEFINITIONS AND MACROS
typedef void(*tx_callback_t)(void*);

typedef struct{
	 uint8_t* data_to_send;
	 uint16_t data_size;
	 tx_callback_t pre_trans_callback;
	 tx_callback_t post_trans_callback;
	 void* callback_args;
	 
} uart_tx_struct_t;

//========================================================================================================== VARIABLES

//========================================================================================================== FUNCTIONS DECLARATIONS


#ifdef __cplusplus
}
#endif
#endif /* SHARED_COMPONENTS_UART_INC_UART_DATA_STRUCT_H_ */
