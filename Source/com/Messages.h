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

#include "Module.h"

namespace WPEFramework {
namespace RPC {

    // As COMRPC might run between a 32 bit and 64 bit system, the largest must be accommodated.
    template<typename INCOMING>
    Core::instance_id instance_cast(INCOMING value) {
        return ((Core::instance_id) value);
    }

    namespace Data {
        static const uint16_t IPC_BLOCK_SIZE = 512;

        class Frame : public Core::FrameType<IPC_BLOCK_SIZE, true, uint32_t> {
        private:
            using BaseClass = Core::FrameType<IPC_BLOCK_SIZE, true, uint32_t>;

        public:
            Frame(Frame&) = delete;
            Frame& operator=(const Frame&) = delete;

            Frame() : BaseClass() {
            }
            ~Frame() = default;

        public:
            friend class Input;
            friend class Output;
            friend class ObjectInterface;

            uint16_t Serialize(const uint32_t offset, uint8_t stream[], const uint16_t maxLength) const
            {
                uint16_t copiedBytes((Size() - offset) > maxLength ? maxLength : (Size() - offset));

                ::memcpy(stream, &(operator[](offset)), copiedBytes);

                return (copiedBytes);
            }
            uint16_t Deserialize(const uint32_t offset, const uint8_t stream[], const uint16_t maxLength)
            {
                Size(offset + maxLength);

                ::memcpy(&(operator[](offset)), stream, maxLength);

                return (maxLength);
            }
        };

        class Input {
        public:
            Input(const Input&) = delete;
            Input& operator=(const Input&) = delete;

            Input() : _data() {
            }
            ~Input() = default;

        public:
            inline void Clear()
            {
                _data.Clear();
            }
            void Set(Core::instance_id implementation, const uint32_t interfaceId, const uint8_t methodId)
            {
                uint16_t result = _data.SetNumber<Core::instance_id>(0, implementation);
                result += _data.SetNumber<uint32_t>(result, interfaceId);
                _data.SetNumber(result, methodId);
            }
            Core::instance_id Implementation()
            {
                Core::instance_id result = 0;

                _data.GetNumber<Core::instance_id>(0, result);

                return (result);
            }
            uint32_t InterfaceId() const
            {
                uint32_t result = 0;

                _data.GetNumber<uint32_t>(sizeof(Core::instance_id), result);

                return (result);
            }
            uint8_t MethodId() const
            {
                uint8_t result = 0;

                _data.GetNumber(sizeof(Core::instance_id) + sizeof(uint32_t), result);

                return (result);
            }
            uint32_t Length() const
            {
                return (_data.Size());
            }
            inline Frame::Writer Writer()
            {
                return (Frame::Writer(_data, (sizeof(Core::instance_id) + sizeof(uint32_t) + sizeof(uint8_t))));
            }
            inline const Frame::Reader Reader() const
            {
                return (Frame::Reader(_data, (sizeof(Core::instance_id) + sizeof(uint32_t) + sizeof(uint8_t))));
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return (_data.Serialize(offset, stream, maxLength));
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                return (_data.Deserialize(offset, stream, maxLength));
            }

        private:
            Frame _data;
        };

        class Output {
        public:
            enum mode : uint8_t {
                NONE            = 0x00,
                CACHED_ADDREF   = 0x01,
                CACHED_RELEASE  = 0x02
            };

        public:
            Output(const Output&) = delete;
            Output& operator=(const Output&) = delete;

            Output() : _data() {
            }
            ~Output() = default;

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
            inline void AddImplementation(Core::instance_id implementation, const uint32_t id, const mode how)
            {
                _data.SetNumber<Core::instance_id>(_data.Size(), implementation);
                _data.SetNumber<uint32_t>(_data.Size(), id);
                _data.SetNumber<mode>(_data.Size(), how);
            }
            inline uint32_t Length() const
            {
                return (static_cast<uint32_t>(_data.Size()));
            }
            inline uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return (_data.Serialize(offset, stream, maxLength));
            }
            inline uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                return (_data.Deserialize(offset, stream, maxLength));
            }

        private:
            Frame _data;
        };

        class Init {
        public:
            enum type : uint8_t {
                ACQUIRE = 0,
                OFFER = 1,
                REVOKE = 2,
                REQUEST = 3
            };

        public:
            Init(const Init&) = delete;
            Init& operator=(const Init&) = delete;

            Init()
                : _data()
            {
            }
            ~Init() = default;

        private:
            static constexpr uint8_t IMPLEMENTATION_OFFSET = 0;
            static constexpr uint8_t ID_OFFSET = (IMPLEMENTATION_OFFSET + sizeof(Core::instance_id));
            static constexpr uint8_t INTERFACEID_OFFSET = (ID_OFFSET + sizeof(uint32_t));
            static constexpr uint8_t EXCHANGEID_OFFSET = (INTERFACEID_OFFSET + sizeof(uint32_t));
            static constexpr uint8_t VERSIONID_OFFSET = (EXCHANGEID_OFFSET + sizeof(uint32_t));
            static constexpr uint8_t TYPE_OFFSET = (VERSIONID_OFFSET + sizeof(uint32_t));
            static constexpr uint8_t STRINGS_OFFSET = (TYPE_OFFSET + sizeof(type));

        private:
            template<typename TYPE>
            TYPE GetNumber(const uint8_t offset) const
            {
                ASSERT(_data.Size() >= (offset + sizeof(TYPE)));

                TYPE result;
                _data.GetNumber<TYPE>(offset, result);

                return (result);
            }
            string GetText(const uint8_t offset) const
            {
                ASSERT(_data.Size() >= (offset + sizeof(uint16_t)));

                string result;
                _data.GetText(offset, result);

                return (result);
            }
            void Set(const uint32_t myId, const string& className, const Core::instance_id implementation,
                     const uint32_t interfaceId, const uint32_t myExchangeId, const uint32_t versionId, const type whatKind)
            {
                uint32_t exchangeId = myExchangeId;
                string callsign;

                string parentInfo;
                Core::SystemInfo::GetEnvironment(_T("COM_PARENT_INFO"), parentInfo);

                if (parentInfo.empty() == false) {
                    const size_t delimiter = std::min(parentInfo.find(','), parentInfo.length());

                    if (exchangeId == ~0UL) {
                        exchangeId = Core::NumberType<uint32_t>(parentInfo.c_str(), static_cast<uint32_t>(delimiter)).Value();
                    }

                    if (delimiter != parentInfo.length()) {
                        callsign = parentInfo.substr(delimiter + 1);
                    }
                }

                _data.SetNumber<Core::instance_id>(IMPLEMENTATION_OFFSET, implementation);
                _data.SetNumber<uint32_t>(ID_OFFSET, myId);
                _data.SetNumber<uint32_t>(INTERFACEID_OFFSET, interfaceId);
                _data.SetNumber<uint32_t>(EXCHANGEID_OFFSET, exchangeId);
                _data.SetNumber<uint32_t>(VERSIONID_OFFSET, versionId);
                _data.SetNumber<type>(TYPE_OFFSET, whatKind);
                const uint16_t classNameLength = _data.SetText(STRINGS_OFFSET, className);
                _data.SetText((STRINGS_OFFSET + classNameLength), callsign);
            }

        private:
            type Type() const
            {
                return (GetNumber<type>(TYPE_OFFSET));
            }

        public:
            bool IsOffer() const
            {
                return (Type() == OFFER);
            }
            bool IsRevoke() const
            {
                return (Type() == REVOKE);
            }
            bool IsRequested() const
            {
                return (Type() == REQUEST);
            }
            bool IsAcquire() const
            {
                return (Type() == ACQUIRE);
            }
            void Set(const uint32_t myId)
            {
                Set(myId, _T(""), 0, ~0, ~0, ~0, ACQUIRE);
            }
            void Set(const uint32_t myId, const uint32_t interfaceId, Core::instance_id implementation, const uint32_t exchangeId)
            {
                ASSERT(exchangeId != 0);
                Set(myId, _T(""), implementation, interfaceId, exchangeId, 0, REQUEST);
            }
            void Set(const uint32_t myId, const uint32_t interfaceId, Core::instance_id implementation, const type whatKind)
            {
                ASSERT((whatKind != ACQUIRE) && (whatKind != REQUEST));
                Set(myId, _T(""), implementation, interfaceId, ~0, 0, whatKind);
            }
            void Set(const uint32_t myId, const string& className, const uint32_t interfaceId, const uint32_t versionId)
            {
                Set(myId, className, 0, interfaceId, ~0, versionId, ACQUIRE);
            }
            uint32_t Id() const
            {
                return (GetNumber<uint32_t>(ID_OFFSET));
            }
            Core::instance_id Implementation() const
            {
                return (GetNumber<Core::instance_id>(IMPLEMENTATION_OFFSET));
            }
            uint32_t InterfaceId() const
            {
                return (GetNumber<uint32_t>(INTERFACEID_OFFSET));
            }
            uint32_t ExchangeId() const
            {
                return (GetNumber<uint32_t>(EXCHANGEID_OFFSET));
            }
            uint32_t VersionId() const
            {
                return (GetNumber<uint32_t>(VERSIONID_OFFSET));
            }
            const string ClassName() const
            {
                return GetText(STRINGS_OFFSET);
            }

        public:
            void Clear()
            {
                _data.Clear();
            }
            uint32_t Length() const
            {
                return (_data.Size());
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return(_data.Serialize(offset, stream, maxLength));
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t length, const uint32_t offset)
            {
                return(_data.Deserialize(offset, stream, length));
            }

        private:
            Frame _data;
        };

        class Setup {
        public:
            Setup(const Setup&) = delete;
            Setup& operator=(const Setup&) = delete;

            Setup() : _data() {
            }
            ~Setup() = default;

        public:
            inline void Clear()
            {
                _data.Clear();
            }
            void Set(Core::instance_id implementation, const uint32_t sequenceNumber,const string& proxyStubPath, const string& messagingSettings, const string& warningReportingSettings)
            {
                uint16_t length = 0;
                _data.SetNumber<Core::instance_id>(0, implementation);
                _data.SetNumber<uint32_t>(sizeof(Core::instance_id), sequenceNumber);

                length = _data.SetText(sizeof(Core::instance_id) + sizeof(uint32_t) + sizeof(Output::mode), proxyStubPath);
                length += _data.SetText(sizeof(Core::instance_id)+ sizeof(uint32_t) + sizeof(Output::mode) + length, messagingSettings);
                _data.SetText(sizeof(Core::instance_id)+ sizeof(uint32_t) + sizeof(Output::mode) + length, warningReportingSettings);
            }
            inline bool IsSet() const {
                return (_data.Size() > 0);
            }
            uint32_t SequenceNumber() const
            {
                uint32_t result;
                _data.GetNumber<uint32_t>(sizeof(Core::instance_id), result);
                return (result);
            }
            void Action(const Output::mode remoteAction)
            {
                _data.SetNumber<Output::mode>(sizeof(Core::instance_id) + sizeof(uint32_t), remoteAction);
            }
            Output::mode Action() const
            {
                Output::mode result;
                _data.GetNumber<Output::mode>(sizeof(Core::instance_id) + sizeof(uint32_t), result);
                return (result);
            }
            string ProxyStubPath() const
            {
                string value;

                uint16_t length = sizeof(Core::instance_id) + sizeof(uint32_t) + sizeof(Output::mode); // skip implentation and sequencenumber

                _data.GetText(length, value);

                return (value);
            }
            string MessagingCategories() const
            {
                string value;

                uint16_t length = sizeof(Core::instance_id) + sizeof(uint32_t) + sizeof(Output::mode); // skip implentation and sequencenumber
                length += _data.GetText(length, value);  // skip proxyStub path

                _data.GetText(length, value);

                return (value);
            }
            string WarningReportingCategories() const
            {
                string value;

                uint16_t length = sizeof(Core::instance_id) + sizeof(uint32_t) + sizeof(Output::mode); // skip implentation and sequencenumber
                length += _data.GetText(length, value);  // skip proxyStub path
                length += _data.GetText(length, value);  // skip messagingcategories

                _data.GetText(length, value);

                return (value);
            }
            Core::instance_id Implementation() const
            {
                Core::instance_id result = 0;
                _data.GetNumber<Core::instance_id>(0, result);
                return (result);
            }
            void Implementation(Core::instance_id implementation)
            {
                _data.SetNumber<Core::instance_id>(0, implementation);
            }
            uint32_t Length() const
            {
                return (_data.Size());
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return (_data.Serialize(offset, stream, maxLength));
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                return (_data.Deserialize(offset, stream, maxLength));
            }

        private:
            Frame _data;
        };
    }

    typedef Core::IPCMessageType<1, Data::Init, Data::Setup> AnnounceMessage;
    typedef Core::IPCMessageType<2, Data::Input, Data::Output> InvokeMessage;
}
}
