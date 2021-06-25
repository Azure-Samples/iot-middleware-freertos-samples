#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

TEST_SCRIPT_DIR=`dirname "$0"`
TEST_FREERTOS_SRC=`pwd`/libs/FreeRTOS
$TEST_SCRIPT_DIR/fetch_freertos.sh $TEST_FREERTOS_SRC

function exit_if_binary_does_not_exist()
{
    local search_dir=$1
    local filename=$2

    if [ $(find "$search_dir" -name "$filename") ]; then 
      echo "$filename is found in $search_dir"
    else
      echo "$filename not found"
      exit 1
    fi
}

function sample_build() {
    local vendor=$1
    local board=$2
    local outdir=$3

    cmake -G Ninja -DBOARD=$board -DVENDOR=$vendor -B$outdir -DFREERTOS_PATH=$TEST_FREERTOS_SRC .
    cmake --build $outdir
}

######################## Simulations #######################

echo -e "::group::Building sample for linux port"
sample_build "PC" "linux" "build_pc_linux"
exit_if_binary_does_not_exist "build_pc_linux" "iot-middleware-sample"

######################## ST Boards #########################

echo -e "::group::Building sample for ST b-l475e-iot01a port"
sample_build "ST" "b-l475e-iot01a" "build_st_b-l475e-iot01a"
exit_if_binary_does_not_exist "build_st_b-l475e-iot01a" "iot-middleware-sample.elf"

echo -e "::group::Building sample for ST stm32h745i_discovery port"
sample_build "ST" "stm32h745i_discovery" "build_st_stm32h745i_discovery"
exit_if_binary_does_not_exist "build_st_stm32h745i_discovery" "iot-middleware-sample.elf"

######################## NXP Boards ########################

echo -e "::group::Building sample for NXP mimxrt1060 port"
sample_build "NXP" "mimxrt1060" "build_nxp_mimxrt1060"
exit_if_binary_does_not_exist "build_nxp_mimxrt1060" "iot-middleware-sample.elf"
