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
#include <stdbool.h>

//========================================================================================================== DEFINITIONS AND MACROS
#define MUTEX_MAX_BLOCKING_TIME 1000
#define NUMBER_OF_SAMPLES 30

//========================================================================================================== VARIABLES
UNIT_status_t unit_status[NUMBER_OF_SAMPLES] = {0};
SKID_status_t skid_status[NUMBER_OF_SAMPLES] = {0};

// We receive unit and skid separately so need to manage separate indexes
// Not good but that is how it is!!
static uint8_t unit_circular_buffer_index = 0;
static uint8_t skid_circular_buffer_index = 0;

//------------------------------------------ mutexes for read write operations on unit/skid status data structs
SemaphoreHandle_t skid_status_rw_mutex;
StaticSemaphore_t skid_mutex_buffer;

SemaphoreHandle_t unit_status_rw_mutex;
StaticSemaphore_t unit_mutex_buffer;

//========================================================================================================== FUNCTIONS DECLARATIONS
void read_unit_status(uint8_t incoming_data[]);
void read_skid_status(uint8_t incoming_data[]);
uint8_t CalcCrc(uint8_t data[], uint8_t nbrOfBytes);

uint8_t get_latest_index(bool skid);
void sensor_average_max_min(sensor_name_t name, uint8_t heater_index, double* avg, double* max, double* min);

//========================================================================================================== FUNCTIONS DEFINITIONS
void system_data_init(void){
  skid_status_rw_mutex = xSemaphoreCreateMutexStatic(&skid_mutex_buffer);
  unit_status_rw_mutex = xSemaphoreCreateMutexStatic(&unit_mutex_buffer);

  // set the avg, max, min to highest value for initialization, so that we can eliminate these
  #if 0
  for(uint8_t i=0;i<NUMBER_OF_SAMPLES;++i){
    skid_status[i].o2_sensor = __DBL_MAX__;
    skid_status[i].mass_flow = __DBL_MAX__;
    skid_status[i].co2_sensor = __DBL_MAX__;
    skid_status[i].tank_pressure = __DBL_MAX__;
    skid_status[i].temperature = __DBL_MAX__;
    skid_status[i].humidity = __DBL_MAX__;
  }

  for(uint8_t i=0;i<NUMBER_OF_SAMPLES;++i){
    unit_status[i].vacuum_sensor = __DBL_MAX__;
    unit_status[i].ambient_humidity = __DBL_MAX__;
    unit_status[i].ambient_temperature = __DBL_MAX__;
  }
  #endif

  gui_comm_init();
}

void read_incoming_system_data(void){
  static uint8_t incoming_data[64] = {0};
  static uint8_t state = 0;

  switch(state){
    case 0:
      if(gui_comm_check_for_received_data(incoming_data, 1) != FAILED){ // Read first header byte
        //configPRINTF(("incoming_data[0] = %c\r\n", (char)(incoming_data[0])));
        if(incoming_data[0] == 'D'){
          state = 1;
          // configPRINTF( ( "---------RECEIVED D---------\r\n" ) );
        }
      }
      break;
    case 1:
      if(gui_comm_check_for_received_data((incoming_data + 1), 3) != FAILED){ // Read three next header bytes
        //configPRINTF(("incoming_data[0] = %c\r\n", (char)(incoming_data[2])));
        if((incoming_data[2] == 'U') || (incoming_data[2] == 'S')){
          state = 2;
          // configPRINTF( ( "---------RECEIVED U or S---------\r\n" ) );
        }
        else{state = 3;}
      }
      break;
    case 2: // Read data
      switch(incoming_data[2]){
        case 'U': // Unit data
          //Read rest data from uart
          if(gui_comm_check_for_received_data((incoming_data + 4), MESSAGE_LENGTH_UNIT_STATUS - 4) != FAILED){ // Read rest of data
              // for (uint8_t i = 0; i < 64; i++)
              // {
              //   configPRINTF(("%X ", incoming_data[i]));
              // }
              //configPRINTF(("CRC: %X\n\r\n", CalcCrc(incoming_data, MESSAGE_LENGTH_SKID_STATUS - 1)));
              if(CalcCrc(incoming_data, MESSAGE_LENGTH_UNIT_STATUS - 1) == incoming_data[MESSAGE_LENGTH_UNIT_STATUS - 1]){ //Check if crc valid
                  read_unit_status(incoming_data);
              }
              state = 3;
          }
          break;
        case 'S': // SKID data
          //Read rest data from uart
          if(gui_comm_check_for_received_data((incoming_data + 4), MESSAGE_LENGTH_SKID_STATUS - 4) != FAILED){ // Read rest of data
              if(CalcCrc(incoming_data, MESSAGE_LENGTH_SKID_STATUS - 1) == incoming_data[MESSAGE_LENGTH_SKID_STATUS - 1]){ //Check if crc valid 
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

  unit_status[unit_circular_buffer_index].unit_status = incoming_data[end++];

  unit_status[unit_circular_buffer_index].unit_state = incoming_data[end++];

  unit_status[unit_circular_buffer_index].heater_status = (((uint16_t)(incoming_data[end++]) << 8) & 0xFF00);
  unit_status[unit_circular_buffer_index].heater_status |= ((uint16_t)(incoming_data[end++]) & 0xFF);

  uint32_t tmp_sensor = 0;
  for(int i = 0;i<NUMBER_OF_HEATERS;i++){
    tmp_sensor = 0;
    tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
    tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
    tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
    tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
    unit_status[unit_circular_buffer_index].heater_temperatures[i] = (double)tmp_sensor / (1 << 16);
  }

  unit_status[unit_circular_buffer_index].valve_status = incoming_data[end++];

  int32_t tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  unit_status[unit_circular_buffer_index].vacuum_sensor = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor = 0;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
  unit_status[unit_circular_buffer_index].ambient_humidity = (double)tmp_sensor / (1 << 16);

  tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  unit_status[unit_circular_buffer_index].ambient_temperature = (double)tmp_sensor_1 / (1 << 16);

  unit_status[unit_circular_buffer_index].errors = 0;
  unit_status[unit_circular_buffer_index].errors |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  unit_status[unit_circular_buffer_index].errors |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  unit_status[unit_circular_buffer_index].errors |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  unit_status[unit_circular_buffer_index].errors |= (uint32_t)(incoming_data[end++]) & 0xFF;

  // Update the circular index for next turn once current index is filled
  unit_circular_buffer_index++;
  if(unit_circular_buffer_index >= NUMBER_OF_SAMPLES){
    unit_circular_buffer_index = 0;
  }

  xSemaphoreGive(unit_status_rw_mutex);
}

void read_skid_status(uint8_t incoming_data[]){
  static uint8_t end = 4;

  xSemaphoreTake(skid_status_rw_mutex, MUTEX_MAX_BLOCKING_TIME);

  end = 4;

  skid_status[skid_circular_buffer_index].skid_status = incoming_data[end++];
  skid_status[skid_circular_buffer_index].skid_state = incoming_data[end++];

  skid_status[skid_circular_buffer_index].outputs_status = (((uint16_t)(incoming_data[end++]) << 8) & 0xFF00); 
  skid_status[skid_circular_buffer_index].outputs_status |= ((uint16_t)(incoming_data[end++]) & 0xFF);

  uint32_t tmp_sensor = 0;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
  skid_status[skid_circular_buffer_index].o2_sensor = (double)tmp_sensor / (1 << 16);

  int32_t tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  skid_status[skid_circular_buffer_index].mass_flow = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  skid_status[skid_circular_buffer_index].co2_sensor = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor = 0;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
  skid_status[skid_circular_buffer_index].tank_pressure = (double)tmp_sensor / (1 << 16);

  tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  skid_status[skid_circular_buffer_index].proportional_valve_pressure = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor_1 = 0;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor_1 |= ((int32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor_1 |= (int32_t)(incoming_data[end++]) & 0xFF;
  skid_status[skid_circular_buffer_index].temperature = (double)tmp_sensor_1 / (1 << 16);

  tmp_sensor = 0;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  tmp_sensor |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  tmp_sensor |= (uint32_t)(incoming_data[end++]) & 0xFF;
  skid_status[skid_circular_buffer_index].humidity = (double)tmp_sensor / (1 << 16);

  skid_status[skid_circular_buffer_index].errors = 0;
  skid_status[skid_circular_buffer_index].errors |= ((uint32_t)(incoming_data[end++]) << 24) & 0xFF000000;
  skid_status[skid_circular_buffer_index].errors |= ((uint32_t)(incoming_data[end++]) << 16) & 0xFF0000;
  skid_status[skid_circular_buffer_index].errors |= ((uint32_t)(incoming_data[end++]) << 8) & 0xFF00;
  skid_status[skid_circular_buffer_index].errors |= (uint32_t)(incoming_data[end++]) & 0xFF;

  // Update the circular index for next turn once current index is filled
  skid_circular_buffer_index++;
  if(skid_circular_buffer_index >= NUMBER_OF_SAMPLES){
    skid_circular_buffer_index = 0;
  }

  xSemaphoreGive(skid_status_rw_mutex);
}

uint8_t get_latest_index(bool skid){
  uint8_t index = unit_circular_buffer_index;
  if (skid){
    index = skid_circular_buffer_index;
  }

  if(index != 0){
    // We do one back as the indexes are always jumped to next after updating
    index--;
  }
  else{
    index = NUMBER_OF_SAMPLES - 1;
  }

  return index;
}

double get_sensor_value(uint8_t sample_index, sensor_name_t name, uint8_t heater_index){
  double sensor_value = 0.0;
  switch(name){
    // Skid sensors
    case SKID_O2:
      sensor_value = skid_status[sample_index].o2_sensor;
    break;
    case SKID_MASS_FLOW:
      sensor_value = skid_status[sample_index].mass_flow;
    break;
    case SKID_CO2:
      sensor_value = skid_status[sample_index].co2_sensor;
    break;
    case SKID_TANK_PRESSURE:
      sensor_value = skid_status[sample_index].tank_pressure;
    break;
    case SKID_PROPOTIONAL_VALVE_SENSOR:
      sensor_value = skid_status[sample_index].proportional_valve_pressure;
    break;
    case SKID_TEMPERATURE:
      sensor_value = skid_status[sample_index].temperature;
    break;
    case SKID_HUMIDITY:
      sensor_value = skid_status[sample_index].humidity;
    break;
    // Unit sensors
    case UNIT_VACUUM_SENSOR:
      sensor_value = unit_status[sample_index].vacuum_sensor;
    break;
    case UNIT_AMBIENT_HUMIDITY:
      sensor_value = unit_status[sample_index].ambient_humidity;
    break;
    case UNIT_AMBIENT_TEMPERATURE:
      sensor_value = unit_status[sample_index].ambient_temperature;
    break;
    case UNIT_HEATER:
      sensor_value = unit_status[sample_index].heater_temperatures[heater_index];
    break;
    default:
      break;
  }

  return sensor_value;
}

void sensor_average_max_min(sensor_name_t name, uint8_t heater_index, double* avg, double* max, double* min){
  double total = 0.0;
  total = *avg = *max = *min = get_sensor_value(0, name, heater_index);
  // uint8_t num_of_valid_samples = 0;

  for(uint8_t i=1;i<NUMBER_OF_SAMPLES;++i){
    double sensor_value = get_sensor_value(i, name, heater_index);

    // if(sensor_value != __DBL_MAX__){
      total += sensor_value;

      if(sensor_value > *max){
        *max = sensor_value;
      }
      if(sensor_value < *min) {
        *min = sensor_value;
      }
      // num_of_valid_samples++;
    // }

  }

  *avg = total / NUMBER_OF_SAMPLES;
}

SKID_iot_status_t get_skid_status(void){
  static SKID_iot_status_t tmp;

  xSemaphoreTake(skid_status_rw_mutex, MUTEX_MAX_BLOCKING_TIME);
  
  // First the items from the latest index
  uint8_t index = get_latest_index(true);
  tmp.error_flag = (skid_status[index].skid_status & 0x01) ? FLAG_SET:FLAG_UNSET;
  tmp.halt_flag = (skid_status[index].skid_status & 0x02) ? FLAG_SET:FLAG_UNSET;
  tmp.reset_flag = (skid_status[index].skid_status & 0x04) ? FLAG_SET:FLAG_UNSET;
  
  tmp.skid_state = skid_status[index].skid_state;

  tmp.two_way_gas_valve_before_water_trap = (skid_status[index].outputs_status & 0x0001) ? ONE:ZERO;
  tmp.two_way_gas_valve_in_water_trap = (skid_status[index].outputs_status & 0x0002) ? ONE:ZERO;
  tmp.two_way_gas_valve_after_water_trap = (skid_status[index].outputs_status & 0x0004) ? ONE:ZERO;
  tmp.vacuum_release_valve_in_water_trap = (skid_status[index].outputs_status & 0x0008) ? ONE:ZERO;
  tmp.three_way_vacuum_release_valve_before_condenser = (skid_status[index].outputs_status & 0x0010) ? ONE:ZERO;
  tmp.three_valve_after_vacuum_pump = (skid_status[index].outputs_status & 0x0020) ? ONE:ZERO;
  tmp.compressor = (skid_status[index].outputs_status & 0x0040) ? ONE:ZERO;
  tmp.vacuum_pump = (skid_status[index].outputs_status & 0x0080) ? ONE:ZERO;
  tmp.condenser = (skid_status[index].outputs_status & 0x0100) ? ONE:ZERO;

  // The transformed ones (Avg, Max and Min)
  sensor_average_max_min(SKID_O2, 0, &tmp.o2_sensor.avg, &tmp.o2_sensor.max, &tmp.o2_sensor.min);
  sensor_average_max_min(SKID_MASS_FLOW, 0, &tmp.mass_flow.avg, &tmp.mass_flow.max, &tmp.mass_flow.min);
  sensor_average_max_min(SKID_CO2, 0, &tmp.co2_sensor.avg, &tmp.co2_sensor.max, &tmp.co2_sensor.min);
  sensor_average_max_min(SKID_TANK_PRESSURE, 0, &tmp.tank_pressure.avg, &tmp.tank_pressure.max, &tmp.tank_pressure.min);
  sensor_average_max_min(SKID_PROPOTIONAL_VALVE_SENSOR, 0, &tmp.proportional_valve_pressure.avg, &tmp.proportional_valve_pressure.max, &tmp.proportional_valve_pressure.min);
  sensor_average_max_min(SKID_TEMPERATURE, 0, &tmp.temperature.avg, &tmp.temperature.max, &tmp.temperature.min);
  sensor_average_max_min(SKID_HUMIDITY, 0, &tmp.humidity.avg, &tmp.humidity.max, &tmp.humidity.min);

  xSemaphoreGive(skid_status_rw_mutex);

  return tmp;
}

UNIT_iot_status_t get_unit_status(void){
  static UNIT_iot_status_t tmp;

  xSemaphoreTake(unit_status_rw_mutex, MUTEX_MAX_BLOCKING_TIME);
  
  // First the items from the latest index
  uint8_t index = get_latest_index(false);
  tmp.error_flag = (unit_status[index].unit_status & 0x01) ? FLAG_SET:FLAG_UNSET;
  tmp.halt_flag = (unit_status[index].unit_status & 0x02) ? FLAG_SET:FLAG_UNSET;
  tmp.reset_flag = (unit_status[index].unit_status & 0x04) ? FLAG_SET:FLAG_UNSET;
  tmp.just_started_flag = (unit_status[index].unit_status & 0x08) ? FLAG_SET:FLAG_UNSET;
  tmp.setup_state_synching_flag = (unit_status[index].unit_status & 0x10) ? FLAG_SET:FLAG_UNSET;

  tmp.unit_state = unit_status[index].unit_state;

  for(uint8_t i=0;i<NUMBER_OF_HEATERS;++i){
    tmp.heater_info[i].status = (unit_status[index].heater_status & (1 << i)) ? ONE:ZERO;

    sensor_average_max_min(UNIT_HEATER, i, &tmp.heater_info[i].sensor_info.avg, &tmp.heater_info[i].sensor_info.max, &tmp.heater_info[i].sensor_info.min);
  }

  tmp.fan_status = (unit_status[index].valve_status & 0x01) ? ONE:ZERO;
  tmp.butterfly_valve_1_status = (unit_status[index].valve_status & 0x02) ? ONE:ZERO;
  tmp.butterfly_valve_2_status = (unit_status[index].valve_status & 0x04) ? ONE:ZERO;

  // The transformed ones (Avg, Max and Min)
  sensor_average_max_min(UNIT_VACUUM_SENSOR, 0, &tmp.vacuum_sensor.avg, &tmp.vacuum_sensor.max, &tmp.vacuum_sensor.min);
  sensor_average_max_min(UNIT_AMBIENT_HUMIDITY, 0, &tmp.ambient_humidity.avg, &tmp.ambient_humidity.max, &tmp.ambient_humidity.min);
  sensor_average_max_min(UNIT_AMBIENT_TEMPERATURE, 0, &tmp.ambient_temperature.avg, &tmp.ambient_temperature.max, &tmp.ambient_temperature.min);

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