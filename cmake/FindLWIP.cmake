
if(NOT LWIP_PATH)
    set(LWIP_PATH /opt/LWIP CACHE PATH "Path to LWIP")
    message(STATUS "No LWIP_PATH specified using default: ${LWIP_PATH}")
endif()
if(NOT LWIP_PATH_OS)
    set(LWIP_PATH_OS /opt/LWIP/System CACHE PATH "Path to LWIP System")
    message(STATUS "No LWIP_PATH_OS specified using default: ${LWIP_PATH_OS}")
endif()

if(NOT (TARGET LWIP))
    add_library(LWIP INTERFACE IMPORTED)
    file(GLOB LWIP_PATH_SOURCES ${LWIP_PATH}/src/api/*.c
            ${LWIP_PATH}/src/core/ipv4/*.c
            ${LWIP_PATH}/src/core/*.c
            ${LWIP_PATH}/src/netif/*.c)
    file(GLOB LWIP_PATH_OS_SOURCES ${LWIP_PATH_OS}/src/*.c)

    target_sources(LWIP INTERFACE ${LWIP_PATH_SOURCES}
                ${LWIP_PATH_OS_SOURCES})
    target_include_directories(LWIP INTERFACE
        ${LWIP_PATH}/src/include
        ${LWIP_PATH_OS}/include)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LWIP
    REQUIRED_VARS LWIP_PATH_OS
    FOUND_VAR LWIP_FOUND
    HANDLE_COMPONENTS
)
