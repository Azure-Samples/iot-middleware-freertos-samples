set(MCU_C_FLAGS "-m32" CACHE INTERNAL "MCU build flags")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MCU_C_FLAGS}")

# set parent scope path
set(BOARD_DEMO_CONFIG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config CACHE INTERNAL "Config path")
set(BOARD_DEMO_FREERTOS_PORT_PATH ${FreeRTOS_Posix_PATH} CACHE INTERNAL "FreeRTOS Port used ")

include_directories(${BOARD_DEMO_CONFIG_PATH})

set(FREERTOS_TCP_PORT_SOURCES
    ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/portable/BufferManagement/BufferAllocation_2.c
    ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux/NetworkInterface.c
    ../../../common/transport/sockets_wrapper_freertos_tcpip.c)

set(PROJECT_SOURCES
    ${FREERTOS_TCP_PORT_SOURCES}
    ${SAMPLE_AZURE_IOT_SOURCE_PATH}
    ${SAMPLE_TRANSPORT_USING_MBEDTLS_SOURCE_PATH}
    main.c)

add_executable(iot-middleware-sample ${PROJECT_SOURCES})
target_include_directories(iot-middleware-sample PUBLIC
    ${SAMPLE_TRANSPORT_USING_MBEDTLS_INC_PATH}
    ${FreeRTOSPlus_PATH}/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux/)
target_link_libraries(iot-middleware-sample PRIVATE
    FreeRTOS::Timers
    FreeRTOS::Heap::3
    FreeRTOS::EventGroups
    FreeRTOS::Posix
    FreeRTOSPlus::Utilities::backoff_algorithm
    FreeRTOSPlus::Utilities::logging
    FreeRTOSPlus::ThirdParty::mbedtls
    FreeRTOSPlus::TCPIP
    az::iot_middleware::freertos
    pthread
    pcap)
target_link_options(iot-middleware-sample PRIVATE -Xlinker -Map=output.map)