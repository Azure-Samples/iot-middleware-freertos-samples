# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

# set parent scope path
set(BOARD_DEMO_CONFIG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config CACHE INTERNAL "Config path")
set(BOARD_DEMO_FREERTOS_PORT_PATH ${FreeRTOS_Posix_PATH} CACHE INTERNAL "FreeRTOS Port used ")

include_directories(${BOARD_DEMO_CONFIG_PATH})

set(FREERTOS_TCP_PORT_SOURCES
    ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/portable/BufferManagement/BufferAllocation_2.c
    ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux/NetworkInterface.c
    ../../../common/transport/sockets_wrapper_freertos_tcpip.c)

set(PROJECT_SOURCES
    ${FREERTOS_TCP_PORT_SOURCES}
    ${SAMPLE_AZURE_IOT_SOURCE_PATH}
    ${SAMPLE_TRANSPORT_USING_MBEDTLS_SOURCE_PATH}
    main.c)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC
    ${SAMPLE_TRANSPORT_USING_MBEDTLS_INC_PATH}
    ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux/)
target_link_libraries(${PROJECT_NAME} PRIVATE
    FreeRTOS::Timers
    FreeRTOS::Heap::3
    FreeRTOS::EventGroups
    FreeRTOS::Posix
    FreeRTOSPlus::Utilities::backoff_algorithm
    FreeRTOSPlus::Utilities::logging
    FreeRTOSPlus::ThirdParty::mbedtls
    FreeRTOSPlus::TCPIP
    az::iot_middleware::freertos
    pthread
    pcap)
target_link_options(${PROJECT_NAME} PRIVATE -Xlinker -Map=output.map)