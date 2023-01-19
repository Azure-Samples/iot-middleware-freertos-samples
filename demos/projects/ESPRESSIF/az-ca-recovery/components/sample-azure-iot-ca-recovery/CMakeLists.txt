# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(ROOT_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../..
)

file(GLOB_RECURSE COMPONENT_SOURCES
    ${ROOT_PATH}/demos/sample_azure_iot_ca_recovery/*.c
    ${ROOT_PATH}/demos/common/azure_ca_recovery/azure_ca_recovery_parse.c
    ${ROOT_PATH}/demos/common/azure_ca_recovery/azure_ca_recovery_mbedtls_rsa_verify.c
)

# kconfig does not support multiline strings.
# For certificates, we use as a workaround escaping the newlines
# in certificates and keys so they can be entered as a single
# string in kconfig.
# The routine below unescapes the newlines so the values
# can be correctly interpreted by the code.
if(EXISTS "${CMAKE_BINARY_DIR}/config/sdkconfig.h")
    file(READ "${CMAKE_BINARY_DIR}/config/sdkconfig.h" config_header)
    string(REPLACE "\\n" "n" client_certificate ${config_header})
    message("CLIENT_CERT: ${client_certificate}")
    file(WRITE "${CMAKE_BINARY_DIR}/config/sdkconfig.h" "${client_certificate}")
endif()

idf_component_get_property(MBEDTLS_DIR mbedtls COMPONENT_DIR)

list(APPEND COMPONENT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/azure_trust_bundle_storage_esp.c
    ${CMAKE_CURRENT_LIST_DIR}/backoff_algorithm.c
    ${CMAKE_CURRENT_LIST_DIR}/transport_tls_esp32.c
    ${CMAKE_CURRENT_LIST_DIR}/crypto_esp32.c
)

set(COMPONENT_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/../../config
    ${CMAKE_CURRENT_LIST_DIR}
    ${MBEDTLS_DIR}/mbedtls/include
    ${ROOT_PATH}/demos/common/ca-recovery
    ${ROOT_PATH}/demos/common/transport
    ${ROOT_PATH}/demos/common/utilities
)

idf_component_register(
    SRCS ${COMPONENT_SOURCES}
    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
    REQUIRES nvs_flash mbedtls tcp_transport coreMQTT azure-sdk-for-c azure-iot-middleware-freertos)
