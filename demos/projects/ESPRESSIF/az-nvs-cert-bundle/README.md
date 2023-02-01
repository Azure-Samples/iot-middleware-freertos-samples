# Non-Volatile Storage (NVS) Trust Bundle Read and Write Example

This sample will read and write a certificate trust bundle (root ca certs) to the ESP32 NVS flash.

## Instructions

In an ESPIDF shell, navigate to the `demos/projects/ESPRESSIF/az-nvs-cert-bundle` and run the following:

```bash
idf.py --no-ccache -B "C:\esptrustbundle" build
```

To flash, run the following:

```bash
idf.py --no-ccache -B "C:\esptrustbundle" -p <COM port> flash
```

And to monitor the output, run the following:

```bash
idf.py -B "C:\esptrustbundle" -p <COM port> monitor
```

At the very end, you should see something like the following:

```log
.......
mJweDyAmH3pvwPuxwXC65B2Xy9J6P9LjrRk5Sxcx0ki69bIImtt2dmefU6xqaWM/5TkshGsRGRxpl/j8nWZjEgQRCHLQzWwa80mMpkg/sTV9HB8Dx6jKXB/ZUhoHHBk2dxEuqPiAppGWSZI1b7rCoucL5mxAyE7+WL85MB+GqQk2dLsmijtWKP6T+MejteD+eMuMZ87zf9dOLITzNy4ZQ5bb0Sr74MTnB8G2+NszKTc0QWbej09+CVgI+WXTik9KveCjCHk9hNAHFiRSdLOkKEW39lt2c0Ui2cFmuqqNh7o0JMcccMyj6D5KbvtwEwXlGjefVwaaZBRA+GsCyRxj3qrg+E
-----END CERTIFICATE-----
Done reading and writing. Moving to infinite loop.
```
