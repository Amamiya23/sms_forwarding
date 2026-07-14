# 优化 WebUI 转发设置与整体 UI

## Goal

让转发设置页更好用（含 SMTP 可测性），修正短信转发默认标题过长问题，并克制地提升整体 WebUI 观感。

## Background

- 转发设置页：`code/web_src/panels/push.html` + `app.js` + `app.css`。
- 推送通道已有 `POST|GET /testpush`；SMTP 无对等测试入口。
- 短信默认 Subject 现为 `短信 · {sender} · {message}`（`components/idf_email/idf_email.cpp`），会把整段短信塞进标题。
- 用户可自定义 Subject；NVS override 存在时优先，否则走编译默认。
- 当前主题：场测仪暗色一等公民 + 琥珀强调 + 2px 直角。
- 静态资源经 `python tools/build_web_assets.py` 打入固件。

## Decisions

| 决策 | 选择 |
|------|------|
| 默认短信 Subject | `短信 · {sender}`（不覆盖已有 NVS override） |
| SMTP 测试数据源 | 表单当前值；密码留空回退已保存密码 |
| UI 范围 | 全站 tokens/配色 + 转发设置页信息架构重排 |

## Requirements

### R1 — 转发设置页信息架构
- 邮件 / 推送 / 过滤与规则 / 邮件模板分层更清晰。
- 不改变既有保存表单契约（字段名与 `emailForm`/`pushForm`/`filterForm`/`callForm`/`rulesForm` 兼容）。

### R2 — SMTP 测试
- SMTP 区提供「测试发送」按钮与结果反馈。
- 使用表单当前值；`smtpPass` 空则回退已存密码。
- 鉴权 + CSRF；发送可观测（异步排队 + 状态轮询，对齐推送测试）。

### R3 — 默认短信标题
- 编译默认 Subject 改为 `短信 · {sender}`。
- 完整短信仅在正文；「恢复默认」得到新默认。
- 同步 `preview/`、docs/spec 中的默认描述。

### R4 — 全站 UI tokens + 转发页重排
- 调整配色、圆角、间距、层次；暗/浅主题可读。
- 工具型控制台气质；不引入前端框架；不改路由与既有元素 ID 契约（除新增 SMTP 测试控件）。

## Acceptance Criteria

- [x] AC1：默认短信 Subject 为 `短信 · {sender}`，不含完整 `{message}`。
- [x] AC2：SMTP 可用表单当前值测试；密码留空回退已存；成功/失败在 UI 可见。
- [x] AC3：转发设置页分区清晰；保存、规则测试、推送测试仍可用。
- [x] AC4：全站 tokens 更现代克制；暗/浅主题对比度正常。
- [x] AC5：`python tools/build_web_assets.py --check`、`node --check code/web_src/app.js` 通过。

## Out of Scope

- 非 SMTP 邮件通道、SPA 重写、规则 DSL 变更、强制迁移自定义 Subject、验证码智能标题。
