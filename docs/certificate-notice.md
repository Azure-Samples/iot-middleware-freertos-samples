# Certificate Notice

__IMPORTANT:__

- This repository is not guaranteed to have a complete list of CA certificates used across Azure Clouds or when using Azure Stack, Azure IoT Edge, or Azure Protocol Gateway.
- Always prefer using the local system's Trusted Root Certificate Authority store instead of hardcoded certificates (e.g. the hardcoded certificates used in our `demo_config.h` files).
- Azure Root certificates may change with or without prior notice (e.g., if they expire or are revoked). It is important that devices are able to add or remove trusted root certificates.
- Support for at least two certificates is required to maintain device connectivity during CA certificate changes.

## Additional Information

The certificates used in these samples are DER formats of the root CA certificates. The following openssl command can be used to generate those byte arrays.

```bash
openssl x509 -in <path-to-cert> -C
```

For additional guidance and important information about certificates, please refer to [this blog post](https://techcommunity.microsoft.com/t5/internet-of-things/azure-iot-tls-critical-changes-are-almost-here-and-why-you/ba-p/2393169) from the security team.
