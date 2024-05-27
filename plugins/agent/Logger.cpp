#include "Logger.h"
#include <iostream>
#include <syslog.h>

void Logger::initialize()
{
    // Initialize the logger
    openlog("Agent", LOG_CONS | LOG_NDELAY | LOG_PID, LOG_USER);
}

void Logger::terminate()
{
    // Terminate the logger
    closelog();
}
