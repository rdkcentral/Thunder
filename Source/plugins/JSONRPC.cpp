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

#include "JSONRPC.h"

namespace WPEFramework {

    namespace PluginHost {

        JSONRPC::JSONRPC()
            : _adminLock()
            , _handlers()
            , _service(nullptr)
            , _callsign()
            , _validate()
        {
            std::vector<uint8_t> versions = { 1 };

            _handlers.emplace_back(versions);
        }

        JSONRPC::JSONRPC(const std::vector<uint8_t>& versions)
            : _adminLock()
            , _handlers()
            , _service(nullptr)
            , _callsign()
            , _validate()
        {
            _handlers.emplace_back(versions);
        }

        JSONRPC::JSONRPC(const TokenCheckFunction& validation)
            : _adminLock()
            , _handlers()
            , _service(nullptr)
            , _callsign()
            , _validate(validation)
        {
            std::vector<uint8_t> versions = { 1 };

            _handlers.emplace_back(versions);
        }

        JSONRPC::JSONRPC(const std::vector<uint8_t>& versions, const TokenCheckFunction& validation)
            : _adminLock()
            , _handlers()
            , _service(nullptr)
            , _callsign()
            , _validate(validation)
        {
            _handlers.emplace_back(versions);
        }

        /* virtual */ JSONRPC::~JSONRPC()
        {
        }
    }
}
