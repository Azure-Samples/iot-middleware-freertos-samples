# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

# set global path
set(BOARD_DEMO_CONFIG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config CACHE INTERNAL "Config path")
set(BOARD_DEMO_FREERTOS_PORT_PATH ${FreeRTOS_Posix_PATH} CACHE INTERNAL "FreeRTOS Port used ")

# include flags
include(${CMAKE_CURRENT_SOURCE_DIR}/gcc_flags.cmake)

# include config path as global
include_directories(${BOARD_DEMO_CONFIG_PATH})

# Add port specific source file
target_sources(FreeRTOSPlus::TCPIP::PORT INTERFACE 
    ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/portable/BufferManagement/BufferAllocation_2.c
    ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux/NetworkInterface.c)
target_include_directories(FreeRTOSPlus::TCPIP::PORT INTERFACE 
    ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux/
    ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/portable/Compiler/GCC/)

# Add demo files and dependencies
add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE
    FreeRTOS::Timers
    FreeRTOS::Heap::3
    FreeRTOS::EventGroups
    FreeRTOS::Posix
    FreeRTOSPlus::Utilities::backoff_algorithm
    FreeRTOSPlus::Utilities::logging
    FreeRTOSPlus::ThirdParty::mbedtls
    FreeRTOSPlus::TCPIP
    FreeRTOSPlus::TCPIP::PORT
    az::iot_middleware::freertos
    pthread
    pcap
    SAMPLE::AZUREIOT
    SAMPLE::TRANSPORT::MBEDTLS
    SAMPLE::SOCKET::FREERTOSTCPIP)

# Add demo files and dependencies for PnP Sample
add_executable(${PROJECT_NAME}-pnp main.c)
target_link_libraries(${PROJECT_NAME}-pnp PRIVATE
    FreeRTOS::Timers
    FreeRTOS::Heap::3
    FreeRTOS::EventGroups
    FreeRTOS::Posix
    FreeRTOSPlus::Utilities::backoff_algorithm
    FreeRTOSPlus::Utilities::logging
    FreeRTOSPlus::ThirdParty::mbedtls
    FreeRTOSPlus::TCPIP
    FreeRTOSPlus::TCPIP::PORT
    az::iot_middleware::freertos
    pthread
    pcap
    SAMPLE::AZUREIOTPNP
    SAMPLE::TRANSPORT::MBEDTLS
    SAMPLE::SOCKET::FREERTOSTCPIP)
