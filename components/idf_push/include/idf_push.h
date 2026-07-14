#pragma once

#include <stdint.h>

#include <string>

#include "esp_err.h"
#include "idf_config.h"
#include "idf_email.h"

esp_err_t idf_push_start(void);

bool idf_push_enqueue_forward(const char* sender, const char* text, const char* timestamp, uint32_t inbox_id);
bool idf_push_enqueue_call(const char* caller, const char* timestamp);
int idf_push_enqueue_notify(const char* title, const char* body, const char* timestamp);
bool idf_push_enqueue_email(const char* subject, const char* body);
bool idf_push_enqueue_email_data(const IdfEmailData& data);
int idf_push_forward_queue_depth(void);
int idf_push_retry_queue_depth(void);
int idf_push_email_queue_depth(void);
bool idf_push_busy(void);

bool idf_push_enqueue_test(uint8_t channel, std::string& message);
std::string idf_push_test_status_json(uint8_t channel);
bool idf_push_enqueue_smtp_test(const IdfEmailSettingsView& cfg, std::string& message);
std::string idf_push_smtp_test_status_json(void);
