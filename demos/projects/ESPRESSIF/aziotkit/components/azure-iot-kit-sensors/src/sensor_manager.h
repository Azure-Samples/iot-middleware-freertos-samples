// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#define LED_STATE_ON  1
#define LED_STATE_OFF 0

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*button_cb)(void *);

    void initialize_sensors();
    float get_temperature();
    float get_humidity();
    float get_ambientLight();
    void get_pitch_roll_accel(int *pitch, int *roll, int *accelX, int *accelY, int *accelZ);
    void get_pressure_altitude(float *pressure, float *altitude);
    void get_magnetometer(int *magnetometerX, int *magnetometerY, int *magnetometerZ);
    void oled_clean_screen();
    void oled_show_message( const uint8_t * pucMessage, uint32_t ulMessageLength );
    void led1_set_state(uint32_t ulLedState);
    void led2_set_state(uint32_t ulLedState);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_MANAGER_H */