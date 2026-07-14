# Design: WebUI 转发设置与整体 UI

## Architecture / Boundaries

| 层 | 改动 | 不变 |
|----|------|------|
| Email 默认模板 | 改编译默认 SMS subject | 存储 key、placeholder 集合、render 单遍替换 |
| Push worker | 新增 SMTP 测试入队/状态 | 正式转发队列语义、重试策略 |
| Web API | 新增 `/testsmtp` | 既有 `/testpush`、`/save`、`/emailtemplate` |
| WebUI | `push.html` 重排 + `app.js` 测试逻辑 + `app.css` tokens | 表单字段名、面板 ID、`/ui` 分片加载 |

## R3 — Default SMS subject

**Change**

```text
DEFAULT_SUBJECTS[Sms]: "短信 · {sender} · {message}"  →  "短信 · {sender}"
```

- 仅编译默认；已有 `emailTplSubject.sms` NVS override 不变。
- `preview/build_preview.py` 与任何文档/spec 中的默认样例同步。
- 邮件 HTML body 仍含完整 `{message}`，不影响可读性。

## R2 — SMTP test

### API

```text
POST /testsmtp
  Auth + CSRF
  Body: application/x-www-form-urlencoded
    smtpServer, smtpPort, smtpUser, smtpPass, smtpSendTo
  Response JSON:
    { success, queued, running?, message }

GET /testsmtp?action=status
  Auth
  Response JSON:
    { queued, running, done, success, message }
```

### Semantics

1. Handler 从 body 组装 `IdfEmailSettingsView`（`emailEnabled=true`）。
2. `smtpPass` 为空时填入 `idf_config_get_email_settings_view().smtpPass`。
3. 校验：server / user / sendTo 非空，port 合法，WiFi STA 已连接 → 否则 `success:false` 立即返回原因。
4. `emailConfigured` 对组装后的 view 现算（与正式配置同一完整度规则）。
5. 入队 SMTP 测试任务（单槽，防并发）：`pending → running → done`，与 `TestJob` 推送测试同构。
6. Worker 调用现有 `send_smtp_email` 路径，载荷为 `IdfEmailKind::System`：
   - `title` = `SMTP 配置测试`
   - `message` = 简短说明（设备已用当前参数试发）
7. **不写入 NVS**；测试不影响已保存配置。
8. SMTP 会话最长约 120s，必须异步 + 轮询，避免阻塞 httpd。

### Public API (push)

```cpp
bool idf_push_enqueue_smtp_test(const IdfEmailSettingsView& cfg, std::string& message);
std::string idf_push_smtp_test_status_json(void);
```

存储一份 `IdfEmailSettingsView` 快照在测试槽内，避免 worker 读到半改配置。

### Frontend

- SMTP 卡片底部：`测试发送` 按钮 + `#smtpTestResult` result-box。
- 读 `#mainForm2` 内字段（name 匹配），`csrfFetch POST /testsmtp`，再 poll status。
- 按钮 disabled 期间防重入；文案对齐推送测试风格，但**不**要求先保存。

## R1 — Forward settings layout

`push.html` 信息架构（仍一页）：

1. **邮件通知**：SMTP 字段 + 启用开关 + 保存 + 测试
2. **推送通道**：全局启用 + 通道列表 + 保存
3. **过滤与通知**：管理员号码、来电通知、号码黑名单（右列或次级区）
4. **转发规则**：可视化规则 + 本地测试 + 保存
5. **邮件模板**：现有 workspace 全宽置于下方

保留 `card-grid` 双列，但：
- 左列主路径：邮件 + 推送
- 右列次路径：管理员 / 来电 / 黑名单 / 规则
- 卡片 header 可加简短状态 chip（如邮件已配置/未配置）仅 UI 展示，不改后端

保存按钮保持「卡片后紧跟本表单 submit」，避免合并破坏现有 multipart 表单边界。

## R4 — Global UI tokens

仅 CSS 变量与全局组件微调，不改 DOM 结构（除 push 页）：

| Token 方向 | 建议 |
|------------|------|
| radius | `2px` → `6px`（sm）/ `8px`（md），仍克制 |
| accent | 保留琥珀作主强调，略降饱和；开关成功色与 accent 协调 |
| surfaces | 暗色背景略抬亮层次差；浅色减少灰闷 |
| hairline | 更柔和分隔，减少「仪器网格」压迫（可弱化 body grid 或降低 opacity） |
| type | 保持系统字体 + mono 数据；page-title 略收紧 |

暗/浅主题并行改 `:root` 与 `html[data-theme="light"]`。

禁止：渐变球、大 hero、卡片套卡片、新图标库。

## Data flow (SMTP test)

```text
[UI form fields] --POST /testsmtp--> [idf_web handler]
                                         | assemble view + empty-pass fallback
                                         v
                               idf_push_enqueue_smtp_test(view)
                                         | store snapshot, wake worker
                                         v
                               push worker: send_smtp_email(snapshot, system data)
                                         |
[UI poll GET status] <---------------- status JSON
```

## Compatibility / Rollback

- 升级后：无 custom subject 的设备自动用短标题；有 override 的保持原样。
- 回滚固件：恢复旧默认 subject 与无 `/testsmtp`。
- Web 资源与固件同包，无需单独迁移脚本。

## Trade-offs

| 选项 | 取舍 |
|------|------|
| 异步测试 vs 同步 HTTP | 异步：避免 httpd 超时；代价是轮询复杂度（已有推送先例） |
| 表单值 vs 仅已存 | 表单值：改完即测；实现多一份 snapshot 与 pass 回退 |
| 全站 tokens vs 仅 push | tokens 全站收益大、结构改动小 |
