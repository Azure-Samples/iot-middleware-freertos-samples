/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/*

# For any < > value, substitute in the value of your choice.
az login
az account set --subscription <subscription id>

# To see a list of locations, run the following and look for `name` values.
az account list-locations

# Create a resource group
az group create --name '<name>' --location '<location>'

# Deploy the resources
az deployment group create --name '<deployment name>' --resource-group '<name>' --template-file './azure-iot-arm.bicep' --parameters location='<location>' resourcePrefix='<your prefix'

*/

@description('Prefix to apply to all of the resources')
param resourcePrefix string = 'az-demo'

@description('Location for all resources.')
param location string = resourceGroup().location

var iothubName = '${resourcePrefix}-iothub'
var dpsName = '${resourcePrefix}-dps'

resource iothub 'Microsoft.Devices/IotHubs@2021-07-02' = {
  location: location
  name: iothubName
  sku: {
    capacity: 1
    name: 'S1'
  }
}

resource dps 'Microsoft.Devices/provisioningServices@2022-02-05' = {
  location: location
  name: dpsName
  sku: {
    capacity: 1
    name: 'S1'
  }
  properties: {
    iotHubs: [
      {
        connectionString: 'HostName=${iothub.properties.hostName};SharedAccessKeyName=iothubowner;SharedAccessKey=${iothub.listkeys().value[0].primaryKey}'
        location: location
      }
    ]
  }
}
