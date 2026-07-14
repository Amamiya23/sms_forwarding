# Channel And SMTP Test UI Contracts

## Scenario: Asynchronous Channel Tests In Dashboard

### 1. Scope / Trigger

- Applies when adding push-channel or SMTP test buttons, result boxes, or polling UX on the forward-settings panel.

### 2. Signatures

```javascript
testPush(idx, btn)
pollTestPush(idx)
testSmtp(btn)
pollTestSmtp()
csrfFetch(url, opt)
```

### 3. Contracts

- Push test: `POST /testpush?ch=` then poll `GET /testpush?action=status&ch=` (saved config; save-first messaging is allowed).
- SMTP test: read current form values from `#mainForm2` (`smtpServer`, `smtpPort`, `smtpUser`, `smtpPass`, `smtpSendTo`), `POST /testsmtp` with urlencoded body, poll `GET /testsmtp?action=status`. Empty password is intentional (backend uses saved pass).
- Disable the clicked button while a test is in flight; re-enable on terminal status or request error.
- Result boxes use `result-box` + `result-loading` / `result-success` / `result-error`.
- Preserve existing form field names (`emailForm`, `pushForm`, …) when rearranging layout.

### 4. Validation & Error Matrix

| Condition | Required behavior |
|---|---|
| HTTP non-OK | Show request failure in result box; re-enable button |
| `success` false on enqueue | Show server `message`; do not start poll loop |
| `queued` or `running` | Keep loading state; poll again (~3s) |
| Terminal `done` | Map `success` to success/error styling |

### 5. Good/Base/Bad Cases

- Good: edit SMTP fields without saving; test uses form values and shows final status.
- Base: push test on a saved channel mirrors existing worker status JSON.
- Bad: put template-preview HTML into the dashboard DOM, or block the UI thread waiting for SMTP.

### 6. Tests Required

- `node --check code/web_src/app.js`
- `python tools/build_web_assets.py --check`
- Preview mocks for `/testpush` and `/testsmtp`

### 7. Wrong vs Correct

#### Wrong

```javascript
// Require save before SMTP test even though backend accepts form body
r.textContent = '请先保存邮件设置';
```

#### Correct

```javascript
// POST current form fields; empty smtpPass is valid
params.set('smtpPass', fd.get('smtpPass') || '');
csrfFetch('/testsmtp', { method: 'POST', body: params.toString(), ... });
```
