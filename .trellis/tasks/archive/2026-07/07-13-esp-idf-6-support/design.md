# ESP-IDF 6.0 Compatibility Design

## Baseline

The project will target ESP-IDF 6.0, validated against the locally installed 6.0.2 toolchain. ESP-IDF 5.5.x compatibility is intentionally dropped.

## Compatibility Boundary

`components/idf_push` is the primary boundary because it directly owns HTTP(S), SMTP implicit TLS, SMTP STARTTLS, Base64, and HMAC-SHA256 operations. `components/idf_web` also uses MbedTLS digest helpers and must compile unchanged or receive the smallest required API migration.

MbedTLS 4 in ESP-IDF 6 uses PSA-backed randomness for TLS. The STARTTLS path will stop owning `mbedtls_ctr_drbg_context` and `mbedtls_entropy_context`, and will no longer call the removed `mbedtls_ssl_conf_rng`. The existing public SSL, socket, certificate-bundle, Base64, and digest APIs will remain where ESP-IDF 6 still exposes them.

No private MbedTLS headers will be added. In particular, the implementation must not include `mbedtls/private/ctr_drbg.h` merely to preserve old code.

## Build And Release

- `tools/idf.ps1` will default to/detect an ESP-IDF 6.0 installation instead of 5.5.4.
- GitHub Actions will build with an ESP-IDF 6.0 container.
- README and developer documentation will describe ESP-IDF 6.0 as the supported baseline.
- `sdkconfig.defaults` will be reconciled with ESP-IDF 6.0 only when the build reports renamed or removed options.

## Validation

Run a clean/reconfigured ESP-IDF 6.0.2 build through the repository wrapper. Iterate only on concrete compiler, configuration, and linker failures. Verify generated application, bootloader, and partition-table images exist. Run available repository checks that do not require physical hardware or external credentials.

## Risk And Rollback

TLS behavior is the highest-risk area. Certificate verification remains required and the certificate bundle remains attached. If a public MbedTLS 4 API cannot preserve STARTTLS behavior, prefer an ESP-IDF public TLS API over private headers. Rollback is the task-scoped source/config/documentation diff; NVS and on-device data formats are unchanged.
