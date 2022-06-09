# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

# set global path
set(BOARD_DEMO_CONFIG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config CACHE INTERNAL "Config path")
set(BOARD_DEMO_PORT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/port CACHE INTERNAL "Port path")
set(BOARD_DEMO_FREERTOS_PORT_PATH ${FreeRTOS_Posix_PATH} CACHE INTERNAL "FreeRTOS Port used ")

# include flags
include(${CMAKE_CURRENT_SOURCE_DIR}/gcc_flags.cmake)

# include config path as global
include_directories(${BOARD_DEMO_CONFIG_PATH}
${CMAKE_CURRENT_LIST_DIR}/port)

# Add port specific source file
target_sources(FreeRTOSPlus::TCPIP::PORT INTERFACE 
    ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/portable/BufferManagement/BufferAllocation_2.c
    ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux/NetworkInterface.c)
target_include_directories(FreeRTOSPlus::TCPIP::PORT INTERFACE 
    ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux/
    ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/portable/Compiler/GCC/)

# Add demo files and dependencies
add_executable(${PROJECT_NAME}
  main.c
)
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

add_map_file(${PROJECT_NAME} ${PROJECT_NAME}.map)

# ADU demo files and dependencies
# add_executable(${PROJECT_NAME}-adu
#   main.c
#   ${CMAKE_CURRENT_LIST_DIR}/port/azure_iot_flash_platform.c
# )
# target_link_libraries(${PROJECT_NAME}-adu PRIVATE
#     FreeRTOS::Timers
#     FreeRTOS::Heap::3
#     FreeRTOS::EventGroups
#     FreeRTOS::Posix
#     FreeRTOSPlus::Utilities::backoff_algorithm
#     FreeRTOSPlus::Utilities::logging
#     FreeRTOSPlus::ThirdParty::mbedtls
#     FreeRTOSPlus::TCPIP
#     FreeRTOSPlus::TCPIP::PORT
#     az::iot_middleware::freertos
#     pthread
#     pcap
#     SAMPLE::AZUREIOTADU
#     SAMPLE::TRANSPORT::MBEDTLS
#     SAMPLE::TRANSPORT::SOCKET
#     SAMPLE::SOCKET::FREERTOSTCPIP)

# target_include_directories(${PROJECT_NAME}-adu
#     PUBLIC
#         ${CMAKE_CURRENT_LIST_DIR}/port
# )

# add_map_file(${PROJECT_NAME}-adu ${PROJECT_NAME}-adu.map)

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

add_map_file(${PROJECT_NAME}-pnp ${PROJECT_NAME}-pnp.map)

# Add demo files and dependencies for PnP Sample
add_executable(jws_using_mbedtls_ut
${CMAKE_CURRENT_LIST_DIR}/tests/main.c
${CMAKE_CURRENT_LIST_DIR}/tests/aux_needed_functions.c
${CMAKE_CURRENT_LIST_DIR}/../../../../demos/common/tests/jws_using_mbedtls_ut.c
)
target_link_libraries(jws_using_mbedtls_ut PRIVATE
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
