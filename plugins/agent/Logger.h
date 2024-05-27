#pragma once

#include <syslog.h>
#include <string>

const std::string level_prefix[] = {
    "",
    "",
    "FATAL",
    "ERROR",
    "WARN",
    "",
    "INFO",
    "DEBUG"};

#define logd(level, format, ...) \
    syslog(level | LOG_USER, "%s %s:%d " format, level_prefix[level].c_str(), __FILE__, __LINE__, ##__VA_ARGS__);

class Logger
{
public:
    static void initialize();
    static void terminate();
};