# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(ROOT_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../..
)

set(AZURE_IOT_MIDDLEWARE_FREERTOS
    ${ROOT_PATH}/libs/azure-iot-middleware-freertos
)

file(GLOB_RECURSE COMPONENT_SOURCES
    ${AZURE_IOT_MIDDLEWARE_FREERTOS}/source/*.c
)

list(APPEND COMPONENT_SOURCES
    ${AZURE_IOT_MIDDLEWARE_FREERTOS}/ports/coreMQTT/azure_iot_core_mqtt.c
)

idf_component_get_property(FREERTOS_DIR freertos COMPONENT_DIR)

set(COMPONENT_INCLUDE_DIRS
    ${FREERTOS_DIR}/include/freertos
    ${AZURE_IOT_MIDDLEWARE_FREERTOS}/source/include
    ${AZURE_IOT_MIDDLEWARE_FREERTOS}/source/interface
    ${AZURE_IOT_MIDDLEWARE_FREERTOS}/ports/coreMQTT
)

idf_component_register(
    SRCS ${COMPONENT_SOURCES}
    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
    REQUIRES freertos azure-sdk-for-c coreMQTT)
