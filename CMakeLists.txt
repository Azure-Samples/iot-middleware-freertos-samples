cmake_minimum_required(VERSION 3.13)
set(CMAKE_TOOLCHAIN_FILE cmake/${VENDOR}_gcc.cmake)

project(iot-middleware-sample C ASM)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

# Fetch freertos and set 
#free_rtos_fetch()
set(FREERTOS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/FreeRTOS CACHE STRING "Path to FreeRTOS")
find_package(FreeRTOS REQUIRED)
find_package(FreeRTOSPlus REQUIRED)

# Include logging globally
include_directories("${FreeRTOSPlus_PATH}/Source/Utilities/logging")

# Add demo
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/demos)

# Add MCU flags, set using board cmake
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MCU_C_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MCU_LINKER_FLAGS}")

# Set middleware specific path's
set(FREERTOS_DIRECTORY ${FREERTOS_PATH} CACHE STRING "FreeRTOS Path")
set(CONFIG_DIRECTORY ${BOARD_DEMO_CONFIG_PATH} CACHE STRING "Demo config")
set(FREERTOS_PORT_DIRECTORY ${BOARD_DEMO_FREERTOS_PORT_PATH} CACHE STRING "FreeRTOS port")

# Add middleware
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/azure-iot-middleware-freertos)