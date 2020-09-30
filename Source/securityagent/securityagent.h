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

#pragma once

#ifndef EXTERNAL
#ifdef _MSVC_LANG
#ifdef SECURITYAGENT_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#endif
#else
#define EXTERNAL __attribute__ ((visibility ("default")))
#endif
#endif

#if defined(_WINDOWS) && !defined(SECURITYAGENT_EXPORTS)
#pragma comment(lib, "securityagent.lib")
#endif

#ifdef __cplusplus
extern "C" {
#endif

	/*
	 * GetToken - function to obtain a token from the SecurityAgent
	 *
	 * Parameters
	 *  maxLength   - holds the maximum uint8_t length of the buffer
	 *  inLength    - holds the length of the current that needs to be tokenized.
	 *  Id          - Buffer holds the data to tokenize on its way in, and returns in the same buffer the token.
	 *
	 * Return value
	 *  < 0 - failure, absolute value returned is the length required to store the token
	 *  > 0 - success, char length of the returned token
	 *
	 * Post-condition; return value 0 should not occur
	 *
	 */
	int EXTERNAL GetToken(unsigned short maxLength, unsigned short inLength, unsigned char buffer[]);

#ifdef __cplusplus
}
#endif