# Local MSISDN Contracts

## Scenario: Display And Email Receiver Number

### 1. Scope / Trigger

- Applies when changing CNUM parsing, overview `phone` field, email `{receiver}`, or push local-number placeholders.
- Some modules/SIMs mis-decode international TOA byte `0x91` as BCD digits `"19"`, turning `+4475…` into `+194475…`.

### 2. Signatures

```cpp
// idf_modem: after AT+CNUM parse
static std::string normalize_msisdn(std::string phone);
static std::string parse_cnum_phone(const std::string& resp);

// Display / email local number sources
// Web status JSON "phone"
// idf_push local_phone_number()
```

### 3. Contracts

- **Manual override wins**: if config `phoneNumber` is non-empty, overview and email/push local number use it; else use modem CNUM phone.
- CNUM path: extract second quoted field from `+CNUM:` line, then run `normalize_msisdn`.
- `normalize_msisdn` strips a leading digit prefix `19` only when the remainder starts with a known country calling code **other than** `1` (NANP), and remaining length is long enough for a national number. Example: `+1944756828…` → `+44756828…`.
- Do not strip `19` when the remainder would be treated as bare NANP (`cc == "1"`) to avoid mangling real `+1917…` style numbers.
- Never log raw SMTP or SIM secrets; phone numbers may appear in status JSON and email templates as user-facing identity.
- Email preview samples must use a plausible E.164 receiver (not a known corrupted `+19…` sample).

### 4. Validation & Error Matrix

| Condition | Required behavior |
|---|---|
| Empty CNUM and empty manual | Display `--` / empty receiver; do not invent a number |
| Non-empty manual | Always prefer manual for overview and mail `{receiver}` |
| CNUM `+1944…` (TOA artifact) | Normalize to `+44…` before storing modem.phone |
| Letters / non-phone symbols in value | Leave value unchanged (except trim) |
| Firmware upgrade with old wrong modem.phone in RAM | Reboot clears status; next CNUM re-query runs normalize |

### 5. Good/Base/Bad Cases

- Good: user sets network-settings phone to `+4475…`; overview and mail show that even if CNUM is wrong.
- Base: CNUM returns clean `+4475…`; manual empty; display uses modem phone.
- Bad: prefer modem over manual when both set; or store unnormalized `+1944…` into email receiver.

### 6. Tests Required

- Unit-style cases for normalize: `+1944756…` → `+44756…`; `+1917555…` unchanged; short junk unchanged.
- Status JSON: with `phoneNumber` set, `phone` equals manual regardless of modem.phone.
- After merge, regenerate web assets if UI copy for the manual-phone hint changes.

### 7. Wrong vs Correct

#### Wrong

```cpp
json_prop(body, "phone", modem.phone.empty() ? cfg.phoneNumber : modem.phone);
// CNUM garbage always wins over the user override
```

#### Correct

```cpp
json_prop(body, "phone", !cfg.phoneNumber.empty() ? cfg.phoneNumber : modem.phone);
// manual first; CNUM only as fallback after normalize_msisdn
```
