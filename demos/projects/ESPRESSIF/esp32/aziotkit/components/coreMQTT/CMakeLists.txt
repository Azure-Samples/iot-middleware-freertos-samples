# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(ROOT_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../..
)

set(AZURE_IOT_MIDDLEWARE_FREERTOS
    ${ROOT_PATH}/libs/azure-iot-middleware-freertos
)

set(CORE_MQTT_PATH
    ${AZURE_IOT_MIDDLEWARE_FREERTOS}/libraries/coreMQTT/source
)

set(CORE_MQTT_CONFIG_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../config
)

file(GLOB COMPONENT_SOURCES
    ${CORE_MQTT_PATH}/*.c
)

set(COMPONENT_INCLUDE_DIRS
    ${CORE_MQTT_CONFIG_PATH}
    ${CORE_MQTT_PATH}/include
    ${CORE_MQTT_PATH}/interface
)

idf_component_register(
    SRCS ${COMPONENT_SOURCES}
    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS})
