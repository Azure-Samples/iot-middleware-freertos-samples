
if(NOT LWIP_PATH)
    set(LWIP_PATH /opt/LWIP CACHE PATH "Path to LWIP")
    message(STATUS "No LWIP_PATH specified using default: ${LWIP_PATH}")
endif()

if(NOT (TARGET LWIP))
    add_library(LWIP INTERFACE IMPORTED)
    file(GLOB LWIP_PATH_SOURCES ${LWIP_PATH}/src/api/*.c
            ${LWIP_PATH}/src/core/ipv4/*.c
            ${LWIP_PATH}/src/core/*.c
            ${LWIP_PATH}/src/netif/*.c
            ${LWIP_PATH}/src/apps/sntp/*.c)

    target_sources(LWIP INTERFACE ${LWIP_PATH_SOURCES})
    target_include_directories(LWIP INTERFACE
        ${LWIP_PATH}/src/include)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LWIP
    REQUIRED_VARS LWIP_PATH
    FOUND_VAR LWIP_FOUND
    HANDLE_COMPONENTS
)
