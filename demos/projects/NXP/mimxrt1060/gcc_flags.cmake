# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(MCU_C_FLAGS -mcpu=cortex-m7
    -mfloat-abi=hard
    -mfpu=fpv5-d16
    -mthumb -MMD -MP
    -fno-common
    -ffunction-sections
    -fdata-sections
    -ffreestanding
    -fno-builtin
    -mapcs -std=gnu99 CACHE INTERNAL "MCU build flags")
string (REPLACE ";" " " MCU_C_FLAGS_STR "${MCU_C_FLAGS}")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} " \
    -DMIMXRT1060 \
    -DXIP_EXTERNAL_FLASH=1 \
    -DSERIAL_PORT_TYPE_UART=1 \
    -DXIP_BOOT_HEADER_ENABLE=1 \
    -DXIP_BOOT_HEADER_DCD_ENABLE=1 \
    -DSKIP_SYSCLK_INIT \
    -DDATA_SECTION_IS_CACHEABLE=1 \
    -DCPU_MIMXRT1062DVL6A \
    -DPRINTF_FLOAT_ENABLE=0 \
    -DSCANF_FLOAT_ENABLE=0 \
    -DPRINTF_ADVANCED_ENABLE=1\
    -DCR_INTEGER_PRINTF \
    -DSCANF_ADVANCED_ENABLE=0 ${MCU_C_FLAGS_STR}")

set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} " \
    -fno-exceptions \
    -fno-rtti \
    -mcpu=cortex-m7 \
    -Wall -mfloat-abi=hard \
    -mfpu=fpv5-d16 \
    --specs=nano.specs \
    --specs=nosys.specs \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -ffreestanding \
    -fno-builtin -mthumb -mapcs \
    -Xlinker --gc-sections \ 
    -Xlinker -static \ 
    -Xlinker -z -Xlinker muldefs \
    -T${CMAKE_CURRENT_SOURCE_DIR}/MIMXRT1062xxxxx_sdram.ld \
    -static -Wl,-Map=output.map ")
