# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT
set(MCU_C_FLAGS -mcpu=cortex-m4
    -mfpu=fpv4-sp-d16 -mfloat-abi=hard
    -ffunction-sections CACHE INTERNAL "MCU build flags")
string (REPLACE ";" " " MCU_C_FLAGS_STR "${MCU_C_FLAGS}")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${MCU_C_FLAGS_STR})
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-Wl,--gc-sections,-print-memory-usage \
    -static -z muldefs -mthumb -Wl,--start-group -lc -lm -Wl,--end-group")

function(add_map_file TARGET_NAME MAP_FILE_NAME)
    target_link_options(${TARGET_NAME} PRIVATE -Wl,-Map=${MAP_FILE_NAME})
endfunction()
