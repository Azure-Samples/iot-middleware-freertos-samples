/*
 * gui_comm_api_ll.h
 *
 *  Created on: May 28, 2023
 *      Author: Wiktor Komorowski
 */

#ifndef GUI_COMM_INC_GUI_COMM_API_LL_H_
#define GUI_COMM_INC_GUI_COMM_API_LL_H_

#ifdef __cplusplus
 extern "C" {
#endif

//========================================================================================================== INCLUDES

//========================================================================================================== DEFINITIONS AND MACROS

//========================================================================================================== VARIABLES

//========================================================================================================== FUNCTIONS DECLARATIONS
void gui_comm_rx_buffer_add(uint8_t message_buf);

#ifdef __cplusplus
}
#endif

#endif /* GUI_COMM_INC_GUI_COMM_API_LL_H_ */
