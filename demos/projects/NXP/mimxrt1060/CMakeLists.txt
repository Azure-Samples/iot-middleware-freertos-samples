# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(MCUX_SDK_PROJECT_NAME ${PROJECT_NAME})

if(NOT NXP_MCUX_SDK_PATH)
    set(NXP_MCUX_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../../libs/mcux-sdk)
    message(INFO " NXP_MCUX_SDK_PATH not specified setting to default ${NXP_MCUX_SDK_PATH}")
endif()
list(APPEND CMAKE_MODULE_PATH ${NXP_MCUX_SDK_PATH})

set(LWIP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../../libs/lwip)

# CMAKE_C_FLAGS_FLEXSPI_NOR_SDRAM_DEBUG
set(MCU_C_FLAGS " -DMIMXRT1060 -D__REDLIB__ -DXIP_EXTERNAL_FLASH=1 -DSERIAL_PORT_TYPE_UART=1  -DXIP_BOOT_HEADER_ENABLE=1 -DXIP_BOOT_HEADER_DCD_ENABLE=1 -DSKIP_SYSCLK_INIT -DDATA_SECTION_IS_CACHEABLE=1 -DCPU_MIMXRT1062DVL6A -DPRINTF_FLOAT_ENABLE=0 -DSCANF_FLOAT_ENABLE=0 -DPRINTF_ADVANCED_ENABLE=1 -DCR_INTEGER_PRINTF -DSCANF_ADVANCED_ENABLE=0 -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16 -mthumb -MMD -MP -fno-common  -ffunction-sections  -fdata-sections -ffreestanding  -fno-builtin  -mapcs -std=gnu99" CACHE INTERNAL "MCU build flags")
#set(MCU_C_FLAGS "-DCPU_MIMXRT1062DVL6A  -DSERIAL_PORT_TYPE_UART=1 -mcpu=cortex-m7 -Wall -mfloat-abi=hard -mfpu=fpv5-d16 -mthumb -MMD -MP -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mapcs -std=gnu99")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${MCU_C_FLAGS})

# set parent scope path
set(BOARD_DEMO_CONFIG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config CACHE INTERNAL "Config path")
set(BOARD_DEMO_FREERTOS_PORT_PATH ${FreeRTOS_ARM_CM4F_PATH} CACHE INTERNAL "FreeRTOS Port used ")

find_package(LWIP)

include_directories(${BOARD_DEMO_CONFIG_PATH})
include_directories(nxp_code)
include_directories(nxp_code/lwip)

file(GLOB NXPCODE_SOURCES nxp_code/*.c nxp_code/lwip/*.c)
set(PROJECT_SOURCES
    ${SAMPLE_AZURE_IOT_SOURCE_PATH}
    ${SAMPLE_TRANSPORT_USING_MBEDTLS_SOURCE_PATH}
    ${NXPCODE_SOURCES}
    ../../../common/transport/sockets_wrapper_lwip.c
    main.c)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# configure modules
set(CONFIG_USE_driver_lpuart true)
set(MCUX_DEVICE "MIMXRT1062")

# Define
add_definitions(-DFSL_FEATURE_PHYKSZ8081_USE_RMII50M_MODE=1 -DSDK_DEBUGCONSOLE=1 -DSDK_DEBUGCONSOLE_UART -DLWIP_DHCP=1 -DLWIP_DNS=1 -DUSE_RTOS=1 -DFSL_RTOS_FREE_RTOS -DLWIP_TIMEVAL_PRIVATE=0 -D__STARTUP_CLEAR_BSS -D__STARTUP_INITIALIZE_NONCACHEDATA  )

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

target_include_directories(${PROJECT_NAME} PUBLIC
    config . nxp_code/lwip
    ${SAMPLE_TRANSPORT_USING_MBEDTLS_INC_PATH})
target_link_libraries(${PROJECT_NAME} PRIVATE
    FreeRTOS::Timers
    FreeRTOS::Heap::5
    FreeRTOS::ARM_CM4F
    FreeRTOS::EventGroups
    FreeRTOSPlus::Utilities::backoff_algorithm
    FreeRTOSPlus::Utilities::logging
    FreeRTOSPlus::ThirdParty::mbedtls
    az::iot_middleware::freertos
    LWIP)

target_link_options(${PROJECT_NAME} PRIVATE -fno-exceptions -fno-rtti -mcpu=cortex-m7 -Wall -mfloat-abi=hard -mfpu=fpv5-d16 --specs=nano.specs --specs=nosys.specs -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mthumb -mapcs -Xlinker --gc-sections -Xlinker -static -Xlinker -z -Xlinker muldefs -T${CMAKE_CURRENT_SOURCE_DIR}/MIMXRT1062xxxxx_sdram.ld -static -Wl,-Map=output.map)

add_custom_command(TARGET ${PROJECT_NAME}
    # Run after all other rules within the target have been executed
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMENT "Generate Bin file"
    VERBATIM)
