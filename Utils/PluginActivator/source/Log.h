#pragma once

#include <stdio.h>
#include <string.h>

#define LOG_LEVEL 3

#ifndef __FILENAME__
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define LEVEL_DEBUG 3
#define LEVEL_INFO 2
#define LEVEL_WARN 1
#define LEVEL_ERROR 0


#define LOG_DBG(plugin, fmt, ...) \
    __LOG(LEVEL_DEBUG, plugin, fmt, ##__VA_ARGS__)

#define LOG_INF(plugin, fmt, ...) \
    __LOG(LEVEL_INFO, plugin, fmt, ##__VA_ARGS__)

#define LOG_WARN(plugin, fmt, ...) \
    __LOG(LEVEL_WARN, plugin, fmt, ##__VA_ARGS__)

#define LOG_ERROR(plugin, fmt, ...) \
    __LOG(LEVEL_ERROR, plugin, fmt, ##__VA_ARGS__)

#define __LOG(level, plugin, fmt, ...)                                                                                                         \
    do                                                                                                                                         \
    {                                                                                                                                          \
        if (__builtin_expect(((level) <= LOG_LEVEL), 0))                                                                                       \
            fprintf(stderr, "%s[%s:%d][%s] (%s) " fmt "\n", getLogLevel(level), __FILENAME__, __LINE__, __FUNCTION__, plugin, ##__VA_ARGS__); \
    } while (0)

inline const char *getLogLevel(int level)
{
    switch (level)
    {
    case LEVEL_DEBUG:
        return "[DBG]";
    case LEVEL_INFO:
        return "[NFO]";
    case LEVEL_WARN:
        return "[WRN]";
    case LEVEL_ERROR:
        return "[ERR]";
    default:
        return "";
    }
}