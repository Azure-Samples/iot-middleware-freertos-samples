# Azure IoT samples using Azure IoT Middleware for FreeRTOS

<!-- markdown-link-check-disable -->
[![Linux CI Tests](https://github.com/Azure-Samples/iot-middleware-freertos-samples/actions/workflows/ci_tests_linux.yml/badge.svg)](https://github.com/Azure-Samples/iot-middleware-freertos-samples/actions/workflows/ci_tests_linux.yml)
<!-- markdown-link-check-enable-->

## Getting Started

The Azure IoT Middleware for FreeRTOS simplifies the connection of devices running FreeRTOS to Azure IoT services. It implements a modular approach that brings flexibility to IoT developers by allowing them to bring their own network stack (MQTT, TLS and Socket).

The [Azure IoT Middleware for FreeRTOS repo](https://github.com/Azure/azure-iot-middleware-freertos) has the core functionalities of the middleware and no external dependencies, however to implement working samples, we need to bring a network stack which is the objective of this repo. 

Below you will find samples for development kits and simulators showing how to use the Azure IoT Middleware for FreeRTOS. The following development kits are currently supported:

* NXP:
  * [MIMXRT1060-EVK](demos/projects/NXP/mimxrt1060/)

* STMicroelectronics:
  * [B-L475E-IOT01A](demos/projects/ST/b-l475e-iot01a/)

* PC Simulation:
  * [Linux](demos/projects/PC/linux/)
  * [Windows](demos/projects/PC/windows/)
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
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies
