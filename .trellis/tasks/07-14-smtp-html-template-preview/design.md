# Technical Design

## Architecture

The feature spans configuration storage, SMTP rendering, notification producers, HTTP APIs, and the Dashboard.

1. `idf_config` owns persisted template overrides in the existing `smsdata` partition under a new `mailtpl` namespace.
2. `idf_push` owns the template catalog, field rendering, HTML escaping, default templates, plain-text fallback generation, and MIME assembly.
3. Notification producers pass an explicit email kind and structured fields instead of pre-flattening every message into one text body.
4. `idf_web` exposes authenticated template read/save/reset/preview endpoints and keeps the existing SMTP credential form unchanged.
5. `code/web_src` provides the editor and sandboxed preview; generated `code/web_assets.*` remains the firmware source of truth.

## Template Model

`IdfEmailTemplateKind` has five values: `Sms`, `Call`, `Heartbeat`, `Keepalive`, and `System`.

Stored parts:

- `base`: shared complete HTML/CSS document, required `{content}`, maximum 8192 UTF-8 bytes.
- `body.<kind>`: one HTML fragment per kind, maximum 4096 UTF-8 bytes.
- `subject.<kind>`: one plain-text subject per kind, maximum 256 UTF-8 bytes.

Only overrides are stored. Missing keys resolve to compiled defaults. Reset deletes the selected override key, so future firmware can improve defaults without migrations.

Use NVS blobs rather than strings for HTML parts so the base template is not constrained by the NVS string entry limit. The template namespace is opened lazily from the initialized `smsdata` partition. Access is serialized independently of inbox locks.

## Placeholder Contract

Replacement is a single left-to-right scan. Replacement output is never scanned again.

| Kind | HTML and Subject fields |
|---|---|
| SMS | `{sender}`, `{receiver}`, `{timestamp}`, `{message}` |
| Call | `{caller}`, `{receiver}`, `{timestamp}` |
| Heartbeat | `{title}`, `{message}`, `{timestamp}`, `{sms_total}`, `{free_heap}` |
| Keepalive | `{title}`, `{message}`, `{timestamp}`, `{action}`, `{result}` |
| System | `{title}`, `{message}`, `{timestamp}` |

HTML field values are escaped. Multiline values convert normalized newlines to `<br>` after escaping. Subject values remain plain text and are stripped of CR/LF before RFC 2047 encoding. The base template only receives rendered body HTML through `{content}`.

## Validation

Saving validates UTF-8, byte limits, required placeholders, and active content. Base requires `{content}`. Each body requires the field that carries its primary information: SMS `{message}`, Call `{caller}`, Heartbeat `{message}`, Keepalive `{result}` or `{message}`, System `{message}`.

Case-insensitive validation rejects at least `script`, `iframe`, `object`, `embed`, `form`, inline event-handler attributes, `javascript:` URLs, and unsafe meta refresh. Preview uses an iframe without sandbox permissions as defense in depth.

Validation failure returns a JSON error and leaves the previous override untouched. Runtime load or render failure logs the template kind and uses compiled defaults.

## Email Job Contract

Replace the implicit subject/body-only email job with a typed payload containing:

- kind
- title/message
- sender/receiver/caller
- timestamp
- heartbeat counters when present
- keepalive action/result when present
- inbox/completion retry metadata

The queue stores message fields, not rendered HTML or all templates. Rendering occurs immediately before SMTP send, so the latest saved template applies to queued mail and retry copies remain bounded.

Add a dedicated call enqueue path or an explicit forward kind so incoming calls remain subject to existing routing while selecting the Call template.

## MIME Output

SMTP assembly uses a generated boundary and emits:

1. Standard headers and encoded Subject.
2. `Content-Type: multipart/alternative`.
3. Base64 UTF-8 `text/plain` generated from structured fields.
4. Base64 UTF-8 `text/html` generated from base plus body templates.

The terminating SMTP dot remains outside the MIME body. Existing TLS, authentication, retry, completion, and inbox-forwarded behavior remains unchanged.

## HTTP API

Use a dedicated authenticated endpoint rather than enlarging the existing `/save` form:

- `GET /emailtemplate?part=base`
- `GET /emailtemplate?part=body&kind=sms`
- `GET /emailtemplate?part=subject&kind=sms`
- `POST /emailtemplate?...` with raw UTF-8 body to validate and save one part.
- `DELETE /emailtemplate?...` to erase one override and restore the compiled default.
- `POST /emailpreview?kind=sms` to render unsaved base/body/subject plus bounded sample fields and return rendered HTML/Subject or validation errors.

The raw-body API avoids URL-encoding expansion for HTML. Requests enforce per-part lengths before allocation and use existing authentication/CSRF conventions. Responses are no-cache JSON with escaped values.

## Dashboard UX

The SMTP credentials remain at the top of the forwarding settings. A full-width template workspace follows:

- Tabs: Base, SMS, Call, Heartbeat, Keepalive, System.
- Base tab: complete HTML/CSS editor and shared preview selector.
- Type tabs: Subject input, body HTML editor, compact placeholder insertion controls, Save and Restore Default commands.
- Desktop: editor and preview in two stable columns.
- Mobile: editor then preview, with fixed-height controls and touch-sized actions.
- Preview: light neutral canvas containing a sandboxed iframe; iframe content uses the selected type's sample data.
- Status: dirty, saving, saved, invalid, or default/customized without layout shift.

The default email is a restrained system-notification design with type-specific accents and no external assets.

## Backup And Reset

Configuration export appends escaped template override records. Import validates every template before committing overrides; invalid template records fail without replacing existing valid values. Factory reset erases the `mailtpl` namespace. Redacted and full exports both include templates because they are not credentials.

## Compatibility And Rollback

- No schema migration is required: absent keys mean defaults.
- Existing SMTP credentials and enable flags stay in the current NVS namespace.
- Old firmware ignores the separate `mailtpl` namespace.
- Rolling back loses HTML use but preserves current SMTP settings and message forwarding.
- If `smsdata` template storage is unavailable, sending continues with compiled defaults.

## Main Risks

- Heap pressure during HTML plus MIME base64 assembly: bound templates, render only one kind, avoid storing rendered HTML in the queue, and verify maximum allocation during firmware build/device testing where available.
- Preview/editor mismatch: route preview through the firmware renderer rather than implementing a second placeholder engine only in JavaScript.
- Mail client CSS inconsistency: defaults use table-based structure, inline critical styles, and conservative CSS; custom templates remain user responsibility after validation.
- Notification misclassification: update every SMTP producer and add explicit kind coverage in review checks.
