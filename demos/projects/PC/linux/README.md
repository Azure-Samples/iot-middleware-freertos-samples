# Use Linux to simulate a device using Azure IoT Middleware for FreeRTOS

## Prerequisite Note

If using WSL to build this project, it is **HIGHLY** recommended to clone the project to the file system inside the WSL instance. Do not clone the repo to your Windows file system and open the WSL working directory to a Windows formatted directory. Clone times, build times, directory max lengths, among other problems may arise.

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

* To run this sample you can use a device previously created in your IoT Hub or have the Azure IoT Middleware for FreeRTOS provision your device automatically using DPS. **Note** that even when using DPS, you still need an IoT Hub created and connected to DPS. If you haven't deployed the necessary Azure resources yet, [you may use the guide here](https://github.com/Azure-Samples/iot-middleware-freertos-samples/blob/main/docs/azure-bicep-deployment.md).

IoT Hub | DPS
---------|----------
Have an [Azure IoT Hub](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal) created | Have an instance of [IoT Hub Device Provisioning Service](https://docs.microsoft.com/azure/iot-dps/quick-setup-auto-provision#create-a-new-iot-hub-device-provisioning-service)
Have a [logical device](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal#register-a-new-device-in-the-iot-hub) created in your Azure IoT Hub using your preferred authentication method* | Have an [individual or group enrollment](https://docs.microsoft.com/azure/iot-dps/how-to-manage-enrollments#create-a-device-enrollment) created in your instance of DPS using your preferred authentication method*

*While this sample supports SAS keys and Certificates, this guide will refer only to SAS keys.

## Prepare the simulation

To connect the simulated device to Azure, you'll modify a configuration file for Azure IoT settings, rebuild the image, and run it.

Update the file `demo_config.h` with your configuration values.

```bash
nano demos/projects/PC/linux/config/demo_config.h
```

If you're using a device previously created in your **IoT Hub** with SAS authentication, disable DPS by commenting out `#define democonfigENABLE_DPS_SAMPLE` and set the following parameters:

Parameter | Value
---------|----------
 `democonfigDEVICE_ID` | _{Your Device ID value}_
 `democonfigHOSTNAME` | _{Your Host name value}_
 `democonfigDEVICE_SYMMETRIC_KEY` | _{Your Primary Key value}_

If you're using **DPS** with an individual or group enrollment with SAS authentication, set the following parameters:

Parameter | Value
---------|----------
 `democonfigID_SCOPE` | _{Your ID scope value}_
 `democonfigREGISTRATION_ID` | _{Your Device Registration ID value}_
 `democonfigDEVICE_SYMMETRIC_KEY` | _{Your Primary Key value}_*

 *For group enrollment, generate the device symmetric key using the registration ID and the group primary key. See the [official instructions](https://learn.microsoft.com/en-us/azure/iot-dps/how-to-legacy-device-symm-key?tabs=windows&pivots=programming-language-csharp#derive-a-device-key) for details.

To use the **DPS** Certificate Signing Request feature, provide also the following parameters:

Parameter | Value
---------|----------
 `democonfigENABLE_DPS_CSR` | _{Uncomment/define to enable DPS CSR}_
 `democonfigCERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY_PEM` | _{The certificate private key used to generate the certificate signing request}_
 `democonfigCERTIFICATE_SIGNING_REQUEST_DATA` | _{A Certificate Signing Request generated using the client private key}_

To use the **IoT Hub** Certificate Signing Request feature (CSR sent after IoT Hub connection), provide also the following parameters:

Parameter | Value
---------|----------
 `democonfigENABLE_IOT_HUB_CSR` | _{Uncomment/define to enable IoT Hub CSR}_
 `democonfigCERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY_PEM` | _{The certificate private key used to generate the certificate signing request}_
 `democonfigCERTIFICATE_SIGNING_REQUEST_DATA` | _{A Certificate Signing Request generated using the client private key}_
 `democonfigCERTIFICATE_SIGNING_REQUEST_ID` | _{A unique request ID (4-36 ASCII alphanumeric chars and dashes)}_

> **Note**: You can enable both `democonfigENABLE_DPS_CSR` and `democonfigENABLE_IOT_HUB_CSR` to perform CSR during DPS provisioning and then again after connecting to IoT Hub. The `democonfigCERTIFICATE_SIGNING_REQUEST_DATA` and `democonfigCERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY_PEM` macros are shared by both flows.

**For non-production purposes (i.e., for test-only)**, the parameters above can be generated using the script below:

Linux:

```bash
REGISTRATION_ID="your registration id"
CSR_KEY_PEM_FILE_PATH=$(pwd)/${REGISTRATION_ID}-csr-private-key.pem

openssl ecparam -name prime256v1 -genkey -noout | openssl pkcs8 -topk8 -nocrypt -out $CSR_KEY_PEM_FILE_PATH
CSR_BASE64=$(openssl req -new -key $CSR_KEY_PEM_FILE_PATH -subj "/CN=$REGISTRATION_ID" -outform DER | openssl base64 -A)

echo "#define democonfigCERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY_PEM \\"; cat $CSR_KEY_PEM_FILE_PATH | sed "s/^/\"/g" | sed "s/$/\\\r\\\n\" \\\/g" | sed '$s/ \\$//'

echo "#define democonfigCERTIFICATE_SIGNING_REQUEST_DATA    \"$CSR_BASE64\""
```

Windows (PowerShell):

```powershell
$REGISTRATION_ID = "your registration id";

$privateKey = [System.Security.Cryptography.ECDsa]::Create([System.Security.Cryptography.ECCurve]::CreateFromFriendlyName("nistP256"))
if ($PSVersionTable.PSVersion.Major -lt 7) {
  $base64pkcs8PrivateKey = [Convert]::ToBase64String($privateKey.Key.Export([System.Security.Cryptography.CngKeyBlobFormat]::Pkcs8PrivateBlob), 'InsertLineBreaks')
} else {
  $base64pkcs8PrivateKey = [Convert]::ToBase64String($privateKey.ExportPkcs8PrivateKey(), 'InsertLineBreaks')
}

$dn = New-Object System.Security.Cryptography.X509Certificates.X500DistinguishedName("CN=$REGISTRATION_ID")
$csr = New-Object System.Security.Cryptography.X509Certificates.CertificateRequest($dn, $privateKey, [System.Security.Cryptography.HashAlgorithmName]::SHA256)
$CSR_BASE64 = [Convert]::ToBase64String($csr.CreateSigningRequest())

$FORMATTED_CSR_KEY_PEM = "`"-----BEGIN PRIVATE KEY-----\r\n`" \`n`"" + $($base64pkcs8PrivateKey -replace "[`r]*`n", "\r\n`" \`n`"") + "\r\n`"`n`"-----END PRIVATE KEY-----\r\n`"";

echo "#define democonfigCERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY_PEM \`n$FORMATTED_CSR_KEY_PEM"

echo "#define democonfigCERTIFICATE_SIGNING_REQUEST_DATA    `"$CSR_BASE64`""
```


### Set the Virtual Ethernet Interface

Execute the command below to find which index you got for the `rtosveth1` (index is the number to the left of the interface). Make a note of the number for the next step.

```bash
sudo tcpdump --list-interfaces
```

Look for line #138 in `FreeRTOSConfig.h` and update `configNETWORK_INTERFACE_TO_USE` with the number you got in the previous step.

**Example**: if you got `4.rtosveth1 [Up, Running]` in the previous step, you'll update macro `configNETWORK_INTERFACE_TO_USE` to look like this `#define configNETWORK_INTERFACE_TO_USE ( 4L )`

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
sudo ./build_linux/demos/projects/PC/linux/iot-middleware-sample
```
