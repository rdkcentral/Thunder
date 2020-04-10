 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
 
#ifndef __CRYPTALGO_H
#define __CRYPTALGO_H

#include "Module.h"

#include "AES.h"
#include "HMAC.h"
#include "Hash.h"
#include "HashStream.h"
#include "Random.h"

#if defined(SECURESOCKETS_ENABLED) || defined(__WINDOWS__)
#include "SecureSocketPort.h"
#endif

#ifdef __WINDOWS__
#pragma comment(lib, "cryptalgo.lib")
#endif

#endif // __CRYPTALGO_H
