# OTA Certificates and Keys

This directory should contain your SSL/TLS certificates and keys for HTTPS OTA updates.

## Required Files

### For Server Certificate Validation (Required)
- `server_cert.pem` - Server certificate in PEM format
  - This is the certificate of your OTA server
  - Used to validate the server's identity during HTTPS connections

### For Client Authentication (Optional)
- `client_cert.pem` - Client certificate in PEM format
  - Used for mutual TLS authentication
  - Only needed if your server requires client certificates

- `client_key.pem` - Client private key in PEM format
  - Private key corresponding to the client certificate
  - Only needed if using client certificates

## File Format

All certificates and keys must be in PEM format. Example:

```
-----BEGIN CERTIFICATE-----
MIIDXTCCAkWgAwIBAgIJAKoK/OvJ8mQkMA0GCSqGSIb3DQEBCwUAMEUxCzAJBgNV
BAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBX
aWRnaXRzIFB0eSBMdGQwHhcNMTYwNzE5MTUzNzE5WhcNMTcwNzE5MTUzNzE5WjBF
MQswCQYDVQQGEwJBVTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50
ZXJuZXQgV2lkZ2l0cyBQdHkgTHRkMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB
CgKCAQEA...
-----END CERTIFICATE-----
```


## Example Usage

In your application code, you can reference these files like this:

```c
#include "ota_manager.h"

// Server certificate (required)
extern const char server_cert_pem_start[] asm("_binary_server_cert_pem_start");
extern const char server_cert_pem_end[] asm("_binary_server_cert_pem_end");

// Client certificate (optional)
extern const char client_cert_pem_start[] asm("_binary_client_cert_pem_start");
extern const char client_cert_pem_end[] asm("_binary_client_cert_pem_end");

// Client private key (optional)
extern const char client_key_pem_start[] asm("_binary_client_key_pem_start");
extern const char client_key_pem_end[] asm("_binary_client_key_pem_end");

ota_config_t ota_config = {
    .url = "https://your-ota-server.com/firmware.bin",
    .cert_pem = server_cert_pem_start,
    .client_cert_pem = client_cert_pem_start,
    .client_key_pem = client_key_pem_start,
    .timeout_ms = 10000,
    .skip_cert_common_name_check = false,
};
```

## Getting Certificates

### For Development/Testing
- Use self-signed certificates for testing
- Use Let's Encrypt for free SSL certificates
- Use your organization's certificate authority

### For Production
- Use certificates from trusted certificate authorities
- Implement proper certificate management
- Consider using certificate pinning for additional security 