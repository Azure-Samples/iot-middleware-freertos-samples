# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(ROOT_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../..
)

if (DEFINED CONFIG_AZURE_SAMPLE_USE_PLUG_AND_PLAY AND CONFIG_AZURE_SAMPLE_USE_PLUG_AND_PLAY MATCHES "y")
    file(GLOB_RECURSE COMPONENT_SOURCES
        ${ROOT_PATH}/demos/sample_azure_iot_pnp/*.c
    )
else()
    file(GLOB_RECURSE COMPONENT_SOURCES
        ${ROOT_PATH}/demos/sample_azure_iot/*.c
    )
endif()

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
    ${CMAKE_CURRENT_LIST_DIR}/backoff_algorithm.c
    ${CMAKE_CURRENT_LIST_DIR}/transport_tls_esp32.c
    ${CMAKE_CURRENT_LIST_DIR}/crypto_esp32.c
)

set(COMPONENT_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/../../config
    ${CMAKE_CURRENT_LIST_DIR}
    ${MBEDTLS_DIR}/mbedtls/include
    ${ROOT_PATH}/demos/common/transport
    ${ROOT_PATH}/demos/common/utilities
    ${ROOT_PATH}/demos/common/connection
)

if (DEFINED CONFIG_AZURE_SAMPLE_USE_PLUG_AND_PLAY AND CONFIG_AZURE_SAMPLE_USE_PLUG_AND_PLAY MATCHES "y")
    list(APPEND COMPONENT_INCLUDE_DIRS
        ${ROOT_PATH}/demos/sample_azure_iot_pnp
    )
endif()

if (DEFINED CONFIG_ESP_TLS_USE_SECURE_ELEMENT)
    idf_component_register(
        SRCS ${COMPONENT_SOURCES}
        INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
        REQUIRES mbedtls tcp_transport esp-cryptoauthlib coreMQTT azure-sdk-for-c azure-iot-middleware-freertos)
else()
    idf_component_register(
        SRCS ${COMPONENT_SOURCES}
        INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
        REQUIRES mbedtls tcp_transport coreMQTT azure-sdk-for-c azure-iot-middleware-freertos)
endif()

