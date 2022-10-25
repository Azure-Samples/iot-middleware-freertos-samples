# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

stm32_fetch_cube(L4)

find_package(CMSIS COMPONENTS STM32L475VG REQUIRED)
find_package(HAL COMPONENTS STM32L475VG REQUIRED)
find_package(BSP COMPONENTS STM32L475E-IOT01 REQUIRED)

# set parent scope path
set(BOARD_DEMO_CONFIG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config CACHE INTERNAL "Config path")
set(BOARD_DEMO_PORT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/port CACHE INTERNAL "Port path")
set(BOARD_DEMO_FREERTOS_PORT_PATH ${FreeRTOS_ARM_CM4F_PATH} CACHE INTERNAL "FreeRTOS Port used ")

include(${CMAKE_CURRENT_SOURCE_DIR}/gcc_flags.cmake)

include_directories(${BOARD_DEMO_CONFIG_PATH})
include_directories(port)

file(GLOB STCODE_SOURCES st_code/*.c)
set(PROJECT_SOURCES
    ${STCODE_SOURCES}
    port/sockets_wrapper_stm32l475.c
    sample_gsg_device.c
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
    HAL::STM32::L4::QSPI
    HAL::STM32::L4::I2C
    HAL::STM32::L4::I2CEx
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
    BSP::STM32::L4::LSM6DSL
    BSP::STM32::L4::HTS221
    BSP::STM32::L4::LIS3MDL
    BSP::STM32::L4::LPS22HB
    STM32::NoSys
    az::iot_middleware::freertos
    SAMPLE::AZUREIOT
    SAMPLE::TRANSPORT::MBEDTLS)

add_map_file(${PROJECT_NAME} ${PROJECT_NAME}.map)

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
    HAL::STM32::L4::QSPI
    HAL::STM32::L4::I2C
    HAL::STM32::L4::I2CEx
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
    BSP::STM32::L4::LSM6DSL
    BSP::STM32::L4::HTS221
    BSP::STM32::L4::LIS3MDL
    BSP::STM32::L4::LPS22HB
    STM32::Nano
    STM32::Nano::FloatScan
    STM32::Nano::FloatPrint
    az::iot_middleware::freertos
    SAMPLE::AZUREIOTPNP
    SAMPLE::TRANSPORT::MBEDTLS)

add_map_file(${PROJECT_NAME}-pnp ${PROJECT_NAME}-pnp.map)

add_custom_command(TARGET ${PROJECT_NAME}-pnp
    # Run after all other rules within the target have been executed
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}-pnp> ${PROJECT_NAME}-pnp.bin
    COMMENT "Generate Bin file"
    VERBATIM)

# Add GSG Sample
add_executable(${PROJECT_NAME}-gsg ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME}-gsg PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    st_code)
target_link_libraries(${PROJECT_NAME}-gsg PRIVATE
    FreeRTOS::Timers
    FreeRTOS::Heap::5
    FreeRTOS::ARM_CM4F
    FreeRTOSPlus::Utilities::backoff_algorithm
    FreeRTOSPlus::Utilities::logging
    FreeRTOSPlus::ThirdParty::mbedtls
    HAL::STM32::L4::RCC
    HAL::STM32::L4::RCCEx
    HAL::STM32::L4::SPI
    HAL::STM32::L4::QSPI
    HAL::STM32::L4::I2C
    HAL::STM32::L4::I2CEx
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
    BSP::STM32::L4::LSM6DSL
    BSP::STM32::L4::HTS221
    BSP::STM32::L4::LIS3MDL
    BSP::STM32::L4::LPS22HB
    STM32::Nano
    STM32::Nano::FloatScan
    STM32::Nano::FloatPrint
    az::iot_middleware::freertos
    SAMPLE::AZUREIOTGSG
    SAMPLE::TRANSPORT::MBEDTLS)

add_custom_command(TARGET ${PROJECT_NAME}-gsg
    # Run after all other rules within the target have been executed
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}-gsg> ${PROJECT_NAME}-gsg.bin
    COMMENT "Generate Bin file"
    VERBATIM)

add_executable(
    ${PROJECT_NAME}-adu
        ${PROJECT_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/port/azure_iot_flash_platform.c
    )
target_include_directories(${PROJECT_NAME}-adu PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    st_code)
target_link_libraries(${PROJECT_NAME}-adu PRIVATE
    FreeRTOS::Timers
    FreeRTOS::Heap::5
    FreeRTOS::ARM_CM4F
    FreeRTOSPlus::Utilities::backoff_algorithm
    FreeRTOSPlus::Utilities::logging
    FreeRTOSPlus::ThirdParty::mbedtls
    HAL::STM32::L4::RCC
    HAL::STM32::L4::RCCEx
    HAL::STM32::L4::SPI
    HAL::STM32::L4::QSPI
    HAL::STM32::L4::I2C
    HAL::STM32::L4::I2CEx
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
    HAL::STM32::L4::FLASH
    HAL::STM32::L4::FLASHEx
    CMSIS::STM32::L475xx
    BSP::STM32::STM32L475E_IOT01
    BSP::STM32::L4::LSM6DSL
    BSP::STM32::L4::HTS221
    BSP::STM32::L4::LIS3MDL
    BSP::STM32::L4::LPS22HB
    STM32::Nano
    STM32::Nano::FloatScan
    STM32::Nano::FloatPrint
    az::iot_middleware::freertos
    az::iot_middleware::core_http
    SAMPLE::AZUREIOTADU
    SAMPLE::TRANSPORT::MBEDTLS
    )

add_map_file(${PROJECT_NAME}-adu ${PROJECT_NAME}-adu.map)

add_custom_command(TARGET ${PROJECT_NAME}-adu
    # Run after all other rules within the target have been executed
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}-adu> ${PROJECT_NAME}-adu.bin
    COMMENT "Generate Bin file"
    VERBATIM)
