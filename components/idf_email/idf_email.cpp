#include "idf_email.h"

#include <algorithm>
#include <array>
#include <ctype.h>
#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "idf_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char* TAG = "idf_email";
static constexpr const char* NVS_PART = "smsdata";
static constexpr const char* NVS_NS = "mailtpl";
static constexpr size_t BASE_MAX = 8192;
static constexpr size_t BODY_MAX = 4096;
static constexpr size_t SUBJECT_MAX = 256;
static constexpr size_t SUBJECT_RENDER_MAX = 240;

static SemaphoreHandle_t s_mutex = nullptr;

static const char DEFAULT_BASE[] = R"EMAIL(<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
body{margin:0;padding:0;background:#f3f5f7;color:#20262d;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI","Microsoft YaHei",sans-serif}
.mail-shell{width:100%;padding:28px 12px;background:#f3f5f7}
.mail-card{width:100%;max-width:600px;margin:0 auto;background:#ffffff;border:1px solid #dfe4e8;border-radius:8px;overflow:hidden}
.mail-top{padding:18px 24px;border-bottom:1px solid #e7ebee;color:#66717d;font-size:12px}
.mail-content{padding:24px}
.mail-footer{padding:16px 24px;border-top:1px solid #e7ebee;color:#7b858f;font-size:12px;line-height:1.6}
.eyebrow{margin:0 0 8px;color:#66717d;font-size:12px;font-weight:700;text-transform:uppercase}
.title{margin:0;color:#20262d;font-size:22px;line-height:1.35}
.meta{width:100%;margin:20px 0;border-collapse:collapse}
.meta td{padding:8px 0;border-bottom:1px solid #edf0f2;font-size:14px;vertical-align:top}
.meta-label{width:92px;color:#707a84}
.meta-value{color:#20262d;word-break:break-word}
.message{margin-top:18px;padding:16px;border-left:4px solid #168b83;background:#f0f8f7;color:#20262d;font-size:16px;line-height:1.75;word-break:break-word}
.status{display:inline-block;padding:5px 9px;border-radius:4px;font-size:12px;font-weight:700}
.status-ok{background:#eaf7ef;color:#207a42}.status-warn{background:#fff4df;color:#9a5b00}.status-info{background:#eaf2fb;color:#245f9b}.status-bad{background:#fdecec;color:#a23838}
@media(max-width:620px){.mail-shell{padding:0}.mail-card{border-radius:0;border-left:0;border-right:0}.mail-content{padding:20px 16px}.mail-top,.mail-footer{padding-left:16px;padding-right:16px}.meta-label{width:76px}}
</style>
</head>
<body>
<table role="presentation" class="mail-shell" cellspacing="0" cellpadding="0"><tr><td>
<div class="mail-card">
<div class="mail-top">SMS Forwarding</div>
<div class="mail-content">{content}</div>
<div class="mail-footer">此邮件由短信转发设备自动发送。</div>
</div>
</td></tr></table>
</body>
</html>)EMAIL";

static const char DEFAULT_SMS_BODY[] = R"EMAIL(<p class="eyebrow">短信通知</p>
<h1 class="title">收到一条新短信</h1>
<table role="presentation" class="meta" cellspacing="0" cellpadding="0">
<tr><td class="meta-label">来自</td><td class="meta-value">{sender}</td></tr>
<tr><td class="meta-label">本机号码</td><td class="meta-value">{receiver}</td></tr>
<tr><td class="meta-label">时间</td><td class="meta-value">{timestamp}</td></tr>
</table>
<div class="message">{message}</div>)EMAIL";

static const char DEFAULT_CALL_BODY[] = R"EMAIL(<p class="eyebrow">来电提醒</p>
<h1 class="title">有新的来电</h1>
<table role="presentation" class="meta" cellspacing="0" cellpadding="0">
<tr><td class="meta-label">主叫号码</td><td class="meta-value">{caller}</td></tr>
<tr><td class="meta-label">本机号码</td><td class="meta-value">{receiver}</td></tr>
<tr><td class="meta-label">时间</td><td class="meta-value">{timestamp}</td></tr>
</table>
<div class="message" style="border-left-color:#d58b18;background:#fff8eb">请按需回拨或核实来电。</div>)EMAIL";

static const char DEFAULT_HEARTBEAT_BODY[] = R"EMAIL(<p class="eyebrow">设备状态</p>
<h1 class="title">{title}</h1>
<p><span class="status status-ok">运行正常</span></p>
<table role="presentation" class="meta" cellspacing="0" cellpadding="0">
<tr><td class="meta-label">时间</td><td class="meta-value">{timestamp}</td></tr>
<tr><td class="meta-label">累计转发</td><td class="meta-value">{sms_total} 条</td></tr>
<tr><td class="meta-label">空闲内存</td><td class="meta-value">{free_heap} KB</td></tr>
</table>
<div class="message" style="border-left-color:#2d8a4e;background:#eff8f2">{message}</div>)EMAIL";

static const char DEFAULT_KEEPALIVE_BODY[] = R"EMAIL(<p class="eyebrow">保号任务</p>
<h1 class="title">{title}</h1>
<p><span class="status status-info">任务结果</span></p>
<table role="presentation" class="meta" cellspacing="0" cellpadding="0">
<tr><td class="meta-label">执行方式</td><td class="meta-value">{action}</td></tr>
<tr><td class="meta-label">执行时间</td><td class="meta-value">{timestamp}</td></tr>
</table>
<div class="message" style="border-left-color:#2d8a4e;background:#eff8f2">{result}</div>)EMAIL";

static const char DEFAULT_SYSTEM_BODY[] = R"EMAIL(<p class="eyebrow">系统通知</p>
<h1 class="title">{title}</h1>
<table role="presentation" class="meta" cellspacing="0" cellpadding="0">
<tr><td class="meta-label">时间</td><td class="meta-value">{timestamp}</td></tr>
</table>
<div class="message" style="border-left-color:#3477b8;background:#eef5fb">{message}</div>)EMAIL";

static const char* DEFAULT_SUBJECTS[] = {
    "短信 · {sender}",
    "来电 · {caller}",
    "{title} · {timestamp}",
    "{title} · {timestamp}",
    "{title}",
};

static const char* DEFAULT_BODIES[] = {
    DEFAULT_SMS_BODY,
    DEFAULT_CALL_BODY,
    DEFAULT_HEARTBEAT_BODY,
    DEFAULT_KEEPALIVE_BODY,
    DEFAULT_SYSTEM_BODY,
};

static size_t kind_index(IdfEmailKind kind)
{
    size_t index = static_cast<size_t>(kind);
    return index < 5 ? index : 4;
}

const char* idf_email_kind_name(IdfEmailKind kind)
{
    static const char* names[] = {"sms", "call", "heartbeat", "keepalive", "system"};
    return names[kind_index(kind)];
}

bool idf_email_parse_kind(const std::string& value, IdfEmailKind& kind)
{
    for (size_t i = 0; i < 5; ++i) {
        if (value == idf_email_kind_name(static_cast<IdfEmailKind>(i))) {
            kind = static_cast<IdfEmailKind>(i);
            return true;
        }
    }
    return false;
}

const char* idf_email_part_name(IdfEmailTemplatePart part)
{
    switch (part) {
        case IdfEmailTemplatePart::Base: return "base";
        case IdfEmailTemplatePart::Body: return "body";
        case IdfEmailTemplatePart::Subject: return "subject";
    }
    return "body";
}

bool idf_email_parse_part(const std::string& value, IdfEmailTemplatePart& part)
{
    if (value == "base") part = IdfEmailTemplatePart::Base;
    else if (value == "body") part = IdfEmailTemplatePart::Body;
    else if (value == "subject") part = IdfEmailTemplatePart::Subject;
    else return false;
    return true;
}

static const char* default_value(IdfEmailTemplatePart part, IdfEmailKind kind)
{
    if (part == IdfEmailTemplatePart::Base) return DEFAULT_BASE;
    if (part == IdfEmailTemplatePart::Subject) return DEFAULT_SUBJECTS[kind_index(kind)];
    return DEFAULT_BODIES[kind_index(kind)];
}

static size_t part_limit(IdfEmailTemplatePart part)
{
    if (part == IdfEmailTemplatePart::Base) return BASE_MAX;
    if (part == IdfEmailTemplatePart::Subject) return SUBJECT_MAX;
    return BODY_MAX;
}

static std::string storage_key(IdfEmailTemplatePart part, IdfEmailKind kind)
{
    if (part == IdfEmailTemplatePart::Base) return "base";
    return std::string(part == IdfEmailTemplatePart::Body ? "b_" : "s_") + idf_email_kind_name(kind);
}

static bool ensure_mutex()
{
    if (!s_mutex) s_mutex = xSemaphoreCreateMutex();
    return s_mutex != nullptr;
}

static bool open_nvs(nvs_open_mode_t mode, nvs_handle_t& handle)
{
    esp_err_t err = nvs_flash_init_partition(NVS_PART);
    if (err != ESP_OK) return false;
    return nvs_open_from_partition(NVS_PART, NVS_NS, mode, &handle) == ESP_OK;
}

static bool read_override(const std::string& key, std::string& value)
{
    if (!ensure_mutex() || xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) return false;
    nvs_handle_t nvs = 0;
    bool found = false;
    if (open_nvs(NVS_READONLY, nvs)) {
        size_t len = 0;
        if (nvs_get_blob(nvs, key.c_str(), nullptr, &len) == ESP_OK && len > 0 && len <= BASE_MAX) {
            value.resize(len);
            found = nvs_get_blob(nvs, key.c_str(), value.data(), &len) == ESP_OK;
            if (!found) value.clear();
        }
        nvs_close(nvs);
    }
    xSemaphoreGive(s_mutex);
    return found;
}

static esp_err_t write_override(const std::string& key, const std::string& value)
{
    if (!ensure_mutex()) return ESP_ERR_NO_MEM;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) return ESP_ERR_TIMEOUT;
    nvs_handle_t nvs = 0;
    esp_err_t err = open_nvs(NVS_READWRITE, nvs) ? ESP_OK : ESP_FAIL;
    if (err == ESP_OK) err = nvs_set_blob(nvs, key.c_str(), value.data(), value.size());
    if (err == ESP_OK) err = nvs_commit(nvs);
    if (nvs) nvs_close(nvs);
    xSemaphoreGive(s_mutex);
    return err;
}

static esp_err_t erase_override(const std::string& key)
{
    if (!ensure_mutex()) return ESP_ERR_NO_MEM;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) return ESP_ERR_TIMEOUT;
    nvs_handle_t nvs = 0;
    esp_err_t err = open_nvs(NVS_READWRITE, nvs) ? ESP_OK : ESP_FAIL;
    if (err == ESP_OK) {
        err = nvs_erase_key(nvs, key.c_str());
        if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
    }
    if (err == ESP_OK) err = nvs_commit(nvs);
    if (nvs) nvs_close(nvs);
    xSemaphoreGive(s_mutex);
    return err;
}

static bool valid_utf8(const std::string& value)
{
    const unsigned char* p = reinterpret_cast<const unsigned char*>(value.data());
    size_t i = 0;
    while (i < value.size()) {
        unsigned char c = p[i++];
        if (c < 0x80) continue;
        int need = 0;
        uint32_t cp = 0;
        if ((c & 0xE0) == 0xC0) { need = 1; cp = c & 0x1F; if (cp < 2) return false; }
        else if ((c & 0xF0) == 0xE0) { need = 2; cp = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0) { need = 3; cp = c & 0x07; }
        else return false;
        if (i + need > value.size()) return false;
        for (int n = 0; n < need; ++n) {
            unsigned char t = p[i++];
            if ((t & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (t & 0x3F);
        }
        if ((need == 2 && cp < 0x800) || (need == 3 && cp < 0x10000) ||
            cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return false;
    }
    return true;
}

static std::string lower_ascii(const std::string& value)
{
    std::string out = value;
    for (char& c : out) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return out;
}

static bool has_event_handler(const std::string& lower)
{
    for (size_t pos = 0; pos + 2 < lower.size(); ++pos) {
        if (lower[pos] != 'o' || lower[pos + 1] != 'n') continue;
        if (pos > 0 && !isspace(static_cast<unsigned char>(lower[pos - 1])) &&
            lower[pos - 1] != '/') continue;
        size_t i = pos + 2;
        size_t start = i;
        while (i < lower.size() && ((lower[i] >= 'a' && lower[i] <= 'z') || lower[i] == '-')) ++i;
        if (i == start) continue;
        while (i < lower.size() && isspace(static_cast<unsigned char>(lower[i]))) ++i;
        if (i < lower.size() && lower[i] == '=') return true;
    }
    return false;
}

static bool validate_template(IdfEmailTemplatePart part, IdfEmailKind kind,
                              const std::string& value, std::string& message)
{
    if (value.empty()) { message = "模板不能为空"; return false; }
    if (value.size() > part_limit(part)) {
        message = "模板超过 " + std::to_string(part_limit(part)) + " 字节限制";
        return false;
    }
    if (!valid_utf8(value)) { message = "模板不是有效的 UTF-8 文本"; return false; }
    if (part == IdfEmailTemplatePart::Base && value.find("{content}") == std::string::npos) {
        message = "基础模板必须包含 {content}";
        return false;
    }
    if (part == IdfEmailTemplatePart::Body) {
        const char* required = kind == IdfEmailKind::Sms ? "{message}" :
                               kind == IdfEmailKind::Call ? "{caller}" :
                               kind == IdfEmailKind::Heartbeat ? "{message}" :
                               kind == IdfEmailKind::Keepalive ? "{result}" : "{message}";
        if (value.find(required) == std::string::npos) {
            message = std::string("正文模板必须包含 ") + required;
            return false;
        }
    }
    if (part != IdfEmailTemplatePart::Subject) {
        std::string lower = lower_ascii(value);
        static const char* blocked[] = {"<script", "<iframe", "<object", "<embed", "<form", "javascript:"};
        for (const char* token : blocked) {
            if (lower.find(token) != std::string::npos) {
                message = std::string("模板包含不允许的主动内容: ") + token;
                return false;
            }
        }
        if (has_event_handler(lower)) { message = "模板不能包含 on* 事件处理器"; return false; }
        size_t meta = lower.find("<meta");
        if (meta != std::string::npos && lower.find("http-equiv", meta) != std::string::npos &&
            lower.find("refresh", meta) != std::string::npos) {
            message = "模板不能包含自动跳转 meta refresh";
            return false;
        }
    }
    message.clear();
    return true;
}

IdfEmailTemplateValue idf_email_get_template(IdfEmailTemplatePart part, IdfEmailKind kind)
{
    IdfEmailTemplateValue result;
    result.maxBytes = part_limit(part);
    result.customized = read_override(storage_key(part, kind), result.value);
    if (!result.customized) result.value = default_value(part, kind);
    return result;
}

esp_err_t idf_email_save_template(IdfEmailTemplatePart part, IdfEmailKind kind,
                                  const std::string& value, std::string& message)
{
    if (!validate_template(part, kind, value, message)) return ESP_ERR_INVALID_ARG;
    esp_err_t err = write_override(storage_key(part, kind), value);
    if (err != ESP_OK) message = std::string("保存模板失败: ") + esp_err_to_name(err);
    return err;
}

esp_err_t idf_email_reset_template(IdfEmailTemplatePart part, IdfEmailKind kind)
{
    return erase_override(storage_key(part, kind));
}

esp_err_t idf_email_reset_all_templates(void)
{
    if (!ensure_mutex()) return ESP_ERR_NO_MEM;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) return ESP_ERR_TIMEOUT;
    nvs_handle_t nvs = 0;
    esp_err_t err = open_nvs(NVS_READWRITE, nvs) ? ESP_OK : ESP_FAIL;
    if (err == ESP_OK) err = nvs_erase_all(nvs);
    if (err == ESP_OK) err = nvs_commit(nvs);
    if (nvs) nvs_close(nvs);
    xSemaphoreGive(s_mutex);
    return err;
}

static void html_escape_append(std::string& out, const std::string& value)
{
    for (char c : value) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            case '\r': break;
            case '\n': out += "<br>"; break;
            default: out.push_back(c); break;
        }
    }
}

static std::string html_escape(const std::string& value)
{
    std::string out;
    out.reserve(value.size() + value.size() / 8);
    html_escape_append(out, value);
    return out;
}

static std::string subject_value(const std::string& value)
{
    std::string out;
    out.reserve(value.size());
    for (char c : value) if (c != '\r' && c != '\n') out.push_back(c);
    return out;
}

static void truncate_utf8(std::string& value, size_t max_bytes)
{
    if (value.size() <= max_bytes) return;
    size_t end = max_bytes;
    while (end > 0 && (static_cast<unsigned char>(value[end]) & 0xC0) == 0x80) --end;
    value.resize(end);
}

using Replacements = std::vector<std::pair<std::string, std::string>>;

static std::string replace_once(const std::string& input, const Replacements& replacements)
{
    std::string out;
    out.reserve(input.size() + 256);
    size_t pos = 0;
    while (pos < input.size()) {
        bool matched = false;
        if (input[pos] == '{') {
            for (const auto& item : replacements) {
                if (input.compare(pos, item.first.size(), item.first) == 0) {
                    out += item.second;
                    pos += item.first.size();
                    matched = true;
                    break;
                }
            }
        }
        if (!matched) out.push_back(input[pos++]);
    }
    return out;
}

static Replacements values_for(const IdfEmailData& data, bool html)
{
    auto value = [html](const std::string& v) { return html ? html_escape(v) : subject_value(v); };
    return {
        {"{sender}", value(data.sender)},
        {"{receiver}", value(data.receiver)},
        {"{caller}", value(data.caller)},
        {"{timestamp}", value(data.timestamp)},
        {"{title}", value(data.title)},
        {"{message}", value(data.message)},
        {"{action}", value(data.action)},
        {"{result}", value(data.result)},
        {"{sms_total}", std::to_string(data.smsTotal)},
        {"{free_heap}", std::to_string(data.freeHeapKb)},
    };
}

static std::string plain_body(const IdfEmailData& data)
{
    if (data.kind == IdfEmailKind::Sms) {
        return "来自：" + data.sender + "，本机号码：" + data.receiver + "，时间：" + data.timestamp +
               "，内容：" + data.message;
    }
    if (data.kind == IdfEmailKind::Call) {
        return "来电：" + data.caller + "，本机号码：" + data.receiver + "，时间：" + data.timestamp;
    }
    if (data.kind == IdfEmailKind::Heartbeat) {
        return data.title + "\n时间：" + data.timestamp + "\n累计转发：" + std::to_string(data.smsTotal) +
               " 条\n空闲堆：" + std::to_string(data.freeHeapKb) + " KB\n" + data.message;
    }
    if (data.kind == IdfEmailKind::Keepalive) {
        return data.title + "\n时间：" + data.timestamp + "\n方式：" + data.action + "\n结果：" + data.result;
    }
    return data.title + (data.timestamp.empty() ? "" : "\n时间：" + data.timestamp) + "\n" + data.message;
}

static bool render_with(const IdfEmailData& data, const std::string& base,
                        const std::string& body, const std::string& subject,
                        IdfRenderedEmail& rendered, std::string& message)
{
    if (!validate_template(IdfEmailTemplatePart::Base, data.kind, base, message) ||
        !validate_template(IdfEmailTemplatePart::Body, data.kind, body, message) ||
        !validate_template(IdfEmailTemplatePart::Subject, data.kind, subject, message)) return false;
    std::string body_html = replace_once(body, values_for(data, true));
    rendered.html = replace_once(base, {{"{content}", body_html}});
    rendered.subject = replace_once(subject, values_for(data, false));
    truncate_utf8(rendered.subject, SUBJECT_RENDER_MAX);
    rendered.plain = plain_body(data);
    return true;
}

bool idf_email_render(const IdfEmailData& data, IdfRenderedEmail& rendered, std::string& message)
{
    IdfEmailTemplateValue base = idf_email_get_template(IdfEmailTemplatePart::Base, data.kind);
    IdfEmailTemplateValue body = idf_email_get_template(IdfEmailTemplatePart::Body, data.kind);
    IdfEmailTemplateValue subject = idf_email_get_template(IdfEmailTemplatePart::Subject, data.kind);
    if (render_with(data, base.value, body.value, subject.value, rendered, message)) return true;

    rendered = IdfRenderedEmail();
    rendered.usedFallback = true;
    std::string fallback_error;
    bool ok = render_with(data, DEFAULT_BASE, DEFAULT_BODIES[kind_index(data.kind)],
                          DEFAULT_SUBJECTS[kind_index(data.kind)], rendered, fallback_error);
    if (ok) {
        idf_logf("邮件模板 %s 无效，已回退默认模板: %s", idf_email_kind_name(data.kind), message.c_str());
        ESP_LOGW(TAG, "template %s fallback: %s", idf_email_kind_name(data.kind), message.c_str());
    }
    return ok;
}

bool idf_email_preview(IdfEmailTemplatePart edited_part, IdfEmailKind kind,
                       const std::string& edited_value, const IdfEmailData& sample,
                       IdfRenderedEmail& rendered, std::string& message)
{
    std::string base = idf_email_get_template(IdfEmailTemplatePart::Base, kind).value;
    std::string body = idf_email_get_template(IdfEmailTemplatePart::Body, kind).value;
    std::string subject = idf_email_get_template(IdfEmailTemplatePart::Subject, kind).value;
    if (edited_part == IdfEmailTemplatePart::Base) base = edited_value;
    else if (edited_part == IdfEmailTemplatePart::Body) body = edited_value;
    else subject = edited_value;
    return render_with(sample, base, body, subject, rendered, message);
}

std::vector<std::string> idf_email_placeholders(IdfEmailKind kind)
{
    switch (kind) {
        case IdfEmailKind::Sms: return {"{sender}", "{receiver}", "{timestamp}", "{message}"};
        case IdfEmailKind::Call: return {"{caller}", "{receiver}", "{timestamp}"};
        case IdfEmailKind::Heartbeat: return {"{title}", "{message}", "{timestamp}", "{sms_total}", "{free_heap}"};
        case IdfEmailKind::Keepalive: return {"{title}", "{message}", "{timestamp}", "{action}", "{result}"};
        case IdfEmailKind::System: return {"{title}", "{message}", "{timestamp}"};
    }
    return {};
}

std::vector<std::pair<std::string, std::string>> idf_email_export_overrides(void)
{
    std::vector<std::pair<std::string, std::string>> result;
    std::string value;
    if (read_override("base", value)) result.emplace_back("emailTplBase", value);
    for (size_t i = 0; i < 5; ++i) {
        IdfEmailKind kind = static_cast<IdfEmailKind>(i);
        if (read_override(storage_key(IdfEmailTemplatePart::Body, kind), value))
            result.emplace_back(std::string("emailTplBody.") + idf_email_kind_name(kind), value);
        if (read_override(storage_key(IdfEmailTemplatePart::Subject, kind), value))
            result.emplace_back(std::string("emailTplSubject.") + idf_email_kind_name(kind), value);
    }
    return result;
}

bool idf_email_import_key(const std::string& key, const std::string& value,
                          std::string& message, bool& recognized, bool persist)
{
    recognized = true;
    if (key == "emailTplBase") {
        if (!validate_template(IdfEmailTemplatePart::Base, IdfEmailKind::Sms, value, message)) return false;
        return !persist || idf_email_save_template(IdfEmailTemplatePart::Base, IdfEmailKind::Sms, value, message) == ESP_OK;
    }
    const std::string body_prefix = "emailTplBody.";
    const std::string subject_prefix = "emailTplSubject.";
    IdfEmailTemplatePart part;
    std::string kind_text;
    if (key.rfind(body_prefix, 0) == 0) {
        part = IdfEmailTemplatePart::Body;
        kind_text = key.substr(body_prefix.size());
    } else if (key.rfind(subject_prefix, 0) == 0) {
        part = IdfEmailTemplatePart::Subject;
        kind_text = key.substr(subject_prefix.size());
    } else {
        recognized = false;
        return false;
    }
    IdfEmailKind kind;
    if (!idf_email_parse_kind(kind_text, kind)) { message = "未知邮件模板类型"; return false; }
    if (!validate_template(part, kind, value, message)) return false;
    return !persist || idf_email_save_template(part, kind, value, message) == ESP_OK;
}
