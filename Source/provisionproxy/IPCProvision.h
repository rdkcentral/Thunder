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

#ifndef __IPC_PROVISION_H__
#define __IPC_PROVISION_H__

#include <core/core.h>

using namespace WPEFramework;

namespace IPC {

#ifdef __DEBUG__
    enum { CommunicationTimeOut = Core::infinite }; // Time in ms. Forever
#else
    enum { CommunicationTimeOut = 2000 }; // Time in ms. 2 Seconds
#endif

namespace Provisioning {

    class KeyValue {
    private:
        KeyValue(const KeyValue&);
        KeyValue& operator=(const KeyValue&);

    public:
        KeyValue()
            : _maxSize(0)
            , _filledSize(0)
            , _buffer(nullptr)
        {
        }
        KeyValue(const uint32_t size)
            : _maxSize(size)
            , _filledSize(0)
            , _buffer(new uint8_t[_maxSize])
        {
        }
        KeyValue(const uint32_t size, const uint8_t stream[])
            : _maxSize(size + 1)
            , _filledSize(size)
            , _buffer(new uint8_t[size + 1])
        {
            ::memcpy(_buffer, stream, size);
        }
        ~KeyValue()
        {
        }

    public:
        inline void Clear()
        {
            if (_buffer != nullptr) {
                ::memset(_buffer, 0, _maxSize);
            }
            _filledSize = 0;
        }
        inline uint32_t Length() const
        {
            return (_filledSize);
        }
        inline const uint8_t* Frame() const
        {
            return (_buffer);
        }
        inline void Frame(const uint32_t length, const uint8_t stream[])
        {
            Deserialize(stream, length, 0);
        }

        inline uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
        {
            uint16_t result = 0;

            if (offset < _filledSize) {
                result = ((_filledSize - offset) <= maxLength ? (_filledSize - offset) : maxLength);
                ::memcpy(stream, &(_buffer[offset]), result);
            }
            return (result);
        }
        inline uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
        {
            uint16_t result = maxLength;

            if ((offset + maxLength) > _maxSize) {
                _maxSize = 2 * (offset + maxLength);
                uint8_t* buffer = new uint8_t[_maxSize];

                // We need to expand. Current size does not fit..
                if (_buffer != nullptr) {
                    ::memcpy(buffer, _buffer, _filledSize);
                    delete _buffer;
                }

                _buffer = buffer;
            }

            _filledSize = offset + maxLength;

            ::memcpy(&(_buffer[offset]), stream, maxLength);

            return (result);
        }

    private:
        uint32_t _maxSize;
        uint32_t _filledSize;
        uint8_t* _buffer;
    };

    typedef Core::IPCMessageType<1, Core::IPC::Text<32>, IPC::Provisioning::KeyValue> DrmIdData;
    typedef Core::IPCMessageType<2, Core::IPC::Void, Core::IPC::Text<32>> DeviceIdData;
}
} // namespace IPC::Provisioning

#endif // __IPC_PROVISION_H__
