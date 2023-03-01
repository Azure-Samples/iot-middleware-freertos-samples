# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(ROOT_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../..
)

set(ESP_COMMON_PATH
    ${ROOT_PATH}/demos/projects/ESPRESSIF/common
)

idf_component_get_property(MBEDTLS_DIR mbedtls COMPONENT_DIR)

set(COMPONENT_SOURCES
    ${ROOT_PATH}/demos/sample_azure_iot_adu/sample_azure_iot_adu.c
    ${ROOT_PATH}/demos/sample_azure_iot_adu/sample_azure_iot_pnp_simulated_data.c
    ${ESP_COMMON_PATH}/transport/backoff_algorithm.c
    ${ESP_COMMON_PATH}/transport/transport_tls_esp32.c
    ${ESP_COMMON_PATH}/transport/transport_socket_esp32.c
    ${ESP_COMMON_PATH}/transport/crypto_esp32.c
)

set(COMPONENT_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/../../config
    ${ESP_COMMON_PATH}/transport
    ${MBEDTLS_DIR}/mbedtls/include
    ${ROOT_PATH}/demos/common/transport
    ${ROOT_PATH}/demos/common/utilities
    ${ROOT_PATH}/demos/sample_azure_iot_adu
)

idf_component_register(
    SRCS ${COMPONENT_SOURCES}
    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
    REQUIRES mbedtls tcp_transport azure-iot-middleware-freertos)
