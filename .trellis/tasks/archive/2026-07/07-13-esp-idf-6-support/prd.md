# 支持 ESP-IDF 6.0

## Goal

使固件能够在当前 ESP-IDF 6.0.2 环境下完成构建和烧录，不再因 ESP-IDF 6 引入的 MbedTLS 4 及其他 API 变更而失败。

## Background

- 当前仓库文档、本地构建脚本和 CI 均以 ESP-IDF 5.5.x 为基线：`README.md:109`、`README.md:255`、`tools/idf.ps1:18`、`.github/workflows/build.yml`。
- 用户本机的当前环境是 ESP-IDF 6.0.2，构建在 `components/idf_push/idf_push.cpp:35` 因 `mbedtls/ctr_drbg.h` 缺失而停止。
- `idf_push` 直接使用 MbedTLS 的 base64、HMAC、TLS、entropy/DRBG 和 net socket API，并同时使用 ESP-TLS，是主要兼容边界。
- `idf_web` 直接使用 MbedTLS base64 和 SHA-256 API，也需要在 ESP-IDF 6.0 构建中验证。

## Requirements

- R1. 固件必须在 ESP-IDF 6.0.2 下完成从配置到链接的全量构建。
- R2. HTTP/HTTPS 推送、SMTP 465 隐式 TLS 和 SMTP STARTTLS 的现有行为与证书校验不得被为编译兼容而降级。
- R3. HMAC-SHA256、Base64、Web 鉴权/响应摘要等现有密码学结果必须保持一致。
- R4. 构建脚本、CI 和用户文档必须与实际支持的 ESP-IDF 版本保持一致。
- R5. 兼容实现应尽量使用 ESP-IDF 公开 API，避免依赖 IDF 内部或 OpenThread 私有头文件路径。
- R6. 项目基线整体升级到 ESP-IDF 6.0，不再保证 ESP-IDF 5.5.x 可构建。

## Acceptance Criteria

- [x] AC1. 在 ESP-IDF 6.0.2 环境运行项目构建命令成功，生成 ESP32-C3 的 app、bootloader 和 partition table 镜像。
- [x] AC2. 构建无缺失头文件、移除 API、类型不匹配或链接符号错误。
- [x] AC3. 对可在主机/构建阶段验证的 Base64、HMAC-SHA256 和 SHA-256 路径执行现有或新增的定向检查。
- [x] AC4. README 和 `tools/idf.ps1` 不再默认引导用户使用不匹配的 IDF 版本。
- [x] AC5. CI 至少覆盖 ESP-IDF 6.0 的完整固件构建。

## Scope Boundary

- 包含因 ESP-IDF 6.0 编译必须的源码、CMake、sdkconfig、CI 和文档修改。
- 不包含与 IDF 6 兼容无关的功能重构、UI 调整或业务功能变更。
- 真实 SMTP/HTTPS/WiFi/短信设备联调属于硬件验收；本任务在无可用设备或外部凭据时以构建和静态/定向检查为主。
