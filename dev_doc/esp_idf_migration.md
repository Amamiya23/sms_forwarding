# ESP-IDF 迁移状态

迁移已完成：固件现在只保留原生 ESP-IDF 工程，不再保留 Arduino fallback sketch。

## 工程边界

- 固件入口：`CMakeLists.txt`、`main/`、`components/`。
- Web UI 源文件：`code/web_src/`。
- 生成的 gzip Web 资源：`code/web_assets.h`、`code/web_assets.cpp`，由 `components/web_assets` 作为 IDF component 链接。
- 构建输出：只写入 `build/`。
- 本机推荐入口：`powershell -ExecutionPolicy Bypass -File tools\idf.ps1 build`。
- 烧录入口：`powershell -ExecutionPolicy Bypass -File tools\idf.ps1 flash -Port COM5`。

## 当前实现

- `idf_wifi`：STA、失败兜底 SoftAP、captive DNS、轻量 mDNS、SNTP 三服务器、运行中 BOOT 长按配网、重连看门狗。
- `idf_modem`：UART1 单 owner、AT 串行化、模组上电/重启恢复、注册/信号/身份采样、蜂窝 MHTTP、USSD、短信发送 AT 通道。
- `idf_sms`：PDU 收发、`+CMTI`/`+CMT`/`CMGL` 双路径接收、长短信合并、去重、黑名单、管理员命令、收件箱入库、转发入队。
- `idf_push`：HTTP/HTTPS 推送、SMTP over TLS/STARTTLS、转发规则、重试队列、失败冷却、测试推送、Server酱等 10 类通道。
- `idf_web`：Web UI/API、Basic Auth、状态 JSON、日志、短信收发、WiFi 配网、AT 终端、OTA、保号/每日心跳/定时重启调度。
- `idf_config`：NVS 配置读写，保留旧 key 兼容与默认值迁移。

## 验证

```powershell
python tools\build_web_assets.py --check
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 build
```

### Windows 已验证的显式环境

```powershell
powershell -ExecutionPolicy Bypass -File tools\idf.ps1 build `
  -IdfPath 'C:\tools\esp\.espressif\v6.0.2\esp-idf' `
  -IdfToolsPath 'C:\Espressif\tools' `
  -IdfPythonEnvPath 'C:\Espressif\tools\python\v6.0.2\venv' `
  -BuildDir 'build\idf-local' -Jobs 4
```

成功时 CMake 应显示 Ninja、`Building ESP-IDF components for target esp32c3`，并在所选构建目录生成：

- `sms_forwarding_idf.bin`
- `bootloader/bootloader.bin`
- `partition_table/partition-table.bin`

### 构建故障判别顺序

1. 先确认脚本输出的 `IDF_PATH`、`IDF_TOOLS_PATH`、`IDF_PYTHON_ENV_PATH` 与预期一致。
2. 如果出现 `generator Ninja does not match Visual Studio`，或 C/C++ 编译器被识别为 MSVC，说明构建目录缓存被其他生成器污染。不要继续修改环境变量，直接指定一个新的 `-BuildDir`。
3. 如果在 CMake 启动前出现 Python `asyncio.windows_utils.pipe` 的 `WinError 5`，这是自动化沙箱拒绝创建子进程管道，不是 ESP-IDF 安装错误。获准后在沙箱外重跑同一条脚本命令。
4. 如果自动化命令超时，先检查本轮启动的 Ninja 是否仍在运行。不要立即启动第二个构建；只终止能够通过 PID 和启动时间确认属于本轮的孤立进程，再用更长超时和同一 `-BuildDir` 增量续跑。
5. 不得使用 `Get-Process ninja | Stop-Process` 这类全局终止命令，以免破坏其他终端或项目的构建。

构建脚本是唯一推荐入口：它负责 EIM/`export.ps1` 激活、Ninja 生成器、`build/sdkconfig` 和增量编译。不要用裸 CMake 绕过脚本。

设备侧仍需按场景人工验证：短信收发、长短信合并、推送/邮件实际投递、OTA 上传、WiFi 配网 SoftAP、BOOT 长按配网、保号 MHTTP/USSD、弱信号/无 SIM/漫游场景。
