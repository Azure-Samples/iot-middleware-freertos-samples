# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} " \
    -ffunction-sections \
    -fdata-sections \
    -Xlinker --gc-sections \
    -Xlinker -z -Xlinker muldefs \
    -Wl,-Map=output.map ")
