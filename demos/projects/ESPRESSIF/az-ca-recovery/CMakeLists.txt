# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.13)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(azure_iot_freertos_esp32)

if(${CONFIG_ESP_TLS_USE_SECURE_ELEMENT})
    if(${CONFIG_MBEDTLS_ATCA_HW_ECDSA_SIGN} AND ${CONFIG_MBEDTLS_ATCA_HW_ECDSA_VERIFY})
        include(FetchContent)
        if(NOT esp-cryptoauthlib_POPULATED)
            FetchContent_Populate(esp-cryptoauthlib
                GIT_REPOSITORY    https://github.com/espressif/esp-cryptoauthlib
                GIT_TAG           985ea960f83f67f66881e2dae57097e2a07b92a0
                SOURCE_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/components/esp-cryptoauthlib"
            )
        endif()
    else()
        message(FATAL_ERROR "To use the Microchip ATECC608 secure element, refer to the README_ATECC608_Support.md !!")
    endif()
endif()
