/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Metrological
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

#include "BinderEndpoint.h"

namespace Thunder {
namespace RPC {

BinderEndpoint::BinderEndpoint()
    : _transport()
    , _threads()
{
}

BinderEndpoint::~BinderEndpoint()
{
    Close();
}

uint32_t BinderEndpoint::Open(uint32_t numThreads, bool contextManager, size_t mapSize)
{
    uint32_t result = _transport.Open(mapSize);
    if (result != Core::ERROR_NONE) {
        return result;
    }

    if (contextManager) {
        result = _transport.BecomeContextManager();
        if (result != Core::ERROR_NONE) {
            _transport.Close();
            return result;
        }
    }

    _threads.reserve(numThreads);
    for (uint32_t i = 0; i < numThreads; ++i) {
        _threads.emplace_back([this]() {
            _transport.Loop([this](binder_transaction_data* txn,
                                  Core::BinderBuffer& msg,
                                  Core::BinderBuffer& reply) -> int {
                return HandleTransaction(txn, msg, reply);
            });
        });
    }

    return Core::ERROR_NONE;
}

uint32_t BinderEndpoint::Close()
{
    // Closing the fd causes all Loop() ioctl calls to return EBADF, causing
    // each thread to exit its loop naturally.
    _transport.Close();
    for (auto& t : _threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    _threads.clear();
    return Core::ERROR_NONE;
}

bool BinderEndpoint::IsOpen() const
{
    return _transport.IsOpen();
}

Core::BinderTransport& BinderEndpoint::Transport()
{
    return _transport;
}

} // namespace RPC
} // namespace Thunder
