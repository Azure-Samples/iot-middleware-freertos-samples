# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(MCU_C_FLAGS " \
    -mcpu=cortex-m4 \
    -mfpu=fpv4-sp-d16 \
    -mfloat-abi=hard " CACHE INTERNAL "MCU build flags")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${MCU_C_FLAGS})

set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-Wl,-Map=output.map")
