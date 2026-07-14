# SMTP Test Contracts

## Scenario: Dashboard SMTP Configuration Test

### 1. Scope / Trigger

- Applies when adding or changing the SMTP "send test" path, push-worker test slots, or Web UI SMTP test UX.
- Real SMTP sessions can take tens of seconds; the HTTP handler must not block on the full SMTP dialog.

### 2. Signatures

```cpp
bool idf_push_enqueue_smtp_test(const IdfEmailSettingsView& cfg, std::string& message);
std::string idf_push_smtp_test_status_json(void);
```

```text
POST /testsmtp
GET  /testsmtp?action=status
```

### 3. Contracts

- Auth required for both methods. POST requires CSRF (`X-SMS-CSRF` via existing helpers / `ensure_get_or_post`).
- POST body: `application/x-www-form-urlencoded` fields `smtpServer`, `smtpPort`, `smtpUser`, `smtpPass`, `smtpSendTo`.
- **Form current values**: handler builds an in-memory `IdfEmailSettingsView` from the body. Empty `smtpPass` falls back to the saved password from `idf_config_get_email_settings_view()`. Test does **not** write NVS.
- `emailConfigured` is derived from the assembled view (server, user, pass, sendTo all non-empty).
- Single global SMTP test slot: `pending → running → done`. Concurrent re-queue while busy returns success with message that a test is already running.
- Worker sends through existing `send_smtp_email` with `IdfEmailKind::System`, title `SMTP 配置测试`, short body; clears password from the slot after completion.
- Response shape (POST enqueue): `{ success, queued, message }`.
- Response shape (GET status): `{ queued, running, done, success, message }`.
- Frontend: read fields from `#mainForm2`, `csrfFetch` POST, poll status; do not require save before test.

### 4. Validation & Error Matrix

| Condition | Required behavior |
|---|---|
| WiFi STA not connected | `success:false`, clear reason in `message` |
| Incomplete SMTP fields after pass fallback | `success:false`, do not enqueue |
| Invalid port | `success:false` |
| Auth / CSRF failure | Reject via existing Web helpers |
| Worker SMTP failure | Status `done=true`, `success=false`, log without printing password |
| Second test while pending/running | `success:true`, message indicates already running |

### 5. Good/Base/Bad Cases

- Good: user edits server/user/to, leaves password blank, clicks test; saved pass is used; UI shows success after poll.
- Base: no prior test job; first POST queues, status ends with `done`/`success`.
- Bad: synchronous SMTP inside the HTTP handler, or saving form to NVS as a side effect of test.

### 6. Tests Required

- `python tools/build_web_assets.py --check`, `node --check code/web_src/app.js`.
- Firmware build includes `/testsmtp` registration; `max_uri_handlers` has spare capacity.
- Preview mock for `/testsmtp` returns a completed success JSON.
- Manual: incomplete form → immediate error; complete form with bad host → eventual failure message without crash.

### 7. Wrong vs Correct

#### Wrong

```cpp
// Block httpd for the full SMTP session
send_smtp_email(cfg, data);
httpd_resp_sendstr(req, ok ? "ok" : "fail");
```

#### Correct

```cpp
bool ok = idf_push_enqueue_smtp_test(view, msg);
// return queued JSON; client polls GET /testsmtp?action=status
```
