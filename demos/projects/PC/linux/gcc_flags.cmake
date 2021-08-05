# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} " \
    -ffunction-sections \
    -fdata-sections \
    -Xlinker --gc-sections \
    -Xlinker -z -Xlinker muldefs")

function(add_map_file TARGET_NAME MAP_FILE_NAME)
    target_link_options(${TARGET_NAME} PRIVATE -Wl,-Map=${MAP_FILE_NAME})
endfunction()
