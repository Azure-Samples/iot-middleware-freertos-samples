/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <inttypes.h>

#define LED_STATE_ON  1
#define LED_STATE_OFF 0

#define OLED_DISPLAY_MAX_STRING_LENGTH 48

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * API for interacting with sensors and other peripherals of the Espressif ESP32 Azure IoT Kit board.
     * For more details about the device, please refer to the original documenation at
     * https://www.espressif.com/en/news/Microsoft_Plug-and-Play_with_ESP32-Azure_IoT_Kit
     */

    /**
     * @brief Initializes all the sensors and peripherals of the ESP32 Azure IoT Kit board.
     */
    void initialize_sensors();

    /**
     * @brief Reads the temperature currently measured by the built-in ST HTS221 sensor.
     * 
     * @return float Temperature in Celsius (accuracy: +/- 0.5 °C, range: 15 to +40 °C).
     */
    float get_temperature();

    /**
     * @brief Reads the relative humidity currently measured by the built-in ST HTS221 sensor.
     * 
     * @return float Relative humidity in percentage points.
     */
    float get_humidity();

    /**
     * @brief Reads the ambient iluminance currently measured by the built-in ROHM BH1750FVI sensor.
     * 
     * @return float Iluminance in lux.
     */
    float get_ambientLight();

    /**
     * @brief Reads the acceleration, pitch and roll currently measured by the built-in InvenSense MPU6050 sensor.
     * 
     * @param[out] pitch   Pitch measured by the internal gyroscope, in degrees of arc.
     * @param[out] roll    Roll measured by the internal gyroscope, in degrees of arc.
     * @param[out] accelX  Acceleration on the X axis, in g.
     * @param[out] accelY  Acceleration on the Y axis, in g.
     * @param[out] accelZ  Acceleration on the Z axis, in g.
     */
    void get_pitch_roll_accel(int *pitch, int *roll, int *accelX, int *accelY, int *accelZ);

    /**
     * @brief Reads the pressure and altitude currently measured by the built-in Formosa FBM320 sensor.
     * 
     * @param[out] pressure    Pressure, in Pascal.
     * @param[out] altitude    Altiture, in meters.
     */
    void get_pressure_altitude(float *pressure, float *altitude);

    /**
     * @brief Reads the magnetic field currently measured by the built-in NXP MAG3110 sensor.
     * 
     * @param[out] magnetometerX    Magnetic field on the X axis, in Tesla.
     * @param[out] magnetometerY    Magnetic field on the Y axis, in Tesla.
     * @param[out] magnetometerZ    Magnetic field on the Z axis, in Tesla.
     */
    void get_magnetometer(int *magnetometerX, int *magnetometerY, int *magnetometerZ);

    /**
     * @brief Erases all data currently shown in the built-in Oled display, leaving a blank screen.
     */
    void oled_clean_screen();

    /**
     * @brief Prints a string in the built-in Oled display, including "line-breaks" automatically.
     * 
     * @param[in] message        Buffer containing the string to be printed. Null-terminator is not required.
     * @param[in] messageLength  Length of the string contained in the buffer.
     */
    void oled_show_message(const uint8_t *message, uint32_t messageLength);

    /**
     * @brief Turns the built-in LED 1 on or off.
     * 
     * @param[in] ulLedState  Use the macros LED_STATE_ON and LED_STATE_OFF for switching the LED.
     */
    void led1_set_state(uint32_t ulLedState);

    /**
     * @brief Turns the built-in LED 2 on or off.
     * 
     * @param[in] ulLedState  Use the macros LED_STATE_ON and LED_STATE_OFF for switching the LED.
     */
    void led2_set_state(uint32_t ulLedState);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_MANAGER_H */
