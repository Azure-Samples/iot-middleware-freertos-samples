#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

wget https://developer.arm.com/-/media/Files/downloads/gnu/11.2-2022.02/binrel/gcc-arm-11.2-2022.02-x86_64-arm-none-eabi.tar.xz

sudo mkdir /usr/local/arm-gcc-none-eabi

sudo tar -xvf gcc-arm-11.2-2022.02-x86_64-arm-none-eabi.tar.xz -C /usr/local/arm-gcc-none-eabi --strip-components=1

# Export path in ci_tests.sh
