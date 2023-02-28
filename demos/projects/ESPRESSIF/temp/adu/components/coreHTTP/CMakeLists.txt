# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(ROOT_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../..
)

set(AZURE_IOT_MIDDLEWARE_FREERTOS
    ${ROOT_PATH}/libs/azure-iot-middleware-freertos
)

set(CORE_HTTP_PATH
    ${AZURE_IOT_MIDDLEWARE_FREERTOS}/libraries/coreHTTP/source
)

set(CORE_MQTT_CONFIG_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../config
)

file(GLOB COMPONENT_SOURCES
    ${CORE_HTTP_PATH}/*.c
    ${CORE_HTTP_PATH}/dependency/3rdparty/http_parser/http_parser.c
)

set(COMPONENT_INCLUDE_DIRS
    ${CORE_MQTT_CONFIG_PATH}
    ${CORE_HTTP_PATH}/include
    ${CORE_HTTP_PATH}/interface
    ${CORE_HTTP_PATH}/dependency/3rdparty/http_parser
)

idf_component_register(
    SRCS ${COMPONENT_SOURCES}
    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS})

set_source_files_properties(
  ${CORE_HTTP_PATH}/core_http_client.c
    PROPERTIES COMPILE_FLAGS
      -Wno-stringop-truncation
)
