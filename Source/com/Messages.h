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

#ifndef __COM_MESSAGES_H
#define __COM_MESSAGES_H

#include "Module.h"

namespace WPEFramework {
namespace RPC {

    // As COMRPC might run between a 32 bit and 64 bit system, the largest must be accommodated.
    #if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8) 
    typedef uint64_t instance_id;
    #else
    typedef uint32_t instance_id;
    #endif

    template<typename INCOMING>
    RPC::instance_id instance_cast(INCOMING value) {
        return ((RPC::instance_id) value);
    }

    namespace Data {
        static const uint16_t IPC_BLOCK_SIZE = 512;

        class Frame : public Core::FrameType<IPC_BLOCK_SIZE> {
        private:
            Frame(Frame&) = delete;
            Frame& operator=(const Frame&) = delete;

        public:
            Frame()
            {
            }
            ~Frame()
            {
            }

        public:
            friend class Input;
            friend class Output;
            friend class ObjectInterface;

            uint16_t Serialize(const uint16_t offset, uint8_t stream[], const uint16_t maxLength) const
            {
                uint16_t copiedBytes((Size() - offset) > maxLength ? maxLength : (Size() - offset));

                ::memcpy(stream, &(operator[](offset)), copiedBytes);

                return (copiedBytes);
            }
            uint16_t Deserialize(const uint16_t offset, const uint8_t stream[], const uint16_t maxLength)
            {
                Size(offset + maxLength);

                ::memcpy(&(operator[](offset)), stream, maxLength);

                return (maxLength);
            }
        };

        class Input {
        private:
            Input(const Input&) = delete;
            Input& operator=(const Input&) = delete;

        public:
            Input()
                : _data()
            {
            }
            ~Input()
            {
            }

        public:
            inline void Clear()
            {
                _data.Clear();
            }
            void Set(instance_id implementation, const uint32_t interfaceId, const uint8_t methodId)
            {
                uint16_t result = _data.SetNumber<instance_id>(0, implementation);
                result += _data.SetNumber<uint32_t>(result, interfaceId);
                _data.SetNumber(result, methodId);
            }
            instance_id Implementation()
            {
                instance_id result = 0;

                _data.GetNumber<instance_id>(0, result);

                return (result);
            }
            uint32_t InterfaceId() const
            {
                uint32_t result = 0;

                _data.GetNumber<uint32_t>(sizeof(instance_id), result);

                return (result);
            }
            uint8_t MethodId() const
            {
                uint8_t result = 0;

                _data.GetNumber(sizeof(instance_id) + sizeof(uint32_t), result);

                return (result);
            }
            uint32_t Length() const
            {
                return (_data.Size());
            }
            inline Frame::Writer Writer()
            {
                return (Frame::Writer(_data, (sizeof(instance_id) + sizeof(uint32_t) + sizeof(uint8_t))));
            }
            inline const Frame::Reader Reader() const
            {
                return (Frame::Reader(_data, (sizeof(instance_id) + sizeof(uint32_t) + sizeof(uint8_t))));
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return (_data.Serialize(static_cast<uint16_t>(offset), stream, maxLength));
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                return (_data.Deserialize(static_cast<uint16_t>(offset), stream, maxLength));
            }

        private:
            Frame _data;
        };

        class Output {
        private:
            Output(const Output&) = delete;
            Output& operator=(const Output&) = delete;

        public:
            Output()
                : _data()
            {
            }
            ~Output()
            {
            }

        public:
            inline void Clear()
            {
                _data.Clear();
            }
            inline Frame::Writer Writer()
            {
                return (Frame::Writer(_data, 0));
            }
            inline const Frame::Reader Reader() const
            {
                return (Frame::Reader(_data, 0));
            }
            inline void AddImplementation(instance_id implementation, const uint32_t id)
            {
                _data.SetNumber<instance_id>(_data.Size(), implementation);
                _data.SetNumber<uint32_t>(_data.Size(), id);
            }
            inline uint32_t Length() const
            {
                return (static_cast<uint32_t>(_data.Size()));
            }
            inline uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return (_data.Serialize(static_cast<uint16_t>(offset), stream, maxLength));
            }
            inline uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                return (_data.Deserialize(static_cast<uint16_t>(offset), stream, maxLength));
            }

        private:
            Frame _data;
        };

        class Init {
        private:
            Init(const Init&) = delete;
            Init& operator=(const Init&) = delete;
            uint32_t ParentId() const 
            {
                uint32_t exchangeId = 0;
                string value; Core::SystemInfo::GetEnvironment(_T("COM_PARENT_EXCHANGE_ID"), value);
                if (value.empty() == false) {
                    exchangeId = Core::NumberType<uint32_t>(value.c_str(), static_cast<uint32_t>(value.length())).Value();
                }

                return (exchangeId);
            }

        public:
            enum type : uint8_t {
                AQUIRE = 0,
                OFFER = 1,
                REVOKE = 2,
                REQUEST = 3
            };

        public:
            Init()
                : _id(0)
                , _implementation(0)
                , _interfaceId(~0)
                , _exchangeId(~0)
                , _versionId(0)
            {
            }
            ~Init()
            {
            }

        public:
            bool IsOffer() const
            {
                return (_className[0] == '\0') && (_className[1] == OFFER);
            }
            bool IsRevoke() const
            {
                return (_className[0] == '\0') && (_className[1] == REVOKE);
            }
            bool IsRequested() const
            {
                return (_className[0] == '\0') && (_className[1] == REQUEST);
            }
            bool IsAquire() const
            {
                return (IsRevoke() == false) && (IsOffer() == false) && (IsRequested() == false);
            }
            void Set(const uint32_t myId)
            {
                _exchangeId = ParentId();
                _implementation = 0;
                _interfaceId = ~0;
                _versionId = ~0;
                _id = myId;
                _className[0] = '\0';
                _className[1] = AQUIRE;
            }
            void Set(const uint32_t myId, const uint32_t interfaceId, instance_id implementation, const uint32_t exchangeId)
            {
                _exchangeId = exchangeId;
                _implementation = implementation;
                _interfaceId = interfaceId;
                _versionId = 0;
                _id = myId;
                _className[0] = '\0';
                _className[1] = REQUEST;
            }
            void Set(const uint32_t myId, const uint32_t interfaceId, instance_id implementation, const type whatKind)
            {
                ASSERT((whatKind != AQUIRE) && (whatKind != REQUEST));

                _exchangeId = ParentId();
                _implementation = implementation;
                _interfaceId = interfaceId;
                _versionId = 0;
                _id = myId;
                _className[0] = '\0';
                _className[1] = whatKind;
            }
            void Set(const uint32_t myId, const string& className, const uint32_t interfaceId, const uint32_t versionId)
            {
                _exchangeId = ParentId();
                _implementation = 0;
                _interfaceId = interfaceId;
                _versionId = versionId;
                _id = myId;
                const std::string converted(Core::ToString(className));
                ::strncpy(_className, converted.c_str(), sizeof(_className));
            }
            uint32_t Id() const
            {
                return (_id);
            }
            instance_id Implementation() const
            {
                return (_implementation);
            }
            uint32_t InterfaceId() const
            {
                return (_interfaceId);
            }
            uint32_t ExchangeId() const
            {
                return (_exchangeId);
            }
            uint32_t VersionId() const
            {
                return (_versionId);
            }
            const string ClassName() const
            {
                return (Core::ToString(std::string(_className)));
            }

        private:
            uint32_t _id;
            instance_id _implementation;
            uint32_t _interfaceId;
            uint32_t _exchangeId;
            uint32_t _versionId;
            char _className[64];
        };

        class Setup {
        private:
            Setup(const Setup&) = delete;
            Setup& operator=(const Setup&) = delete;

        public:
            Setup()
                : _data()
            {
            }
            ~Setup()
            {
            }

        public:
            inline void Clear()
            {
                _data.Clear();
            }
            void Set(instance_id implementation, const uint32_t sequenceNumber, const string& proxyStubPath, const string& traceCategories)
            {
                _data.SetNumber<instance_id>(0, implementation);
                _data.SetNumber<uint32_t>(sizeof(instance_id), sequenceNumber);
                uint16_t length = _data.SetText(sizeof(instance_id) + sizeof(uint32_t), proxyStubPath);
                _data.SetText(sizeof(instance_id)+ sizeof(uint32_t) + length, traceCategories);
            }
            inline bool IsSet() const {
                return (_data.Size() > 0);
            }
            uint32_t SequenceNumber() const
            {
                uint32_t result;
                _data.GetNumber<uint32_t>(sizeof(instance_id), result);
                return (result);
            }
            string ProxyStubPath() const
            {
                string value;

                uint16_t length = sizeof(instance_id) + sizeof(uint32_t) ;   // skip implentation and sequencenumber

                _data.GetText(length, value); 
                
                return (value);
            }
            string TraceCategories() const
            {
                string value;

                uint16_t length = sizeof(instance_id) + sizeof(uint32_t) ;   // skip implentation and sequencenumber 
                length += _data.GetText(length, value);  // skip proxyStub path

                _data.GetText(length, value); 

                return (value);
            }
            instance_id Implementation() const
            {
                instance_id result = 0;
                _data.GetNumber<instance_id>(0, result);
                return (result);
            }
            void Implementation(instance_id implementation)
            {
                _data.SetNumber<instance_id>(0, implementation);
            }
            uint32_t Length() const
            {
                return (_data.Size());
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return (_data.Serialize(static_cast<uint16_t>(offset), stream, maxLength));
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                return (_data.Deserialize(static_cast<uint16_t>(offset), stream, maxLength));
            }

        private:
            Frame _data;
        };
    }

    typedef Core::IPCMessageType<1, Data::Init, Data::Setup> AnnounceMessage;
    typedef Core::IPCMessageType<2, Data::Input, Data::Output> InvokeMessage;
}
}

#endif // __COM_MESSAGES_H
