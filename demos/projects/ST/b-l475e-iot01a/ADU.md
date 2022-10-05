
Generate the update manifest using **powershell**.

```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope Process
Import-Module .\iot-hub-device-update\tools\AduCmdlets\AduUpdate.psm1
$updateId = New-AduUpdateId -Provider "ST" -Name "STM32L475" -Version 1.6
$compat = New-AduUpdateCompatibility -Properties @{ deviceManufacturer = 'ST'; deviceModel = 'STM32L475' }
$installStep = New-AduInstallationStep -Handler 'microsoft/swupdate:1'-HandlerProperties @{ installedCriteria = '1.6' } -Files C:\repos\iot-middleware-sample-adu.1.6.bin
$update = New-AduImportManifest -UpdateId $updateId -Compatibility $compat -InstallationSteps $installStep
$update | Out-File "./$($updateId.provider).$($updateId.name).$($updateId.version).importmanifest.json" -Encoding utf8
```
