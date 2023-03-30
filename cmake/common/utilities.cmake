# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

include(FetchContent)

FetchContent_Declare(
        FREERTOS
        GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS.git
        GIT_TAG        202107.00
        GIT_PROGRESS   TRUE
    )

# Note: We can only support this build system through version 2.12.
# Version 2.13 requires running west (https://github.com/nxp-mcuxpresso/mcux-sdk/issues/109)
FetchContent_Declare(
    NXP_MCUX_SDK
    GIT_REPOSITORY https://github.com/NXPmicro/mcux-sdk.git
    GIT_TAG        08209840456f5f543b3debc5603c3dbe2b40a2eb
    GIT_PROGRESS   TRUE
)

FetchContent_Declare(
    NXP_SFW
    GIT_REPOSITORY https://github.com/nxp-mcuxpresso/sfw.git
    GIT_TAG        d7e76dbe1bf481bd7d4a7fbb167005bbfec8db6e
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

function(nxp_sfw_fetch)
    if(NXP_SFW_PATH)
        message(INFO "NXP_SFW_PATH specified: {NXP_SFW_PATH}, skipping fetch for NXP SFW")
    else()
        FetchContent_GetProperties(NXP_SFW)

        if(NOT NXP_SFW_POPULATED)
            set(FETCHCONTENT_QUIET FALSE) # To see progress
            FetchContent_Populate(NXP_SFW)
        endif()

        set(NXP_SFW_PATH ${nxp_sfw_SOURCE_DIR} PARENT_SCOPE)
        message(INFO "NXP_SFW_PATH set to ${NXP_SFW_PATH}")
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
