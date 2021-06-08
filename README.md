# Azure IoT Sample

Run [Azure IoT Sample](demo/sample_azure_iot/sample_azure_iot.c))

## Getting Started

### Prerequisites

- [CMake](https://cmake.org/download/) (> 3.13)
- [Ninja build system](https://github.com/ninja-build/ninja/releases)
- [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) (>v9)

### Quickstart

#### Install prerequisites on Linux (or Windows Subsystem for Linux)

`./.github/scripts/install_software.sh`

#### Install prerequisites on Windows
Ensure that cmake, ninja and the ARM toolset binaries are available in the `PATH` environment variable.

You may also need to enable long path support for both Windows and git:
- Winodws: https://docs.microsoft.com/windows/win32/fileio/maximum-file-path-limitation?tabs=cmd#enable-long-paths-in-windows-10-version-1607-and-later
- Git: as Administrator run `git config --system core.longpaths true`

#### Clone and initialize the repository

`git clone https://github.com/Azure-Samples/iot-middleware-freertos-samples.git`
`cd iot-middleware-freertos-samples`
`git checkout develop_higupt_sample`
`git submodule update --init --recursive --depth 1`

## Demo

We currently have samples for the following devices. 

- Linux

  Configure [azure iot endpoint config](demos/projects/PC/linux/config/demo_config.h) and [network interface](demos/projects/PC/linux/config/FreeRTOSConfig.h)
  ```bash
  cmake -G Ninja -DVENDOR=PC -DBOARD=linux -Bbuild_linux .
  cmake --build build_linux
  ```
  Run ./build_linux/demos/projects/PC/linux/iot-middleware-sample.bin .
  
- b-l475e-iot01a

  Configure [azure iot endpoint config](demos/projects/ST/b-l475e-iot01a/config/demo_config.h)
  ```bash
  cmake -G Ninja -DVENDOR=ST -DBOARD=b-l475e-iot01a -Bb-l475e-iot01a .
  cmake --build b-l475e-iot01a

  ./b-l475e-iot01a/
  ```
  Flash ./b-l475e-iot01a/demos/projects/ST/b-l475e-iot01a/iot-middleware-sample.bin to b-l475e-iot01a device.
  
- stm32h745i_discovery

  Configure [azure iot endpoint config](demos/projects/ST/stm32h745i_discovery/config/demo_config.h)
  ```bash
  cmake -G Ninja -DVENDOR=ST -DBOARD=stm32h745i_discovery -Bstm32h745i_discovery .
  cmake --build stm32h745i_discovery

  ./stm32h745i_discovery/
  ```
  Flash ./stm32h745i_discovery/demos/projects/ST/stm32h745i_discovery/iot-middleware-sample.bin to stm32h745i_discovery device.
  
- mimxrt1060

  Configure [azure iot endpoint config](demos/projects/NXP/mimxrt1060/config/demo_config.h)
  ```bash
  cmake -G Ninja -DBOARD=mimxrt1060 -DVENDOR=NXP -Bmimxrt1060 .
  cmake --build mimxrt1060
  
  ```
  Flash ./mimxrt1060/demos/projects/NXP/mimxrt1060/iot-middleware-sample.bin to mimxrt1060 device.
