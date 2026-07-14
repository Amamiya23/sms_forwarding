# Email Template Editor Contracts

## Scenario: Dashboard Template Editing And Preview

### 1. Scope / Trigger

- Applies to the Dashboard editor for the shared base template and the five email kinds.
- Preview must remain useful for unsaved edits without executing template-provided active content.

### 2. Signatures

```javascript
requestEmailPreview(part, kind, rawUtf8Value, abortSignal)
saveEmailPart(part, kind, rawUtf8Value)
deleteEmailPart(part, kind)
```

### 3. Contracts

- Tabs are Base, SMS, Call, Heartbeat, Keepalive, and System.
- Type tabs edit Subject and body together; Base edits the complete shared HTML/CSS document.
- Fetch and preview use the backend contracts in `backend/email-template-contracts.md`; the browser does not implement a second placeholder engine.
- Preview HTML is assigned only to an iframe with `sandbox=""`; no sandbox permission may be added.
- Desktop uses stable editor/preview columns. Narrow screens stack editor before preview without overlapping controls.

### 4. Validation & Error Matrix

| Condition | Required behavior |
|---|---|
| Backend validation error | Keep editor content, show the returned message, do not report saved |
| Preview request superseded | Abort the older request and ignore its result |
| Body or Subject exceeds its displayed byte limit | Show an over-limit state; backend remains authoritative |
| Panel is no longer active | Do not update detached/stale preview UI |
| Reset succeeds | Reload the backend default and customized state |

### 5. Good/Base/Bad Cases

- Good: independently preview unsaved body and Subject requests, then display HTML from the body response and Subject from the Subject response.
- Base: changing the Base preview kind reuses the current unsaved Base content with another backend sample.
- Bad: render arbitrary editor HTML into the Dashboard DOM with `innerHTML` or grant `allow-scripts` to the preview iframe.

### 6. Tests Required

- Run JavaScript syntax checking and generated asset consistency checking.
- Generate `preview/index.html` and compile every inline script with `new Function`.
- Check Base edits affect each preview kind, while a type body/Subject edit affects only that kind.
- Inspect desktop and mobile layouts when browser tooling permits; verify no overlap and readable editor/preview dimensions.

### 7. Wrong vs Correct

#### Wrong

```javascript
previewHost.innerHTML = templateHtml;
```

#### Correct

```javascript
previewFrame.setAttribute('sandbox', '');
previewFrame.srcdoc = rendered.html;
```
