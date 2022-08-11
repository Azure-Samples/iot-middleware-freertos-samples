# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

$test_freertos_src = Join-Path $PSScriptRoot "..\..\libs\FreeRTOS"

function exit_if_binary_does_not_exist
{
    param (
        [Parameter(Mandatory=$true, Position=0)]
        [string] $search_dir,
        [Parameter(Mandatory=$true, Position=1)]
        [string] $filename
    )

    $binary = Get-ChildItem -Path "$search_dir" -Filter "$filename" -Recurse
    if($binary.count)
    {
        Write-Output "$filename is found in $search_dir"
    }
    else
    {
        Write-Output "$filename not found"
        throw "$filename not found"
    }
}

function sample_build
{
    param (
        [Parameter(Mandatory=$true, Position=0)]
        [string] $vendor,
        [Parameter(Mandatory=$true, Position=1)]
        [string] $board,
        [Parameter(Mandatory=$true, Position=2)]
        [string] $outdir,
        [Parameter(Mandatory=$false, Position=3)]
        [string] $additionalFlags
    )

    
    Write-Output "::group::Cleaning Repo"
    git clean -xdf

    Write-Output "::group::CMake Config & Generate"
    cmake -G "Visual Studio 17 2022" -A Win32 -DBOARD="$board" -DVENDOR="$vendor" -B"$outdir" -DFREERTOS_PATH="$test_freertos_src" "$additionalFlags" .
    Write-Output "::group::CMake Build"
    cmake --build "$outdir"
}

# FreeRTOS recursive download needs support for long path
git config --system core.longpaths true
$freertos_fetch_script=Join-Path $PSScriptRoot "fetch_freertos.ps1"
& "$freertos_fetch_script" -FREERTOS_SRC $test_freertos_src

Write-Output "::group::Building sample for PC windows port"
sample_build -vendor "PC" -board "windows" -outdir "build_windows" -additionalFlags "-DCMAKE_GENERATOR_PLATFORM=Win32"
exit_if_binary_does_not_exist -search_dir "build_windows" -filename "iot-middleware-sample.exe"
exit_if_binary_does_not_exist -search_dir "build_windows" -filename "iot-middleware-sample-pnp.exe"
