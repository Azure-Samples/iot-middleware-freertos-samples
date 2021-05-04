cmake_minimum_required(VERSION 3.13)
set(CMAKE_TOOLCHAIN_FILE cmake/stm32_gcc.cmake)

project(iot-middleware-sample C ASM)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

#include(cmake/utilities)

# Fetch freertos and set 
#free_rtos_fetch()
set(FREERTOS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/FreeRTOS)
find_package(FreeRTOS COMPONENTS ARM_CM4F REQUIRED)
find_package(FreeRTOSPlus REQUIRED)

# Include logging globally
include_directories("${FreeRTOSPlus_PATH}/Source/Utilities/logging")
# TODO: fetch this dependency
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/demo/projects/ST/b-l475e-iot01a/config")

set(freertos_directory ${FREERTOS_PATH} CACHE PATH "Path to freertos_directory")
set(freertos_port_directory ${FreeRTOS_ARM_CM4F_PATH} CACHE PATH "Path to freertos_directory")
set(config_directory ${CMAKE_CURRENT_SOURCE_DIR}/demo/projects/ST/b-l475e-iot01a/config CACHE PATH "Path to config_directory")

# Add board specific demo
# TODO: run time check to see board specific
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/demo/projects/ST/b-l475e-iot01a)

# Add middleware
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/azure-iot-middleware-freertos)