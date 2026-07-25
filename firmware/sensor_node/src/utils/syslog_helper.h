#pragma once

#include <Arduino.h>

void syslog_init();
void send_syslog(const char* msg);
void send_syslog_f(const char* format, ...);
