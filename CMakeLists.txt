# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13)
if(NOT VENDOR MATCHES "^PC$")
    set(CMAKE_TOOLCHAIN_FILE cmake/${VENDOR}_gcc.cmake)
endif()

project(iot-middleware-sample C ASM)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# include common utilities
include(common/utilities)

# Set FreeRTOS path
if(NOT FREERTOS_PATH)
    message(STATUS "No FREERTOS_PATH specified: Fetching FreeRTOS to path <${FREERTOS_PATH}>")
    free_rtos_fetch()
endif()

find_package(FreeRTOS REQUIRED)
find_package(FreeRTOSPlus REQUIRED)

# Include logging globally
include_directories("${FreeRTOSPlus_PATH}/Source/Utilities/logging")

# Add demo
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/demos)

# Add MCU flags, set using board cmake
add_compile_options(${MCU_C_FLAGS})
add_link_options(${MCU_LINKER_FLAGS})

# Set middleware specific path's
set(FREERTOS_DIRECTORY ${FREERTOS_PATH} CACHE STRING "FreeRTOS Path")
set(CONFIG_DIRECTORY ${BOARD_DEMO_CONFIG_PATH} CACHE STRING "Demo config")
set(FREERTOS_PORT_DIRECTORY ${BOARD_DEMO_FREERTOS_PORT_PATH} CACHE STRING "FreeRTOS port")

# Include port path for lib
include_directories(${BOARD_DEMO_PORT_PATH})

# Add middleware
# These boards do not support ADU
if(NOT(${BOARD} STREQUAL "windows"))
  set(USE_COREHTTP ON)
endif()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/azure-iot-middleware-freertos)
