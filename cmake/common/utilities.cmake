# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

include(FetchContent)

FetchContent_Declare(
        FREERTOS
        GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS.git
        GIT_TAG        c8fa483b68c6c1149c2a7a8bc1e901b38860ec9b
        GIT_PROGRESS   TRUE
    )

function(free_rtos_fetch)
    if(FREERTOS_PATH)
        message(INFO "FREERTOS_PATH specified: {FREERTOS_PATH}, skipping fetch for FreeRTOS")
    else()
        FetchContent_GetProperties(FREERTOS)

        if(NOT FREERTOS_POPULATED)
            set(FETCHCONTENT_QUIET FALSE) # To see progress
            FetchContent_Populate(FREERTOS)
        endif()

        set(FREERTOS_PATH ${freertos_SOURCE_DIR} PARENT_SCOPE)
        message(INFO "FREERTOS_PATH set to ${FREERTOS_PATH}")
    endif()
endfunction()
