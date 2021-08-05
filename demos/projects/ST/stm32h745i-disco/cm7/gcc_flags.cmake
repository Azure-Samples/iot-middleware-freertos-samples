# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(MCU_C_FLAGS -mcpu=cortex-m7
    -mfpu=fpv5-sp-d16 -mfloat-abi=hard CACHE INTERNAL "MCU build flags")
string (REPLACE ";" " " MCU_C_FLAGS_STR "${MCU_C_FLAGS}")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${MCU_C_FLAGS_STR})

function(add_map_file TARGET_NAME MAP_FILE_NAME)
    target_link_options(${TARGET_NAME} PRIVATE -Wl,-Map=${MAP_FILE_NAME})
endfunction()
