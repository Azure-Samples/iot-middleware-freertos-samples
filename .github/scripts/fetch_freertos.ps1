# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

param (
    [ Parameter( Position=0 ) ]
    [ string ] $FREERTOS_SRC = "libs/FreeRTOS"
)

Write-Output "::group::FreeRTOS source"
$FREERTOS_TAG="202107.00"

if ( !( Test-Path -Path $FREERTOS_SRC ) )
{
    git clone --recursive --depth=1 --branch $FREERTOS_TAG https://github.com/FreeRTOS/FreeRTOS.git $FREERTOS_SRC
}
else
{
    Write-Output "$FREERTOS_SRC already exists"
}
