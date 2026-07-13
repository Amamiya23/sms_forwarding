# Quality Guidelines

## Scenario: ESP-IDF 6.0 Build And Crypto Compatibility

### 1. Scope / Trigger

- Applies to firmware source, CMake, `sdkconfig.defaults`, build scripts, and CI changes.
- The supported baseline is ESP-IDF 6.0; local validation currently uses 6.0.2.
- ESP-IDF 5.x compatibility is not required.

### 2. Signatures

```powershell
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 build
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 flash -Port COM5
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 monitor -Port COM5
```

`tools/idf.ps1` accepts `-IdfPath`, `-IdfToolsPath`, and `-IdfPythonEnvPath`. When Espressif Installation Manager metadata is present, the script must use the selected installation's generated activation script.

### 3. Contracts

- `IDF_PATH`: ESP-IDF 6.0 source directory.
- `IDF_TOOLS_PATH`: standard ESP-IDF tools root for `export.ps1`, or the value installed by an EIM activation script.
- `IDF_PYTHON_ENV_PATH`: ESP-IDF Python virtual environment root.
- EIM metadata: `C:\Espressif\tools\eim_idf.json`; selected entry fields used by the build wrapper are `path`, `idfToolsPath`, `python`, and `activationScript`.
- MbedTLS 4 cryptography must use public PSA APIs for HMAC and streaming SHA-256. TLS certificate verification must remain `MBEDTLS_SSL_VERIFY_REQUIRED` with `esp_crt_bundle_attach`.

### 4. Validation & Error Matrix

- Missing selected EIM activation script -> fall back to standard `export.ps1` only when `IDF_PATH` and tool/Python roots are valid.
- Missing `export.ps1` -> stop with `ESP-IDF export script not found`.
- Removed/unknown Kconfig symbol -> remove or migrate it; do not leave project-owned warnings in reconfigure output.
- Missing public MbedTLS header/API -> migrate to PSA or ESP-IDF public APIs; never add `mbedtls/private/**` include paths.
- TLS setup or certificate-bundle attachment failure -> fail the SMTP/HTTPS operation without disabling verification.

### 5. Good/Base/Bad Cases

- Good: EIM is selected, `tools/idf.ps1 build` activates its profile and produces app, bootloader, and partition-table binaries.
- Base: a standard ESP-IDF 6.0 installation is supplied through the three environment variables and builds via `export.ps1`.
- Bad: compiling against ESP-IDF 6 by adding private MbedTLS CTR-DRBG headers or disabling certificate verification.

### 6. Tests Required

- Run `powershell -ExecutionPolicy Bypass -File tools\idf.ps1 build` and assert exit code 0.
- Assert `build/idf/sms_forwarding_idf.bin`, `build/idf/bootloader/bootloader.bin`, and `build/idf/partition_table/partition-table.bin` exist.
- Run `python tools/build_web_assets.py --check` and `git diff --check`.
- Inspect the linked ELF for `psa_mac_compute`, `psa_hash_setup`, `psa_hash_update`, `psa_hash_finish`, and `esp_crt_bundle_attach` when crypto APIs change.

### 7. Wrong vs Correct

#### Wrong

```cpp
#include "mbedtls/private/ctr_drbg.h"
mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
```

#### Correct

```cpp
#include "psa/crypto.h"
mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
if (esp_crt_bundle_attach(&conf) != ESP_OK) return false;
```
