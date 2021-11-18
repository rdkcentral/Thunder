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

#ifndef MODULE_NAME
#define MODULE_NAME Bluetooth 
#endif

#include <core/core.h>
#include <tracing/tracing.h>

#include <../include/bluetooth/bluetooth.h>
#include <../include/bluetooth/hci.h>
#include <../include/bluetooth/mgmt.h>
#include <../include/bluetooth/l2cap.h>

//#define BLUETOOTH_CMD_DUMP

#if defined(BLUETOOTH_CMD_DUMP)
#define CMD_DUMP(descr, buffer, length) \
    do { fprintf(stderr, "%s [%i]: ", descr, length); for (int i = 0; i < length; i++) { printf("%02x:", buffer[i]); } printf("\n"); } while(0)
#else
#define CMD_DUMP(descr, buffer, length)
#endif

#if defined(__WINDOWS__) && defined(BLUETOOTH_EXPORTS)
#undef EXTERNAL
#define EXTERNAL EXTERNAL_EXPORT
#endif

