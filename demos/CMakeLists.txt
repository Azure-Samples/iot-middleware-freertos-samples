# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if (NOT BOARD)
    message(FATAL_ERROR "Board is not defined.")
endif()

string(TOLOWER ${BOARD} BOARD_L)
string(TOUPPER ${BOARD} BOARD_U)

# Target for sample task
if(NOT (TARGET SAMPLE::AZUREIOT))
    add_library(SAMPLE::AZUREIOT INTERFACE IMPORTED)
    target_sources(SAMPLE::AZUREIOT INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/sample_azure_iot/sample_azure_iot.c)
endif()

# Target for freertos tcpip socket
if(NOT (TARGET SAMPLE::SOCKET::FREERTOSTCPIP))
    add_library(SAMPLE::SOCKET::FREERTOSTCPIP INTERFACE IMPORTED)
    target_sources(SAMPLE::SOCKET::FREERTOSTCPIP INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/sockets_wrapper_freertos_tcpip.c)
    target_include_directories(SAMPLE::SOCKET::FREERTOSTCPIP INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport)
endif()

# Target for lwip based socket
if(NOT (TARGET SAMPLE::SOCKET::LWIP))
    add_library(SAMPLE::SOCKET::LWIP INTERFACE IMPORTED)
    target_sources(SAMPLE::SOCKET::LWIP INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/sockets_wrapper_lwip.c)
    target_include_directories(SAMPLE::SOCKET::LWIP INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport)
endif()

# Target for transport using Mbedtls
if(NOT (TARGET SAMPLE::TRANSPORT::MBEDTLS))
    add_library(SAMPLE::TRANSPORT::MBEDTLS INTERFACE IMPORTED)
    target_sources(SAMPLE::TRANSPORT::MBEDTLS INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/transport_tls_socket_using_mbedtls.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/utilities/crypto_using_mbedtls.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/utilities/mbedtls_freertos_port.c)
    target_include_directories(SAMPLE::TRANSPORT::MBEDTLS INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/
        ${CMAKE_CURRENT_SOURCE_DIR}/common/utilities/)
endif()

# Add board specific demo
if(BOARD_L STREQUAL "stm32h745i_discovery")
    set(BOARD_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/projects/${VENDOR}/${BOARD_L}/cm7)
else()
    set(BOARD_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/projects/${VENDOR}/${BOARD_L})
endif()

add_subdirectory(${BOARD_SOURCE_PATH})
