# Email Template Contracts

## Scenario: Typed SMTP HTML Rendering

### 1. Scope / Trigger

- Applies when adding an SMTP notification kind, changing template placeholders, changing template storage, or modifying the template HTTP API.
- Keep SMTP credentials in the existing config namespace. Store only template overrides in `smsdata/mailtpl`; a missing key means the compiled default.

### 2. Signatures

```cpp
bool idf_email_render(const IdfEmailData& data, IdfRenderedEmail& out, std::string& error);
esp_err_t idf_email_save_template(IdfEmailTemplatePart part, IdfEmailKind kind,
                                  const std::string& value, std::string& error);
bool idf_push_enqueue_email_data(const IdfEmailData& data);
```

```text
GET|POST|DELETE /emailtemplate?part=base|body|subject&kind=sms|call|heartbeat|keepalive|system
POST /emailpreview?part=base|body|subject&kind=sms|call|heartbeat|keepalive|system
```

### 3. Contracts

- Base is a complete HTML document and must contain `{content}`. Its limit is 8192 UTF-8 bytes.
- Body is a kind-specific HTML fragment with a 4096-byte limit. Subject is plain text with a 256-byte stored limit.
- Render replacement is single-pass. Dynamic HTML values are escaped; only rendered body HTML may fill `{content}`.
- SMTP DATA is UTF-8 `multipart/alternative` with base64 `text/plain` followed by base64 `text/html`.
- Queues retain `IdfEmailData`, not rendered HTML, so retries use the latest valid template.
- Template overrides are included in config export/import and cleared by factory reset.

### 4. Validation & Error Matrix

| Condition | Required behavior |
|---|---|
| Missing required placeholder | Return `ESP_ERR_INVALID_ARG`; preserve the previous override |
| Invalid UTF-8 or byte limit exceeded | Reject before NVS write |
| `script`, `iframe`, `object`, `embed`, `form`, event handler, `javascript:`, or meta refresh | Reject the template |
| Missing/corrupt override | Use the compiled default |
| Runtime render failure | Log the kind and render with compiled defaults |
| Subject contains CR/LF | Strip CR/LF before RFC 2047 encoding |
| Unauthenticated request or missing CSRF on mutation/preview | Reject through the existing Web auth helpers |

### 5. Good/Base/Bad Cases

- Good: enqueue an SMS with structured sender, receiver, timestamp, and message fields; render immediately before SMTP send.
- Base: no NVS overrides exist after firmware upgrade, so all five kinds use compiled defaults without migration.
- Bad: flatten SMS fields into one HTML string before enqueueing or rescan replacement output for placeholders.

### 6. Tests Required

- Build with ESP-IDF 6.0.2 and assert app, bootloader, and partition binaries exist.
- Run `python tools/build_web_assets.py --check`, `node --check code/web_src/app.js`, and `git diff --check`.
- Verify SMS metacharacters are escaped, multiline messages retain line breaks, and placeholder-looking message text is not replaced again.
- Verify all five producers select the matching `IdfEmailKind`, and retry requeues the same structured data.
- Verify invalid imports do not commit configuration or overwrite valid template overrides.

### 7. Wrong vs Correct

#### Wrong

```cpp
idf_push_enqueue_email("短信", "<div>" + untrusted_message + "</div>");
```

#### Correct

```cpp
IdfEmailData data;
data.kind = IdfEmailKind::Sms;
data.sender = sender;
data.message = message;
idf_push_enqueue_email_data(data);
```
