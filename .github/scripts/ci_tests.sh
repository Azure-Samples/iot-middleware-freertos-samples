#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

function binary_exists()
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

function build() {
    local vendor=$1
    local board=$2
    local outdir=$3

    cmake -G Ninja -DBOARD=$board -DVENDOR=$vendor -B$outdir .
    cmake --build $outdir
}

echo -e "Building all sample ports"
build "ST" "b-l475e-iot01a" "build_st_b-l475e-iot01a"
binary_exists "build_st_b-l475e-iot01a" "iot-middleware-sample.elf"

build "ST" "stm32h745i_discovery" "build_st_stm32h745i_discovery"
binary_exists "build_st_stm32h745i_discovery" "iot-middleware-sample.elf"

build "PC" "linux" "build_pc_linux"
binary_exists "build_pc_linux" "iot-middleware-sample.elf"