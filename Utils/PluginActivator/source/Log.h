/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int gActivatorLogLevel;

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

#define __LOG(level, plugin, fmt, ...)                                                                                                        \
    do {                                                                                                                                      \
        if (__builtin_expect(((level) <= gActivatorLogLevel), 0))                                                                                      \
            fprintf(stderr, "%s[%s:%d][%s] (%s) " fmt "\n", getLogLevel(level), __FILENAME__, __LINE__, __FUNCTION__, plugin, ##__VA_ARGS__); \
    } while (0)

void initLogging();
const char* getLogLevel(int level);
