#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
#
# ci_tests.sh [-st] [-nxp] [-pc] [-esp]
#
# if none specified all vendors are build

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

TEST_SCRIPT_DIR=`dirname "$0"`
TEST_FREERTOS_SRC=`pwd`/libs/FreeRTOS
RUN_BOARDS_BUILD=${@:-"-st -nxp -pc -esp"}
FREERTOS_FETCHED=0

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

    if [ $vendor == "ESPRESSIF" ]
    then
      idf.py build -C ./demos/projects/ESPRESSIF/$board
    else
      cmake -G Ninja -DBOARD=$board -DVENDOR=$vendor -B$outdir -DFREERTOS_PATH=$TEST_FREERTOS_SRC .
      cmake --build $outdir
    fi
}

function fetch_freertos()
{
    if [ $FREERTOS_FETCHED == 0 ]
    then
        $TEST_SCRIPT_DIR/fetch_freertos.sh $TEST_FREERTOS_SRC
        FREERTOS_FETCHED=1
    fi
}

for arg in $RUN_BOARDS_BUILD
do
    case "$arg" in
        "-esp")
            echo -e "::group::Building sample for ESPRESSIF ESP32 port"
            # sample_build "ESPRESSIF" "esp32" "build"
            # exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/esp32/build" "azure_iot_freertos_esp32.bin"
            # sample_build "ESPRESSIF" "aziotkit" "build"
            # exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/aziotkit/build" "azure_iot_freertos_esp32.bin"
            sample_build "ESPRESSIF" "adu" "build"
            exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/adu/build" "azure_iot_freertos_esp32.bin"
            ;;
        "-nxp")
            fetch_freertos

            echo -e "::group::Building sample for NXP mimxrt1060 port"
            sample_build "NXP" "mimxrt1060" "build_nxp_mimxrt1060"
            exit_if_binary_does_not_exist "build_nxp_mimxrt1060" "iot-middleware-sample.elf"
            exit_if_binary_does_not_exist "build_nxp_mimxrt1060" "iot-middleware-sample-pnp.elf"
            exit_if_binary_does_not_exist "build_nxp_mimxrt1060" "iot-middleware-sample-adu.elf"
            ;;
        "-st")
            # fetch_freertos

            echo -e "::group::Building sample for ST b-l475e-iot01a port"
            sample_build "ST" "b-l475e-iot01a" "build_st_b-l475e-iot01a"
            exit_if_binary_does_not_exist "build_st_b-l475e-iot01a" "iot-middleware-sample.elf"
            exit_if_binary_does_not_exist "build_st_b-l475e-iot01a" "iot-middleware-sample-pnp.elf"
            exit_if_binary_does_not_exist "build_st_b-l475e-iot01a" "iot-middleware-sample-gsg.elf"

            echo -e "::group::Building sample for ST stm32h745i-disco port"
            sample_build "ST" "stm32h745i-disco" "build_st_stm32h745i-disco"
            exit_if_binary_does_not_exist "build_st_stm32h745i-disco" "iot-middleware-sample.elf"
            exit_if_binary_does_not_exist "build_st_stm32h745i-disco" "iot-middleware-sample-pnp.elf"

            echo -e "::group::Building sample for ST b-l4s5i-iot01a port"
            sample_build "ST" "b-l4s5i-iot01a" "build_st_b-l4s5i-iot01a"
            exit_if_binary_does_not_exist "build_st_b-l4s5i-iot01a" "iot-middleware-sample.elf"
            exit_if_binary_does_not_exist "build_st_b-l4s5i-iot01a" "iot-middleware-sample-pnp.elf"
            ;;
        "-pc")
            fetch_freertos

            echo -e "::group::Building sample for linux port"
            sample_build "PC" "linux" "build_pc_linux"
            exit_if_binary_does_not_exist "build_pc_linux" "iot-middleware-sample"
            exit_if_binary_does_not_exist "build_pc_linux" "iot-middleware-sample-pnp"

            echo -e "::group::Running manifest verification tests"
            ./build_pc_linux/demos/projects/PC/linux/sample_adu_jws_mbedtls_ut
            ;;
        * )
            echo "build for $arg not found";;
    esac
done

