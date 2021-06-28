# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(MCU_C_FLAGS -mcpu=cortex-m7
    -mfpu=fpv5-sp-d16 -mfloat-abi=hard CACHE INTERNAL "MCU build flags")
string (REPLACE ";" " " MCU_C_FLAGS_STR "${MCU_C_FLAGS}")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${MCU_C_FLAGS_STR})

set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-Wl,-Map=output.map")
