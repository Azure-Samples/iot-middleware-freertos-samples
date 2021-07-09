# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

include(FetchContent)

FetchContent_Declare(
        FREERTOS
        GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS.git
        GIT_TAG        c8fa483b68c6c1149c2a7a8bc1e901b38860ec9b
        GIT_PROGRESS   TRUE
    )

FetchContent_Declare(
    NXP_MCUX_SDK
    GIT_REPOSITORY https://github.com/NXPmicro/mcux-sdk.git
    GIT_TAG        8e910ea4ecc093943bdfd3afd7e2bf578029f92b
    GIT_PROGRESS   TRUE
)

FetchContent_Declare(
    LWIP
    GIT_REPOSITORY https://github.com/lwip-tcpip/lwip.git
    GIT_TAG        STABLE-2_1_2_RELEASE
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

function(nxp_mcux_sdk_fetch)
    if(NXP_MCUX_SDK_PATH)
        message(INFO "NXP_MCUX_SDK_PATH specified: {NXP_MCUX_SDK_PATH}, skipping fetch for NXP MCUX SDK")
    else()
        FetchContent_GetProperties(NXP_MCUX_SDK)

        if(NOT NXP_MCUX_SDK_POPULATED)
            set(FETCHCONTENT_QUIET FALSE) # To see progress
            FetchContent_Populate(NXP_MCUX_SDK)
        endif()

        set(NXP_MCUX_SDK_PATH ${nxp_mcux_sdk_SOURCE_DIR} PARENT_SCOPE)
        message(INFO "NXP_MCUX_SDK_PATH set to ${NXP_MCUX_SDK_PATH}")
    endif()
endfunction()

function(lwip_fetch)
    if(LWIP_PATH)
        message(INFO "LWIP_PATH specified: {LWIP_PATH}, skipping fetch for LWIP")
    else()
        FetchContent_GetProperties(LWIP)

        if(NOT LWIP_POPULATED)
            set(FETCHCONTENT_QUIET FALSE) # To see progress
            FetchContent_Populate(LWIP)
        endif()

        set(LWIP_PATH ${lwip_SOURCE_DIR} PARENT_SCOPE)
        message(INFO "LWIP_PATH set to ${LWIP_PATH}")
    endif()
endfunction()
