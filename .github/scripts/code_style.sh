#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

usage() {
    echo "${0} [check|fix]" 1>&2
    exit 1
}

FIX=${1:-""}

# Version 0.67 is the source of truth
if ! [ -x "$(command -v uncrustify)" ]; then
    tmp_dir=$(mktemp -d -t uncrustify-XXXX)
    pushd $tmp_dir
    git clone https://github.com/uncrustify/uncrustify.git
    cd uncrustify
    git checkout uncrustify-0.67
    mkdir build
    cd build
    cmake ..
    cmake --build .
    sudo make install
    popd
fi

if [[ "$FIX" == "check" ]]; then
    RESULT=$(uncrustify -c ./uncrustify.cfg --check           \
    ./demos/sample_azure_iot/*.c                              \
    ./demos/sample_azure_iot_adu/*.c                          \
    ./demos/sample_azure_iot_adu/*.h                          \
    ./demos/sample_azure_iot_pnp/*.c                          \
    ./demos/sample_azure_iot_pnp/*.h                          \
    ./demos/sample_azure_iot_gsg/*.c                          \
    ./demos/sample_azure_iot_gsg/*.h                          \
    ./demos/common/transport/*.c                              \
    ./demos/common/transport/*.h                              \
    ./demos/common/utilities/*.c                              \
    ./demos/common/utilities/*.h                              \
    ./demos/projects/ESPRESSIF/adu/config/*.h                 \
    ./demos/projects/ESPRESSIF/adu/port/*.c                   \
    ./demos/projects/ESPRESSIF/adu/port/*.h                   \
    ./demos/projects/ESPRESSIF/adu/main/*.c                   \
    ./demos/projects/ESPRESSIF/aziotkit/config/*.h            \
    ./demos/projects/ESPRESSIF/aziotkit/main/*.c              \
    ./demos/projects/ESPRESSIF/aziotkit/main/*.h              \
    ./demos/projects/ESPRESSIF/esp32/config/*.h               \
    ./demos/projects/ESPRESSIF/esp32/main/*.c                 \
    ./demos/projects/NXP/mimxrt1060/config/*.h                \
    ./demos/projects/NXP/mimxrt1060/port/*.c                  \
    ./demos/projects/NXP/mimxrt1060/port/*.h                  \
    ./demos/projects/NXP/mimxrt1060/*.c                       \
    ./demos/projects/PC/linux/config/*.h                      \
    ./demos/projects/PC/linux/port/*.c                        \
    ./demos/projects/PC/linux/port/*.h                        \
    ./demos/projects/PC/linux/tests/*.c                       \
    ./demos/projects/PC/linux/*.c                             \
    ./demos/projects/PC/linux/*.h                             \
    ./demos/projects/PC/windows/config/*.h                    \
    ./demos/projects/PC/windows/*.c                           \
    ./demos/projects/ST/b-l4s5i-iot01a/config/*.h             \
    ./demos/projects/ST/b-l475e-iot01a/port/*.c               \
    ./demos/projects/ST/b-l475e-iot01a/port/*.h               \
    ./demos/projects/ST/b-l475e-iot01a/config/*.h             \
    ./demos/projects/ST/b-l475e-iot01a/*.c                    \
    ./demos/projects/ST/b-l475e-iot01a/*.h                    \
    ./demos/projects/ST/stm32h745i-disco/cm7/config/*.h       \
    ./demos/projects/ST/stm32h745i-disco/cm7/port/*.c         \
    ./demos/projects/ST/stm32h745i-disco/cm7/port/*.h         \
    ./demos/projects/ST/stm32h745i-disco/cm7/*.c              \
    ./demos/projects/ST/stm32h745i-disco/cm7/*.h)

    if [ $? -ne 0 ]; then
      echo $RESULT | grep "FAIL"
      exit 1
    fi
elif [[ "$FIX" == "fix" ]]; then
    uncrustify -c ./uncrustify.cfg --no-backup --replace      \
    ./demos/sample_azure_iot/*.c                              \
    ./demos/sample_azure_iot_adu/*.c                          \
    ./demos/sample_azure_iot_adu/*.h                          \
    ./demos/sample_azure_iot_pnp/*.c                          \
    ./demos/sample_azure_iot_pnp/*.h                          \
    ./demos/sample_azure_iot_gsg/*.c                          \
    ./demos/sample_azure_iot_gsg/*.h                          \
    ./demos/common/transport/*.c                              \
    ./demos/common/transport/*.h                              \
    ./demos/common/utilities/*.c                              \
    ./demos/common/utilities/*.h                              \
    ./demos/projects/ESPRESSIF/adu/config/*.h                 \
    ./demos/projects/ESPRESSIF/adu/port/*.c                   \
    ./demos/projects/ESPRESSIF/adu/port/*.h                   \
    ./demos/projects/ESPRESSIF/adu/main/*.c                   \
    ./demos/projects/ESPRESSIF/aziotkit/config/*.h            \
    ./demos/projects/ESPRESSIF/aziotkit/main/*.c              \
    ./demos/projects/ESPRESSIF/aziotkit/main/*.h              \
    ./demos/projects/ESPRESSIF/esp32/config/*.h               \
    ./demos/projects/ESPRESSIF/esp32/main/*.c                 \
    ./demos/projects/NXP/mimxrt1060/config/*.h                \
    ./demos/projects/NXP/mimxrt1060/port/*.c                  \
    ./demos/projects/NXP/mimxrt1060/port/*.h                  \
    ./demos/projects/NXP/mimxrt1060/*.c                       \
    ./demos/projects/PC/linux/config/*.h                      \
    ./demos/projects/PC/linux/port/*.c                        \
    ./demos/projects/PC/linux/port/*.h                        \
    ./demos/projects/PC/linux/tests/*.c                       \
    ./demos/projects/PC/linux/*.c                             \
    ./demos/projects/PC/linux/*.h                             \
    ./demos/projects/PC/windows/config/*.h                    \
    ./demos/projects/PC/windows/*.c                           \
    ./demos/projects/ST/b-l4s5i-iot01a/config/*.h             \
    ./demos/projects/ST/b-l475e-iot01a/port/*.c               \
    ./demos/projects/ST/b-l475e-iot01a/port/*.h               \
    ./demos/projects/ST/b-l475e-iot01a/config/*.h             \
    ./demos/projects/ST/b-l475e-iot01a/*.c                    \
    ./demos/projects/ST/b-l475e-iot01a/*.h                    \
    ./demos/projects/ST/stm32h745i-disco/cm7/config/*.h       \
    ./demos/projects/ST/stm32h745i-disco/cm7/port/*.c         \
    ./demos/projects/ST/stm32h745i-disco/cm7/port/*.h         \
    ./demos/projects/ST/stm32h745i-disco/cm7/*.c              \
    ./demos/projects/ST/stm32h745i-disco/cm7/*.h
else
    usage
fi
