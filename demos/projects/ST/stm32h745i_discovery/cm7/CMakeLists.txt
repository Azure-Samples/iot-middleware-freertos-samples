# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

stm32_fetch_cube(H7)

set(LWIP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../libs/lwip)
set(LWIP_PATH_OS ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../libs/lwip_freertos)

find_package(CMSIS COMPONENTS STM32H7 STM32H7_M7 REQUIRED)
find_package(HAL COMPONENTS STM32H7_M7 REQUIRED)
find_package(BSP COMPONENTS STM32H745XI_M7 REQUIRED)
find_package(LWIP)

set(MCU_C_FLAGS "-mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard" CACHE INTERNAL "MCU build flags")
set(MCU_LINKER_FLAGS "-Wl,--gc-sections -Xlinker -Map=output.map" CACHE INTERNAL "MCU build flags")

# set parent scope path
set(BOARD_DEMO_CONFIG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config CACHE INTERNAL "Config path")
set(BOARD_DEMO_FREERTOS_PORT_PATH ${FreeRTOS_ARM_CM7_PATH} CACHE INTERNAL "FreeRTOS Port used ")

include_directories(${BOARD_DEMO_CONFIG_PATH})

file(GLOB STCODE_SOURCES st_code/*.c)
file(GLOB LWIP_PORT_SOURCES
    st_code/lwip/App/*.c
    st_code/lwip/Target/*.c
    ../../../../common/transport/sockets_wrapper_lwip.c)

set(PROJECT_SOURCES
    ${SAMPLE_AZURE_IOT_SOURCE_PATH}
    ${SAMPLE_TRANSPORT_USING_MBEDTLS_SOURCE_PATH}
    ${LWIP_PORT_SOURCES}
    ${STCODE_SOURCES}
    main.c)

stm32_add_linker_script(CMSIS::STM32::H7::M7 INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/STM32H745XIHX_FLASH.ld")
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC
    config st_code .
    ${SAMPLE_TRANSPORT_USING_MBEDTLS_INC_PATH}
    st_code/lwip/App
    st_code/lwip/Target)
target_link_libraries(${PROJECT_NAME} PRIVATE
    FreeRTOS::Timers
    FreeRTOS::Heap::5
    FreeRTOS::ARM_CM7
    FreeRTOSPlus::Utilities::backoff_algorithm
    FreeRTOSPlus::Utilities::logging
    FreeRTOSPlus::ThirdParty::mbedtls
    HAL::STM32::H7::M7::RCC
    HAL::STM32::H7::M7::RCCEx
    HAL::STM32::H7::M7::SPI
    HAL::STM32::H7::M7::RTC
    HAL::STM32::H7::M7::UART
    HAL::STM32::H7::M7::DMA
    HAL::STM32::H7::M7::PWR
    HAL::STM32::H7::M7::PWREx
    HAL::STM32::H7::M7::GPIO
    HAL::STM32::H7::M7::CORTEX
    HAL::STM32::H7::M7::RNG
    HAL::STM32::H7::M7::TIM
    HAL::STM32::H7::M7::TIMEx
    HAL::STM32::H7::M7::UARTEx
    CMSIS::STM32::H745XI::M7
    STM32::NoSys
    az::iot_middleware::freertos
    LWIP
    HAL::STM32::H7::M7::ETH
    BSP::STM32::H7::M7::LAN8742)

target_link_options(${PROJECT_NAME} PRIVATE -Xlinker -Map=output.map)

add_custom_command(TARGET ${PROJECT_NAME}
    # Run after all other rules within the target have been executed
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMENT "Generate Bin file"
    VERBATIM)
