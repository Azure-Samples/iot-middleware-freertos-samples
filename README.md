# Azure IoT samples using Azure IoT middleware for FreeRTOS

<!-- markdown-link-check-disable -->
[![Windows CI Tests](https://github.com/Azure-Samples/iot-middleware-freertos-samples/actions/workflows/ms_build.yml/badge.svg?branch=main)](https://github.com/Azure-Samples/iot-middleware-freertos-samples/actions/workflows/ms_build.yml)
[![Linux CI Tests](https://github.com/Azure-Samples/iot-middleware-freertos-samples/actions/workflows/ci_tests_linux.yml/badge.svg?branch=main)](https://github.com/Azure-Samples/iot-middleware-freertos-samples/actions/workflows/ci_tests_linux.yml)
<!-- markdown-link-check-enable-->

## Getting Started with Azure IoT middleware for FreeRTOS

The Azure IoT middleware for FreeRTOS simplifies the connection of devices running FreeRTOS to the Azure IoT services. It implements a modular approach that brings flexibility to IoT developers by allowing them to bring their own network stack (MQTT, TLS and Socket).

The [Azure IoT middleware for FreeRTOS repo](https://github.com/Azure/azure-iot-middleware-freertos) has the core functionalities of the middleware and no external dependencies, however to implement the working samples, we need to bring a network stack which is the objective of the samples in this repo. 

We have two sets of samples:

- **Azure IoT Hub**: These samples leverage Azure IoT Hub, DPS (Device Provisioning) and Plug and Play.

- **Azure IoT Central**: These samples leverage Azure IoT Central, Azure DPS (Device Provisioning Service) and, Plug and Play. Azure subscription is not required during the initial 7 days of the [IoT Central Free Trial](https://azure.microsoft.com/pricing/details/iot-central/#pricing).

There are several other alternatives to get MCU-based devices connected to Azure. See [Other Azure IoT SDKs](https://learn.microsoft.com/azure/iot-develop/concepts-using-c-sdk-and-embedded-c-sdk) to learn more.

### IoT Hub Samples:

Below you will find samples for development kits and simulators showing how to use the Azure IoT middleware for FreeRTOS connected to Azure IoT hub. The following samples support these development kits:

- ESPRESSIF:
  - [ESP32](demos/projects/ESPRESSIF/esp32/)
  - [ESP32 Azure IoT Kit](demos/projects/ESPRESSIF/aziotkit/)

- NXP:
  - [MIMXRT1060-EVK](demos/projects/NXP/mimxrt1060/)

- STMicroelectronics:
  - [B-L475E-IOT01A](demos/projects/ST/b-l475e-iot01a/)
  - [B-L4S5I-IOT01A](demos/projects/ST/b-l4s5i-iot01a/)
  - [STM32H745I-DISCO](demos/projects/ST/stm32h745i-disco/cm7)

- PC Simulation:
  - [Linux](demos/projects/PC/linux/)
  - [Windows](demos/projects/PC/windows/)

For a video deep dive into this repo and the source code, please see the Microsoft Developers video below.

[<img src="./docs/resources/deep-dive-video.jpg" width="50%">](https://youtu.be/PNykfuJ3VDs)

### Azure Device Update

You can interact with [Azure Device Update](https://github.com/Azure/azure-iot-middleware-freertos/blob/main/docs/how_to_use_adu_client.md), remotely updating your Azure IoT Hub devices through the samples below:

- ESPRESSIF:
  - [ESP32](demos/projects/ESPRESSIF/adu/README.md)

- NXP:
  - [MIMXRT1060-EVK](demos/projects/NXP/mimxrt1060/ADU.md)

- STMicroelectronics:
  - [B-L475E-IOT01A](demos/projects/ST/b-l475e-iot01a/ADU.md)

- PC Simulation:
  - [Linux](demos/projects/PC/linux/ADU.md)

### IoT Central Samples

Below you will find samples for development kits showing how to use the Azure IoT middleware for FreeRTOS connected to Azure IoT Central. The following samples support these development kits:

- ESPRESSIF:
  - [ESP32-Azure IoT Kit](demos/projects/ESPRESSIF/aziotkit/GSG.md)

- STMicroelectronics:
  - [B-L475E-IOT01A](demos/projects/ST/b-l475e-iot01a/GSG.md)

### Plug and Play Sample

The easiest way to interact with the Plug and Play sample from the IoT service side is to use Azure IoT Explorer.  To interact with the device sample:

- Install [Azure IoT Explorer](https://github.com/Azure/azure-iot-explorer/#plug-and-play).
- Download [the Thermostat model](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json) to a local directory.
- Start Azure IoT Explorer and then:
  - [Configure your hub](https://github.com/Azure/azure-iot-explorer/#configure-an-iot-hub-connection).  Once you've created your thermostat device, you should see it listed in the UX.
  - Go to `IoT Plug and Play Settings` on the home screen, select `Local Folder` for the location of the model definitions, and point to the folder you downloaded the thermostat model.
  - Go to the devices list and select your thermostat device.  Now select `IoT Plug and Play components` and then `Default Component`.
  - You will now be able to interact with the Plug and Play device.

Additional instructions for using Azure IoT Explorer are available [here](https://github.com/Azure/azure-iot-explorer/#plug-and-play).

## Support

If you need support, please see our [SUPPORT.md](./SUPPORT.md) file.

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft
trademarks or logos is subject to and must follow
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies
