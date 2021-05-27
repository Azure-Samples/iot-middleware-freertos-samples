# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(SAMPLE_AZURE_IOT "sample_azure_iot" CACHE STRING "IoT Sample")

if (NOT BOARD)
    message(FATAL_ERROR "Board is not defined.")
endif()

string(TOLOWER ${BOARD} BOARD_L)
string(TOUPPER ${BOARD} BOARD_U)

# Sample path set
set(SAMPLE_AZURE_IOT_SOURCE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/${SAMPLE_AZURE_IOT}/${SAMPLE_AZURE_IOT}.c)

# Transport using Mbedtls
set(SAMPLE_TRANSPORT_USING_MBEDTLS_SOURCE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/transport_tls_socket_using_mbedtls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/utilities/crypto_using_mbedtls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/utilities/mbedtls_freertos_port.c)
set(SAMPLE_TRANSPORT_USING_MBEDTLS_INC_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/common/transport/
    ${CMAKE_CURRENT_SOURCE_DIR}/common/utilities/)

# Add board specific demo
if(BOARD_L STREQUAL "b-l475e-iot01a")
    set(BOARD_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/projects/ST/${BOARD_L})
elseif(BOARD_L STREQUAL "stm32h745i_discovery")
    set(BOARD_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/projects/ST/${BOARD_L}/cm7)
elseif(BOARD_L STREQUAL "mimxrt1060")
    set(BOARD_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/projects/NXP/${BOARD_L})
elseif(BOARD_L STREQUAL "linux")
    set(BOARD_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/projects/PC/${BOARD_L})
else()
    message("sample not supported for : ${BOARD} ")
endif()

add_subdirectory(${BOARD_SOURCE_PATH})
