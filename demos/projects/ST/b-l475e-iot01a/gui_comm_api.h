/*
 * gui_comm_api.h
 *
 *  Created on: May 27, 2023
 *      Author: Wiktor Komorowski
 */

#ifndef GUI_COMM_INC_GUI_COMM_API_H_
#define GUI_COMM_INC_GUI_COMM_API_H_

#ifdef __cplusplus
 extern "C" {
#endif

//========================================================================================================== INCLUDES
#include "errors.h"
#include "uart_data_struct.h"

//========================================================================================================== DEFINITIONS AND MACROS

//========================================================================================================== VARIABLES

//========================================================================================================== FUNCTIONS DECLARATIONS
error_t gui_comm_init(void);
error_t gui_comm_send_data(uint8_t* data, uint16_t data_size, tx_callback_t pre_trans_callback, tx_callback_t post_trans_callback, void* callback_args);
uint16_t gui_comm_check_for_received_data(uint8_t* data, uint16_t data_size);

#ifdef __cplusplus
}
#endif

#endif /* GUI_COMM_INC_GUI_COMM_API_H_ */
