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

Known-good explicit Windows build for the local ESP-IDF 6.0.2 installation:

```powershell
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 build `
  -IdfPath 'C:\tools\esp\.espressif\v6.0.2\esp-idf' `
  -IdfToolsPath 'C:\Espressif\tools' `
  -IdfPythonEnvPath 'C:\Espressif\tools\python\v6.0.2\venv' `
  -BuildDir 'build\idf-local' -Jobs 4
```

`tools/idf.ps1` accepts `-IdfPath`, `-IdfToolsPath`, and `-IdfPythonEnvPath`. When Espressif Installation Manager metadata is present, the script must use the selected installation's generated activation script.

`-BuildDir` optionally selects an isolated output directory. Use it when an existing CMake cache was created with a different generator; do not reuse a Visual Studio cache for the required Ninja build.

### 3. Contracts

- `IDF_PATH`: ESP-IDF 6.0 source directory.
- `IDF_TOOLS_PATH`: standard ESP-IDF tools root for `export.ps1`, or the value installed by an EIM activation script.
- `IDF_PYTHON_ENV_PATH`: ESP-IDF Python virtual environment root.
- EIM metadata: `C:\Espressif\tools\eim_idf.json`; selected entry fields used by the build wrapper are `path`, `idfToolsPath`, `python`, and `activationScript`.
- The wrapper, not a raw `cmake` invocation, owns environment activation, the Ninja generator, `SDKCONFIG=build/sdkconfig`, reconfiguration, and compilation.
- A relative `-BuildDir` is resolved from the repository root. All output assertions must use the selected build directory, not assume `build/idf`.
- Automated runners must allow enough time for the first full build. If the runner times out, check whether the Ninja process started by that invocation is still alive before starting another build.
- MbedTLS 4 cryptography must use public PSA APIs for HMAC and streaming SHA-256. TLS certificate verification must remain `MBEDTLS_SSL_VERIFY_REQUIRED` with `esp_crt_bundle_attach`.

### 4. Validation & Error Matrix

- Missing selected EIM activation script -> fall back to standard `export.ps1` only when `IDF_PATH` and tool/Python roots are valid.
- Missing `export.ps1` -> stop with `ESP-IDF export script not found`.
- `generator: Ninja does not match Visual Studio` or CMake identifies MSVC -> the build directory contains a foreign CMake cache. Do not retry it and do not diagnose this as missing ESP-IDF environment variables; select a fresh `-BuildDir`.
- `PermissionError: [WinError 5]` from Python `asyncio.windows_utils.pipe` before CMake starts -> the managed runner blocked child-process pipes. Re-run the same wrapper command outside that sandbox when authorized; do not change the ESP-IDF paths or CMake cache to work around it.
- Runner timeout with a surviving Ninja process -> do not launch a second Ninja against the same directory. Confirm ownership by PID/start time, stop only that invocation's orphan, then resume the same `-BuildDir` with a longer timeout.
- No surviving build process after timeout -> run the same wrapper command again; Ninja resumes incrementally from completed objects.
- Removed/unknown Kconfig symbol -> remove or migrate it; do not leave project-owned warnings in reconfigure output.
- Missing public MbedTLS header/API -> migrate to PSA or ESP-IDF public APIs; never add `mbedtls/private/**` include paths.
- TLS setup or certificate-bundle attachment failure -> fail the SMTP/HTTPS operation without disabling verification.

### 5. Good/Base/Bad Cases

- Good: run the explicit command above in a fresh directory; CMake reports Ninja and the RISC-V compiler, then produces app, bootloader, and partition-table binaries.
- Base: EIM is selected and `tools/idf.ps1 build` activates its profile with the default `build/idf` directory.
- Bad: invoke raw CMake from a Visual Studio shell, reuse its cache for ESP-IDF, or start overlapping Ninja processes after a runner timeout.

### 6. Tests Required

- Run the wrapper command and assert exit code 0. For managed runners, use a timeout large enough for the first full ESP-IDF build.
- Let `<build-dir>` be the actual default or `-BuildDir` value. Assert `<build-dir>/sms_forwarding_idf.bin`, `<build-dir>/bootloader/bootloader.bin`, and `<build-dir>/partition_table/partition-table.bin` exist.
- Confirm CMake output says `Building ESP-IDF components for target esp32c3`; MSVC as the C/C++ compiler is a failure even if the RISC-V assembler is found.
- Run `python tools/build_web_assets.py --check` and `git diff --check`.
- Inspect the linked ELF for `psa_mac_compute`, `psa_hash_setup`, `psa_hash_update`, `psa_hash_finish`, and `esp_crt_bundle_attach` when crypto APIs change.

### 7. Wrong vs Correct

#### Wrong

```powershell
# Raw CMake can select Visual Studio and poison the cache for later IDF builds.
cmake -B build\idf .
powershell -File tools\idf.ps1 build
```

```powershell
# Never kill every compiler/Ninja process on the machine after a timeout.
Get-Process ninja | Stop-Process -Force
```

#### Correct

```powershell
# Keep environment activation and generator selection in the wrapper.
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 build `
  -IdfPath 'C:\tools\esp\.espressif\v6.0.2\esp-idf' `
  -IdfToolsPath 'C:\Espressif\tools' `
  -IdfPythonEnvPath 'C:\Espressif\tools\python\v6.0.2\venv' `
  -BuildDir 'build\idf-local' -Jobs 4
```

If a managed runner timed out, inspect processes first and stop only the Ninja PID known to belong to that invocation.

#### Wrong Crypto

```cpp
#include "mbedtls/private/ctr_drbg.h"
mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
```

#### Correct Crypto

```cpp
#include "psa/crypto.h"
mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
if (esp_crt_bundle_attach(&conf) != ESP_OK) return false;
```
