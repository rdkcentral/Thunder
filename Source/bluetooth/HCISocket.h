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

#include "Module.h"
#include "UUID.h"

namespace WPEFramework {

namespace Bluetooth {

    class Address {
    public:
        Address()
            : _length(0)
        {
        }
        Address(const uint16_t deviceId)
            : _length(0)
        {
            if (hci_devba(deviceId, &_address) >= 0) {
                _length = sizeof(_address);
            }
        }
        Address(const bdaddr_t& address)
            : _length(sizeof(_address))
        {
            ::memcpy(&_address, &address, sizeof(_address));
        }
        Address(const TCHAR address[])
            : _length(sizeof(_address))
        {
            ::memset(&_address, 0, sizeof(_address));
            str2ba(address, &_address);
        }
        Address(const Address& address)
            : _length(address._length)
        {
            ::memcpy(&_address, &(address._address), sizeof(_address));
        }
        ~Address()
        {
        }

        enum type : uint8_t {
            BREDR_ADDRESS = 0x00,
            LE_PUBLIC_ADDRESS = 0x01,
            LE_RANDOM_ADDRESS = 0x02
        };

    public:
        Address& operator=(const Address& rhs)
        {
            _length = rhs._length;
            ::memcpy(&_address, &(rhs._address), sizeof(_address));
            return (*this);
        }
        bool IsValid() const
        {
            return (_length == sizeof(_address));
        }
        static Address Default()
        {
            int deviceId = hci_get_route(nullptr);
            return ((deviceId >= 0) ? Address(static_cast<uint16_t>(deviceId)) : Address());
        }
        static Address AnyInterface()
        {
            static bdaddr_t g_anyAddress = { 0 };
            return (Address(g_anyAddress));
        }
        static Address LocalInterface()
        {
            static bdaddr_t g_localAddress = { 0, 0, 0, 0xFF, 0xFF, 0xFF };
            return (Address(g_localAddress));
        }
        const bdaddr_t* Data() const
        {
            return (IsValid() ? &_address : nullptr);
        }
        uint8_t Length() const
        {
            return (_length);
        }
        Core::NodeId NodeId(const uint16_t channelType) const
        {
            Core::NodeId result;
            int deviceId = hci_get_route(const_cast<bdaddr_t*>(Data()));

            if (deviceId >= 0) {
                result = Core::NodeId(static_cast<uint16_t>(deviceId), channelType);
            }

            return (result);
        }
        Core::NodeId NodeId(const uint8_t addressType, const uint16_t cid, const uint16_t psm) const
        {
            return (Core::NodeId(_address, addressType, cid, psm));
        }
        bool operator==(const Address& rhs) const
        {
            return ((_length == rhs._length) && (memcmp(rhs._address.b, _address.b, _length) == 0));
        }
        bool operator!=(const Address& rhs) const
        {
            return (!operator==(rhs));
        }
        void OUI(char oui[9]) const
        {
            ba2oui(Data(), oui);
        }
        string ToString() const
        {
            static constexpr TCHAR _hexArray[] = "0123456789ABCDEF";
            string result;

            if (IsValid() == true) {
                for (uint8_t index = 0; index < _length; index++) {
                    if (result.empty() == false) {
                        result += ':';
                    }
                    result += _hexArray[(_address.b[(_length - 1) - index] >> 4) & 0x0F];
                    result += _hexArray[_address.b[(_length - 1) - index] & 0x0F];
                }
            }

            return (result);
        }

    private:
        bdaddr_t _address;
        uint8_t _length;
    };

    class EIR {
        static constexpr uint8_t  EIR_UUID16_SOME = 0x02;
        static constexpr uint8_t  EIR_UUID16_ALL = 0x03;
        static constexpr uint8_t  EIR_UUID32_SOME = 0x04;
        static constexpr uint8_t  EIR_UUID32_ALL = 0x05;
        static constexpr uint8_t  EIR_UUID128_SOME = 0x06;
        static constexpr uint8_t  EIR_UUID128_ALL = 0x07;
        static constexpr uint8_t  EIR_NAME_SHORT = 0x08;
        static constexpr uint8_t  EIR_NAME_COMPLETE = 0x09;
        static constexpr uint8_t  EIR_CLASS_OF_DEV = 0x0D;

    public:
        EIR()
            : _shortName()
            , _completeName()
            , _class(0)
            , _UUIDs()
        {
        }
        explicit EIR(const string& name, const uint32_t deviceClass = 0)
            : _shortName(name)
            , _completeName(name)
            , _class(deviceClass)
            , _UUIDs()
        {
        }
        EIR(const uint8_t buffer[], const uint16_t bufferLength)
            : EIR()
        {
            Ingest(buffer, bufferLength);
        }
        ~EIR() = default;

    public:
        uint32_t Class() const
        {
            return _class;
        }
        const string& ShortName() const
        {
            if (_shortName.empty() == true) {
                return (_completeName);
            } else {
                return _shortName;
            }
        }
        const string& CompleteName() const
        {
            if (_completeName.empty() == true) {
                return (_shortName);
            } else {
                return (_completeName);
            }
        }
        const std::list<UUID>& UUIDs() const
        {
            return _UUIDs;
        }

        void Ingest(const uint8_t buffer[], const uint16_t bufferLength);

    private:
        string _shortName;
        string _completeName;
        uint32_t _class;
        std::list<UUID> _UUIDs;
    };

    template<typename KEYTYPE>
    class KeyListType {
    public:
        typedef KEYTYPE type;

        KeyListType() : _list() {
        }
        KeyListType(const KeyListType<KEYTYPE>& copy) : _list(copy._list) {
        }
        ~KeyListType() {
        }

        KeyListType<KEYTYPE>& operator= (const KeyListType<KEYTYPE>&) = delete;

    public:
        using ConstIterator =  Core::IteratorType<const std::list<KEYTYPE>, const KEYTYPE&, typename std::list<KEYTYPE>::const_iterator>;

        static uint8_t Length() {
            return (KEYTYPE::Length());
        }
        void Add(const KEYTYPE& key) {
            _list.push_back(key);
        }
        void Add(const KeyListType<KEYTYPE>& keylist) {
            auto index = keylist.Elements();
            while (index.Next() == true) {
                Add(index.Current());
            }
        }
        uint8_t Entries() const {
            return (_list.size());
        }
        void Clear() {
            _list.clear();
        }
        bool IsValid() const {
            auto index = Elements();
            while (index.Next() == true) {
                if (index.Current().IsValid() != true) {
                    return (false);
                }
            }
            return (true);
        }
        uint16_t Clone(const uint16_t length, uint8_t buffer[]) const {
            uint16_t result = 0;
            typename std::list<KEYTYPE>::const_iterator index (_list.begin());
            while ( (index != _list.end()) && (result <= (length - KEYTYPE::Length())) ) {
                ::memcpy(&(buffer[result]), index->Data(), KEYTYPE::Length());
                result += KEYTYPE::Length();
                index++;
            }
            return (result);
        }
        ConstIterator Elements() const {
            return (ConstIterator(_list));
        }

    private:
        std::list<KEYTYPE> _list;
    };

    class LinkKey {
    public:
        LinkKey() {
            ::memset(&_key, 0, sizeof(_key));
            _key.addr.type = ~0;
        }
        LinkKey(const Address& address, const uint8_t key[16], const uint8_t pinLength, const uint8_t type) {
            ::memcpy(&(_key.addr.bdaddr), address.Data(), sizeof(_key.addr.bdaddr));
            ::memcpy(&(_key.val), key, sizeof(_key.val));
            _key.addr.type = BDADDR_BREDR;
            _key.pin_len = pinLength;
            _key.type = type;
        }
        LinkKey(const uint8_t buffer[], const uint16_t length) {
            if (length == sizeof(_key)) {
                ::memcpy(&_key, buffer, sizeof(_key));
            } else {
                LinkKey();
            }
        }
        LinkKey(const Address& address, const uint8_t address_type, const string& keyString) {
            ::memcpy(&(_key.addr.bdaddr), address.Data(), sizeof(_key.addr.bdaddr));
            _key.addr.type = address_type;

            ASSERT(address_type == BDADDR_BREDR);

            // The first two charaters are for the pin and the type, extract those...
            _key.pin_len = keyString[0] - 'A';
            _key.type    = keyString[1] - 'A';

            uint16_t  length = sizeof(_key.val);
            Core::FromString(string(&(keyString.c_str()[2]), keyString.length() - 2), reinterpret_cast<uint8_t*>(_key.val), length, nullptr);
            if (length != sizeof(_key.val)) {
                // Seems the value is not properly restored, invalidate the object!!
                _key.pin_len = 0xFF;
            }
        }
        LinkKey(const LinkKey& copy) {
            ::memcpy(&_key, &copy._key, sizeof(_key));
        }
        ~LinkKey() {
        }

    public:
        bool IsValid() const {
            return ((Pin() <= 16) && (Type() <= 8) && (LocatorType() == Bluetooth::Address::BREDR_ADDRESS));
        }
        Address Locator() const {
            return (_key.addr.bdaddr);
        }
        uint8_t LocatorType() const {
            return (_key.addr.type);
        }
        uint8_t Pin() const {
            return(_key.pin_len);
        }
        uint8_t Type() const {
            return(_key.type);
        }
        const uint8_t* Key() const {
            return (_key.val);
        }
        const uint8_t* Data() const {
            return (reinterpret_cast<const uint8_t*>(&_key));
        }
        static uint8_t Length() {
            return (sizeof(_key));
        }
        string ToString() const {
            string baseKey;
            Core::ToString(&(reinterpret_cast<const uint8_t*>(&_key)[sizeof(_key.addr)]), sizeof(_key) - sizeof(_key.addr), false, baseKey);
            baseKey = static_cast<const char>(_key.type + 'A') + baseKey;
            baseKey = static_cast<const char>(_key.pin_len + 'A') + baseKey;
            return (baseKey);
        }

    private:
        struct mgmt_link_key_info _key;
    };

    class LongTermKey {
    public:
        LongTermKey() {
            ::memset(&_key, 0, sizeof(_key));
            _key.addr.type = ~0;
        }
        LongTermKey(const Address& address, const uint8_t address_type, const uint8_t type, const uint8_t master, const uint8_t encryptionSize,
                    const uint16_t diversifier, const uint64_t random, const uint8_t value[16])
        {
            ::memcpy(&(_key.addr.bdaddr), address.Data(), sizeof(_key.addr.bdaddr));
            ::memcpy(&(_key.val), value, sizeof(_key.val));
            _key.addr.type = address_type;
            _key.type = type;
            _key.master = master;
            _key.enc_size = encryptionSize;
            _key.ediv = htobs(diversifier); // 16 bits
            _key.rand = htobll(random); // 64 bits
        }
        LongTermKey(const uint8_t buffer[], const uint16_t length) {
            if (length == sizeof(_key)) {
                ::memcpy(&_key, buffer, sizeof(_key));
            } else {
                LongTermKey();
            }
        }
        LongTermKey(const Address& address, const uint8_t address_type, const string& keyString) {
            ::memcpy(&(_key.addr.bdaddr), address.Data(), sizeof(_key.addr.bdaddr));
            _key.addr.type = address_type;

            uint16_t  length = sizeof(_key) - sizeof(_key.addr);
            Core::FromString(keyString, &(reinterpret_cast<uint8_t*>(&_key)[sizeof(_key.addr)]), length, nullptr);
            if (length != (sizeof(_key) - sizeof(_key.addr))) {
                // Seems the value is not properly restored, invalidate the object!!
                _key.enc_size = 0;
            }
        }
        LongTermKey(const LongTermKey& copy) {
            ::memcpy(&_key, &copy._key, sizeof(_key));
        }
        ~LongTermKey() {
        }

    public:
        bool IsValid() const {
            return ((EncryptionSize() == sizeof(_key.val)) && (Authenticated() <= 4) && (Master() <= 1)
                    && ((LocatorType() == Address::LE_PUBLIC_ADDRESS) || ((LocatorType() == Address::LE_RANDOM_ADDRESS) && (_key.addr.bdaddr.b[5] & 0xc0) /* static random */)));
        }
        Address Locator() const {
            return (_key.addr.bdaddr);
        }
        uint8_t LocatorType() const {
            return (_key.addr.type);
        }
        uint8_t Master() const {
            return(_key.master);
        }
        uint8_t Authenticated() const {
            return(_key.type);
        }
        uint8_t EncryptionSize() const {
            return(_key.enc_size);
        }
        uint16_t Diversifier() const {
            return(btohs(_key.ediv));
        }
        uint64_t Random() const {
            return(btohll(_key.rand));
        }
        const uint8_t* Value() const {
            return (_key.val);
        }
        const uint8_t* Data() const {
            return (reinterpret_cast<const uint8_t*>(&_key));
        }
        static uint8_t Length() {
            return (sizeof(_key));
        }
        string ToString() const {
            string baseKey;
            Core::ToString(&(reinterpret_cast<const uint8_t*>(&_key)[sizeof(_key.addr)]), sizeof(_key) - sizeof(_key.addr), false, baseKey);
            return (baseKey);
        }

    private:
        struct mgmt_ltk_info _key;
    };

    class IdentityKey {
    public:
        IdentityKey() {
            ::memset(&_key, 0, sizeof(_key));
            _key.addr.type = ~0;
        }
        IdentityKey(const Address& address, const uint8_t addressType, const uint8_t value[16]) {
            ::memcpy(&(_key.addr.bdaddr), address.Data(), sizeof(_key.addr.bdaddr));
            ::memcpy(&(_key.val), value, sizeof(_key.val));
            _key.addr.type = addressType;
        }
        IdentityKey(const uint8_t buffer[], const uint16_t length) {
            if (length == sizeof(_key)) {
                ::memcpy(&_key, buffer, sizeof(_key));
            } else {
                IdentityKey();
            }
        }
        IdentityKey(const Address& address, const uint8_t address_type, const string& keyString) {
             ::memcpy(&(_key.addr.bdaddr), address.Data(), sizeof(_key.addr.bdaddr));
            _key.addr.type = address_type;

            uint16_t  length = sizeof(_key) - sizeof(_key.addr);
            Core::FromString(keyString, &(reinterpret_cast<uint8_t*>(&_key)[sizeof(_key.addr)]), length, nullptr);
            if (length != (sizeof(_key) - sizeof(_key.addr))) {
                // Seems the value is not properly restored, invalidate the object!!
                _key.addr.type  = 0xFF;
            }
        }
        IdentityKey(const IdentityKey& copy) {
            ::memcpy(&_key, &copy._key, sizeof(_key));
        }
        ~IdentityKey() {
        }

    public:
        bool IsValid() const {
            return ((LocatorType() == Address::LE_PUBLIC_ADDRESS) || ((LocatorType() == Address::LE_RANDOM_ADDRESS) && (_key.addr.bdaddr.b[5] & 0xc0) /* static random */));
        }
        Address Locator() const {
            return (_key.addr.bdaddr);
        }
        Address LocatorType() const {
            return (_key.addr.type);
        }
        const uint8_t* Value() const {
            return (_key.val);
        }
        const uint8_t* Data() const {
            return (reinterpret_cast<const uint8_t*>(&_key));
        }
        static uint8_t Length() {
            return (sizeof(_key));
        }
        string ToString() const {
            string baseKey;
            Core::ToString(&(reinterpret_cast<const uint8_t*>(&_key)[sizeof(_key.addr)]), sizeof(_key) - sizeof(_key.addr), false, baseKey);
            return (baseKey);
        }

    private:
        struct mgmt_irk_info _key;
    };

    class SignatureKey {
    public:
        SignatureKey() {
            ::memset(&_key, 0, sizeof(_key));
            _key.addr.type = ~0;
        }
        SignatureKey(const Address& address, const uint8_t address_type, const uint8_t type, const uint8_t value[16]) {
            ::memcpy(&(_key.addr.bdaddr), address.Data(), sizeof(_key.addr.bdaddr));
            ::memcpy(&(_key.val), value, sizeof(_key.val));
            _key.addr.type = address_type;
            _key.type = type;
        }
        SignatureKey(const uint8_t buffer[], const uint16_t length) {
            if (length == sizeof(_key)) {
                ::memcpy(&_key, buffer, sizeof(_key));
            } else {
                SignatureKey();
            }
        }
        SignatureKey(const Address& address, const uint8_t address_type, const string& keyString) {
            ::memcpy(&(_key.addr.bdaddr), address.Data(), sizeof(_key.addr.bdaddr));
            _key.addr.type = address_type;

            uint16_t  length = sizeof(_key) - sizeof(_key.addr);
            Core::FromString(keyString, &(reinterpret_cast<uint8_t*>(&_key)[sizeof(_key.addr)]), length, nullptr);
            if (length != (sizeof(_key) - sizeof(_key.addr))) {
                // Seems the value is not properly restored, invalidate the object!!
                _key.type = 0xFF;
            }
        }
        SignatureKey(const SignatureKey& copy) {
            ::memcpy(&_key, &copy._key, sizeof(_key));
        }
        ~SignatureKey() {
        }

    public:
        bool IsValid() const {
            return ((Type() <= 3)
                    && ((LocatorType() == Address::LE_PUBLIC_ADDRESS) || ((LocatorType() == Address::LE_RANDOM_ADDRESS))));
        }
        Address Locator() const {
            return (_key.addr.bdaddr);
        }
        uint8_t LocatorType() const {
            return (_key.addr.type);
        }
        uint8_t Type() const {
            return(_key.type);
        }
        const uint8_t* Value() const {
            return (_key.val);
        }
        const uint8_t* Data() const {
            return (reinterpret_cast<const uint8_t*>(&_key));
        }
        static uint8_t Length() {
            return (sizeof(_key));
        }
        string ToString() const {
            string baseKey;
            Core::ToString(&(reinterpret_cast<const uint8_t*>(&_key)[sizeof(_key.addr)]), sizeof(_key) - sizeof(_key.addr), false, baseKey);
            return (baseKey);
        }

    private:
        struct mgmt_csrk_info _key;
    };

    typedef KeyListType<LinkKey> LinkKeys;
    typedef KeyListType<LongTermKey> LongTermKeys;
    typedef KeyListType<IdentityKey> IdentityKeys;
    typedef KeyListType<SignatureKey> SignatureKeys;


    class HCISocket : public Core::SynchronousChannelType<Core::SocketPort> {
    private:
        static constexpr int      SCAN_TIMEOUT = 1000;
        static constexpr uint8_t  SCAN_TYPE = 0x01;
        static constexpr uint8_t  SCAN_FILTER_POLICY = 0x00;
        static constexpr uint8_t  SCAN_FILTER_DUPLICATES = 0x01;
        static constexpr uint32_t MAX_ACTION_TIMEOUT = 2000; /* 2 Seconds for commands to complete ? */
        static constexpr uint16_t ACTION_MASK = 0x3FFF;

    public:

        template<const uint16_t OPCODE, typename OUTBOUND, typename INBOUND, const uint8_t RESPONSECODE = ~0>
        class CommandType : public Core::IOutbound, public Core::IInbound {
        private:
            CommandType<OPCODE, OUTBOUND, INBOUND, RESPONSECODE>& operator=(const CommandType<OPCODE, OUTBOUND, INBOUND, RESPONSECODE>&) = delete;

        public:
            enum : uint16_t { ID = OPCODE };

        public:
            CommandType()
                : _offset(sizeof(_buffer))
                , _error(~0)
            {
                _buffer[0] = HCI_COMMAND_PKT;
                _buffer[1] = (OPCODE & 0xFF);
                _buffer[2] = ((OPCODE >> 8) & 0xFF);
                _buffer[3] = static_cast<uint8_t>(sizeof(OUTBOUND));

                ::memset(&_response, 0, sizeof(_response));
            }
            CommandType(const CommandType<OPCODE, OUTBOUND, INBOUND, RESPONSECODE>& copy)
                : _offset(copy._offset)
                , _error(~0)
            {
                ::memcpy(_buffer, copy._buffer, sizeof(_buffer));
                ::memcpy(&_response, &copy._response, sizeof(_response));
            }
            virtual ~CommandType()
            {
            }

        public:
            inline void Clear()
            {
                ::memset(&(_buffer[4]), 0, sizeof(_buffer) - 4);
            }
            inline uint32_t Error() const
            {
                return (_error);
            }
            virtual void Reload() const override
            {
                _offset = 0;
            }
            virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const override
            {
                uint16_t result = std::min(static_cast<uint16_t>(sizeof(_buffer) - _offset), length);
                if (result > 0) {

                    ::memcpy(stream, &(_buffer[_offset]), result);
                    _offset += result;

                    //printf("SEND: ");cfor (uint16_t loop = 0; loop < result; loop++) { printf("%02X:", stream[loop]); } printf("\n");
                    //printf(_T("HCI command: %X:%03X\n"), cmd_opcode_ogf(OPCODE), cmd_opcode_ocf(OPCODE));
                }
                return (result);
            }
            OUTBOUND* operator->()
            {
                return (reinterpret_cast<OUTBOUND*>(&(_buffer[4])));
            }
            const INBOUND& Response() const
            {
                return (_response);
            }

        private:
            virtual Core::IInbound::state IsCompleted() const override
            {
                return (_error != static_cast<uint16_t>(~0) ? Core::IInbound::COMPLETED : Core::IInbound::INPROGRESS);
            }
            virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override
            {
                uint16_t result = 0;
                if (length >= (HCI_EVENT_HDR_SIZE + 1)) {
                    const hci_event_hdr* hdr = reinterpret_cast<const hci_event_hdr*>(&(stream[1]));
                    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&(stream[1 + HCI_EVENT_HDR_SIZE]));
                    uint16_t len = (length - (1 + HCI_EVENT_HDR_SIZE));

                    //printf("RECEIVE: "); for (uint16_t loop = 0; loop < length; loop++) { printf("%02X:", stream[loop]); } printf("\n");

                    if (hdr->evt == EVT_CMD_STATUS) {
                        const evt_cmd_status* cs = reinterpret_cast<const evt_cmd_status*>(ptr);
                        if (btohs(cs->opcode) == OPCODE) {
                            printf(_T("HCI command status: %X:%03X Status=%d\n"),
                                      cmd_opcode_ogf(cs->opcode), cmd_opcode_ocf(cs->opcode), cs->status);

                            if (cs->status == 0) {
                                // See if we are waiting for an event...
                                if (RESPONSECODE == static_cast<uint8_t>(~0)) {
                                    _error = Core::ERROR_NONE;
                                }
                            }
                            else {
                                _error =  Core::ERROR_GENERAL;
                            }
                            result = length;
                        }
                    } else if (hdr->evt == EVT_CMD_COMPLETE) {
                        const evt_cmd_complete* cc = reinterpret_cast<const evt_cmd_complete*>(ptr);
                        if (btohs(cc->opcode) == OPCODE) {
                            // printf(_T("HCI command complete: %X:%03X %s\n"),
                            //          cmd_opcode_ogf(cc->opcode), cmd_opcode_ocf(cc->opcode), len <= EVT_CMD_COMPLETE_SIZE? "FAILURE" : "");

                            if (len <= EVT_CMD_COMPLETE_SIZE) {
                                _error = Core::ERROR_GENERAL;
                            } else {
                                // See if we are waiting for an event...
                                if (RESPONSECODE == static_cast<uint8_t>(~0)) {
                                    _error = Core::ERROR_NONE;
                                    uint16_t toCopy = std::min(static_cast<uint16_t>(sizeof(_response)), static_cast<uint16_t>(len - EVT_CMD_COMPLETE_SIZE));
                                    ::memcpy(&_response, &(ptr[EVT_CMD_COMPLETE_SIZE]), toCopy);
                                }
                            }
                            result = length;
                        }
                    } else if ((hdr->evt == EVT_LE_META_EVENT) && (cmd_opcode_ogf(OPCODE) == OGF_LE_CTL)) {
                        const evt_le_meta_event* eventMetaData = reinterpret_cast<const evt_le_meta_event*>(ptr);
                        if (eventMetaData->subevent == RESPONSECODE) {
                            uint16_t toCopy = std::min(static_cast<uint16_t>(sizeof(_response)), static_cast<uint16_t>(len - EVT_LE_META_EVENT_SIZE));
                            ::memcpy(&_response, &(ptr[EVT_LE_META_EVENT_SIZE]), toCopy);
                            _error = Core::ERROR_NONE;
                            result = length;
                        }
                    } else if (hdr->evt == RESPONSECODE) {
                        ::memcpy(&_response, ptr, std::min(static_cast<uint16_t>(sizeof(_response)), len));
                        _error = Core::ERROR_NONE;
                        result = length;
                    }
                }
                return (result);
            }

        private:
            mutable uint16_t _offset;
            uint8_t _buffer[1 + 3 + sizeof(OUTBOUND)];
            INBOUND _response;
            uint16_t _error;
        };

    public:
        class FeatureIterator {
        public:
            FeatureIterator()
                : _index(-1)
            {
                ::memset(_features, 0, sizeof(_features));
            }
            FeatureIterator(const uint8_t length, const uint8_t data[])
                : _index(-1)
            {
                uint8_t copyLength = std::min(length, static_cast<uint8_t>(sizeof(_features)));
                ::memcpy(_features, data, copyLength);
                if (copyLength < sizeof(_features)) {
                    ::memset(&_features[copyLength], 0, (sizeof(_features) - copyLength));
                }
            }
            FeatureIterator(const FeatureIterator& copy)
                : _index(copy._index)
            {
                ::memcpy(_features, copy._features, sizeof(_features));
            }
            ~FeatureIterator()
            {
            }

            public:
            FeatureIterator& operator=(const FeatureIterator& rhs)
            {
                _index = rhs._index;
                ::memcpy(_features, rhs._features, sizeof(_features));

                return (*this);
            }

            void Reset()
            {
                _index = -1;
            }
            bool IsValid() const
            {
                return ((_index >= 0) && (_index < static_cast<int16_t>(sizeof(_features) * 8)));
            }
            bool Next()
            {
                _index++;

                while ((_index < static_cast<int16_t>(sizeof(_features) * 8)) && ((_features[_index >> 3] & (1 << (_index & 0x7))) == 0)) {
                    _index++;
                }
                return (_index < static_cast<int16_t>(sizeof(_features) * 8));
            }
            uint8_t Feature() const
            {
                return (_index);
            }
            const TCHAR* Text() const
            {
                uint16_t index = (((index & 0xF8) << 5) | (1 << (_index & 0x7)));
                return (FeatureToText(index));
            }
            bool HasFeatures(const uint8_t byte, uint8_t bit) const
            {
                return (byte < sizeof(_features) ? (_features[byte] & bit) != 0 : false);
            }

        private:
            const TCHAR* FeatureToText(const uint16_t index) const;

        private:
            int16_t _index;
            uint8_t _features[8];
        };

        // ------------------------------------------------------------------------
        // Create definitions for the HCI commands
        // ------------------------------------------------------------------------
        struct Command {
            typedef CommandType<cmd_opcode_pack(OGF_LINK_CTL, OCF_CREATE_CONN), create_conn_cp, evt_conn_complete>
                Connect;

            typedef CommandType<cmd_opcode_pack(OGF_LINK_CTL, OCF_AUTH_REQUESTED), auth_requested_cp, evt_auth_complete>
                Authenticate;

            typedef CommandType<cmd_opcode_pack(OGF_LINK_CTL, OCF_USER_CONFIRM_REPLY), user_confirm_reply_cp, Core::Void>
                UserConfirmReply;

            typedef CommandType<cmd_opcode_pack(OGF_LINK_CTL, OCF_USER_CONFIRM_NEG_REPLY), user_confirm_reply_cp, Core::Void>
                UserConfirmNegReply;

            typedef CommandType<cmd_opcode_pack(OGF_LINK_CTL, OCF_DISCONNECT), disconnect_cp, evt_disconn_complete>
                Disconnect;

            typedef CommandType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_CREATE_CONN), le_create_connection_cp, evt_le_connection_complete, EVT_LE_CONN_COMPLETE>
                ConnectLE;

            typedef CommandType<cmd_opcode_pack(OGF_LINK_CTL, OCF_SET_CONN_ENCRYPT), set_conn_encrypt_cp, evt_encrypt_change, EVT_ENCRYPT_CHANGE>
                Encrypt;

            typedef CommandType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_START_ENCRYPTION), le_start_encryption_cp, uint8_t>
                EncryptLE;

            typedef CommandType<cmd_opcode_pack(OGF_LINK_CTL, OCF_REMOTE_NAME_REQ), remote_name_req_cp, Core::Void>
                RemoteName;

            typedef CommandType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_SCAN_PARAMETERS), le_set_scan_parameters_cp, uint8_t>
                ScanParametersLE;

            typedef CommandType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE), le_set_scan_enable_cp, uint8_t>
                ScanEnableLE;

            typedef CommandType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_CLEAR_WHITE_LIST), Core::Void, Core::Void>
                ClearWhiteList;

            typedef CommandType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_READ_WHITE_LIST_SIZE), Core::Void, le_read_white_list_size_rp>
                ReadWhiteListSize;

            typedef CommandType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_ADD_DEVICE_TO_WHITE_LIST), le_add_device_to_white_list_cp, Core::Void>
                AddDeviceToWhiteList;

            typedef CommandType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_REMOVE_DEVICE_FROM_WHITE_LIST), le_remove_device_from_white_list_cp, uint8_t>
                RemoveDeviceFromWhiteList;

            typedef CommandType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_READ_REMOTE_USED_FEATURES), le_read_remote_used_features_cp, evt_le_read_remote_used_features_complete, EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE>
                RemoteFeaturesLE;

            typedef CommandType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_ADVERTISING_PARAMETERS), le_set_advertising_parameters_cp, uint8_t>
                AdvertisingParametersLE;

            typedef CommandType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_ADVERTISE_ENABLE), le_set_advertise_enable_cp, uint8_t>
                AdvertisingEnableLE;

            typedef CommandType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_CONN_UPDATE), le_connection_update_cp, evt_le_connection_update_complete, EVT_LE_CONN_UPDATE_COMPLETE>
                ConnectionUpdate;

            typedef CommandType<cmd_opcode_pack(OGF_HOST_CTL, OCF_READ_STORED_LINK_KEY), read_stored_link_key_cp, read_stored_link_key_rp>
                ReadStoredLinkKey;
        };

        enum state : uint16_t {
            IDLE        = 0x0000,
            SCANNING    = 0x0001,
            PAIRING     = 0x0002,
            ADVERTISING = 0x4000,
            ABORT       = 0x8000
        };

    public:
        HCISocket(const HCISocket&) = delete;
        HCISocket& operator=(const HCISocket&) = delete;

        HCISocket()
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::RAW, Core::NodeId(), Core::NodeId(), 1024, 1024)
            , _state(IDLE)
        {
        }
        HCISocket(const Core::NodeId& sourceNode)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::RAW, sourceNode, Core::NodeId(), 1024, 1024)
            , _state(IDLE)
        {
        }
        virtual ~HCISocket()
        {
            Close(Core::infinite);
        }

    public:
        bool IsScanning() const
        {
            return ((_state & SCANNING) != 0);
        }
        bool IsAdvertising() const
        {
            return ((_state & ADVERTISING) != 0);
        }
        uint32_t Advertising(const bool enable, const uint8_t mode = 0);
        void Scan(const uint16_t scanTime, const uint32_t type, const uint8_t flags);
        void Scan(const uint16_t scanTime, const bool limited, const bool passive);
        void Abort();

        uint32_t ReadStoredLinkKeys(const Address adr, const bool all, LinkKeys& keys);

        template<typename COMMAND>
        void Execute(const uint32_t waitTime, const COMMAND& cmd, std::function<void(COMMAND&, const uint32_t error)> handler)
        {
            class Handler : public Core::IOutbound::ICallback {
            public:
                Handler() = delete;
                Handler(const Handler&) = delete;
                Handler(const COMMAND& cmd, const std::function<void(COMMAND&, const uint32_t error)> handler)
                    : _cmd(cmd)
                    , _handler(handler){
                }
                virtual ~Handler() {
                }

            public:
                COMMAND& Cmd() {
                    return (_cmd);
                }
                void Updated(const Core::IOutbound& data, const uint32_t error_code) override {
                    //ASSERT(_cmd == data);
                    _handler(_cmd, error_code);
                    delete this;
                }

            private:
                COMMAND _cmd;
                std::function<void(COMMAND&, const uint32_t error)> _handler;
            };
            Handler* entry = new Handler(cmd, handler);

            Send(waitTime, entry->Cmd(), entry, &(entry->Cmd()));
        }

    protected:
        virtual void Update(const le_advertising_info& eventData);
        virtual void Update(const hci_event_hdr& eventData);
        virtual void Discovered(const bool lowEnergy, const Bluetooth::Address& address, const Bluetooth::EIR& info);

    private:
        virtual void StateChange() override;
        virtual uint16_t Deserialize(const uint8_t* dataFrame, const uint16_t availableData) override;
        void SetOpcode(const uint16_t opcode);

    private:
        Core::StateTrigger<state> _state;
        struct hci_filter _filter;
    };

    class ManagementSocket : public Core::SynchronousChannelType<Core::SocketPort> {
    public:
        class Info {
        public:
            class Properties {
            public:
                Properties() : _value(0) {}
                Properties(const uint32_t value) : _value(value) {}
                Properties(const Properties& copy) : _value(copy._value) {}
                ~Properties() {}

                Properties& operator= (const Properties& rhs) {
                    _value = rhs._value;
                    return (*this);
                }

            public:
                bool IsPowered() const {
                    return ((_value & MGMT_SETTING_POWERED) != 0);
                }
		bool IsConnectable() const {
                    return ((_value & MGMT_SETTING_CONNECTABLE) != 0);
                }
		bool IsFastConnectable() const {
                    return ((_value & MGMT_SETTING_FAST_CONNECTABLE) != 0);
                }
		bool IsDiscoverable() const {
                    return ((_value & MGMT_SETTING_DISCOVERABLE) != 0);
                }
		bool IsBondable() const {
                    return ((_value & MGMT_SETTING_BONDABLE) != 0);
                }
		bool HasLinkLevelSecurity() const {
                    return ((_value & MGMT_SETTING_LINK_SECURITY) != 0);
                }
		bool HasSecureSimplePairing() const {
                    return ((_value & MGMT_SETTING_SSP) != 0);
                }
		bool HasBasicEnhancedRate() const {
                    return ((_value & MGMT_SETTING_BREDR) != 0);
                }
		bool HasHighSpeed() const {
                    return ((_value & MGMT_SETTING_HS) != 0);
                }
		bool HasLowEnergy() const {
                    return ((_value & MGMT_SETTING_LE) != 0);
                }
		bool IsAdvertising() const {
                    return ((_value & MGMT_SETTING_ADVERTISING) != 0);
                }
		bool HasSecureConnections() const {
                    return ((_value & MGMT_SETTING_SECURE_CONN) != 0);
                }
		bool HasDebugKeys() const {
                    return ((_value & MGMT_SETTING_DEBUG_KEYS) != 0);
                }
		bool HasPrivacy() const {
                    return ((_value & MGMT_SETTING_PRIVACY) != 0);
                }
		bool HasConfiguration() const {
                    return ((_value & MGMT_SETTING_CONFIGURATION) != 0);
                }
		bool HasStaticAddress() const {
                    return ((_value & MGMT_SETTING_STATIC_ADDRESS) != 0);
                }

            private:
                uint32_t _value;
            };
        public:
            Info()
                : _address()
                , _version(0)
                , _manufacturer(0)
                , _supported(0)
                , _settings(0)
                , _deviceClass(0)
                , _name()
                , _shortName()
            {
            }
            Info(const Info& copy)
                : _address(copy._address)
                , _version(copy._version)
                , _manufacturer(copy._manufacturer)
                , _supported(copy._supported)
                , _settings(copy._settings)
                , _deviceClass(copy._deviceClass)
                , _name(copy._name)
                , _shortName(copy._shortName)
            {
            }
            Info(const mgmt_rp_read_info& copy)
                : _address(copy.bdaddr)
                , _version(copy.version)
                , _manufacturer(copy.manufacturer)
                , _supported(copy.supported_settings)
                , _settings(copy.current_settings)
                , _deviceClass((copy.dev_class[2] << 16) | (copy.dev_class[1] << 8) | copy.dev_class[0])
                , _name(Core::ToString(reinterpret_cast<const char*>(copy.name)))
                , _shortName(Core::ToString(reinterpret_cast<const char*>(copy.short_name)))
            {
            }
            ~Info()
            {
            }

        public:
            const Bluetooth::Address& Address() const {
                return (_address);
            }
            uint8_t Version () const {
                return (_version);
            }
            uint16_t Manufacturer() const {
                return (_manufacturer);
            }
            Properties Supported () const {
                return (Properties(_supported));
            }
            Properties Actuals() const {
                return (Properties(_settings));
            }
            uint32_t DeviceClass() const {
                return (_deviceClass);
            }
            const string& ShortName() const {
                return (_shortName);
            }
            const string& Name() const {
                return (_name);
            }

        private:
            Bluetooth::Address _address;
            uint8_t _version;
            uint16_t _manufacturer;
            uint32_t _supported;
            uint32_t _settings;
            uint32_t _deviceClass;
            string _name;
            string _shortName;
        };

        enum capabilities : uint8_t {
            DISPLAY_ONLY = 0x00,
            DISPLAY_YES_NO = 0x01,
            KEYBOARD_ONLY = 0x02,
            NO_INPUT_NO_OUTPUT = 0x03,
            KEYBOARD_DISPLAY = 0x04,
            INVALID = 0xFF
        };

        enum autoconnmode : uint8_t {
            REPORT = 0x00,
            DIRECT = 0x01, // reconnect on direct advertisement
            ALWAYS = 0x02
        };

    public:
        ManagementSocket(const ManagementSocket&) = delete;
        ManagementSocket& operator=(const ManagementSocket&) = delete;

        ManagementSocket()
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::RAW, Core::NodeId(HCI_DEV_NONE, HCI_CHANNEL_CONTROL), Core::NodeId(), 1024, 1024)
            , _deviceId(~0)
        {
            if (Core::SynchronousChannelType<Core::SocketPort>::Open(Core::infinite) != Core::ERROR_NONE) {
            }
        }
        virtual ~ManagementSocket()
        {
            Core::SynchronousChannelType<Core::SocketPort>::Close(Core::infinite);
        }

    public:
        static void Devices(std::list<uint16_t>& list);

        void DeviceId (const uint16_t deviceId)
        {
            ASSERT((_deviceId == static_cast<uint16_t>(~0)) ^ (deviceId == static_cast<uint16_t>(~0)));

            _deviceId = deviceId;
        }
        uint16_t DeviceId() const
        {
            return (_deviceId);
        }
        static bool Up(const uint16_t deviceId)
        {
            bool result = false;
            int descriptor;

            if ((descriptor = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
                TRACE_L1("Could not open a socket. Error: %d", errno);
            }
            else {
                if ( (::ioctl(descriptor, HCIDEVUP, deviceId) == 0) || (errno == EALREADY) ) {
                    result = true;
                }
                else {
                    TRACE_L1("Could not bring up the interface [%d]. Error: %d", deviceId, errno);
                }
                ::close(descriptor);
            }
            return (result);
        }
        static bool Down(const uint16_t deviceId)
        {
            bool result = false;
            int descriptor;
            if ((descriptor = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
                TRACE_L1("Could not open a socket. Error: %d", errno);
            }
            else {
                if ( (::ioctl(descriptor, HCIDEVDOWN, deviceId) == 0) || (errno == EALREADY) ) {
                    result = true;
                }
                else {
                    TRACE_L1("Could not bring up the interface [%d]. Error: %d\n", deviceId, errno);
                }
                ::close(descriptor);
            }
            return (result);
        }

        Info Settings() const;
        uint32_t Power(bool enabled);
        uint32_t Bondable(bool enabled);
        uint32_t Connectable(const bool enabled);
        uint32_t FastConnectable(const bool enabled);
        uint32_t Discoverable(const bool enabled);
        uint32_t Advertising(bool enabled);
        uint32_t SimplePairing(bool enabled);
        uint32_t LowEnergy(bool enabled);
        uint32_t SecureLink(bool enabled);
        uint32_t SecureConnection(bool enabled);
        uint32_t DeviceClass(const uint8_t major, const uint8_t minor);
        uint32_t Block(const Address::type type, const Address& address);
        uint32_t Unblock(const Address::type type, const Address& address);
        uint32_t Privacy(const uint8_t mode, const uint8_t identity[16]);
        uint32_t LinkKey(const LinkKeys& keys, const bool debugKeys = false);
        uint32_t LongTermKey(const LongTermKeys& keys);
        uint32_t IdentityKey(const IdentityKeys& keys);
        uint32_t Name(const string& shortName, const string& longName);
        uint32_t PublicAddress(const Address& address);

        uint32_t AddDevice(const Address::type type, const Address& address, const autoconnmode value = REPORT);
        uint32_t RemoveDevice(const Address::type type, const Address& address);
        uint32_t Discovering(const bool on, const bool regular, const bool LowEnergy);
        uint32_t Pair(const Address& remote, const Address::type type, const capabilities cap = NO_INPUT_NO_OUTPUT);
        uint32_t Unpair(const Address& remote, const Address::type type);
        uint32_t PairAbort(const Address& remote, const Address::type type);
        uint32_t UserPINCodeReply(const Address& remote, const Address::type type, const string& pinCode);
        uint32_t UserPasskeyReply(const Address& remote, const Address::type type, const uint32_t passkey);
        uint32_t UserPasskeyConfirmReply(const Address& remote, const Address::type type, const bool confirm);

        uint32_t Notifications(const bool enabled);

    protected:
        virtual void Update(const mgmt_hdr& eventData);

    private:
        virtual uint16_t Deserialize(const uint8_t* dataFrame, const uint16_t availableData) override;

    private:
        uint16_t _deviceId;
        struct hci_filter _filter;
    };


} // namespace Bluetooth

} // namespace WPEFramework
