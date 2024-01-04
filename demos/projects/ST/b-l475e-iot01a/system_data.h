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
#define MESSAGE_LENGTH_UNIT_STATUS 62 //UNIT_status 57 bytes + header 4 bytes + crc 1 byte = 62 bytes
#define MESSAGE_LENGTH_SKID_STATUS 37 //SKID_status 32 bytes + header 4 bytes + crc 1 byte = 37 bytes
#define MESSAGE_LENGTH_IOT_COMMAND 6 //IOT_COMMAND header 5 bytes + crc 1 byte = 6 bytes
#define NUMBER_OF_HEATERS 9

#if 0
// Structures in controllino for reference
typedef struct{
	uint8_t unit_status; //Flags//000//setup_state_synching_flag//just_started_flag//reset_flag//halt_flag//error_flag
    uint8_t unit_state;
    uint16_t heater_status;  //Heater//000000//9//8//7//6//5//4//3//2//1
    uint32_t heater_temperatures[9];
    uint8_t valve_status; //00000//Butterfly2//Butterfly1//Fan1
    int32_t vacuum_sensor;
    uint32_t ambient_humidity;
    int32_t ambient_temperature;
    uint32_t errors;
}UNIT_status; //57 bytes

typedef struct{
	uint8_t skid_status; //Flags//0000//reset_flag//halt_flag//error_flag
    uint8_t skid_state;
    uint16_t outputs_status; 
//0000000//CONDENSER//VACUUM_PUMP//COMPRESSOR//THREE_WAY_VALVE_AFTER_VACUUM_PUMP//THREE_WAY_VACUUM_RELEASE_VALVE_BEFORE_CONDENSATOR//VACUUM_RELEASE_VALVE_IN_WT//TWO_WAY_GAS_VALVE_AFTER_WT//TWO_WAY_WATER_OUTLET_VALVE_IN_WT//TWO_WAY_GAS_VALVE_BEFORE_WT
    uint32_t o2_sensor;
    int32_t mass_flow;
    int32_t co2_sensor;
    uint32_t tank_pressure;
    int32_t proportional_valve_pressure;
    int32_t temperature;
    uint32_t humidity;
    uint32_t errors;
}SKID_status; //32 bytes
#endif

typedef struct{
    uint8_t unit_status; //Flags//000//setup_state_synching_flag//just_started_flag//reset_flag//halt_flag//error_flag
    uint8_t unit_state;
    uint16_t heater_status;  //Heater//000000//9//8//7//6//5//4//3//2//1
    double heater_temperatures[NUMBER_OF_HEATERS];
    uint8_t valve_status; //00000//Butterfly2//Butterfly1//Fan1
    double vacuum_sensor;
    double ambient_humidity;
    double ambient_temperature;
    uint32_t errors;
}UNIT_status_t;

typedef struct{
    uint8_t skid_status; //Flags//0000//reset_flag//halt_flag//error_flag
    uint8_t skid_state;
    uint16_t outputs_status;
//0000000//CONDENSER//VACUUM_PUMP//COMPRESSOR//THREE_WAY_VALVE_AFTER_VACUUM_PUMP//THREE_WAY_VACUUM_RELEASE_VALVE_BEFORE_CONDENSATOR//VACUUM_RELEASE_VALVE_IN_WT//TWO_WAY_GAS_VALVE_AFTER_WT//TWO_WAY_WATER_OUTLET_VALVE_IN_WT//TWO_WAY_GAS_VALVE_BEFORE_WT
    double o2_sensor;
    double mass_flow;
    double co2_sensor;
    double tank_pressure;
    double proportional_valve_pressure;
    double temperature;
    double humidity;
    uint32_t errors;
}SKID_status_t;

// Data structures that the sample iot runner will receive on get calls
// Keeping things simple now, we will anyway ditch this and have SDK on MEGA
typedef struct{
    double avg;
    double max;
    double min;
}sensor_info_t;

typedef enum{
    ZERO,
    ONE
}component_status_t;
typedef struct{
    component_status_t status;
    sensor_info_t sensor_info; 
}heater_info_t;

const char* valve_status_stringified[] = {
    "CLOSED",        // 0
    "OPENED"         // 1
};

const char* three_way_vacuum_release_valve_before_condensator_stringified[] = {
    "three_way_vacuum_release_valve_before_condensator_to_TANK",        // 0
    "three_way_vacuum_release_valve_before_condensator_to_AIR"          // 1
};

const char* three_way_valve_after_vacuum_pump_stringified[] = {
    "three_way_valve_after_vacuum_pump_to_TANK",        // 0
    "three_way_valve_after_vacuum_pump_to_AIR"          // 1
};

const char* component_status_stringified[] = {
    "OFF",      // 0
    "ON"        // 1
};

typedef enum{
    FLAG_UNSET,
    FLAG_SET
}flag_state_t;

const char* flag_state_stringified[] = {
    "UNSET",   // 0
    "SET"      // 1
};

// Enum for unit/skid high level state
typedef enum{
    Error_Handling = 0,
    Init_State = 1,
    Adsorb_State = 2,
    Evacuation_State = 3,
    Desorb_State = 4,
    Vacuum_Release_State = 5,
    Lock_State = 6,
    Desorb_Setup_State = 7
}sequence_state_t;

const char* sequence_state_stringified[] = {
    "Error_Handling",        // 0
    "Init_State",            // 1
    "Adsorb_State",          // 2
    "Evacuation_State",      // 3
    "Desorb_State",          // 4
    "Vacuum_Release_State",  // 5
    "Lock_State",            // 6
    "Desorb_Setup_State"     // 7
};

typedef enum{
    SKID_O2,
    SKID_MASS_FLOW,
    SKID_CO2,
    SKID_TANK_PRESSURE,
    SKID_PROPOTIONAL_VALVE_SENSOR,
    SKID_TEMPERATURE,
    SKID_HUMIDITY,
    UNIT_VACUUM_SENSOR,
    UNIT_AMBIENT_HUMIDITY,
    UNIT_AMBIENT_TEMPERATURE,
    UNIT_HEATER
}sensor_name_t;

typedef enum{
    CATRIDGE11,
    CATRIDGE12,
    CATRIDGE13,
    CATRIDGE21,
    CATRIDGE22,
    CATRIDGE23,
    CATRIDGE31,
    CATRIDGE32,
    CATRIDGE33
}heater_index_t;

const char* catridge_index_stringified[] = {
    "CATRIDGE11",
    "CATRIDGE12",
    "CATRIDGE13",
    "CATRIDGE21",
    "CATRIDGE22",
    "CATRIDGE23",
    "CATRIDGE31",
    "CATRIDGE32",
    "CATRIDGE33"
};

typedef struct{
	// uint8_t unit_status; //Flags//000//setup_state_synching_flag//just_started_flag//reset_flag//halt_flag//error_flag
    flag_state_t error_flag;
    flag_state_t halt_flag;
    flag_state_t reset_flag;
    flag_state_t just_started_flag;
    flag_state_t setup_state_synching_flag;
    // uint8_t unit_state;
    sequence_state_t unit_state;
    // uint16_t heater_status;  //Heater//000000//9//8//7//6//5//4//3//2//1
    // uint32_t heater_temperatures[9];
    heater_info_t heater_info[NUMBER_OF_HEATERS];
    // uint8_t valve_status; //00000//Butterfly2//Butterfly1//Fan1
    component_status_t fan_status;
    component_status_t butterfly_valve_1_status;
    component_status_t butterfly_valve_2_status;
    // uint16_t vacuum_sensor;
    // uint16_t ambient_humidity;
    // uint16_t ambient_temperature;
    sensor_info_t vacuum_sensor;
    sensor_info_t ambient_humidity;
    sensor_info_t ambient_temperature;
    // uint32_t errors;    // @todo
}UNIT_iot_status_t;

typedef struct{
    // uint8_t skid_status; //Flags//0000//reset_flag//halt_flag//error_flag
    flag_state_t error_flag;
    flag_state_t halt_flag;
    flag_state_t reset_flag;
    // uint8_t skid_state;
    sequence_state_t skid_state;
    // uint16_t outputs_status;
    //0000000//CONDENSER//VACUUM_PUMP//COMPRESSOR//THREE_WAY_VALVE_AFTER_VACUUM_PUMP//THREE_WAY_VACUUM_RELEASE_VALVE_BEFORE_CONDENSATOR
    //VACUUM_RELEASE_VALVE_IN_WT//TWO_WAY_GAS_VALVE_AFTER_WT//TWO_WAY_WATER_OUTLET_VALVE_IN_WT//TWO_WAY_GAS_VALVE_BEFORE_WT
    component_status_t two_way_gas_valve_before_water_trap;
    component_status_t two_way_gas_valve_in_water_trap;
    component_status_t two_way_gas_valve_after_water_trap;
    component_status_t vacuum_release_valve_in_water_trap;
    component_status_t three_way_vacuum_release_valve_before_condenser;
    component_status_t three_valve_after_vacuum_pump;
    component_status_t compressor;
    component_status_t vacuum_pump;
    component_status_t condenser;
    // uint32_t o2_sensor;
    // int32_t mass_flow;
    // uint32_t co2_sensor;
    // uint32_t tank_pressure;
    // uint32_t temperature;
    // uint32_t humidity;
    sensor_info_t o2_sensor;
    sensor_info_t mass_flow;
    sensor_info_t co2_sensor;
    sensor_info_t tank_pressure;
    sensor_info_t proportional_valve_pressure;
    sensor_info_t temperature;
    sensor_info_t humidity;
    // uint32_t errors;    // @todo
}SKID_iot_status_t;

//Functions
void system_data_init(void);
void read_incoming_system_data(void);
void send_lock_status(void);
void send_unlock_status(void);

UNIT_iot_status_t get_unit_status(void);
SKID_iot_status_t get_skid_status(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_DATA_H_ */