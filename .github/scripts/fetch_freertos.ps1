# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

param (
    [ Parameter( Position=0 ) ]
    [ string ] $FREERTOS_SRC = "libs/FreeRTOS"
)

Write-Output "::group::FreeRTOS source"
$FREERTOS_COMMIT_ID="c8fa483b68c6c1149c2a7a8bc1e901b38860ec9b"

if ( !( Test-Path -Path $FREERTOS_SRC ) )
{
    git clone https://github.com/FreeRTOS/FreeRTOS.git $FREERTOS_SRC
    cd $FREERTOS_SRC
    git checkout $FREERTOS_COMMIT_ID
    git submodule sync
    git submodule update --init --recursive --depth=1
}
else
{
    Write-Output "$FREERTOS_SRC already exists"
}
