#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

sudo apt-get install wget

pushd libs/azure-iot-middleware-freertos
./.github/scripts/code_style.sh check

wget https://releases.llvm.org/9.0.0/clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz
tar -xvf clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz
mv clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/clang-format /usr/local/bin
pushd libs/azure-iot-middleware-freertos/libraries/azure-sdk-for-c

find . \( -iname '*.h' -o -iname '*.c' \) -exec clang-format -i {} \;

git status --untracked-files=no --porcelain

if [[ `git status --untracked-files=no --porcelain` ]]; then
  echo Some files were not formatted correctly according to the .clang-format file.
  echo Please run clang-format to fix the issue by using this bash command at the root of the repo:
  echo "find . \( -iname '*.h' -o -iname '*.c' \) -exec clang-format -i {} \;"
  exit 1
fi

echo Success, all files are formatted correctly according to the .clang-format file.
exit 0
