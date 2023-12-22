/*
 * system_data.c
 *
 *  Created on: November 29, 2023
 *      Author: Maciej Rogalinski
 */

//========================================================================================================== INCLUDES
#include "system_data.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <memory.h>

//========================================================================================================== DEFINITIONS AND MACROS
#define MUTEX_MAX_BLOCKING_TIME 1000

//========================================================================================================== VARIABLES
UNIT_status_t unit_status = {0};
SKID_status_t skid_status = {0};

//------------------------------------------ mutexes for read write operations on unit/skid status data structs
SemaphoreHandle_t skid_status_rw_mutex;
StaticSemaphore_t skid_mutex_buffer;

SemaphoreHandle_t unit_status_rw_mutex;
StaticSemaphore_t unit_mutex_buffer;

//========================================================================================================== FUNCTIONS DECLARATIONS
void read_unit_status(uint8_t incoming_data[]);
void read_skid_status(uint8_t incoming_data[]);
uint8_t CalcCrc(uint8_t data[], uint8_t nbrOfBytes);

//========================================================================================================== FUNCTIONS DEFINITIONS
void system_data_init(void){
  skid_status_rw_mutex = xSemaphoreCreateMutexStatic(&skid_mutex_buffer);
  unit_status_rw_mutex = xSemaphoreCreateMutexStatic(&unit_mutex_buffer);

  gui_comm_init();
}

// void read_incoming_system_data(){
//     uint8_t incoming_data[64] = {0};

//     //Read one byte from uart
//     if(gui_comm_check_for_received_data(incoming_data, 1) != FAILED){ // Read first byte
//         switch(incoming_data[0]){
//             case 'D':
//             configPRINTF(("incoming_data[0] = %c\r\n", (char)(incoming_data[0])));
//                 if(gui_comm_check_for_received_data((incoming_data+1), 3) != FAILED){ // Read to two next byte
//                     switch(incoming_data[2]){
//                         case 'U': // Unit data
//                         configPRINTF(("incoming_data[0] = %c\r\n", (char)incoming_data[2]));
//                             //Read rest data from uart
//                             if(gui_comm_check_for_received_data((incoming_data+4), MESSAGE_LENGTH_UNIT_STATUS-4) != FAILED){ // Read rest data
//                                 if(CalcCrc(incoming_data,MESSAGE_LENGTH_UNIT_STATUS) == incoming_data[MESSAGE_LENGTH_UNIT_STATUS-1]){ //Check if crc valid 
//                                     read_unit_status(incoming_data);
//                                 }
//                             }
//                             break;
//                         case 'S': // SKID data
//                             //Read rest data from uart
//                             if(gui_comm_check_for_received_data((incoming_data+4), MESSAGE_LENGTH_SKID_STATUS-4) != FAILED){ // Read rest data
//                                 if(CalcCrc(incoming_data,MESSAGE_LENGTH_SKID_STATUS) == incoming_data[MESSAGE_LENGTH_SKID_STATUS-1]){ //Check if crc valid 
//                                     read_skid_status(incoming_data);
//                                 }
//                             }
//                             break;
//                     }
//                 }
//                 break;
//         }
//     }
// }

void read_incoming_system_data(void){
  static uint8_t incoming_data[64] = {0};
  static uint8_t state = 0;

  switch(state){
    case 0:
      if(gui_comm_check_for_received_data(incoming_data, 1) != FAILED){ // Read first header byte
      //configPRINTF(("incoming_data[0] = %c\r\n", (char)(incoming_data[0])));
        if(incoming_data[0] == 'D'){
          state = 1;
          configPRINTF( ( "---------RECEIVED D---------\r\n" ) );
        }
      }
      break;
    case 1:
      if(gui_comm_check_for_received_data((incoming_data + 1), 3) != FAILED){ // Read three next header bytes
      //configPRINTF(("incoming_data[0] = %c\r\n", (char)(incoming_data[2])));
        if((incoming_data[2] == 'U') || (incoming_data[2] == 'S')){
          state = 2;
          configPRINTF( ( "---------RECEIVED U or S---------\r\n" ) );
          }
        else{state = 3;}
      }
      break;
    case 2: // Read data
      switch(incoming_data[2]){
        case 'U': // Unit data
          //Read rest data from uart
          if(gui_comm_check_for_received_data((incoming_data + 4), MESSAGE_LENGTH_UNIT_STATUS - 4) != FAILED){ // Read rest of data
          //configPRINTF(("U_Here\n\r\n"));
          // for (uint8_t i = 0; i < 64; i++)
          // {
          //   configPRINTF(("%X ", incoming_data[i]));
          // }
          // configPRINTF(("\n\r\n"));
          //configPRINTF(("CRC: %X\n\r\n", CalcCrc(incoming_data, MESSAGE_LENGTH_SKID_STATUS - 1)));
              if(CalcCrc(incoming_data, MESSAGE_LENGTH_UNIT_STATUS - 1) == incoming_data[MESSAGE_LENGTH_UNIT_STATUS - 1]){ //Check if crc valid
              //configPRINTF(("U_Here\n\r\n"));
                  read_unit_status(incoming_data);
              }
              state = 3;
          }
          break;
        case 'S': // SKID data
          //Read rest data from uart
          if(gui_comm_check_for_received_data((incoming_data + 4), MESSAGE_LENGTH_SKID_STATUS - 4) != FAILED){ // Read rest of data
          //configPRINTF(("S_Here\n\r\n"));
              if(CalcCrc(incoming_data, MESSAGE_LENGTH_SKID_STATUS - 1) == incoming_data[MESSAGE_LENGTH_SKID_STATUS - 1]){ //Check if crc valid 
              //configPRINTF(("S_Here\n\r\n"));
                  read_skid_status(incoming_data);
              }
              state = 3;
          }
          break;
      }
      break;
    case 3: //Reset state 
      state = 0;
      memset(incoming_data, 0, sizeof(incoming_data));
      break;
  }
}

void send_lock_status(void){
    static uint8_t data_to_send[MESSAGE_LENGTH_IOT_COMMAND] = {'C', 0, 'L', 0, 'L', 0x88};
    gui_comm_send_data(data_to_send, MESSAGE_LENGTH_IOT_COMMAND, NULL, NULL, NULL);
}
void send_unlock_status(void){
    static uint8_t data_to_send[MESSAGE_LENGTH_IOT_COMMAND] = {'C', 0, 'L', 0, 'U', 0x34};
    gui_comm_send_data(data_to_send, MESSAGE_LENGTH_IOT_COMMAND, NULL, NULL, NULL);
}

void read_unit_status(uint8_t incoming_data[]){
  static uint8_t end = 4;

  xSemaphoreTake(unit_status_rw_mutex, MUTEX_MAX_BLOCKING_TIME);

  end = 4;

  unit_status.unit_status = incoming_data[end++];

  unit_status.unit_state = incoming_data[end++];

  unit_status.heater_status = (((uint16_t)(incoming_data[end++]) << 8) & 0xFF00);
  unit_status.heater_status |= ((uint16_t)(incoming_data[end++]) & 0xFF);

  for(int i = 0;i<9;i++){
    unit_status.heater_temperatures[i] = 0;
    unit_status.heater_temperatures[i] |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
    unit_status.heater_temperatures[i] |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
    unit_status.heater_temperatures[i] |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
    unit_status.heater_temperatures[i] |= (uint32_t)(incoming_data[end++]) & 0xFF;
  }

  unit_status.valve_status = incoming_data[end++];

  unit_status.vacuum_sensor = (((uint16_t)(incoming_data[end++]) << 8) & 0xFF00);
  unit_status.vacuum_sensor |= ((uint16_t)(incoming_data[end++]) & 0xFF);

  unit_status.errors = 0;
  unit_status.errors |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  unit_status.errors |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  unit_status.errors |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  unit_status.errors |= (uint32_t)(incoming_data[end++]) & 0xFF;

  xSemaphoreGive(unit_status_rw_mutex);
}

void read_skid_status(uint8_t incoming_data[]){
  static uint8_t end = 4;

  xSemaphoreTake(skid_status_rw_mutex, MUTEX_MAX_BLOCKING_TIME);

  end = 4;

  skid_status.skid_status = incoming_data[end++];
  skid_status.skid_state = incoming_data[end++];

  skid_status.outputs_status = (((uint16_t)(incoming_data[end++]) << 8) & 0xFF00); 
  skid_status.outputs_status |= ((uint16_t)(incoming_data[end++]) & 0xFF);

  skid_status.o2_sensor[0] = incoming_data[end++]; skid_status.o2_sensor[1] = incoming_data[end++];
  skid_status.o2_sensor[2] = incoming_data[end++]; skid_status.o2_sensor[3] = incoming_data[end++];
  skid_status.o2_sensor[4] = incoming_data[end++];

  skid_status.mass_flow = (((uint16_t)(incoming_data[end++]) << 8) & 0xFF00);
  skid_status.mass_flow |= ((uint16_t)(incoming_data[end++]) & 0xFF);

  skid_status.co2_sensor = (((uint16_t)(incoming_data[end++]) << 8) & 0xFF00);
  skid_status.co2_sensor |= ((uint16_t)(incoming_data[end++]) & 0xFF);

  skid_status.tank_pressure = (((uint16_t)(incoming_data[end++]) << 8) & 0xFF00);
  skid_status.tank_pressure |= ((uint16_t)(incoming_data[end++]) & 0xFF);

  skid_status.ambient_temperature = (((uint16_t)(incoming_data[end++]) << 8) & 0xFF00);
  skid_status.ambient_temperature |= ((uint16_t)(incoming_data[end++]) & 0xFF);

  skid_status.ambient_humidity = (((uint16_t)(incoming_data[end++]) << 8) & 0xFF00);
  skid_status.ambient_humidity |= ((uint16_t)(incoming_data[end++]) & 0xFF);

  skid_status.errors = 0;
  skid_status.errors |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  skid_status.errors |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  skid_status.errors |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  skid_status.errors |= (uint32_t)(incoming_data[end++]) & 0xFF;

  xSemaphoreGive(skid_status_rw_mutex);
}

SKID_status_t get_skid_status(void){
  static SKID_status_t tmp;

  xSemaphoreTake(skid_status_rw_mutex, MUTEX_MAX_BLOCKING_TIME);
  tmp = skid_status;
  xSemaphoreGive(skid_status_rw_mutex);

  return tmp;
}

UNIT_status_t get_unit_status(void){
  static UNIT_status_t tmp;

  xSemaphoreTake(unit_status_rw_mutex, MUTEX_MAX_BLOCKING_TIME);
  tmp = unit_status;
  xSemaphoreGive(unit_status_rw_mutex);

  return tmp;
}

uint8_t CalcCrc(uint8_t data[], uint8_t nbrOfBytes)
{
  uint8_t bit;        // bit mask
  uint8_t crc = 0xFF; // calculated checksum
  uint8_t byteCtr;    // byte counter
  
  // calculates 8-Bit checksum with given polynomial
  for(byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++) {
    crc ^= data[byteCtr];
    for(bit = 8; bit > 0; --bit) {
      if(crc & 0x80) {
        crc = (crc << 1) ^ CRC_POLYNOMIAL;
      } else {
        crc = (crc << 1);
      }
    }
  }
  
  return crc;
}