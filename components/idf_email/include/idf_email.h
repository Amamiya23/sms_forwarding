#pragma once

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <utility>
#include <vector>

#include "esp_err.h"

enum class IdfEmailKind : uint8_t {
    Sms = 0,
    Call,
    Heartbeat,
    Keepalive,
    System,
};

enum class IdfEmailTemplatePart : uint8_t {
    Base = 0,
    Body,
    Subject,
};

struct IdfEmailData {
    IdfEmailKind kind = IdfEmailKind::System;
    std::string title;
    std::string message;
    std::string sender;
    std::string receiver;
    std::string caller;
    std::string timestamp;
    std::string action;
    std::string result;
    uint32_t smsTotal = 0;
    uint32_t freeHeapKb = 0;
};

struct IdfEmailTemplateValue {
    std::string value;
    bool customized = false;
    size_t maxBytes = 0;
};

struct IdfRenderedEmail {
    std::string subject;
    std::string plain;
    std::string html;
    bool usedFallback = false;
};

const char* idf_email_kind_name(IdfEmailKind kind);
bool idf_email_parse_kind(const std::string& value, IdfEmailKind& kind);
const char* idf_email_part_name(IdfEmailTemplatePart part);
bool idf_email_parse_part(const std::string& value, IdfEmailTemplatePart& part);

IdfEmailTemplateValue idf_email_get_template(IdfEmailTemplatePart part, IdfEmailKind kind);
esp_err_t idf_email_save_template(IdfEmailTemplatePart part, IdfEmailKind kind,
                                  const std::string& value, std::string& message);
esp_err_t idf_email_reset_template(IdfEmailTemplatePart part, IdfEmailKind kind);
esp_err_t idf_email_reset_all_templates(void);

bool idf_email_render(const IdfEmailData& data, IdfRenderedEmail& rendered, std::string& message);
bool idf_email_preview(IdfEmailTemplatePart edited_part, IdfEmailKind kind,
                       const std::string& edited_value, const IdfEmailData& sample,
                       IdfRenderedEmail& rendered, std::string& message);

std::vector<std::string> idf_email_placeholders(IdfEmailKind kind);
std::vector<std::pair<std::string, std::string>> idf_email_export_overrides(void);
bool idf_email_import_key(const std::string& key, const std::string& value,
                          std::string& message, bool& recognized, bool persist = true);
