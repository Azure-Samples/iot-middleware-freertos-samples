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
az deployment group create --name '<deployment name>' --resource-group '<name>' --template-file './ca-recovery-arm.bicep' --parameters location='<location>' resourcePrefix='<your prefix'

*/

@description('Prefix to apply to all of the resources')
param resourcePrefix string = 'azrecov'

@description('Location for all resources.')
param location string = resourceGroup().location

var runtime = 'dotnet'
var iothubName = '${resourcePrefix}-iothub'
var dpsName = '${resourcePrefix}-dps'
var functionAppName = '${resourcePrefix}-fnapp'
var functionName = '${resourcePrefix}-fn'
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
          name: 'AzureWebJobsStorage'
          value: 'DefaultEndpointsProtocol=https;AccountName=${storageAccountName};EndpointSuffix=${environment().suffixes.storage};AccountKey=${storageAccount.listKeys().keys[0].value}'
        }
        {
          name: 'WEBSITE_CONTENTAZUREFILECONNECTIONSTRING'
          value: 'DefaultEndpointsProtocol=https;AccountName=${storageAccountName};EndpointSuffix=${environment().suffixes.storage};AccountKey=${storageAccount.listKeys().keys[0].value}'
        }
        {
          name: 'FUNCTIONS_EXTENSION_VERSION'
          value: '~4'
        }
        {
          name: 'WEBSITE_LOAD_CERTIFICATES'
          value: '*'
        }
        {
          name: 'APPINSIGHTS_INSTRUMENTATIONKEY'
          value: applicationInsights.properties.InstrumentationKey
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

resource function 'Microsoft.Web/sites/functions@2021-03-01' = {
  name: functionName
  parent: functionApp
  properties: {
    config: {
      disabled: false
      bindings: [
        {
          name: 'req'
          type: 'httpTrigger'
          direction: 'in'
          authLevel: 'function'
          methods: [
            'get'
            'post'
          ]
        }
        {
          name: '$return'
          type: 'http'
          direction: 'out'
        }
      ]
    }
    files: {
      'recovery.csx': loadTextContent('./az-function/recovery.csx')
    }
  }
}
