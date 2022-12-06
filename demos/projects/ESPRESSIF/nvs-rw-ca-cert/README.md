# Non-Volatile Storage (NVS) Trust Bundle Read and Write Example

This sample will read and write a certificate trust bundle (root ca certs) to the ESP32 NVS flash.

## Instructions

In an ESPIDF shell, run the following:

```bash
idf.py --no-ccache -B "C:\esptrustbundle" build
```

To flash, run the following:

```bash
idf.py --no-ccache -B "C:\esptrustbundle" -p <COM port> flash    
```
