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

    target_include_directories(SAMPLE::AZUREIOT INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/sample_azure_iot)
endif()

# Target for sample task
if(NOT (TARGET SAMPLE::AZUREIOTCARECOVERY))
    add_library(SAMPLE::AZUREIOTCARECOVERY INTERFACE IMPORTED)

    target_sources(SAMPLE::AZUREIOTCARECOVERY INTERFACE 
      ${CMAKE_CURRENT_SOURCE_DIR}/sample_azure_iot_ca_recovery/sample_azure_iot_ca_recovery.c
      )
endif()

# Target for adu sample task
if(NOT (TARGET SAMPLE::AZUREIOTADU))
    add_library(SAMPLE::AZUREIOTADU INTERFACE IMPORTED)

    target_sources(SAMPLE::AZUREIOTADU INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/sample_azure_iot_adu/sample_azure_iot_adu.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sample_azure_iot_adu/sample_azure_iot_pnp_simulated_data.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../libs/azure-iot-middleware-freertos/ports/mbedTLS/azure_iot_jws_mbedtls.c)
endif()

# Target for pnp sample task
if(NOT (TARGET SAMPLE::AZUREIOTPNP))
    add_library(SAMPLE::AZUREIOTPNP INTERFACE IMPORTED)

    target_sources(SAMPLE::AZUREIOTPNP INTERFACE
      ${CMAKE_CURRENT_SOURCE_DIR}/sample_azure_iot_pnp/sample_azure_iot_pnp.c
      ${CMAKE_CURRENT_SOURCE_DIR}/sample_azure_iot_pnp/sample_azure_iot_pnp_simulated_data.c)
endif()

# Target for gsg sample task
if(NOT (TARGET SAMPLE::AZUREIOTGSG))
    add_library(SAMPLE::AZUREIOTGSG INTERFACE IMPORTED)

    target_sources(SAMPLE::AZUREIOTGSG INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/sample_azure_iot_gsg/sample_azure_iot_gsg.c)
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

# Target for transport using sockets
if(NOT (TARGET SAMPLE::TRANSPORT::SOCKET))
    add_library(SAMPLE::TRANSPORT::SOCKET INTERFACE IMPORTED)
    target_sources(SAMPLE::TRANSPORT::SOCKET INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/transport_socket.c)
    target_include_directories(SAMPLE::TRANSPORT::SOCKET INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/
        ${CMAKE_CURRENT_SOURCE_DIR}/common/utilities/)
endif()

# Target for transport using Mbedtls
if(NOT (TARGET SAMPLE::TRANSPORT::MBEDTLS))
    add_library(SAMPLE::TRANSPORT::MBEDTLS INTERFACE IMPORTED)
    target_sources(SAMPLE::TRANSPORT::MBEDTLS INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/transport_tls_socket_using_mbedtls.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/transport_socket.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/utilities/azure_sample_crypto_mbedtls.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/utilities/mbedtls_freertos_port.c)
    target_include_directories(SAMPLE::TRANSPORT::MBEDTLS INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/
        ${CMAKE_CURRENT_SOURCE_DIR}/common/utilities/)
endif()

# Target for sample connection module
if(NOT (TARGET SAMPLE::COMMON::CONNECTION))
    add_library(SAMPLE::COMMON::CONNECTION INTERFACE IMPORTED)
    target_include_directories(SAMPLE::COMMON::CONNECTION INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/common/connection/)
endif()

# Add board specific demo
if(BOARD_L STREQUAL "stm32h745i-disco")
    set(BOARD_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/projects/${VENDOR}/${BOARD_L}/cm7)
else()
    set(BOARD_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/projects/${VENDOR}/${BOARD_L})
endif()

add_subdirectory(${BOARD_SOURCE_PATH})
