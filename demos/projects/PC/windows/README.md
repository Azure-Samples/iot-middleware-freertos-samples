# Use Windows to simulate a device using Azure IoT Middleware for FreeRTOS

## Get the middleware

Clone the following repo to download all sample device code, setup scripts, and offline versions of the documentation.

**If you previously cloned this repo in another sample, you don't need to do it again.**

```bash
git clone https://github.com/Azure-Samples/iot-middleware-freertos-samples.git
```

To initialize the repo, run the following command:

```bash
cd iot-middleware-freertos-samples
git submodule update --init --recursive
```

## Install Prerequisites

- [CMake](https://cmake.org/download/) (Version 3.13 or higher)
- An installed version of [WinPcap](https://www.winpcap.org/default.htm) (NPCap may also work). If you have Wireshark installed then this step might not be necessary. This is needed to create an Ethernet interface to be used by the simulator.
- To run this sample you can use a device previously created in your IoT Hub or have the Azure IoT Middleware for FreeRTOS provision your device automatically using DPS. **Note** that even when using DPS, you still need an IoT Hub created and connected to DPS.

IoT Hub | DPS
---------|----------
Have an [Azure IoT Hub](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal) created | Have an instance of [IoT Hub Device Provisioning Service](https://docs.microsoft.com/azure/iot-dps/quick-setup-auto-provision#create-a-new-iot-hub-device-provisioning-service)
Have a [logical device](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal#register-a-new-device-in-the-iot-hub) created in your Azure IoT Hub using your preferred authentication method* | Have an [individual enrollment](https://docs.microsoft.com/azure/iot-dps/how-to-manage-enrollments#create-a-device-enrollment) created in your instance of DPS using your preferred authentication method*

*While this sample supports SAS keys and Certificates, this guide will refer only to SAS keys.

## Prepare the simulation

To connect the simulated device to Azure, you'll modify a configuration file for Azure IoT settings, rebuild the image, and run it.

Update the file `demos/projects/PC/windows/config/demo_config.h` with your configuration values.

If you're using a device previously created in your **IoT Hub** with SAS authentication, disable DPS by commenting out `#define democonfigENABLE_DPS_SAMPLE` and setting the following parameters:

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

To build the device image, run the following commands from the root of the cloned Repo:

  ```bash
cmake -DVENDOR=PC -DCMAKE_GENERATOR_PLATFORM=Win32 -DBOARD=windows -Bbuild_windows . 

cmake --build build_windows 
  ```

In the output of the second command you'll find the path to the `iot-middleware-sample.exe`. Navigate to its folder and execute it. You should get an output similar to the below:

```bash
The following network interfaces are available:

Interface 1 - rpcap://\Device\NPF_{F2D50519-58E5-4655-9257-357F1D4977AD}
              (Network adapter 'Microsoft' on local host)

Interface 2 - rpcap://\Device\NPF_{14EF9902-36A1-4BEA-8F13-51CFC8C556B0}
              (Network adapter 'Microsoft' on local host)

Interface 3 - rpcap://\Device\NPF_{14B13215-DB7A-4E0F-8227-6106FB7C4161}
              (Network adapter 'Intel(R) Ethernet Connection I217-LM' on local host)

Interface 4 - rpcap://\Device\NPF_{703B43D3-8B10-4831-957E-79D4A18C5E7D}
              (Network adapter 'Microsoft' on local host)


The interface that will be opened is set by "configNETWORK_INTERFACE_TO_USE", which
should be defined in FreeRTOSConfig.h

ERROR:  configNETWORK_INTERFACE_TO_USE is set to 0, which is an invalid value.
Please set configNETWORK_INTERFACE_TO_USE to one of the interface numbers listed above,
then re-compile and re-start the application.  Only Ethernet (as opposed to WiFi)
interfaces are supported.

HALTING
```

This error message is normal since we still need to setup the Interface Adapter. It shows the list of interfaces on your PC. Identify and make a note of the active interface (in this case Interface 3, which is the one with Ethernet connection).

Edit the `demos/projects/PC/windows/config/FreeRTOSConfig.h` file and replace the `( 0L )` on line 116 by the number of your active interface. In the sample case above the number will be 3 or `( 3L )`. Save and close this file.

## Confirm simulated device connection details

Rebuild the sample with the new Network Adaptor information:

  ```bash
cmake --build build_windows 
  ```

In the output of this command you'll find the path to the `iot-middleware-sample.exe`. Navigate to its folder and execute it.