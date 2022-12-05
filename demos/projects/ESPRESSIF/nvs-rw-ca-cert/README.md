# Non-Volatile Storage (NVS) Trust Bundle Read and Write Example

This sample will read and write a trust bundle (root ca certs) to the ESP32 NVS flash.

## Sample Options

The sample has two macro defines near the top:

```c
#define WRITE_BALTIMORE
#define WRITE_DIGICERT
```

Make sure only one is defined to write the selected bundle to the NVS. If the bundle has already been written (checked with a trust bundle version), the writing of the bundle is skipped to save on the number of NVS writes.

## Instructions

In an ESPIDF shell, run the following:

```bash
idf.py --no-ccache -B "C:\esptrustbundle" build
```

To flash, run the following:

```bash
idf.py --no-ccache -B "C:\esptrustbundle" -p <COM port> flash    
```
