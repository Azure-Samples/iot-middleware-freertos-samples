#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

echo -e "::group::FreeRTOS source"
FREERTOS_TAG=202107.00
FREERTOS_SRC=${1:-"libs/FreeRTOS"}

if [ ! -d $FREERTOS_SRC ]; then
    git clone --recursive --depth=1 --branch $FREERTOS_TAG https://github.com/FreeRTOS/FreeRTOS.git $FREERTOS_SRC
else
    echo -e "$FREERTOS_SRC already exists"
fi
