# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

# set parent scope path
set(BOARD_DEMO_CONFIG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config CACHE INTERNAL "Config path")
set(BOARD_DEMO_PORT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/port CACHE INTERNAL "Port path")
set(BOARD_DEMO_FREERTOS_PORT_PATH ${FreeRTOS_ARM_CM4F_PATH} CACHE INTERNAL "FreeRTOS Port used ")

# Define
add_definitions(-DFSL_FEATURE_PHYKSZ8081_USE_RMII50M_MODE=1 -DSDK_DEBUGCONSOLE=1 -DSDK_DEBUGCONSOLE_UART -DLWIP_DHCP=1 -DLWIP_DNS=1 -DUSE_RTOS=1 -DFSL_RTOS_FREE_RTOS -DLWIP_TIMEVAL_PRIVATE=0 -D__STARTUP_CLEAR_BSS -D__STARTUP_INITIALIZE_NONCACHEDATA)
include(${CMAKE_CURRENT_SOURCE_DIR}/gcc_flags.cmake)

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
nxp_sfw_fetch()
lwip_fetch()

list(APPEND CMAKE_MODULE_PATH ${NXP_MCUX_SDK_PATH})
list(APPEND CMAKE_MODULE_PATH ${NXP_MCUX_SDK_PATH}/CMSIS/Include)
list(APPEND CMAKE_MODULE_PATH ${NXP_SFW_PATH})

find_package(LWIP)

include_directories(${BOARD_DEMO_CONFIG_PATH})
include_directories(${NXP_SFW_PATH}/target/evkmimxrt1060/board)
include_directories(${NXP_SFW_PATH}/target/evkmimxrt1060/board/MCUX_Config)
include_directories(${NXP_SFW_PATH}/component/lwip/port)
include_directories(port)

file(GLOB NXPCODE_SOURCES 
    ${NXP_SFW_PATH}/target/evkmimxrt1060/board/MCUX_Config/*.c
    ${NXP_SFW_PATH}/target/evkmimxrt1060/board/board.c
    ${NXP_SFW_PATH}/component/lwip/port/*.c)

file(GLOB NXPCODE_SOURCES_ADU 
    ${NXP_SFW_PATH}/target/evkmimxrt1060/board/flexspi_nor_flash_ops.c
    ${NXP_SFW_PATH}/firmware/ota_flag/sbl_ota_flag.c
    ${NXP_SFW_PATH}/component/flashiap/flexspi_flash.c)

file(GLOB NXPCODE_SOURCES_XIP
    ${NXP_MCUX_SDK_PATH}/boards/evkmimxrt1060/project_template/dcd.c)

set(PROJECT_SOURCES_XIP ${NXPCODE_SOURCES} ${NXPCODE_SOURCES_XIP} main.c)
set(PROJECT_SOURCES_ADU ${NXPCODE_SOURCES} ${NXPCODE_SOURCES_ADU} main.c)

# configure modules
set(CONFIG_USE_driver_lpuart true)
set(MCUX_DEVICE "MIMXRT1062")

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

include(driver_nor_flash-controller-flexspi)

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

include(driver_snvs_hp)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES_XIP})
target_include_directories(${PROJECT_NAME} PRIVATE ${NXP_MCUX_SDK_PATH}/boards/evkmimxrt1060/project_template)
add_linker(${PROJECT_NAME})
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
    SAMPLE::COMMON::CONNECTION
    SAMPLE::AZUREIOT
    SAMPLE::TRANSPORT::MBEDTLS
    ${MCUX_SDK_PROJECT_NAME}
    )

add_map_file(${PROJECT_NAME} ${PROJECT_NAME}.map)

add_custom_command(TARGET ${PROJECT_NAME}
    # Run after all other rules within the target have been executed
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMENT "Generate Bin file"
    VERBATIM)

add_executable(${PROJECT_NAME}-pnp ${PROJECT_SOURCES_XIP})
target_include_directories(${PROJECT_NAME}-pnp PRIVATE ${NXP_MCUX_SDK_PATH}/boards/evkmimxrt1060/project_template)
add_linker(${PROJECT_NAME}-pnp)
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
    SAMPLE::COMMON::CONNECTION
    SAMPLE::AZUREIOTPNP
    SAMPLE::TRANSPORT::MBEDTLS
    ${MCUX_SDK_PROJECT_NAME}
    )

add_map_file(${PROJECT_NAME}-pnp ${PROJECT_NAME}-pnp.map)

add_custom_command(TARGET ${PROJECT_NAME}-pnp
    # Run after all other rules within the target have been executed
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}-pnp> ${PROJECT_NAME}-pnp.bin
    COMMENT "Generate Bin file"
    VERBATIM)

add_executable(
  ${PROJECT_NAME}-adu
    ${PROJECT_SOURCES_ADU}
    ${CMAKE_CURRENT_LIST_DIR}/port/azure_iot_flash_platform.c
)
target_include_directories(${PROJECT_NAME}-adu PRIVATE ${NXP_SFW_PATH}/firmware/ota_flag)
target_include_directories(${PROJECT_NAME}-adu PRIVATE ${NXP_SFW_PATH}/firmware/include)
target_include_directories(${PROJECT_NAME}-adu PRIVATE ${NXP_SFW_PATH}/component/flashiap)
add_adu_linker(${PROJECT_NAME}-adu)
target_compile_definitions(${PROJECT_NAME}-adu PRIVATE -DADU_SAMPLE)
target_link_libraries(${PROJECT_NAME}-adu PRIVATE
    FreeRTOS::Timers
    FreeRTOS::Heap::5
    FreeRTOS::ARM_CM4F
    FreeRTOS::EventGroups
    FreeRTOSPlus::Utilities::backoff_algorithm
    FreeRTOSPlus::Utilities::logging
    FreeRTOSPlus::ThirdParty::mbedtls
    az::iot_middleware::freertos
    az::iot_middleware::core_http
    LWIP
    SAMPLE::SOCKET::LWIP
    SAMPLE::COMMON::CONNECTION
    SAMPLE::AZUREIOTADU
    SAMPLE::TRANSPORT::MBEDTLS
    ${MCUX_SDK_PROJECT_NAME}
    )

add_map_file(${PROJECT_NAME}-adu ${PROJECT_NAME}-adu.map)

add_custom_command(TARGET ${PROJECT_NAME}-adu
    # Run after all other rules within the target have been executed
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}-adu> ${PROJECT_NAME}-adu.bin
    COMMENT "Generate Bin file"
    VERBATIM)
