# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(ROOT_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../..
)

file(GLOB_RECURSE COMPONENT_SOURCES
    ${ROOT_PATH}/demos/projects/ESPRESSIF/common/azure_trust_bundle_storage_esp.c
)

set(COMPONENT_INCLUDE_DIRS
    ${ROOT_PATH}/demos/common/azure_ca_recovery
    ${ROOT_PATH}/libs/azure-iot-middleware-freertos/source/include
)

idf_component_register(
    SRCS ${COMPONENT_SOURCES}
    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
    REQUIRES nvs_flash)
