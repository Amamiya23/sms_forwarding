# Implementation Plan

## 1. Template Domain And Persistence

- Add email kind, template part, limits, defaults, validation, escaping, and single-pass rendering contracts.
- Add lazy `smsdata/mailtpl` blob persistence for overrides with get/save/reset APIs.
- Include overrides in config export/import and erase them during factory reset.
- Verify: invalid/oversized values do not replace existing overrides; missing storage resolves to defaults.

## 2. Typed Email Queue And MIME

- Extend email jobs to retain structured typed fields and existing retry/completion metadata.
- Classify SMS, Call, Heartbeat, Keepalive, and System producers explicitly.
- Render Subject, plain text, and HTML immediately before SMTP send.
- Change SMTP body to `multipart/alternative` with base64 text/plain and text/html parts.
- Verify: both provided SMS examples, HTML metacharacters, multiline text, unknown caller, retry requeue, and default fallback.

## 3. Template HTTP API

- Add authenticated, CSRF-protected get/save/reset endpoints for one template part at a time.
- Add bounded preview endpoint using the same backend renderer and representative fields.
- Return precise validation errors and default/customized metadata.
- Verify: unauthorized access rejected, limits checked before allocation, reset returns defaults, preview cannot persist data.

## 4. Dashboard Editor

- Add full-width template workspace below SMTP settings with Base plus five type tabs.
- Add Subject/body editors, placeholder insertion controls, dirty/saving/error state, Save and Restore Default actions.
- Render debounced preview responses in a permissionless sandbox iframe.
- Add responsive desktop/mobile layout without nested cards or unstable dimensions.
- Update `preview/build_preview.py` mocks for the new endpoints and representative templates.
- Verify: base edits affect all preview types; body/subject edits affect only selected type; mobile layout does not overlap.

## 5. Generated Assets And Documentation

- Regenerate `code/web_assets.cpp` and `code/web_assets.h` from `code/web_src`.
- Update relevant API/module documentation for template storage, endpoints, kinds, placeholders, and MIME behavior.
- Verify: `python tools/build_web_assets.py --check` and `python preview/build_preview.py`.

## 6. Full Quality Gate

- Run the Trellis check workflow against backend, frontend, cross-layer data flow, and acceptance criteria.
- Run the configured ESP-IDF build through `tools/idf.ps1 build` or the available project build command.
- Inspect generated Dashboard preview at desktop and mobile sizes if browser tooling is available.
- Review `git diff` for generated-only churn and confirm existing user changes remain untouched.

## Risk And Rollback Points

- Keep persistence changes additive and isolate them in `smsdata/mailtpl`.
- Do not change SMTP credential storage keys.
- Preserve the old plain-text field generation as the multipart fallback.
- If a type-specific producer cannot provide structured fields, classify it as System instead of guessing another type.
- A rollback can remove new endpoints and rendering while leaving ignored template override data in `smsdata`.
