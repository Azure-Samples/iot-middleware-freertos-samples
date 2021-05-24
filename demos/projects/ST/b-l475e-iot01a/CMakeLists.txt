stm32_fetch_cube(L4)

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} " -g -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard" PARENT_SCOPE)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections  -Xlinker -Map=output.map " CACHE INTERNAL "exe link flags" PARENT_SCOPE)

find_package(CMSIS COMPONENTS STM32L475VG REQUIRED)
find_package(HAL COMPONENTS STM32L475VG REQUIRED)
find_package(BSP COMPONENTS STM32L475E-IOT01 REQUIRED)

set(DEMO_SOURCES
    ../../../sample_azure_iot/sample_azure_iot_task.c
    ../../../common/utilities/crypto_using_mbedtls.c
    ../../../common/utilities/mbedtls_freertos_port.c
    ../../../common/transport/transport_tls_socket_using_mbedtls.c
)

file(GLOB STCODE_SOURCES st_code/*.c)
set(PROJECT_SOURCES
    ${STCODE_SOURCES}
    ${DEMO_SOURCES}
    port/sockets_wrapper_stm32l475.c
    main.c
)

stm32_add_linker_script(CMSIS::STM32::L4 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VGTx_FLASH.ld")
add_executable(iot-middleware-sample ${PROJECT_SOURCES})
target_include_directories(iot-middleware-sample PUBLIC
    config st_code .
    ../../../common/transport/
    ../../../common/utilities/)
target_link_libraries(iot-middleware-sample PRIVATE
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
)