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

#include "Module.h"
#include "Errors.h"
#include "Number.h"

namespace Thunder {

namespace Core {

#ifndef __DISABLE_USE_COMPLEMENTARY_CODE_SET__

namespace {

    static CustomCodeToStringHandler customerrorcodehandler = nullptr;

    const TCHAR* HandleCustomErrorCodeToString(const int32_t customcode)
    {
        const TCHAR* text = nullptr;

        if (customcode == std::numeric_limits<int32_t>::max()) {
            text = _T("Invalid Custom ErrorCode set");
        } else if ((customerrorcodehandler == nullptr) || ((text = customerrorcodehandler(customcode)) == nullptr)) {
            text = _T("Undefined Custom Error");
        } 

        return text;
    }

    string HandleCustomErrorCodeToStringExtended(const int32_t customcode)
    {
        string result;

        const TCHAR* text = nullptr;
        if (customcode == std::numeric_limits<int32_t>::max()) {
            result = _T("Invalid Custom ErrorCode set");
        } else if ((customerrorcodehandler == nullptr) || ((text = customerrorcodehandler(customcode)) == nullptr)) {
            result = _T("Undefined Custom Error: ") + Core::NumberType<int32_t>(customcode).Text();
        } else {
            result = text;
        }

        return result;
    }

}
    void SetCustomCodeToStringHandler(CustomCodeToStringHandler handler) {
        customerrorcodehandler = handler;
    }

    hresult CustomCode(const int24_t customCode)
    {
        static_assert(CUSTOM_ERROR == 0x1000000, "Code below assumes 25th bit used for CUSTOM_ERROR");

        hresult result = Core::ERROR_NONE;

        if (customCode != 0) {
            if (Core::Overflowed(customCode) == false) {
                result = static_cast<hresult>(customCode.AsSInt24());
            } else {
                result = 0; // set invalid customCode result;
            }
            result |= CUSTOM_ERROR; // set custom code bit
        }

        return result;
    }

    int24_t IsCustomCode(const Core::hresult code)
    {
        static_assert(CUSTOM_ERROR == 0x1000000, "Code below assumes 25th bit used for CUSTOM_ERROR");

        int24_t result = 0;

        if ((code & CUSTOM_ERROR) != 0) {
            result = static_cast<uint32_t>(code & 0xFFFFFF); // remove custom error bit before assigning
            if (result == 0) {
                result = std::numeric_limits<int32_t>::max(); // this will assert in debug, but if that happens one managed to fill an hresult with an overflowed core result, that should have asserted already when using CustomCode to fill it, os this is probably caused by either manually incorrectly filling the hresult or memory corruption
            }
        }

        return result;
    }

#endif

    const TCHAR* ErrorToString(const Core::hresult code)
    {
#ifndef __DISABLE_USE_COMPLEMENTARY_CODE_SET__
        int24_t customcode = IsCustomCode(code);

        if (customcode != 0) {
            return HandleCustomErrorCodeToString(customcode);
        }
#endif
        return _bogus_ErrorToString<>(code & (~COM_ERROR));
    }

    string ErrorToStringExtended(const Core::hresult code)
    {
#ifndef __DISABLE_USE_COMPLEMENTARY_CODE_SET__
        int24_t customcode = IsCustomCode(code);

        if (customcode != 0) {
            return HandleCustomErrorCodeToStringExtended(customcode);
        }
#endif
        string result = _bogus_ErrorToString<>(code & (~COM_ERROR));

        if (result.empty() == true) {
            result = _T("Undefined Thunder error code: ") + Core::NumberType<Core::hresult>(code).Text();
        }
        return result;
    }


} // namespace Core
} // namespace Thunder
