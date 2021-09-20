# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

file(GLOB_RECURSE COMPONENT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.c
)

idf_component_get_property(ESP_ROM_DIR esp_rom COMPONENT_DIR)

set(COMPONENT_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/sensors
)

idf_component_register(
    SRCS ${COMPONENT_SOURCES}
    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
    REQUIRES esp_system driver
)
