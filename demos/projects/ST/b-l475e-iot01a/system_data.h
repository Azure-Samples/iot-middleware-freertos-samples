/*
 * system_data.h
 *
 *  Created on: November 29, 2023
 *      Author: Maciej Rogalinski
 */

#ifndef SYSTEM_DATA_H_
#define SYSTEM_DATA_H_

#ifdef __cplusplus
 extern "C" {
#endif

//Includes
#include "gui_comm_api.h"
#include <stdio.h>

//Defines
#define CRC_POLYNOMIAL 0x42
#define MESSAGE_LENGTH_UNIT_STATUS 52 //UNIT_status 47 bytes + header 4 bytes + crc 1 byte = 52 bytes
#define MESSAGE_LENGTH_SKID_STATUS 28 //SKID_status 23 bytes + header 4 bytes + crc 1 byte = 28 bytes
#define MESSAGE_LENGTH_IOT_COMMAND 6 //IOT_COMMAND header 5 bytes + crc 1 byte = 6 bytes

//Structures
typedef struct{
	uint8_t unit_status; //Flags//000000//halt_flag//error_flag
    uint8_t unit_state;
    uint16_t heater_status;  //Heater//000000//9//8//7//6//5//4//3//2//1
    uint32_t heater_temperatures[9];
    uint8_t valve_status; //00000//Butterfly2//Butterfly1//Fan1
    uint16_t vacuum_sensor;
    uint32_t errors;

}UNIT_status_t; //47 bytes

typedef struct{
	uint8_t skid_status; //Flags//000000//halt_flag//error_flag
    uint8_t skid_state;
    uint16_t outputs_status; 
//0000000//CONDENSER//VACUUM_PUMP//COMPRESSOR//THREE_WAY_VALVE_AFTER_VACUUM_PUMP//THREE_WAY_VACUUM_RELEASE_VALVE_BEFORE_CONDENSATOR//VACUUM_RELEASE_VALVE_IN_WT//TWO_WAY_GAS_VALVE_AFTER_WT//TWO_WAY_WATER_OUTLET_VALVE_IN_WT//TWO_WAY_GAS_VALVE_BEFORE_WT
    char o2_sensor[5];
    uint16_t mass_flow;
    uint16_t co2_sensor;
    uint16_t tank_pressure;
    uint16_t ambient_temperature;
    uint16_t ambient_humidity;
    uint32_t errors;

}SKID_status_t; //23 bytes

//Functions
void system_data_init(void);
void read_incoming_system_data(void);
void send_lock_status(void);
void send_unlock_status(void);

UNIT_status_t get_unit_status(void);
SKID_status_t get_skid_status(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_DATA_H_ */