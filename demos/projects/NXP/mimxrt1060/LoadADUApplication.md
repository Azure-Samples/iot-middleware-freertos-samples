# Load an ADU Application to an NXP MIMXRT1060-EVK Evaluation kit

This document contains instructions for 3 options to load an ADU application to an NXP MIMXRT1060-EVK Evaluation kit.

- [What you need](#what-you-need)
- [Using Command Line](#using-command-line)
  - [Command Line Prerequisites](#command-line-prerequisites)
  - [Load image to device](#load-image-to-device)
- [Using VSCode](#using-vscode)
- [Using MCUXpresso IDE](#using-mcuxpresso-ide)
  - [MCUXpresso IDE Prerequisites](#mcuxpresso-ide-prerequisites)
  - [Load the image to the device](#load-the-image-to-the-device)

## What you need

* Successfully build the ADU sample for the NXP Evaluation Kit DevKit: [MIMXRT1060-EVK](https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/mimxrt1060-evk-i-mx-rt1060-evaluation-kit:MIMXRT1060-EVK).

* Make sure to run this git command before building and cloning:

 ```powershell
 # As **administrator** 
 git config --system core.longpaths true
```

* Have the bootloader flashed to the device (instructions [here](ADU.md#flash-the-bootloader)).

## Using Command Line

In order to debug the MIMXRT1060-EVK over command line, this guide requires Segger J-Link. While it works, currently this  experimental and [not suported scenario](https://forum.segger.com/index.php/Thread/8140-SOLVED-CMSIS-DAP-support-seems-to-be-disabled/?postID=29869) for Segger, so try it at your own risk.  

### Command Line Prerequisites

- Install OpenOCD
  - Get OpenOCD pre-built for Windows [here](https://gnutoolchains.com/arm-eabi/openocd/).
  - Don't forget to add the folder `bin/openocd.exe` to PATH.

- Install Segger JLink driver
  - If you don’t have it yet, get the J-Link software from Segger - V7.22b, 32bit (download from the [Segger page](https://www.segger.com/downloads/jlink/)).
  - Don't forget to add the folder `JLink_V722b` to PATH if not added during setup.

### Load image to device

1. In a command prompt, run the following to start the JLink session

    ```cmd
    JLinkGDBServerCL.exe -singlerun -nogui -if swd -port 50000 -swoport 50001 -telnetport 50002 -device MIMXRT1062xxx6A -rtos GDBServer/RTOSPlugin_FreeRTOS.dll
    ```

1. In a second command prompt at the root of this repo, run the following command to start the gdb console

    ```cmd
    arm-none-eabi-gdb.exe -q --interpreter=mi2
    ```

1. Within the gdb console, run the following commands:

    ```cmd
    file ./mimxrt1060/demos/projects/NXP/mimxrt1060/iot-middleware-sample-adu.elf
    target remote localhost:50000
    monitor reset
    load
    ```

1. Wait until the JLink console shows that programming flash has completed

    ```log
    ...
    Downloading 208 bytes @ address 0x6018C52C
    Comparing flash   [....................] Done.
    Erasing flash     [....................] Done.
    Programming flash [....................] Done.
    Writing register (PC = 0x601008bc)
    ```

1. Exit in the gdb console (this will close the JLink connection as well)

    ```cmd
    quit
    ```

1. Reset the device, and the new image should be loaded.

## Using VSCode

Please see instructions for debugging using VSCode [here](VSCodeDebug.md). To load the ADU application, you only need to go through the steps through where it is paused at the `reset_handler`. At this point, you can stop the debugger and reset the device to run the application.

## Using MCUXpresso IDE

### MCUXpresso IDE Prerequisites

* Install the [MCUXpresso IDE](https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools-/mcuxpresso-integrated-development-environment-ide:MCUXpresso-IDE).
* Download the device SDK for the EVK-MIMXRT1060
    1. Open MCUXpresso IDE
    1. Click `Help` -> `Welcome` if the welcome screen is not already opened
    1. Click `Download and Install SDKs`
    1. Filter by `evkmimxrt1060`
    1. Click `Install`
* Create a new dummy MCUXpresso IDE project for the NXP MIMXRT1060-EVK Evaluation kit
![img](media/MCUXpresso-board-selection.png)
* Setup the Debugger

    1. Right click on the project in the Project Explorer -> Debug As -> Debug Configurations.
    1. Add a new Launch Configuration under `C/C++ (NXP Semiconductors) MCU Application`.
    1. In the C/C++ Application field, find the ADU .elf file generated at `iot-middleware-freertos-samples/mimxrt1060/demo/projects/NXP/mimxrt1060/iot-middleware-sample-adu.elf`.
    1. Click Apply.

### Load the image to the device

* Reopen the Debug Configurations and select the one you have just created.
* Click Debug.
![img](media/MCUXpresso-debug.png)
* The image is now placed correctly on the device and you can stop the debugger, reset the board, and have the application running as normal.
