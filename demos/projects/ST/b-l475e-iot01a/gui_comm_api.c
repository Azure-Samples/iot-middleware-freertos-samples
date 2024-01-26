/*
 * gui_comm_api.c
 *
 *  Created on: May 27, 2023
 *      Author: Wiktor Komorowski
 */


//========================================================================================================== INCLUDES
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

#include "gui_comm_api.h"
#include "uart_api.h"

#include "FreeRTOS.h"
#include "task.h"

//========================================================================================================== DEFINITIONS AND MACROS
#define CIRCULAR_BUFFER_LENGTH 500

typedef struct{
	uint8_t buffer[CIRCULAR_BUFFER_LENGTH];
	const uint8_t* buffer_end;

	uint8_t* buffer_tail;
	uint8_t* buffer_head;

}gui_comm_rx_buffers_t;

//========================================================================================================== VARIABLES
gui_comm_rx_buffers_t rx;


//========================================================================================================== FUNCTIONS DECLARATIONS

//========================================================================================================== FUNCTIONS DEFINITIONS
error_t gui_comm_init(void){
	rx.buffer_end = rx.buffer + CIRCULAR_BUFFER_LENGTH;
	rx.buffer_tail = rx.buffer;
	rx.buffer_head = rx.buffer;

	uart_api_init();

	return DONE;
}

error_t gui_comm_send_data(uint8_t* data, uint16_t data_size, tx_callback_t pre_trans_callback, tx_callback_t post_trans_callback, void* callback_args){
	if(uart_send_data(data, data_size, pre_trans_callback, post_trans_callback, callback_args) != DONE ) return FAILED;

	return DONE;
}

void gui_comm_rx_buffer_add(uint8_t message_buf){
	static uint8_t *next = NULL;

	taskDISABLE_INTERRUPTS();

	next = rx.buffer_head + 1;

	if(next >= rx.buffer_end){
		next = rx.buffer;
	}

	if(next == rx.buffer_tail){
		taskENABLE_INTERRUPTS();
		return;
	}

	*rx.buffer_head = message_buf;
	rx.buffer_head = next;

	taskENABLE_INTERRUPTS();
}

uint16_t gui_comm_check_for_received_data(uint8_t* data, uint16_t data_size){
	static uint8_t *next = NULL;
	static uint8_t first_part_len = 0;

	taskDISABLE_INTERRUPTS();

	if((rx.buffer_head == rx.buffer_tail) ||

	  ((rx.buffer_head > rx.buffer_tail) && ((rx.buffer_head - rx.buffer_tail) < data_size)) ||

	  ((rx.buffer_head > rx.buffer_tail) && ((rx.buffer_end - rx.buffer_tail) + (rx.buffer_head - rx.buffer) < data_size))){

		taskENABLE_INTERRUPTS();
		return FAILED;
	}

	next = rx.buffer_tail + data_size;

	first_part_len = 0;
	if(next >= rx.buffer_end){
		first_part_len = ((rx.buffer_end) - rx.buffer_tail);
		next = rx.buffer + data_size - first_part_len;

		memcpy((void*)data, (const void*)rx.buffer_tail, first_part_len);

		rx.buffer_tail = rx.buffer;
	}

	memcpy((void*)(data + first_part_len), (const void*)rx.buffer_tail, data_size - first_part_len);

	rx.buffer_tail = next;

	taskENABLE_INTERRUPTS();

	return DONE;
}
