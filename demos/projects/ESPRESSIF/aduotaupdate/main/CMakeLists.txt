# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(COMPONENT_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../config
)

set(COMPONENT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/azure_iot_freertos_esp32_main.c
    ${CMAKE_CURRENT_LIST_DIR}/azure_iot_freertos_esp32_pnp_model.c
)

idf_component_register(SRCS ${COMPONENT_SOURCES}
                    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
                    REQUIRES freertos nvs_flash coreMQTT azure-sdk-for-c azure-iot-middleware-freertos sample-azure-iot)

