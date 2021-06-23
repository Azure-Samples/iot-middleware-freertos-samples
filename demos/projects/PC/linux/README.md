# Use Linux to simulate a device using Azure IoT Middleware for FreeRTOS

## Get the middleware

Clone the following repo to download all sample device code, setup scripts, and offline versions of the documentation.

**If you previously cloned this repo in another sample, you don't need to do it again.**

```bash
    git clone https://github.com/Azure-Samples/iot-middleware-freertos-samples.git
```

To initialize the repo, run the following command:

```bash
    cd iot-middleware-freertos-samples
    git submodule update --init --recursive --depth 1
```


## Install Prerequisites

* [CMake](https://cmake.org/download/) (Version 3.13 or higher)

* Execute the installation script for additional prerequisites:

```bash
    sudo ./.github/scripts/install_software.sh
```

* Execute the Network setup script which will create virtual interfaces rtosveth0 and rtosveth1:

```bash
    sudo .github/scripts/init_linux_port_vm_network.sh
```

> After running the sample, to remove any changes done by this script run it again with `--clean`. 

* To run this sample you can use a device previously created in your IoT Hub or have the Azure IoT Middleware for FreeRTOS provision your device automatically using DPS.

IoT Hub | DPS
---------|----------
Have an [Azure IoT Hub](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal) created | Have an instance of [IoT Hub Device Provisioning Service](https://docs.microsoft.com/en-us/azure/iot-dps/quick-setup-auto-provision#create-a-new-iot-hub-device-provisioning-service)
Have a [logical device](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal#register-a-new-device-in-the-iot-hub) created in your Azure IoT Hub using your preferred authentication method* | Have an [individual enrollment](https://docs.microsoft.com/en-us/azure/iot-dps/how-to-manage-enrollments#create-a-device-enrollment) created in your instance of DPS using your preferred authentication method*

*While this sample supports SAS keys and Certificates, this guide will refer only to SAS keys.

## Prepare the simulation

To connect the simulated device to Azure, you'll modify a configuration file for Azure IoT settings, rebuild the image, and run it.

Update the file `demo_config.h` with your configuration values.

```bash
    nano demos/projects/PC/linux/config/demo_config.h
```

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

### Set the Virtual Ethernet Interface

Execute the command below to find which index you got for the ``rtosveth1`` (index is the number to the left of the interface). Make a note of the number for the next step.

```bash
    sudo tcpdump --list-interfaces
```
Look for line #138 in `FreeRTOSConfig.h` and update `configNETWORK_INTERFACE_TO_USE` with the number you got in the previous step.

**Example**: if you got ``4.rtosveth1 [Up, Running]`` in the previous step, you'll update line #138 to look like this ``#define configNETWORK_INTERFACE_TO_USE ( 4L )``

```bash
    nano demos/projects/PC/linux/config/FreeRTOSConfig.h
```

## Build the image

To build the device image, run the following commands from the root of the cloned Repo:

  ```bash
    cmake -G Ninja -DVENDOR=PC -DBOARD=linux -Bbuild_linux .
    cmake --build build_linux
  ```

## Confirm simulated device connection details

To monitor communication and confirm that your device is set up correctly, execute the command below.

```Bash
    sudo ./build_linux/demos/projects/PC/linux/iot-middleware-sample.elf
```
