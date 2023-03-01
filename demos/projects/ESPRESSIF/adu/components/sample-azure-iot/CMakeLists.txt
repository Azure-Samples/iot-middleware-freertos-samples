# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(ROOT_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../..
)

set(COMMON_PATH
    ${ROOT_PATH}/demos/projects/ESPRESSIF/common
)

file(GLOB_RECURSE COMPONENT_SOURCES
    ${ROOT_PATH}/demos/sample_azure_iot_ca_recovery/*.c
    ${ROOT_PATH}/demos/common/azure_ca_recovery/azure_ca_recovery_parse.c
    ${ROOT_PATH}/demos/common/azure_ca_recovery/azure_ca_recovery_mbedtls_rsa_verify.c
)

idf_component_get_property(MBEDTLS_DIR mbedtls COMPONENT_DIR)

list(APPEND COMPONENT_SOURCES
    ${COMMON_PATH}/azure_trust_bundle_storage_esp.c
    ${TRANSPORT_PATH}/transport/backoff_algorithm.c
    ${TRANSPORT_PATH}/transport/transport_tls_esp32.c
    ${TRANSPORT_PATH}/transport/crypto_esp32.c
)

set(COMPONENT_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/../../config
    ${COMMON_PATH}/transport
    ${MBEDTLS_DIR}/mbedtls/include
    ${ROOT_PATH}/demos/common/azure_ca_recovery
    ${ROOT_PATH}/demos/common/transport
    ${ROOT_PATH}/demos/common/utilities
)

idf_component_register(
    SRCS ${COMPONENT_SOURCES}
    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
    REQUIRES nvs_flash mbedtls tcp_transport coreMQTT azure-sdk-for-c azure-iot-middleware-freertos)
