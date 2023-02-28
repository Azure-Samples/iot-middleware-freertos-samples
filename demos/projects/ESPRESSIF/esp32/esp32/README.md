# Connect an ESPRESSIF ESP32 using Azure IoT Middleware for FreeRTOS

## What you need

- [ESPRESSIF ESP32 Board](https://www.espressif.com/en/products/devkits)
- Wi-Fi 2.4 GHz
- USB 2.0 A male to Micro USB male data cable
- [ESP-IDF](https://idf.espressif.com/): This guide was tested against [ESP-IDF v4.4.3](https://github.com/espressif/esp-idf/tree/v4.4.3).
- To run this sample you can use a device previously created in your IoT Hub or have the Azure IoT Middleware for FreeRTOS provision your device automatically using DPS. **Note** that even when using DPS, you still need an IoT Hub created and connected to DPS. If you haven't deployed the necessary Azure resources yet, [you may use the guide here](https://github.com/Azure-Samples/iot-middleware-freertos-samples/blob/main/docs/azure-bicep-deployment.md).

  IoT Hub | DPS
  ---------|----------
  Have an [Azure IoT Hub](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal) created | Have an instance of [IoT Hub Device Provisioning Service](https://docs.microsoft.com/azure/iot-dps/quick-setup-auto-provision#create-a-new-iot-hub-device-provisioning-service)
  Have a [logical device](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal#register-a-new-device-in-the-iot-hub) created in your Azure IoT Hub using your preferred authentication method* | Have an [individual enrollment](https://docs.microsoft.com/azure/iot-dps/how-to-manage-enrollments#create-a-device-enrollment) created in your instance of DPS using your preferred authentication method*

  **Instructions on how to create an X.509 cert for tests can be found [here](https://github.com/Azure/azure-sdk-for-c/blob/main/sdk/samples/iot/docs/how_to_iot_hub_samples_linux.md#configure-and-run-the-samples) (Step 1). Please note that you might need to install some of the [prerequisites](https://github.com/Azure/azure-sdk-for-c/blob/main/sdk/samples/iot/docs/how_to_iot_hub_samples_linux.md#prerequisites) like OpenSSL.** 

## Install prerequisites

1. GIT

    Install `git` following the [official website](https://git-scm.com/).

2. ESP-IDF

    On Windows, install the ESPRESSIF ESP-IDF using this [download link](https://dl.espressif.com/dl/esp-idf/?idf=4.4).

    For other Operating Systems or to update an existing installation, follow [Espressif official documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#get-started).

3. Azure IoT Embedded middleware for FreeRTOS


Clone the following repo to download all sample device code, setup scripts, and offline versions of the documentation.

**If you previously cloned this repo in another sample, you don't need to do it again.**

```bash
git clone https://github.com/Azure-Samples/iot-middleware-freertos-samples.git
```

To initialize the repo, run the following commands:

```bash
cd iot-middleware-freertos-samples
git submodule update --init --recursive
```

You may also need to enable long path support for both Microsoft Windows and git:

- Windows: <https://docs.microsoft.com/windows/win32/fileio/maximum-file-path-limitation?tabs=cmd#enable-long-paths-in-windows-10-version-1607-and-later>
- Git: as Administrator run `git config --system core.longpaths true`


## Prepare the sample

To connect the ESPRESSIF ESP32 to Azure, you will update the sample configuration, build the image, and flash the image to the device.

### Update sample configuration

The configuration of the ESPRESSIF ESP32 sample uses ESP-IDF' samples standard [kconfig](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html) configuration.

On a [console with ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#step-4-set-up-the-environment-variables), navigate to the ESP-Azure IoT Kit project directory: `demos\projects\ESPRESSIF\esp32` and run the following commands:

```shell
idf.py menuconfig
```

Under menu item `Azure IoT middleware for FreeRTOS Sample Configuration`, update the following configuration values:

Parameter | Value
---------|----------
 `WiFi SSID` | _{Your WiFi SSID}_
 `WiFi Password` | _{Your WiFi password}_


Under menu item `Azure IoT middleware for FreeRTOS Main Task Configuration`, update the following configuration values:

Parameter | Value
---------|----------
 `Use PnP in Azure Sample` | Enabled by default. Disable this option to build a simpler sample without Azure Plug-and-Play.
 `Azure IoT Hub FQDN` | _{Your Azure IoT Hub Host FQDN}_ (Unused if Device Provisioning is enabled below)
 `Azure IoT Device ID` | _{Your Azure IoT Hub device ID}_
 `Azure IoT Module ID` | _{Your Azure IoT Hub Module ID}_ (optional, specify module id if using a device module; else leave blank if not)

Select your desired authentication method with the `Azure IoT Authentication Method () --->`. The default option is `Symmetric Key`:

Parameter | Value
---------|----------
 `Azure IoT Device Symmetric Key` | _{Your Azure IoT Hub device symmetric key}_

If you would like to use x509 certificates, select `X509 Certificates` and update the following values:

Parameter | Value
---------|----------
 `Azure IoT Device Client Certificate` | _{Your Azure IoT Hub device certificate}_
 `Azure IoT Device Client Certificate Private Key` | _{Your Azure IoT Hub device certificate private key}_

Note that the certificate and private key must be a single line string with `\n` characters at the appropriate line breaks. For example:

```txt
# PEM Formatted (WRONG)
-----BEGIN CERTIFICATE-----
MIIBJDCBywIUfeHrebBVa2eZAbouBgACp9R3BncwCgYIKoZIzj0EAwIwETEPMA0G
...
vTfQahwsxN3xink9z1gtirrjQlqDAiEAyU+6TUJcG6d9JF+uJqsLFpsbbF3IzGAw
yC+koNRC0MU=
-----END CERTIFICATE-----

# Single Line (CORRECT)
-----BEGIN CERTIFICATE-----\nMIIBJDCB...\nyC+koNRC0MU=\n-----END CERTIFICATE-----
```

> Some parameters contain default values that do not need to be updated.

If you're using **DPS** with an individual enrollment with SAS authentication, set the following parameters:

Parameter | Value
---------|----------
 `Enable Device Provisioning Sample` | _{Check this option to enable DPS in the sample}_
 `Azure Device Provisioning Service ID Scope` | _{Your ID scope value}_
 `Azure Device Provisioning Service Registration ID` | _{Your Device Registration ID value}_

> Some parameters contain default values that do not need to be updated.

Save the configuration (`Shift + S`) inside the sample folder in a file with name `sdkconfig`.
After that, close the configuration utility (`Shift + Q`).

## Build the image

To build the device image, run the following command (the path `"C:\espbuild"` is only a suggestion, feel free to use a different one, as long as it is near your root directory, for a shorter path):

  ```bash
  idf.py --no-ccache -B "C:\espbuild" build
  ```

## Flash the image

1. Connect the Micro USB cable to the Micro USB port on the ESPRESSIF ESP32 board, and then connect it to your computer.

2. Find the COM port mapped for the device on your system.

    On **Windows** (and using powershell), run the following:

    ```powershell
    Get-WMIObject Win32_SerialPort | Select-Object Name,DeviceID,Description,PNPDeviceID
    ```

    Look for a device with `CP210x-` in the title. The COM port should be something similar to `COM5`.

    On **Linux**, run the following:

    ```shell
    ls -l /dev/serial/by-id/
    ```

    Look for a "CP2102"-based entry and take note of the path mapped for your device (e.g. "/dev/ttyUSB0").

3. Run the following command:

    > This step assumes you are in the ESPRESSIF ESP32 sample directory (same as configuration step above).

    ```bash
    idf.py --no-ccache -B "C:\espbuild" -p <COM port> flash
    ```

    <details>
    <summary>Example...</summary>

    On **Windows**:

    ```shell
    idf.py --no-ccache -B "C:\espbuild" -p COM5 flash
    ```

    On **Linux**:

    ```shell
    idf.py -p /dev/ttyUSB0 flash
    ```
    </details>

## Confirm device connection

You can use any terminal application to monitor the operation of the device and confirm it is set up correctly.

Alternatively you can use ESP-IDF monitor:

```bash
idf.py -B "C:\espbuild" -p <COM port> monitor
```

The output should show traces similar to:

<details>
<summary>See more...</summary>

```shell
$ idf.py -p /dev/ttyUSB0 monitor
Executing action: monitor
Running idf_monitor in directory /iot-middleware-freertos-samples-ew/demos/projects/ESPRESSIF/esp32
Executing "/home/user/.espressif/python_env/idf4.4_py3.8_env/bin/python /esp/esp-idf/tools/idf_monitor.py -p /dev/ttyUSB0 -b 115200 --toolchain-prefix xtensa-esp32-elf- --target esp32 --revision 0 /iot-middleware-freertos-samples-ew/demos/projects/ESPRESSIF/esp32/build/azure_iot_freertos_esp32.elf -m '/home/user/.espressif/python_env/idf4.4_py3.8_env/bin/python' '/esp/esp-idf/tools/idf.py' '-p' '/dev/ttyUSB0'"...
--- idf_monitor on /dev/ttyUSB0 115200 ---
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
ets Jun  8 2016 00:22:57

rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0030,len:6744
load:0x40078000,len:14268
ho 0 tail 12 room 4
load:0x40080400,len:3716
0x40080400: _init at ??:?

entry 0x40080680
I (28) boot: ESP-IDF v4.4-dev-1853-g06c08a9d9 2nd stage bootloader
I (28) boot: compile time 18:49:13
I (28) boot: chip revision: 1
I (33) boot_comm: chip revision: 1, min. bootloader chip revision: 0
I (49) boot.esp32: SPI Speed      : 40MHz
I (49) boot.esp32: SPI Mode       : DIO
I (49) boot.esp32: SPI Flash Size : 2MB
I (53) boot: Enabling RNG early entropy source...
I (59) boot: Partition Table:
I (62) boot: ## Label            Usage          Type ST Offset   Length
I (70) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (77) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (85) boot:  2 factory          factory app      00 00 00010000 00100000
I (92) boot: End of partition table
I (96) boot_comm: chip revision: 1, min. application chip revision: 0
I (103) esp_image: segment 0: paddr=00010020 vaddr=3f400020 size=1d2c8h (119496) map
I (155) esp_image: segment 1: paddr=0002d2f0 vaddr=3ffb0000 size=02d28h ( 11560) load
I (160) esp_image: segment 2: paddr=00030020 vaddr=400d0020 size=89ce0h (564448) map
I (365) esp_image: segment 3: paddr=000b9d08 vaddr=3ffb2d28 size=00a5ch (  2652) load
I (367) esp_image: segment 4: paddr=000ba76c vaddr=40080000 size=1437ch ( 82812) load
I (405) esp_image: segment 5: paddr=000ceaf0 vaddr=50000000 size=00010h (    16) load
I (415) boot: Loaded app from partition at offset 0x10000
I (415) boot: Disabling RNG early entropy source...
I (427) cpu_start: Pro cpu up.
I (428) cpu_start: Starting app cpu, entry point is 0x4008113c
0x4008113c: call_start_cpu1 at /esp/esp-idf/components/esp_system/port/cpu_start.c:150

I (0) cpu_start: App cpu up.
I (442) cpu_start: Pro cpu start user code
I (442) cpu_start: cpu freq: 160000000
I (442) cpu_start: Application information:
I (446) cpu_start: Project name:     azure_iot_freertos_esp32
I (453) cpu_start: App version:      7ac616e-dirty
I (458) cpu_start: Compile time:     Jul 15 2021 22:32:07
I (464) cpu_start: ELF file SHA256:  965ee48390cd1e56...
I (470) cpu_start: ESP-IDF:          v4.4-dev-1853-g06c08a9d9
I (477) heap_init: Initializing. RAM available for dynamic allocation:
I (484) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (490) heap_init: At 3FFB8C48 len 000273B8 (156 KiB): DRAM
I (496) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (503) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (509) heap_init: At 4009437C len 0000BC84 (47 KiB): IRAM
I (516) spi_flash: detected chip: winbond
I (520) spi_flash: flash io: dio
W (524) spi_flash: Detected size(8192k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (538) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (732) wifi:wifi driver task: 3ffba728, prio:23, stack:6656, core=0
I (732) system_api: Base MAC address is not set
I (732) system_api: read default base MAC address from EFUSE
I (752) wifi:wifi firmware version: ff5f4ea
I (752) wifi:wifi certification version: v7.0
I (752) wifi:config NVS flash: enabled
I (752) wifi:config nano formating: disabled
I (762) wifi:Init data frame dynamic rx buffer num: 32
I (762) wifi:Init management frame dynamic rx buffer num: 32
I (772) wifi:Init management short buffer num: 32
I (772) wifi:Init dynamic tx buffer num: 32
I (782) wifi:Init static rx buffer size: 1600
I (782) wifi:Init static rx buffer num: 10
I (782) wifi:Init dynamic rx buffer num: 32
I (792) wifi_init: rx ba win: 6
I (792) wifi_init: tcpip mbox: 32
I (792) wifi_init: udp mbox: 6
I (802) wifi_init: tcp mbox: 6
I (802) wifi_init: tcp tx win: 5744
I (812) wifi_init: tcp rx win: 5744
I (812) wifi_init: tcp mss: 1440
I (812) wifi_init: WiFi IRAM OP enabled
I (822) wifi_init: WiFi RX IRAM OP enabled
I (822) example_connect: Connecting to ContosoWiFi...
I (832) phy_init: phy_version 4670,719f9f6,Feb 18 2021,17:07:07
I (932) wifi:mode : sta (84:cc:a8:4c:7e:fc)
I (932) wifi:enable tsf
I (942) example_connect: Waiting for IP(s)
I (2992) wifi:new:<11,0>, old:<1,0>, ap:<255,255>, sta:<11,0>, prof:1
I (2992) wifi:state: init -> auth (b0)
I (3002) wifi:state: auth -> assoc (0)
I (3012) wifi:state: assoc -> run (10)
I (3022) wifi:connected with ContosoWiFi, aid = 1, channel 11, BW20, bssid = 74:ac:b9:c1:39:76
I (3022) wifi:security: WPA2-PSK, phy: bgn, rssi: -61
I (3022) wifi:pm start, type: 1

I (3042) wifi:AP's beacon interval = 102400 us, DTIM period = 3
W (3042) wifi:<ba-add>idx:0 (ifx:0, 74:ac:b9:c1:39:76), tid:0, ssn:0, winSize:64
I (3612) esp_netif_handlers: example_connect: sta ip: 192.168.1.220, mask: 255.255.255.0, gw: 192.168.1.1
I (3612) example_connect: Got IPv4 event: Interface "example_connect: sta" address: 192.168.1.220
I (3622) example_connect: Connected to example_connect: sta
I (3622) example_connect: - IPv4 address: 192.168.1.220
[INFO] [AzureIoTDemo] [sample_azure_iot.c:580] Creating a TLS connection to contoso-iot-hub.azure-devices.net:8883.

I (6322) tls_freertos: (Network connection 0x3ffc8c4c) Connection to contoso-iot-hub.azure-devices.net established.
[INFO] [AzureIoTDemo] [sample_azure_iot.c:384] Creating an MQTT connection to contoso-iot-hub.azure-devices.net.
...
```
</details>

## Size Chart

The following chart shows the RAM and ROM usage for the ESPRESSIF ESP32 microcontroller.
Build options: Compile optimized for size (-Os) and no logging (-DLIBRARY_LOG_LEVEL=LOG_NONE).
This sample can include either IoT Hub only or both IoT Hub and DPS services. Also it can optionally use IoT Plug-and-Play. The table below shows RAM/ROM sizes considering:

- Middleware libraries only – represents the libraries for Azure IoT connection and features.
- Total size – which includes the Azure IoT middleware for FreeRTOS, Mbed TLS, FreeRTOS, CoreMQTT and the HAL for the dev kit.

|  | Middleware library size | | Total Size | |
|---------|----------|---------|---------|---------
| **Sample** | **Flash (code,rodata)** | **DRAM,IRAM (bss,data)** | **Flash (code,rodata)** | **DRAM,IRAM (bss,data)** |
| IoT Hub + DPS + PnP | 38.13 KB | 12 bytes | 704.81 KB | 119.69 KB
| IoT Hub + DPS | 38.13 KB | 12 bytes | 704.81 KB | 119.69 KB
| IoT Hub + PnP | 28.74 KB | 12 bytes | 694.81 KB | 118.34 KB
| IoT Hub only | 28.73 KB | 12 bytes | 694.65 KB | 118.34 KB
