# Azure IoT Bicep Deployment

This guide will detail how to easily deploy the necessary Azure resources to run our samples using the command line and the Azure CLI. The following resources and sku levels will be deployed:

- Azure IoT Hub (S1)
- Azure Device Provisioning Service (S1)

## Prerequisites

- An Azure Account ([link here if you do not already have one](https://azure.microsoft.com/free/search/)).

- One of the following:
    - Have open an Azure Portal "Cloud Shell". This doesn't require any installation of further tools and may be easier for most people.
    - The latest version of Azure CLI and Azure IoT Module.
        See steps to install both [here](https://learn.microsoft.com/azure/iot-hub-device-update/create-update?source=recommendations#prerequisites).

## Set Up Azure Resources

In order to create the resources, we are going to use an Azure Resource Manager (ARM) template, written using [Bicep](https://learn.microsoft.com/azure/azure-resource-manager/bicep/overview?tabs=bicep). The file detailing all the resources to be deployed is located in [azure-iot-arm.bicep](./azure-iot-arm.bicep).

### Deploy Resources

If you chose the Cloud Shell option, open an instance of the Azure Cloud Shell by going to the top of your Azure Portal browser window and selecting the terminal icon. On hover, it should say "Cloud Shell".

Once that is open, click the "Upload/Download" icon and find and upload the `azure-iot-arm.bicep` file in the same directory as this README. You should now see the file listed if you run the `ls` command in your terminal.

If you chose the local CLI option, on your command line with the Azure CLI installed, navigate to the `demos/` directory where this README is located. Run the `az login` command to make sure you are logged in to your account.

Once you have done either of those, run the following

```bash

# For any < > value, substitute in the value of your choice.

# Find the subscription ID of your choice
az account list
az account set --subscription <subscription id>

# To see a list of locations, run the following and look for `name` values.
az account list-locations | ConvertFrom-Json | format-table -Property name

# Create a resource group
az group create --name '<name>' --location '<location>'

# Deploy the resources. This will take a couple minutes.
#  - `resourcePrefix`: choose a unique, short name with only lower-case letters which
#                      will be prepended to all of the deployed resources.
#  - `name`: choose a name to give this deployment, between 3 and 24 characters in length and
#            use numbers and lower-case letters only.
az deployment group create --name '<deployment name>' --resource-group '<name>' --template-file './azure-iot-arm.bicep' --parameters location='<location>' resourcePrefix='<your prefix>'
```

Once this function returns, you should be able to see your deployed resources on the [Azure Portal](https://portal.azure.com). You may use the deployed resources for the walkthroughs of the samples in this repository.
