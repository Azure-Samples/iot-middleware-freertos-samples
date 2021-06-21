# Connect an NXP MIMXRT1060-EVK Evaluation kit using Azure IoT Middleware for FreeRTOS

## What you need

* NXP Evaluation Kit: [MIMXRT1060-EVK](https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/mimxrt1060-evk-i-mx-rt1060-evaluation-kit:MIMXRT1060-EVK)

* USB 2.0 A male to Micro USB male cable

* Wired Ethernet access with DHCP

* Ethernet cable

## Prerequisites

* [CMake](https://cmake.org/download/) (Version 3.13 or higher)

* [Ninja build system](https://github.com/ninja-build/ninja/releases) (Version 1.10 or higher)

- [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) (Version 9 or higher)
* Terminal tool like [Termite](https://www.compuphase.com/software_termite.htm), Putty, Tera Term, etc.

* To run this sample you can use a device previously created in your IoT Hub or have the Azure IoT Middleware for FreeRTOS provision your device automatically using DPS.

IoT Hub | DPS 
---------|----------
Have an [Azure IoT Hub](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal) created | Have an instance of [IoT Hub Device Provisioning Service](https://docs.microsoft.com/en-us/azure/iot-dps/quick-setup-auto-provision#create-a-new-iot-hub-device-provisioning-service)
Have a [logical device](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal#register-a-new-device-in-the-iot-hub) created in your Azure IoT Hub using your preferred authentication method* | Have an [individual enrollment](https://docs.microsoft.com/en-us/azure/iot-dps/how-to-manage-enrollments#create-a-device-enrollment) created in your instance of DPS using your preferred authentication method*

*While this sample supports SAS keys and Certificates, this guide will refer only to SAS keys. 

### Install prerequisites on Windows

Ensure that cmake, ninja and the ARM toolset binaries are available in the `PATH` environment variable.

You may also need to enable long path support for both Windows and git:
* Windows: <https://docs.microsoft.com/windows/win32/fileio/maximum-file-path-limitation?tabs=cmd#enable-long-paths-in-windows-10-version-1607-and-later>
* Git: as Administrator run `git config --system core.longpaths true`

## Get the middleware

Clone the following repo to download all sample device code, setup scripts, and offline versions of the documentation. 

**If you previously cloned this repo in another sample, you don't need to do it again.**

```bash
git clone https://github.com/Azure-Samples/iot-middleware-freertos-samples.git
```

To initialize the repo, run the following command:

```bash
git submodule update --init --recursive --depth 1
```

## Prepare the device
To connect the MIMXRT1060-EVK to Azure, you'll modify a configuration file for Azure IoT settings, rebuild the image, and flash the image to the device.

Update the file `iot-middleware-freertos-samples/demos/projects/NXP/mimxrt1060/config/demo_config.h` with your configuration values.

If you're using a device previously created in your **IoT Hub** with SAS authentication, comment out line #53 (`#define democonfigENABLE_DPS_SAMPLE`) and set the following parameters:

Parameter | Value 
---------|----------
 `democonfigDEVICE_ID` | _{Your Device ID value}_
 `democonfigHOSTNAME` | _{Your Host name value}_ 
 `democonfigDEVICE_SYMMETRIC_KEY` | _{Your Primary Key value}_ 

If you're using **DPS** with an individual enrollment with SAS authentication, set the following parameters:

Parameter | Value 
---------|----------
 `democonfigID_SCOPE` | _{Your ID scope value}_
 `democonfigREGISTRATION_ID` | _{Your Device Registration ID value}_ 
 `democonfigDEVICE_SYMMETRIC_KEY` | _{Your Primary Key value}_ 

## Build the image

To build the device image, navigate to the `iot-middleware-freertos-samples` directory and run the following commands:

  ```bash
  cmake -G Ninja -DBOARD=mimxrt1060 -DVENDOR=NXP -Bmimxrt1060 .
  cmake --build mimxrt1060
  ```

After the build completes, confirm that a folder named `/mimxrt1060/` was created and it contains a file named `iot-middleware-sample.bin`. 

## Flash the image

1. Connect the Micro USB cable to the Micro USB port on the NXP EVK, and then connect it to your computer. After the device powers up, a solid green LED shows the power status.

1. Use the Ethernet cable to connect the NXP EVK to an Ethernet port.

2. In File Explorer, find the binary file that you created in the previous section and copy it.

3. In File Explorer, find the NXP EVK device connected to your computer. The device appears as a drive on your system with the drive label RT1060-EVK.

4. Paste the binary file into the root folder of the NXP EVK. Flashing starts automatically and completes in a few seconds.

1. Reset the device once flashing completes.

## Confirm device connection details

You can use one of the terminal applications to monitor communication and confirm that your device is set up correctly. Go to Device Manager in Windows to determine which COM port your device was assigned.

The following settings can be used to monitor serial data:

- Baud Rate: `115200`
- Data Bits: `8`
- Stop Bits: `1`
- Parity: none
- Flow Control: none
