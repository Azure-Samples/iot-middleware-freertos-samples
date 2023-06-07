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
RUN_BOARDS_BUILD=${@:-"-st -nxp -pc -esp -esp-atecc"}
FREERTOS_FETCHED=0

# Add installed arm compiler to path
export PATH="/usr/local/arm-gcc-none-eabi/bin:$PATH"

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
    local buildver=$4

    if [ $vendor == "ESPRESSIF" ]
    then
      idf.py build -DCMAKE_BUILD_TYPE=$buildver -C ./demos/projects/ESPRESSIF/$board
      echo -e "::group::Print Size for $board $buildver"
      ninja -C ./demos/projects/ESPRESSIF/$board/build size-components
    elif [ $vendor == "ESPRESSIF-ATECC" ]
    then
      echo -e "::group::Cleaning Repo"
      git config --global --add safe.directory ${PWD}
      git clean -xdf
      echo -e "::group::Configuring ESP32 with ATECC"
      rm -f ./demos/projects/ESPRESSIF/$board/sdkconfig.defaults
      cp -f ./.github/scripts/atecc-sdkconfig.defaults ./demos/projects/ESPRESSIF/$board/sdkconfig.defaults
      echo -e "::group::IDF reconfigure - download ESP-CryptoauthLib"
      idf.py reconfigure -C ./demos/projects/ESPRESSIF/$board
      echo -e "::group::IDF reconfigure - configure ESP-CryptoauthLib"
      idf.py reconfigure -C ./demos/projects/ESPRESSIF/$board
      echo -e "::group::IDF build"
      idf.py build -DCMAKE_BUILD_TYPE=$buildver -C ./demos/projects/ESPRESSIF/$board
      echo -e "::group::Print Size for $board $buildver"
      ninja -C ./demos/projects/ESPRESSIF/$board/build size-components
    elif [ $vendor == "PC" ]
    then
      echo -e "::group::Build PC with GCC"
      cmake -G Ninja -DBOARD=$board -DVENDOR=$vendor -B$outdir -DFREERTOS_PATH=$TEST_FREERTOS_SRC -DCMAKE_BUILD_TYPE=$buildver .
      cmake --build $outdir | tee build.txt

      rm -rf $outdir

      echo -e "::group::Build PC with Clang"
      cmake -DCMAKE_C_COMPILER=clang -G Ninja -DBOARD=$board -DVENDOR=$vendor -B$outdir -DFREERTOS_PATH=$TEST_FREERTOS_SRC -DCMAKE_BUILD_TYPE=$buildver .
      cmake --build $outdir | tee build.txt
    else
      cmake -G Ninja -DBOARD=$board -DVENDOR=$vendor -B$outdir -DFREERTOS_PATH=$TEST_FREERTOS_SRC -DCMAKE_BUILD_TYPE=$buildver .
      cmake --build $outdir | tee build.txt
    fi
}

function fetch_freertos()
{
    if [ ! -d "`pwd`/libs/FreeRTOS" ]
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
            sample_build "ESPRESSIF" "adu" "build" "Debug"
            exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/adu/build" "azure_iot_freertos_esp32.bin"
            echo -e "::group::Building sample for ESPRESSIF ESP32 port - Debug"
            sample_build "ESPRESSIF" "esp32" "build" "Debug"
            exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/esp32/build" "azure_iot_freertos_esp32.bin"
            sample_build "ESPRESSIF" "aziotkit" "build" "Debug"
            exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/aziotkit/build" "azure_iot_freertos_esp32.bin"
            sample_build "ESPRESSIF" "az-nvs-cert-bundle" "build" "Debug"
            exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/az-nvs-cert-bundle/build" "az-nvs-cert-bundle.bin"
            sample_build "ESPRESSIF" "az-ca-recovery" "build" "Debug"
            exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/az-ca-recovery/build" "azure_iot_freertos_esp32.bin"

            rm -rf build

            echo -e "::group::Building sample for ESPRESSIF ESP32 port - Release"
            sample_build "ESPRESSIF" "adu" "build" "Release"
            exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/adu/build" "azure_iot_freertos_esp32.bin"
            sample_build "ESPRESSIF" "esp32" "build" "Release"
            exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/esp32/build" "azure_iot_freertos_esp32.bin"
            sample_build "ESPRESSIF" "aziotkit" "build" "Release"
            exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/aziotkit/build" "azure_iot_freertos_esp32.bin"
            sample_build "ESPRESSIF" "az-nvs-cert-bundle" "build" "Release"
            exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/az-nvs-cert-bundle/build" "az-nvs-cert-bundle.bin"
            sample_build "ESPRESSIF" "az-ca-recovery" "build" "Release"
            exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/az-ca-recovery/build" "azure_iot_freertos_esp32.bin"
            ;;
        "-esp-atecc")
            echo -e "::group::Building sample for ESPRESSIF ESP32 with ATECC608 port - Debug"
            sample_build "ESPRESSIF-ATECC" "esp32" "build" "Debug"
            exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/esp32/build" "azure_iot_freertos_esp32.bin"

            rm -rf build

            echo -e "::group::Building sample for ESPRESSIF ESP32 with ATECC608 port - Release"
            sample_build "ESPRESSIF-ATECC" "esp32" "build" "Release"
            exit_if_binary_does_not_exist "./demos/projects/ESPRESSIF/esp32/build" "azure_iot_freertos_esp32.bin"
            ;;
        "-nxp")
            fetch_freertos

            echo -e "::group::Building sample for NXP mimxrt1060 port - Debug"
            sample_build "NXP" "mimxrt1060" "build_nxp_mimxrt1060" "Debug"
            exit_if_binary_does_not_exist "build_nxp_mimxrt1060" "iot-middleware-sample.elf"
            exit_if_binary_does_not_exist "build_nxp_mimxrt1060" "iot-middleware-sample-pnp.elf"
            exit_if_binary_does_not_exist "build_nxp_mimxrt1060" "iot-middleware-sample-adu.elf"

            echo -e "::group::Print Size for Debug"
            grep -A9 "Linking C executable demos/projects/NXP/mimxrt1060/iot-middleware-sample.elf" build.txt
            grep -A9 "Linking C executable demos/projects/NXP/mimxrt1060/iot-middleware-sample-pnp.elf" build.txt
            grep -A9 "Linking C executable demos/projects/NXP/mimxrt1060/iot-middleware-sample-adu.elf" build.txt

            rm -rf build_nxp_mimxrt1060

            echo -e "::group::Building sample for NXP mimxrt1060 port - Release"
            sample_build "NXP" "mimxrt1060" "build_nxp_mimxrt1060" "Release"
            exit_if_binary_does_not_exist "build_nxp_mimxrt1060" "iot-middleware-sample.elf"
            exit_if_binary_does_not_exist "build_nxp_mimxrt1060" "iot-middleware-sample-pnp.elf"
            exit_if_binary_does_not_exist "build_nxp_mimxrt1060" "iot-middleware-sample-adu.elf"
            
            echo -e "::group::Print Size for Release"
            grep -A9 "Linking C executable demos/projects/NXP/mimxrt1060/iot-middleware-sample.elf" build.txt
            grep -A9 "Linking C executable demos/projects/NXP/mimxrt1060/iot-middleware-sample-pnp.elf" build.txt
            grep -A9 "Linking C executable demos/projects/NXP/mimxrt1060/iot-middleware-sample-adu.elf" build.txt
            ;;
        "-st")
            fetch_freertos

            # STM32 BL475E-IoT1A
            echo -e "::group::Building sample for ST b-l475e-iot01a port - Debug"
            sample_build "ST" "b-l475e-iot01a" "build_st_b-l475e-iot01a" "Debug"
            exit_if_binary_does_not_exist "build_st_b-l475e-iot01a" "iot-middleware-sample.elf"
            exit_if_binary_does_not_exist "build_st_b-l475e-iot01a" "iot-middleware-sample-pnp.elf"
            exit_if_binary_does_not_exist "build_st_b-l475e-iot01a" "iot-middleware-sample-gsg.elf"
            exit_if_binary_does_not_exist "build_st_b-l475e-iot01a" "iot-middleware-sample-adu.elf"

            echo -e "::group::Print Size for Debug"
            grep -A5 "Linking C executable demos/projects/ST/b-l475e-iot01a/iot-middleware-sample.elf" build.txt
            grep -A5 "Linking C executable demos/projects/ST/b-l475e-iot01a/iot-middleware-sample-pnp.elf" build.txt
            grep -A5 "Linking C executable demos/projects/ST/b-l475e-iot01a/iot-middleware-sample-gsg.elf" build.txt
            grep -A5 "Linking C executable demos/projects/ST/b-l475e-iot01a/iot-middleware-sample-adu.elf" build.txt

            rm -rf build_st_b-l475e-iot01a

            echo -e "::group::Building sample for ST b-l475e-iot01a port - Release"
            sample_build "ST" "b-l475e-iot01a" "build_st_b-l475e-iot01a" "Release"
            exit_if_binary_does_not_exist "build_st_b-l475e-iot01a" "iot-middleware-sample.elf"
            exit_if_binary_does_not_exist "build_st_b-l475e-iot01a" "iot-middleware-sample-pnp.elf"
            exit_if_binary_does_not_exist "build_st_b-l475e-iot01a" "iot-middleware-sample-gsg.elf"
            exit_if_binary_does_not_exist "build_st_b-l475e-iot01a" "iot-middleware-sample-adu.elf"
            echo -e "::group::Print Size for Release"
            grep -A5 "Linking C executable demos/projects/ST/b-l475e-iot01a/iot-middleware-sample.elf" build.txt
            grep -A5 "Linking C executable demos/projects/ST/b-l475e-iot01a/iot-middleware-sample-pnp.elf" build.txt
            grep -A5 "Linking C executable demos/projects/ST/b-l475e-iot01a/iot-middleware-sample-gsg.elf" build.txt
            grep -A5 "Linking C executable demos/projects/ST/b-l475e-iot01a/iot-middleware-sample-adu.elf" build.txt

            # STM32 H745 Discovery
            echo -e "::group::Building sample for ST stm32h745i-disco port - Debug"
            sample_build "ST" "stm32h745i-disco" "build_st_stm32h745i-disco" "Debug"
            exit_if_binary_does_not_exist "build_st_stm32h745i-disco" "iot-middleware-sample.elf"
            exit_if_binary_does_not_exist "build_st_stm32h745i-disco" "iot-middleware-sample-pnp.elf"
            exit_if_binary_does_not_exist "build_st_stm32h745i-disco" "iot-middleware-sample-adu.elf"
            echo -e "::group::Print Size for Debug"
            grep -A5 "Linking C executable demos/projects/ST/stm32h745i-disco/cm7/iot-middleware-sample.elf" build.txt
            grep -A5 "Linking C executable demos/projects/ST/stm32h745i-disco/cm7/iot-middleware-sample-pnp.elf" build.txt
            grep -A5 "Linking C executable demos/projects/ST/stm32h745i-disco/cm7/iot-middleware-sample-adu.elf" build.txt

            rm -rf build_st_stm32h745i-disco

            echo -e "::group::Building sample for ST stm32h745i-disco port - Release"
            sample_build "ST" "stm32h745i-disco" "build_st_stm32h745i-disco" "Release"
            exit_if_binary_does_not_exist "build_st_stm32h745i-disco" "iot-middleware-sample.elf"
            exit_if_binary_does_not_exist "build_st_stm32h745i-disco" "iot-middleware-sample-pnp.elf"
            exit_if_binary_does_not_exist "build_st_stm32h745i-disco" "iot-middleware-sample-adu.elf"
            echo -e "::group::Print Size for Release"
            grep -A5 "Linking C executable demos/projects/ST/stm32h745i-disco/cm7/iot-middleware-sample.elf" build.txt
            grep -A5 "Linking C executable demos/projects/ST/stm32h745i-disco/cm7/iot-middleware-sample-pnp.elf" build.txt
            grep -A5 "Linking C executable demos/projects/ST/stm32h745i-disco/cm7/iot-middleware-sample-adu.elf" build.txt

            # STM32 BL4S5I-IoT1A
            echo -e "::group::Building sample for ST b-l4s5i-iot01a port - Debug"
            sample_build "ST" "b-l4s5i-iot01a" "build_st_b-l4s5i-iot01a" "Debug"
            exit_if_binary_does_not_exist "build_st_b-l4s5i-iot01a" "iot-middleware-sample.elf"
            exit_if_binary_does_not_exist "build_st_b-l4s5i-iot01a" "iot-middleware-sample-pnp.elf"
            exit_if_binary_does_not_exist "build_st_b-l4s5i-iot01a" "iot-middleware-sample-adu.elf"
            echo -e "::group::Print Size for Debug"
            grep -A5 "Linking C executable demos/projects/ST/b-l4s5i-iot01a/iot-middleware-sample.elf" build.txt
            grep -A5 "Linking C executable demos/projects/ST/b-l4s5i-iot01a/iot-middleware-sample-pnp.elf" build.txt
            grep -A5 "Linking C executable demos/projects/ST/b-l4s5i-iot01a/iot-middleware-sample-adu.elf" build.txt

            rm -rf build_st_b-l4s5i-iot01a

            echo -e "::group::Building sample for ST b-l4s5i-iot01a port - Release"
            sample_build "ST" "b-l4s5i-iot01a" "build_st_b-l4s5i-iot01a" "Release"
            exit_if_binary_does_not_exist "build_st_b-l4s5i-iot01a" "iot-middleware-sample.elf"
            exit_if_binary_does_not_exist "build_st_b-l4s5i-iot01a" "iot-middleware-sample-pnp.elf"
            exit_if_binary_does_not_exist "build_st_b-l4s5i-iot01a" "iot-middleware-sample-adu.elf"
            echo -e "::group::Print Size for Release"
            grep -A5 "Linking C executable demos/projects/ST/b-l4s5i-iot01a/iot-middleware-sample.elf" build.txt
            grep -A5 "Linking C executable demos/projects/ST/b-l4s5i-iot01a/iot-middleware-sample-pnp.elf" build.txt
            grep -A5 "Linking C executable demos/projects/ST/b-l4s5i-iot01a/iot-middleware-sample-adu.elf" build.txt
            ;;
        "-pc")
            fetch_freertos

            echo -e "::group::Building sample for linux port - Debug"
            sample_build "PC" "linux" "build_pc_linux" "Debug"
            exit_if_binary_does_not_exist "build_pc_linux" "iot-middleware-sample"
            exit_if_binary_does_not_exist "build_pc_linux" "iot-middleware-sample-pnp"

            rm -rf build_pc_linux

            echo -e "::group::Building sample for linux port - Release"
            sample_build "PC" "linux" "build_pc_linux" "Release"
            exit_if_binary_does_not_exist "build_pc_linux" "iot-middleware-sample"
            exit_if_binary_does_not_exist "build_pc_linux" "iot-middleware-sample-pnp"
            exit_if_binary_does_not_exist "build_pc_linux" "iot-middleware-sample-adu"

            echo -e "::group::Running CA Recovery Unit Tests"
            ./build_pc_linux/demos/projects/PC/linux/test_ca_recovery

            ;;
        * )
            echo "build for $arg not found";;
    esac
done

