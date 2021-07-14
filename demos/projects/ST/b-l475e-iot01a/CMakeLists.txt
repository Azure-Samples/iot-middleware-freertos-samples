# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

stm32_fetch_cube(L4)

find_package(CMSIS COMPONENTS STM32L475VG REQUIRED)
find_package(HAL COMPONENTS STM32L475VG REQUIRED)
find_package(BSP COMPONENTS STM32L475E-IOT01 REQUIRED)

# set parent scope path
set(BOARD_DEMO_CONFIG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config CACHE INTERNAL "Config path")
set(BOARD_DEMO_FREERTOS_PORT_PATH ${FreeRTOS_ARM_CM4F_PATH} CACHE INTERNAL "FreeRTOS Port used ")

include(${CMAKE_CURRENT_SOURCE_DIR}/gcc_flags.cmake)

include_directories(${BOARD_DEMO_CONFIG_PATH})

file(GLOB STCODE_SOURCES st_code/*.c)
set(PROJECT_SOURCES
    ${STCODE_SOURCES}
    port/sockets_wrapper_stm32l475.c
    main.c)

stm32_add_linker_script(CMSIS::STM32::L4 INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VGTx_FLASH.ld")
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    st_code)
target_link_libraries(${PROJECT_NAME} PRIVATE
    FreeRTOS::Timers
    FreeRTOS::Heap::5
    FreeRTOS::ARM_CM4F
    FreeRTOSPlus::Utilities::backoff_algorithm
    FreeRTOSPlus::Utilities::logging
    FreeRTOSPlus::ThirdParty::mbedtls
    HAL::STM32::L4::RCC
    HAL::STM32::L4::RCCEx
    HAL::STM32::L4::SPI
    HAL::STM32::L4::RTC
    HAL::STM32::L4::UART
    HAL::STM32::L4::DMA
    HAL::STM32::L4::PWR
    HAL::STM32::L4::PWREx
    HAL::STM32::L4::GPIO
    HAL::STM32::L4::CORTEX
    HAL::STM32::L4::RNG
    HAL::STM32::L4::TIM
    HAL::STM32::L4::TIMEx
    CMSIS::STM32::L475xx
    BSP::STM32::STM32L475E_IOT01
    STM32::NoSys
    az::iot_middleware::freertos
    SAMPLE::AZUREIOT
    SAMPLE::TRANSPORT::MBEDTLS)

add_custom_command(TARGET ${PROJECT_NAME}
    # Run after all other rules within the target have been executed
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMENT "Generate Bin file"
    VERBATIM)

# Add PnP Sample
add_executable(${PROJECT_NAME}-pnp ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME}-pnp PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    st_code)
target_link_libraries(${PROJECT_NAME}-pnp PRIVATE
    FreeRTOS::Timers
    FreeRTOS::Heap::5
    FreeRTOS::ARM_CM4F
    FreeRTOSPlus::Utilities::backoff_algorithm
    FreeRTOSPlus::Utilities::logging
    FreeRTOSPlus::ThirdParty::mbedtls
    HAL::STM32::L4::RCC
    HAL::STM32::L4::RCCEx
    HAL::STM32::L4::SPI
    HAL::STM32::L4::RTC
    HAL::STM32::L4::UART
    HAL::STM32::L4::DMA
    HAL::STM32::L4::PWR
    HAL::STM32::L4::PWREx
    HAL::STM32::L4::GPIO
    HAL::STM32::L4::CORTEX
    HAL::STM32::L4::RNG
    HAL::STM32::L4::TIM
    HAL::STM32::L4::TIMEx
    CMSIS::STM32::L475xx
    BSP::STM32::STM32L475E_IOT01
    STM32::Nano
    STM32::Nano::FloatScan
    STM32::Nano::FloatPrint
    az::iot_middleware::freertos
    SAMPLE::AZUREIOTPNP
    SAMPLE::TRANSPORT::MBEDTLS)

add_custom_command(TARGET ${PROJECT_NAME}-pnp
    # Run after all other rules within the target have been executed
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}-pnp> ${PROJECT_NAME}-pnp.bin
    COMMENT "Generate Bin file"
    VERBATIM)
