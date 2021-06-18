#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

echo -e "::group::FreeRTOS source"
TEST_FREERTOS_COMMIT_ID=c8fa483b68c6c1149c2a7a8bc1e901b38860ec9b
TEST_FREERTOS_SRC=${1:-"libs/FreeRTOS"}

if [ ! -d $TEST_FREERTOS_SRC ]; then
    git clone https://github.com/FreeRTOS/FreeRTOS.git $TEST_FREERTOS_SRC
    pushd libs/FreeRTOS
    git checkout -b ${TEST_FREERTOS_COMMIT_ID}
    git submodule sync
    git submodule update --init --recursive --depth=1
    popd
fi
