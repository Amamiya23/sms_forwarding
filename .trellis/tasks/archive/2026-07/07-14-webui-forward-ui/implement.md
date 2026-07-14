# Implement: WebUI 转发设置与整体 UI

## Ordered checklist

1. **Default SMS subject**
   - [x] `components/idf_email/idf_email.cpp`：`DEFAULT_SUBJECTS` SMS → `短信 · {sender}`
   - [x] `preview/build_preview.py` 同步默认 subject
   - [x] 如有 `dev_doc` / `.trellis/spec` 写死旧默认字符串则更新

2. **SMTP test backend**
   - [x] `idf_push.h` / `idf_push.cpp`：`idf_push_enqueue_smtp_test` + `idf_push_smtp_test_status_json` + worker 槽
   - [x] `idf_web.cpp`：`handle_test_smtp` + 注册 `/testsmtp`（GET status / POST enqueue）
   - [x] Auth + CSRF；pass 空回退已存；不写 NVS
   - [x] `dev_doc/api_reference.md` 补 API 一行说明

3. **SMTP test frontend**
   - [x] `panels/push.html`：测试按钮 + `#smtpTestResult`
   - [x] `app.js`：读表单字段、POST、轮询、按钮态（可复用 testPush 模式）
   - [x] `preview/index.html` mock `/testsmtp`（若预览走 mock）

4. **Forward page layout**
   - [x] 重排 `push.html` 分区与文案（左邮件/推送、右过滤/规则、底模板）
   - [x] 必要的 panel-scoped CSS（避免破坏其他页）

5. **Global tokens**
   - [x] `app.css`：`:root` + light theme radius/accent/surface/grid
   - [x] 扫一眼按钮/开关/card/result-box 是否仍协调

6. **Assets + checks**
   - [x] `python tools/build_web_assets.py`（或 `--check`）
   - [x] `node --check code/web_src/app.js`
   - [x] `git diff --check`
   - [x] 有 ESP-IDF 环境时再做全量固件构建（可选本机）

## Validation commands

```bash
python tools/build_web_assets.py --check
node --check code/web_src/app.js
git diff --check
```

可选：`preview/build_preview.py` 后打开预览核对 push 页与主题。

## Risky files / rollback points

| 文件 | 风险 |
|------|------|
| `idf_push.cpp` | worker 并发/SMTP 超时；测试槽必须互斥 |
| `idf_web.cpp` | handler 数量上限 `max_uri_handlers`；确认仍够 |
| `app.css` | 全站视觉回归 |
| `push.html` / `app.js` | 表单 name/id 破坏保存或模板编辑器 |

回滚：还原上述文件即可；无数据迁移。

## Review gates before `task.py start`

- [x] PRD 收敛（无未决 Open Questions）
- [x] design.md 覆盖 subject / SMTP test / layout / tokens
- [x] implement.md 有序可执行
- [ ] 用户确认可进入实现
