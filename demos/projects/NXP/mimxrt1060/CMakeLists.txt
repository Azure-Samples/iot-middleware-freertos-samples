# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

# set parent scope path
set(BOARD_DEMO_CONFIG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config CACHE INTERNAL "Config path")
set(BOARD_DEMO_FREERTOS_PORT_PATH ${FreeRTOS_ARM_CM4F_PATH} CACHE INTERNAL "FreeRTOS Port used ")

set(MCUX_SDK_PROJECT_NAME mcux-sdk-lib)

add_library(${MCUX_SDK_PROJECT_NAME})
target_include_directories(${MCUX_SDK_PROJECT_NAME}
  PUBLIC
    ${BOARD_DEMO_FREERTOS_PORT_PATH}
)
target_link_libraries(${MCUX_SDK_PROJECT_NAME}
  PUBLIC
    FreeRTOS
)

nxp_mcux_sdk_fetch()
lwip_fetch()

list(APPEND CMAKE_MODULE_PATH ${NXP_MCUX_SDK_PATH})

find_package(LWIP)

include(${CMAKE_CURRENT_SOURCE_DIR}/gcc_flags.cmake)

include_directories(${BOARD_DEMO_CONFIG_PATH})
include_directories(nxp_code)
include_directories(nxp_code/lwip)

file(GLOB NXPCODE_SOURCES nxp_code/*.c nxp_code/lwip/*.c)
set(PROJECT_SOURCES ${NXPCODE_SOURCES} main.c)

# configure modules
set(CONFIG_USE_driver_lpuart true)
set(MCUX_DEVICE "MIMXRT1062")

# Define
add_definitions(-DFSL_FEATURE_PHYKSZ8081_USE_RMII50M_MODE=1 -DSDK_DEBUGCONSOLE=1 -DSDK_DEBUGCONSOLE_UART -DLWIP_DHCP=1 -DLWIP_DNS=1 -DUSE_RTOS=1 -DFSL_RTOS_FREE_RTOS -DLWIP_TIMEVAL_PRIVATE=0 -D__STARTUP_CLEAR_BSS -D__STARTUP_INITIALIZE_NONCACHEDATA)

# Include sdk
include(all_devices)

include(driver_mdio-enet)

include(driver_phy-device-ksz8081)

# include modules
include(driver_igpio)

include(driver_clock)

include(driver_common)

include(device_CMSIS)

include(utility_debug_console)

include(component_lpuart_adapter)

include(component_serial_manager)

include(component_lists)

include(component_serial_manager_uart)

include(driver_lpuart)

include(device_startup)

include(driver_iomuxc)

include(utility_assert)

include(driver_xip_device)

include(driver_xip_board)

include(CMSIS_Include_core_cm7)

include(driver_mdio-common)

include(CMSIS_Include_common)

include(CMSIS_Include_dsp)

include(utilities_misc_utilities)

include(device_system)

include(driver_enet)

include(driver_trng)

include(driver_phy-common)

include(driver_dcp)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE
    FreeRTOS::Timers
    FreeRTOS::Heap::5
    FreeRTOS::ARM_CM4F
    FreeRTOS::EventGroups
    FreeRTOSPlus::Utilities::backoff_algorithm
    FreeRTOSPlus::Utilities::logging
    FreeRTOSPlus::ThirdParty::mbedtls
    az::iot_middleware::freertos
    LWIP
    SAMPLE::SOCKET::LWIP
    SAMPLE::AZUREIOT
    SAMPLE::TRANSPORT::MBEDTLS
    ${MCUX_SDK_PROJECT_NAME}
    )

add_custom_command(TARGET ${PROJECT_NAME}
    # Run after all other rules within the target have been executed
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMENT "Generate Bin file"
    VERBATIM)

add_executable(${PROJECT_NAME}-pnp ${PROJECT_SOURCES})
target_link_libraries(${PROJECT_NAME}-pnp PRIVATE
    FreeRTOS::Timers
    FreeRTOS::Heap::5
    FreeRTOS::ARM_CM4F
    FreeRTOS::EventGroups
    FreeRTOSPlus::Utilities::backoff_algorithm
    FreeRTOSPlus::Utilities::logging
    FreeRTOSPlus::ThirdParty::mbedtls
    az::iot_middleware::freertos
    LWIP
    SAMPLE::SOCKET::LWIP
    SAMPLE::AZUREIOTPNP
    SAMPLE::TRANSPORT::MBEDTLS
    ${MCUX_SDK_PROJECT_NAME}
    )

add_custom_command(TARGET ${PROJECT_NAME}-pnp
    # Run after all other rules within the target have been executed
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}-pnp> ${PROJECT_NAME}-pnp.bin
    COMMENT "Generate Bin file"
    VERBATIM)
