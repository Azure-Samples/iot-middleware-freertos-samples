# ADU Update

## ESP32 Board Directions

- Build the image with the next version: [LINK HERE](https://github.com/Azure-Samples/iot-middleware-freertos-samples/blob/1f63c83e2642203fa003b74067a7421f76ad1cfc/demos/sample_azure_iot_adu/sample_azure_iot_adu.c#L160)
- Upload the image to blob storage.
- Update the partition layout on the board by running the following

```bash
idf.py menuconfig

# Go to "Serial flasher config" and change the Flash size to 4MB
# Go back to partition table and enter the "Custom partition CSV file" to partitions_ota.csv
# Save the config

```

- Then change the version back and build the image again and flash.

## Current Stats

This uses an HTTP buffer of 4KB.

| Stage | Time |
| ----- | ---- |
| Boot to First Request | 10 seconds |
| Download Whole Image | 120 seconds |
| Reboot and Show New Version | 20 seconds |
| Total | 150 seconds |
