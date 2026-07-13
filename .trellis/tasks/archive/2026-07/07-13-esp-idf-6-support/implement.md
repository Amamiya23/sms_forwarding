# Implementation Plan

1. Load backend/project guidelines and inspect ESP-IDF 6.0 public headers for each directly used MbedTLS/ESP-TLS API.
2. Migrate `components/idf_push` away from removed MbedTLS entropy/CTR-DRBG ownership while preserving SMTP implicit TLS and STARTTLS certificate verification.
3. Run an ESP-IDF 6.0.2 build and fix only concrete additional source, CMake, or sdkconfig incompatibilities.
4. Update `tools/idf.ps1`, GitHub Actions, README, and relevant developer docs to make ESP-IDF 6.0 the sole documented baseline.
5. Run the full ESP-IDF build, generated-asset check, and targeted static/test checks; inspect firmware artifacts and final diff.

## Validation Commands

```powershell
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 build -IdfPath C:\tools\esp\.espressif\v6.0.2\esp-idf -IdfToolsPath C:\Espressif
python tools\build_web_assets.py --check
git diff --check
```

## Risky Files

- `components/idf_push/idf_push.cpp`: TLS lifecycle, randomness, and certificate verification.
- `sdkconfig.defaults`: removed options can silently change binary size or TLS behavior.
- `.github/workflows/build.yml`: container tag availability and release build behavior.

## Review Gates

- No private MbedTLS headers or disabled certificate verification.
- No unrelated refactors.
- Build output must identify ESP-IDF 6.0.x and complete linking/size checks.
- Documentation and scripts must not retain ESP-IDF 5.5.x as a supported path.
