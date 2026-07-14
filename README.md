# SMS Forwarding ESP-IDF 固件

<p align="center">
  <a href="https://github.com/MineSunshineone/sms_forwarding/actions/workflows/build.yml"><img alt="CI" src="https://github.com/MineSunshineone/sms_forwarding/actions/workflows/build.yml/badge.svg" /></a>
  <a href="LICENSE"><img alt="License: MIT" src="https://img.shields.io/badge/License-MIT-green.svg" /></a>
  <a href="https://linux.do"><img alt="LINUX DO" src="https://img.shields.io/badge/LINUX-DO-FFB003.svg?logo=data:image/svg%2bxml;base64,DQo8c3ZnIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyIgd2lkdGg9IjEwMCIgaGVpZ2h0PSIxMDAiPjxwYXRoIGQ9Ik00Ni44Mi0uMDU1aDYuMjVxMjMuOTY5IDIuMDYyIDM4IDIxLjQyNmM1LjI1OCA3LjY3NiA4LjIxNSAxNi4xNTYgOC44NzUgMjUuNDV2Ni4yNXEtMi4wNjQgMjMuOTY4LTIxLjQzIDM4LTExLjUxMiA3Ljg4NS0yNS40NDUgOC44NzRoLTYuMjVxLTIzLjk3LTIuMDY0LTM4LjAwNC0yMS40M1EuOTcxIDY3LjA1Ni0uMDU0IDUzLjE4di02LjQ3M0MxLjM2MiAzMC43ODEgOC41MDMgMTguMTQ4IDIxLjM3IDguODE3IDI5LjA0NyAzLjU2MiAzNy41MjcuNjA0IDQ2LjgyMS0uMDU2IiBzdHlsZT0ic3Ryb2tlOm5vbmU7ZmlsbC1ydWxlOmV2ZW5vZGQ7ZmlsbDojZWNlY2VjO2ZpbGwtb3BhY2l0eToxIi8+PHBhdGggZD0iTTQ3LjI2NiAyLjk1N3EyMi41My0uNjUgMzcuNzc3IDE1LjczOWE0OS43IDQ5LjcgMCAwIDEgNi44NjcgMTAuMTU3cS00MS45NjQuMjIyLTgzLjkzIDAgOS43NS0xOC42MTYgMzAuMDI0LTI0LjM4N2E2MSA2MSAwIDAgMSA5LjI2Mi0xLjUwOCIgc3R5bGU9InN0cm9rZTpub25lO2ZpbGwtcnVsZTpldmVubm9kZDtmaWxsOiMxOTE5MTk7ZmlsbC1vcGFjaXR5OjEiLz48cGF0aCBkPSJNNy45OCA3MC45MjZjMjcuOTc3LS4wMzUgNTUuOTU0IDAgODMuOTMuMTEzUTgzLjQyNiA4Ny40NzMgNjYuMTMgOTQuMDg2cS0xOC44MSA2LjU0NC0zNi44MzItMS44OTgtMTQuMjAzLTcuMDktMjEuMzE3LTIxLjI2MiIgc3R5bGU9InN0cm9rZTpub25lO2ZpbGwtcnVsZTpldmVubm9kZDtmaWxsOiNmOWFmMDA7ZmlsbC1vcGFjaXR5OjEiLz48L3N2Zz4=" /></a>
</p>

ESP32-C3 + ML307 系列 4G/LTE 模组的轻量短信转发固件。设备通过 UART/AT 接收 PDU 短信，在本地完成中文短信、长短信合并、去重和规则判断，再经 WiFi 转发到邮件与最多 5 个推送通道；同时支持来电通知、SIM 卡热插拔、eSIM 管理，并提供 Web UI 完成配置、诊断、日志、OTA、保号和定时任务。仅保留 **原生 ESP-IDF** 固件（基于 ESP-IDF 6.0，目标 ESP32-C3）。

## 功能

- **短信收发**：PDU 模式，支持中文/Unicode、长短信合并；双路径补收（`+CMT`/`+CMTI` + `CMGL` 轮询）降低漏收；本地收件箱、管理员命令、黑名单。
- **来电与 SIM**：来电经相同通道通知（仅通知不接听）；运行中 SIM 热插拔自动重初始化注册，换卡免重启。
- **转发通道**：SMTP 邮件 + 最多 5 个推送通道（POST JSON / GET / 自定义模板 / Bark / 钉钉 / 飞书 / PushPlus / Server 酱 / Gotify / Telegram）。
- **转发规则**：按发件人、关键词、正则匹配并分流到指定通道或丢弃，支持页面内本地预览测试。
- **定时任务与保号**：SIM 保号、每日心跳、每日重启，最多 6 个自定义任务，各页保存互不影响。
- **eSIM 管理**：读取 Profile 列表、启用指定 Profile，支持任务后切回原 Profile。
- **Web 管理**：SoftAP 配网 / mDNS / NTP / WiFi 历史、账号密码、蜂窝设置、AT 终端、系统日志、OTA、配置导入导出。

## 快速开始

### 1. 准备环境

- Windows + PowerShell。
- ESP32-C3 开发板（Super Mini 等）+ ML307 系列 4G/LTE 模组 + USB 数据线。
- ESP-IDF 6.0（本地验证版本 6.0.2）。脚本会依次读取 EIM 元数据 `C:\Espressif\tools\eim_idf.json`、或环境变量 `IDF_PATH` / `IDF_TOOLS_PATH` / `IDF_PYTHON_ENV_PATH`。
- Python 3（仅改 Web UI 时用于打包静态资源）。

确认串口号：

```powershell
Get-CimInstance Win32_SerialPort | Select-Object DeviceID,Description
```

### 2. 硬件接线

ESP32-C3 Super Mini 与 ML307x-DC UART 连接：

```text
ESP32-C3 GPIO5  -> ML307 EN
ESP32-C3 GPIO3  -> ML307 RX
ESP32-C3 GPIO4  -> ML307 TX
ESP32-C3 GND    -> ML307 GND
ESP32-C3 5V     -> ML307 VCC
```

默认引脚定义在 ESP-IDF 模组组件中：TXD=GPIO3，RXD=GPIO4，MODEM_EN=GPIO5，LED=GPIO8。

### 3. 编译 + 烧录

所有构建通过 `tools\idf.ps1`，不要直接跑裸 `cmake`（详见下文「常用命令」）。

#### 一键编译 + 烧录（推荐）

`idf.ps1` 提供两个组合动作，一次激活 ESP-IDF 环境即完成编译+烧录（+监控）：

| Action | 流程 |
| --- | --- |
| `buildflash` | 编译 + 烧录 |
| `all` | 编译 + 烧录 + 串口监控 |

```powershell
# 编译 + 烧录 + 监控（烧完直接看日志，Ctrl+] 退出）
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 all -Port COM5

# 只要编译 + 烧录
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 buildflash -Port COM5
```

不想每次手填串口号，用 `tools\go.ps1` 自动探测 USB 串口（CH340/CP210x/JTAG 等）：

```powershell
# 自动探测串口，编译 + 烧录 + 监控
powershell -ExecutionPolicy Bypass -File tools\go.ps1

# 只编译 + 烧录，不进监控
powershell -ExecutionPolicy Bypass -File tools\go.ps1 -NoMonitor

# 探测到多个串口或想指定时
powershell -ExecutionPolicy Bypass -File tools\go.ps1 -Port COM22
```

`go.ps1` 会把 `-BuildDir`、`-IdfPath`、`-IdfToolsPath`、`-IdfPythonEnvPath`、`-Jobs` 透传给 `idf.ps1`；若探测到多个 USB 串口，会列出全部并要求用 `-Port` 指定。

如果自动进入下载模式失败，按住开发板 `BOOT`，轻点 `RST/EN`，开始烧录后松开 `BOOT`（按钮名以板子丝印为准）。

#### 仅编译 / 仅烧录 / 监控

```powershell
# 只编译
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 build

# 已编译过，只重新烧录
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 flash -Port COM5

# 只看串口日志
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 monitor -Port COM5
```

#### 显式指定 ESP-IDF 路径

自动识别到错误安装、或旧构建目录被 Visual Studio CMake 污染时，换新目录并写死路径（路径按本机修改）：

```powershell
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 build `
  -IdfPath 'C:\tools\esp\.espressif\v6.0.2\esp-idf' `
  -IdfToolsPath 'C:\Espressif\tools' `
  -IdfPythonEnvPath 'C:\Espressif\tools\python\v6.0.2\venv' `
  -BuildDir 'build\idf-local' `
  -Jobs 4
```

此时产物在 `build\idf-local\`（例如 `build\idf-local\sms_forwarding_idf.bin`），烧录也要带上同一 `-BuildDir`。

#### 产物

默认构建目录 `build\idf`，默认 `sdkconfig` 在 `build\sdkconfig`。主要产物：

| 文件 | 说明 |
| --- | --- |
| `build\idf\sms_forwarding_idf.bin` | 应用固件（网页 OTA 也可上传这个） |
| `build\idf\bootloader\bootloader.bin` | Bootloader |
| `build\idf\partition_table\partition-table.bin` | 分区表 |

#### 改了 Web UI 后

`code/web_src/` 改完后先打包再编译固件：

```powershell
python tools\build_web_assets.py
python tools\build_web_assets.py --check
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 build
```

`--check` 用于确认产物与源码一致。

#### 清理与重配

```powershell
# 清理当前构建目录对象文件
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 clean

# 彻底清空构建目录后再编（缓存损坏时用）
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 fullclean

# 仅重新 CMake 配置
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 reconfigure
```

#### 注意

- **不要**执行 `cmake -B build\idf .`。Windows 上可能选中 Visual Studio 生成器，与 ESP-IDF 所需的 Ninja 冲突。
- 若报错 `generator: Ninja does not match Visual Studio`，换一个新的 `-BuildDir`（如 `build\idf-local`），不要复用被污染的目录。
- 首次全量编译较久；中断后对同一 `-BuildDir` 再次执行 `build`，Ninja 会增量续编。

### 4. 首次访问 Web UI

首次启动设备优先连接已保存 WiFi；未配置或失败则开启开放热点 `SMS-Forwarder-XXXX`。连接热点后访问 `http://192.168.1.1` 配网；连上路由器后，通过串口日志里的 IP 或 `http://sms.local` 打开 Web UI（主机名可在「系统设置 → 局域网域名」修改，多台设备请设不同主机名）。

默认账号密码 `admin` / `admin123`，首次使用请立即修改。

## 出厂 WiFi 配置

可选：本地创建 `code/wifi_config.h` 作为出厂 WiFi seed（该文件被 `.gitignore` 忽略）：

```cpp
#define WIFI_SSID "your-wifi"
#define WIFI_PASS "your-password"
```

也可以留空，完全走 Web 配网。

## Bark 推送

推送方式选择 `Bark（iOS推送）` 后：

- `Bark 服务器 URL` 留空使用 `https://api.day.app`；自建服务填根地址，如 `https://bark.example.com`，固件发送到 `/push`。
- `Device Key` 填 Bark App 显示的 key；兼容旧配置里直接填 `https://api.day.app/<key>`。
- `可选参数` 用 URL query 风格，例如 `group=SMS&sound=bell&level=timeSensitive`；支持 `subtitle`、`level`、`badge`、`sound`、`group`、`icon`、`url`、`copy`、`isArchive` 等。

## Web UI 预览

截图由 `preview/build_preview.py` 基于 `code/web_src/` 生成，号码/邮箱/Device Key/IMEI 均为 mock 数据。

```powershell
python preview\build_preview.py
start preview\index.html
```

| 系统概览 | 转发设置 |
| --- | --- |
| ![系统概览](assets/ui-overview.png) | ![转发设置](assets/ui-push.png) |

| 收发短信 | 定时任务 |
| --- | --- |
| ![收发短信](assets/ui-inbox.png) | ![定时任务](assets/ui-keepalive.png) |

| 诊断与控制 | 系统日志 |
| --- | --- |
| ![诊断与控制](assets/ui-diagnose.png) | ![系统日志](assets/ui-log.png) |

## 固件分区

面向 4MB Flash，使用自定义分区表 `partitions_ota_1m6.csv`：

| 分区 | 大小 | 用途 |
| --- | ---: | --- |
| `nvs` | 20KB | 系统配置与基础参数 |
| `otadata` | 8KB | OTA 启动选择 |
| `app0` / `app1` | 各 1.625MB | 双 OTA 槽位 |
| `smsdata` | 640KB | 短信/收件箱本地数据（NVS 磨损均衡、掉电原子写入） |
| `coredump` | 64KB | 崩溃转储 |

## 项目架构

固件入口 `main/app_main.cpp`，按「配置 → 网络 → 模组 → 短信 → 推送/Web」顺序初始化。模组 AT 只由 `idf_modem` 访问，短信接收只负责解码入队，推送/邮件交给后台 worker，Web handler 只做参数解析和轻量响应。

```text
main/app_main.cpp
  ├─ idf_config      NVS 配置、默认值、表单保存、导入导出
  ├─ idf_wifi        STA/SoftAP 配网、DNS 门户、mDNS、SNTP、重连 watchdog
  ├─ idf_modem       UART1 单 owner、AT 串行化、蜂窝 HTTP/SMS/USSD
  ├─ idf_sms         PDU 收发、长短信合并、去重、黑名单、管理员命令
  ├─ idf_push        邮件/推送 worker、失败重试、通道冷却、规则判断
  ├─ idf_web         HTTP 路由、Web UI、OTA、状态、日志、定时任务
  ├─ idf_esim        eUICC APDU/ES10c 基础能力与配置入口
  ├─ idf_inbox       收件箱/已发送记录的本地环形缓存
  ├─ idf_logbuf      Web 可见日志环形缓冲
  ├─ idf_pdu         PDU 编解码组件
  └─ web_assets      gzip Web 静态资源
```

## 开发入口

- `main/`：ESP-IDF app entry。
- `components/idf_*`：配置、WiFi、Web、模组、短信、推送、日志、收件箱。
- `components/web_assets`：打包后的 Web 静态资源组件。
- `code/web_src/`：可编辑 Web UI 源。
- `code/web_assets.*`：生成文件，不手改。
- `dev_doc/`：架构和迁移说明。

更多工程约定见 `AGENTS.md`。

## 致谢

感谢 [LINUX DO](https://linux.do) 社区提供交流与灵感。
