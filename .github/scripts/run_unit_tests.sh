#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

sudo apt-get install libcmocka-dev libcmocka0

pushd libs/azure-iot-middleware-freertos

.github/scripts/fetch_freertos.sh
.github/scripts/ci_tests.sh TEST_RUN_E2E_TESTS=0

popd

pushd libs/azure-iot-middleware-freertos/libraries/azure-sdk-for-c
rm -rf build
mkdir build
cd build
cmake -DUNIT_TESTING=ON ..
cmake --build .
ctest --output-on-failure

popd
