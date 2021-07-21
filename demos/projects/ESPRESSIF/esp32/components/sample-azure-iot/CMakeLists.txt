set(ROOT_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../..
)

file(GLOB_RECURSE COMPONENT_SOURCES
    ${ROOT_PATH}/demos/sample_azure_iot/*.c  
)

idf_component_get_property(LWIP_DIR lwip COMPONENT_DIR)
idf_component_get_property(MBEDTLS_DIR mbedtls COMPONENT_DIR)

list(APPEND COMPONENT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/backoff_algorithm.c
    ${CMAKE_CURRENT_LIST_DIR}/transport_tls_esp32.c
    ${CMAKE_CURRENT_LIST_DIR}/crypto_esp32.c
    ${CMAKE_CURRENT_LIST_DIR}/mbedtls_error.c
)

set(COMPONENT_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/../../config
    ${CMAKE_CURRENT_LIST_DIR}
    ${MBEDTLS_DIR}/mbedtls/include
    ${ROOT_PATH}/demos/common/transport
    ${ROOT_PATH}/demos/common/utilities
    ${LWIP_DIR}/port/esp32/freertos
)

idf_component_register(
    SRCS ${COMPONENT_SOURCES}
    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
    REQUIRES mbedtls tcp_transport coreMQTT azure-sdk-for-c azure-iot-middleware-freertos)

