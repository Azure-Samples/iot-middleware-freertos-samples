/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/*
// Usage (from this directory)

az login
az account set --subscription <subscription id>
az group create --name '<name>' --location '<location>'
az deployment group create --name <deployment name --resource-group '<name>' --template-file './ca-recovery-arm.bicep' --parameters location='<location>'

*/

@description('Prefix to apply to all of the resources')
param resourcePrefix string = 'azrecov'

@description('Location for all resources.')
param location string = resourceGroup().location

var runtime = 'dotnet'
var iothubName = '${resourcePrefix}-iothub'
var dpsName = '${resourcePrefix}-dps'
var functionAppName = '${resourcePrefix}-fnapp'
var hostingPlanName = '${resourcePrefix}-hosting'
var applicationInsightsName = '${resourcePrefix}-insights'
var storageAccountName = '${resourcePrefix}storage'

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

resource storageAccount 'Microsoft.Storage/storageAccounts@2021-06-01' = {
  name: storageAccountName
  location: location
  sku: {
    name: 'Standard_LRS'
  }
  kind: 'StorageV2'
  properties: {
    accessTier: 'Hot'
  }
}

resource hostingPlan 'Microsoft.Web/serverfarms@2021-03-01' = {
  name: hostingPlanName
  location: location
  sku: {
    name: 'Y1'
    tier: 'Dynamic'
  }
  properties: {}
}

resource functionApp 'Microsoft.Web/sites@2021-03-01' = {
  name: functionAppName
  location: location
  kind: 'functionapp'
  identity: {
    type: 'SystemAssigned'
  }
  properties: {
    serverFarmId: hostingPlan.id
    siteConfig: {
      appSettings: [
        {
          name: 'WEBSITE_LOAD_CERTIFICATES=*"'
          value: '*'
        }
        {
          name: 'FUNCTIONS_WORKER_RUNTIME'
          value: runtime
        }
      ]
      ftpsState: 'FtpsOnly'
      minTlsVersion: '1.2'
    }
    httpsOnly: true
  }
}

resource applicationInsights 'Microsoft.Insights/components@2020-02-02' = {
  name: applicationInsightsName
  location: location
  kind: 'web'
  properties: {
    Application_Type: 'web'
    Request_Source: 'rest'
  }
}
