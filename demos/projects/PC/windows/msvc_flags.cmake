# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(MCU_C_FLAGS "/wd4127" /D_DEBUG
    /D_CRT_SECURE_NO_WARNINGS
    /DWIN32 CACHE INTERNAL "MCU build flags")
add_compile_options(
      $<$<CONFIG:>:/MT> #---------|
      $<$<CONFIG:Debug>:/MTd> #---|-- Statically link the runtime libraries
      $<$<CONFIG:Release>:/MT> #--|
      ${MCU_C_FLAGS})
add_link_options(/NODEFAULTLIB:libcmtd.lib)

function(add_map_file TARGET_NAME MAP_FILE_NAME)
    target_link_options(${TARGET_NAME} PRIVATE /MAP)
endfunction()
