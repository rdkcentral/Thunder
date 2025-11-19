/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
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

#include "Module.h"
#include "ExtraNumberDefinitions.h"

#define COM_ERROR (0x80000000)
#define CUSTOM_ERROR (0x1000000)

namespace Thunder {
namespace Core {

#ifndef __DISABLE_USE_COMPLEMENTARY_CODE_SET__

    // transform a custum code into an hresult
    EXTERNAL Core::hresult CustomCode(const int24_t customCode);
    // query if the hresult is a custom code and if so extract the value, returns 0 if the hresult was not a custom code
    EXTERNAL int24_t IsCustomCode(const Core::hresult code);

#endif

    #define ERROR_CODES \
        ERROR_CODE(ERROR_NONE, 0) \
        ERROR_CODE(ERROR_GENERAL, 1) \
        ERROR_CODE(ERROR_UNAVAILABLE, 2) \
        ERROR_CODE(ERROR_ASYNC_FAILED, 3) \
        ERROR_CODE(ERROR_ASYNC_ABORTED, 4) \
        ERROR_CODE(ERROR_ILLEGAL_STATE, 5) \
        ERROR_CODE(ERROR_OPENING_FAILED, 6) \
        ERROR_CODE(ERROR_ACCEPT_FAILED, 7) \
        ERROR_CODE(ERROR_PENDING_SHUTDOWN, 8) \
        ERROR_CODE(ERROR_ALREADY_CONNECTED, 9) \
        ERROR_CODE(ERROR_CONNECTION_CLOSED, 10) \
        ERROR_CODE(ERROR_TIMEDOUT, 11) \
        ERROR_CODE(ERROR_INPROGRESS, 12) \
        ERROR_CODE(ERROR_COULD_NOT_SET_ADDRESS, 13) \
        ERROR_CODE(ERROR_INCORRECT_HASH, 14) \
        ERROR_CODE(ERROR_INCORRECT_URL, 15) \
        ERROR_CODE(ERROR_INVALID_INPUT_LENGTH, 16) \
        ERROR_CODE(ERROR_DESTRUCTION_SUCCEEDED, 17) \
        ERROR_CODE(ERROR_DESTRUCTION_FAILED, 18) \
        ERROR_CODE(ERROR_CLOSING_FAILED, 19) \
        ERROR_CODE(ERROR_PROCESS_TERMINATED, 20) \
        ERROR_CODE(ERROR_PROCESS_KILLED, 21) \
        ERROR_CODE(ERROR_UNKNOWN_KEY, 22) \
        ERROR_CODE(ERROR_INCOMPLETE_CONFIG, 23) \
        ERROR_CODE(ERROR_PRIVILIGED_REQUEST, 24) \
        ERROR_CODE(ERROR_RPC_CALL_FAILED, 25) \
        ERROR_CODE(ERROR_UNREACHABLE_NETWORK, 26) \
        ERROR_CODE(ERROR_REQUEST_SUBMITTED, 27) \
        ERROR_CODE(ERROR_UNKNOWN_TABLE, 28) \
        ERROR_CODE(ERROR_DUPLICATE_KEY, 29) \
        ERROR_CODE(ERROR_BAD_REQUEST, 30) \
        ERROR_CODE(ERROR_PENDING_CONDITIONS, 31) \
        ERROR_CODE(ERROR_SURFACE_UNAVAILABLE, 32) \
        ERROR_CODE(ERROR_PLAYER_UNAVAILABLE, 33) \
        ERROR_CODE(ERROR_FIRST_RESOURCE_NOT_FOUND, 34) \
        ERROR_CODE(ERROR_SECOND_RESOURCE_NOT_FOUND, 35) \
        ERROR_CODE(ERROR_ALREADY_RELEASED, 36) \
        ERROR_CODE(ERROR_NEGATIVE_ACKNOWLEDGE, 37) \
        ERROR_CODE(ERROR_INVALID_SIGNATURE, 38) \
        ERROR_CODE(ERROR_READ_ERROR, 39) \
        ERROR_CODE(ERROR_WRITE_ERROR, 40) \
        ERROR_CODE(ERROR_INVALID_DESIGNATOR, 41) \
        ERROR_CODE(ERROR_UNAUTHENTICATED, 42) \
        ERROR_CODE(ERROR_NOT_EXIST, 43) \
        ERROR_CODE(ERROR_NOT_SUPPORTED, 44) \
        ERROR_CODE(ERROR_INVALID_RANGE, 45) \
        ERROR_CODE(ERROR_HIBERNATED, 46) \
        ERROR_CODE(ERROR_INPROC, 47) \
        ERROR_CODE(ERROR_FAILED_REGISTERED, 48) \
        ERROR_CODE(ERROR_FAILED_UNREGISTERED, 49) \
        ERROR_CODE(ERROR_PARSE_FAILURE, 50) \
        ERROR_CODE(ERROR_PRIVILIGED_DEFERRED, 51) \
        ERROR_CODE(ERROR_INVALID_ENVELOPPE, 52) \
        ERROR_CODE(ERROR_UNKNOWN_METHOD, 53) \
        ERROR_CODE(ERROR_INVALID_PARAMETER, 54) \
        ERROR_CODE(ERROR_INTERNAL_JSONRPC, 55) \
        ERROR_CODE(ERROR_PARSING_ENVELOPPE, 56) \
        ERROR_CODE(ERROR_COMPOSIT_OBJECT, 57) \
        ERROR_CODE(ERROR_ABORTED, 58)

    #define ERROR_CODE(CODE, VALUE) CODE = VALUE,

    enum ErrorCodes {
        ERROR_CODES
        ERROR_COUNT
    };

    #undef ERROR_CODE

    // Convert error enumerations to string

    template<uint32_t N>
    inline const TCHAR* _Err2Str()
    {
        return _T("");
    };

    #define ERROR_CODE(CODE, VALUE) \
        template<> inline const TCHAR* _Err2Str<VALUE>() { return _T(#CODE); }

    ERROR_CODES;

    #undef ERROR_CODE

    template<uint32_t N = (ERROR_COUNT - 1)>
    inline const TCHAR* _bogus_ErrorToString(uint32_t code)
    {
        return (code == N? _Err2Str<N>() : _bogus_ErrorToString<N-1>(code));
    };

    template<>
    inline const TCHAR* _bogus_ErrorToString<0u>(uint32_t code)
    {
        return (code == 0? _Err2Str<0u>() : _Err2Str<~0u>());
    };

    EXTERNAL const TCHAR* ErrorToString(const Core::hresult code);
    EXTERNAL string ErrorToStringExtended(const Core::hresult code);

#ifndef __DISABLE_USE_COMPLEMENTARY_CODE_SET__

    using CustomCodeToStringHandler = const TCHAR* (*)(const int32_t code);

    // can only set one, not multithreaded safe
    EXTERNAL void SetCustomCodeToStringHandler(CustomCodeToStringHandler handler);

#endif

    
}
}

