#pragma once

#include "Module.h"

namespace WPEFramework {

namespace Bluetooth {

        template <const uint16_t OPCODE, typename OUTBOUND>
        class ManagementType : public Core::IOutbound {
        private:
            ManagementType() = delete;
            ManagementType(const ManagementType<OPCODE, OUTBOUND>&) = delete;
            ManagementType<OPCODE, OUTBOUND>& operator=(const ManagementType<OPCODE, OUTBOUND>&) = delete;

        public:
            ManagementType(const uint16_t adapterIndex)
                : _offset(sizeof(_buffer))
            {
                _buffer[0] = (OPCODE & 0xFF);
                _buffer[1] = (OPCODE >> 8) & 0xFF;
                _buffer[2] = (adapterIndex & 0xFF);
                _buffer[3] = ((adapterIndex >> 8) & 0xFF);
                _buffer[4] = static_cast<uint8_t>(sizeof(OUTBOUND) & 0xFF);
                _buffer[5] = static_cast<uint8_t>((sizeof(OUTBOUND) >> 8) & 0xFF);
            }
            virtual ~ManagementType()
            {
            }

        public:
            void Clear()
            {
                ::memset(&(_buffer[6]), 0, sizeof(_buffer) - 6);
            }
            OUTBOUND* operator->()
            {
                return (reinterpret_cast<OUTBOUND*>(&(_buffer[6])));
            }
            Core::IOutbound& OpCode(const uint16_t opCode)
            {
                _buffer[0] = (opCode & 0xFF);
                _buffer[1] = ((opCode >> 8) & 0xFF);
                return (*this);
            }

        private:
            virtual uint16_t Id() const override
            {
                return ((_buffer[0] << 8) | _buffer[1]);
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
                    // for (uint8_t index = 0; index < result; index++) { printf("%02X:", stream[index]); } printf("\n");
                    _offset += result;
                }
                return (result);
            }

        private:
            mutable uint16_t _offset;
            uint8_t _buffer[6 + sizeof(OUTBOUND)];
        };

        template <const uint16_t OPCODE, typename OUTBOUND>
        class CommandType : public Core::IOutbound {
        private:
            CommandType(const CommandType<OPCODE, OUTBOUND>&) = delete;
            CommandType<OPCODE, OUTBOUND>& operator=(const CommandType<OPCODE, OUTBOUND>&) = delete;

        public:
            enum : uint16_t { ID = OPCODE };

        public:
            CommandType()
                : _offset(sizeof(_buffer))
            {
                _buffer[0] = HCI_COMMAND_PKT;
                _buffer[1] = (OPCODE & 0xFF);
                _buffer[2] = ((OPCODE >> 8) & 0xFF);
                _buffer[3] = static_cast<uint8_t>(sizeof(OUTBOUND));
            }
            virtual ~CommandType()
            {
            }

        public:
            OUTBOUND* operator->()
            {
                return (reinterpret_cast<OUTBOUND*>(&(_buffer[4])));
            }
            void Clear()
            {
                ::memset(&(_buffer[4]), 0, sizeof(_buffer) - 4);
            }

        private:
            virtual uint16_t Id() const override
            {
                return (ID);
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
                    for (uint8_t index = 0; index < result; index++) {
                        printf("%02X:", stream[index]);
                    }
                    printf("\n");
                    _offset += result;
                }
                return (result);
            }

        private:
            mutable uint16_t _offset;
            uint8_t _buffer[1 + 3 + sizeof(OUTBOUND)];
        };

        template <const uint16_t OPCODE, typename OUTBOUND, typename INBOUND, const uint16_t RESPONSECODE>
        class ExchangeType : public CommandType<OPCODE, OUTBOUND>, public Core::IInbound {
        private:
            ExchangeType(const ExchangeType<OPCODE, OUTBOUND, INBOUND, RESPONSECODE>&) = delete;
            ExchangeType<OPCODE, OUTBOUND, INBOUND, RESPONSECODE>& operator=(const ExchangeType<OPCODE, OUTBOUND, INBOUND, RESPONSECODE>&) = delete;

        public:
            ExchangeType()
                : CommandType<OPCODE, OUTBOUND>()
                , _error(~0)
            {
            }
            virtual ~ExchangeType()
            {
            }

        public:
            uint32_t Error() const
            {
                return (_error);
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

                    if (hdr->evt == EVT_CMD_STATUS) {
                        const evt_cmd_status* cs = reinterpret_cast<const evt_cmd_status*>(ptr);
                        if (cs->opcode == CommandType<OPCODE, OUTBOUND>::ID) {

                            if (RESPONSECODE == EVT_CMD_STATUS) {
                                _error = (cs->status != 0 ? Core::ERROR_GENERAL : Core::ERROR_NONE);
                            } else if (cs->status != 0) {
                                _error = cs->status;
                            }
                            result = length;
                            TRACE(Trace::Information, (_T(">>EVT_CMD_STATUS: %X-%03X expected: %d"), (cs->opcode >> 10) & 0xF, (cs->opcode & 0x3FF), cs->status ));
                        } else {
                            TRACE(Trace::Information, (_T(">>EVT_CMD_STATUS: %X-%03X unexpected: %d"), (cs->opcode >> 10) & 0xF, (cs->opcode & 0x3FF), cs->status));
                        }
                    } else if (hdr->evt == EVT_CMD_COMPLETE) {
                        const evt_cmd_complete* cc = reinterpret_cast<const evt_cmd_complete*>(ptr);
                        ;
                        if (cc->opcode == CommandType<OPCODE, OUTBOUND>::ID) {
                            if (len <= EVT_CMD_COMPLETE_SIZE) {
                                _error = Core::ERROR_GENERAL;
                            } else {
                                uint16_t toCopy = std::min(static_cast<uint16_t>(sizeof(INBOUND)), static_cast<uint16_t>(len - EVT_CMD_COMPLETE_SIZE));
                                ::memcpy(reinterpret_cast<uint8_t*>(&_response), &(ptr[EVT_CMD_COMPLETE_SIZE]), toCopy);
                                _error = Core::ERROR_NONE;
                            }
                            result = length;
                            TRACE(Trace::Information, (_T(">>EVT_CMD_COMPLETED: %X-%03X expected: %d"), (cc->opcode >> 10) & 0xF, (cc->opcode & 0x3FF), _error));
                        } else {
                            TRACE(Trace::Information, (_T(">>EVT_CMD_COMPLETED: %X-%03X unexpected: %d"), (cc->opcode >> 10) & 0xF, (cc->opcode & 0x3FF), _error));
                        }
                    } else if ((((CommandType<OPCODE, OUTBOUND>::ID >> 10) & 0x3F) == OGF_LE_CTL) && (hdr->evt == EVT_LE_META_EVENT)) {
                        const evt_le_meta_event* eventMetaData = reinterpret_cast<const evt_le_meta_event*>(ptr);

                        if (eventMetaData->subevent == RESPONSECODE) {
                            TRACE(Trace::Information, (_T(">>EVT_COMPLETE: expected")));

                            uint16_t toCopy = std::min(static_cast<uint16_t>(sizeof(INBOUND)), static_cast<uint16_t>(len - EVT_LE_META_EVENT_SIZE));
                            ::memcpy(reinterpret_cast<uint8_t*>(&_response), &(ptr[EVT_LE_META_EVENT_SIZE]), toCopy);

                            _error = Core::ERROR_NONE;
                            result = length;
                        } else {
                            TRACE(Trace::Information, (_T(">>EVT_COMPLETE: unexpected [%d]"), eventMetaData->subevent));
                        }
                    }
                }
                return (result);
            }

        private:
            INBOUND _response;
            uint16_t _error;
        };

        static constexpr uint32_t MAX_ACTION_TIMEOUT = 2000; /* 2 Seconds for commands to complete ? */
        static constexpr uint16_t ACTION_MASK = 0x3FFF;

    public:
        enum state : uint16_t {
            IDLE = 0x0000,
            SCANNING = 0x0001,
            PAIRING = 0x0002,
            ADVERTISING = 0x4000,
            ABORT = 0x8000
        };

// ------------------------------------------------------------------------
// Create definitions for the HCI commands
// ------------------------------------------------------------------------
struct Command {
    typedef ExchangeType<cmd_opcode_pack(OGF_LINK_CTL, OCF_CREATE_CONN), create_conn_cp, evt_conn_complete, EVT_CONN_COMPLETE>
                Connect;

    typedef ExchangeType<cmd_opcode_pack(OGF_LINK_CTL, OCF_AUTH_REQUESTED), auth_requested_cp, evt_auth_complete, EVT_AUTH_COMPLETE>
                Authenticate;

    typedef ExchangeType<cmd_opcode_pack(OGF_LINK_CTL, OCF_DISCONNECT), disconnect_cp, evt_disconn_complete, EVT_DISCONN_COMPLETE>
                Disconnect;

    typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_CREATE_CONN), le_create_connection_cp, evt_le_connection_complete, EVT_LE_CONN_COMPLETE>
                ConnectLE;

    typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_START_ENCRYPTION), le_start_encryption_cp, uint8_t, EVT_LE_CONN_COMPLETE>
                EncryptLE;

    typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL, OCF_REMOTE_NAME_REQ), remote_name_req_cp, evt_remote_name_req_complete, EVT_REMOTE_NAME_REQ_COMPLETE>
                RemoteName;

    typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_SCAN_PARAMETERS), le_set_scan_parameters_cp, uint8_t, EVT_CMD_COMPLETE>
                ScanParametersLE;

    typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE), le_set_scan_enable_cp, uint8_t, EVT_CMD_COMPLETE>
                ScanEnableLE;

    typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_CLEAR_WHITE_LIST), Core::Void, Core::Void, EVT_CMD_STATUS>
                ClearWhiteList;

    typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_READ_WHITE_LIST_SIZE), Core::Void, le_read_white_list_size_rp, EVT_CMD_STATUS>
                ReadWhiteListSize;

    typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_ADD_DEVICE_TO_WHITE_LIST), le_add_device_to_white_list_cp, Core::Void, EVT_CMD_STATUS>
                AddDeviceToWhiteList;

    typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_REMOVE_DEVICE_FROM_WHITE_LIST), le_remove_device_from_white_list_cp, uint8_t, EVT_CMD_STATUS>
                RemoveDeviceFromWhiteList;

    typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_READ_REMOTE_USED_FEATURES), le_read_remote_used_features_cp, evt_le_read_remote_used_features_complete, EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE>
                RemoteFeaturesLE;

    typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_ADVERTISING_PARAMETERS), le_set_advertising_parameters_cp, uint8_t, EVT_CMD_COMPLETE>
                AdvertisingParametersLE;

    typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_ADVERTISE_ENABLE), le_set_advertise_enable_cp, uint8_t, EVT_CMD_COMPLETE>
                AdvertisingEnableLE;
};

// ------------------------------------------------------------------------
// Create definitions for the Management commands
// ------------------------------------------------------------------------
struct Management {
    typedef ManagementType<~0, mgmt_mode> OperationalMode;
    typedef ManagementType<MGMT_OP_PAIR_DEVICE, mgmt_cp_pair_device> Pair;
    typedef ManagementType<MGMT_OP_UNPAIR_DEVICE, mgmt_cp_unpair_device> Unpair;
};

uint32_t HCISocket::Advertising(const bool enable, const uint8_t mode = 0)
{
    uint32_t result = Core::ERROR_ILLEGAL_STATE;

    if (enable == true) {
        if (IsAdvertising() == false) {
            result = Core::ERROR_BAD_REQUEST;
            Command::AdvertisingParametersLE parameters;

            parameters.Clear();
            parameters->min_interval = htobs(0x0800);
            parameters->max_interval = htobs(0x0800);
            parameters->advtype = mode;
            parameters->chan_map = 7;

            if ((Exchange(MAX_ACTION_TIMEOUT, parameters, parameters) == Core::ERROR_NONE) && (parameters.Response() == 0)) {

                Command::AdvertisingEnableLE advertising;

                advertising.Clear();
                advertising->enable = 1;

                if ((Exchange(MAX_ACTION_TIMEOUT, advertising, advertising) == Core::ERROR_NONE) && (advertising.Response() == 0)) {
                    _state.SetState(static_cast<state>(_state.GetState() | ADVERTISING));
                    result = Core::ERROR_NONE;
                }
            }
        }
    } else if (IsAdvertising() == true) {
        result = Core::ERROR_BAD_REQUEST;
        Command::AdvertisingEnableLE advertising;

        advertising.Clear();
        advertising->enable = 0;

        if ((Exchange(MAX_ACTION_TIMEOUT, advertising, advertising) == Core::ERROR_NONE) && (advertising.Response() == 0)) {
            _state.SetState(static_cast<state>(_state.GetState() & (~ADVERTISING)));
            result = Core::ERROR_NONE;
        }
    }
    return (result);
}

void HCISocket::Scan(IScanning* callback, const uint16_t scanTime, const uint32_t type, const uint8_t flags)
{
    ASSERT(scanTime <= 326);

    _state.Lock();

    if ((_state & ACTION_MASK) == 0) {
        int descriptor = Handle();

        _state.SetState(static_cast<state>(_state.GetState() | SCANNING));

        _state.Unlock();

        ASSERT(descriptor >= 0);

        void* buf = ALLOCA(sizeof(struct hci_inquiry_req) + (sizeof(inquiry_info) * 128));
        struct hci_inquiry_req* ir = reinterpret_cast<struct hci_inquiry_req*>(buf);
        std::list<Address> reported;

        ir->dev_id = hci_get_route(nullptr);
        ir->num_rsp = 128;
        ir->length = (((scanTime * 100) + 50) / 128);
        ir->flags = flags | IREQ_CACHE_FLUSH;
        ir->lap[0] = (type >> 16) & 0xFF; // 0x33;
        ir->lap[1] = (type >> 8) & 0xFF; // 0x8b;
        ir->lap[2] = type & 0xFF; // 0x9e;
        // Core::Time endTime = Core::Time::Now().Add(scanTime * 1000);

        // while ((ir->length != 0) && (ioctl(descriptor, HCIINQUIRY, reinterpret_cast<unsigned long>(buf)) >= 0)) {
            if (ioctl(descriptor, HCIINQUIRY, reinterpret_cast<unsigned long>(buf)) >= 0) {

            for (uint8_t index = 0; index < (ir->num_rsp); index++) {
                inquiry_info* info = reinterpret_cast<inquiry_info*>(&(reinterpret_cast<uint8_t*>(buf)[sizeof(hci_inquiry_req)]));

                bdaddr_t* address = &(info[index].bdaddr);
                Address newSource(*address);

                std::list<Address>::const_iterator finder(std::find(reported.begin(), reported.end(), newSource));

                if (finder == reported.end()) {
                    reported.push_back(newSource);
                    callback->DiscoveredDevice(false, newSource, _T("[Unknown]"));
                }
            }

            // Reset go for the next round !!
            // ir->length  = (endTime <= Core::Time::Now() ? 0 : 1);
            // ir->num_rsp = 128;
            // ir->flags  &= ~IREQ_CACHE_FLUSH;
        }

        _state.Lock();

        _state.SetState(static_cast<state>(_state.GetState() & (~(ABORT | SCANNING))));
    }

    _state.Unlock();
}

void HCISocket::Scan(IScanning* callback, const uint16_t scanTime, const bool limited, const bool passive)
{
    _state.Lock();

    if ((_state & ACTION_MASK) == 0) {
        Command::ScanParametersLE parameters;

        parameters.Clear();
        parameters->type = (passive ? 0x00 : 0x01);
        parameters->interval = htobs((limited ? 0x12 : 0x10));
        parameters->window = htobs((limited ? 0x12 : 0x10));
        parameters->own_bdaddr_type = LE_PUBLIC_ADDRESS;
        parameters->filter = SCAN_FILTER_POLICY;

        if ((Exchange(MAX_ACTION_TIMEOUT, parameters, parameters) == Core::ERROR_NONE) && (parameters.Response() == 0)) {

            Command::ScanEnableLE scanner;
            _callback = callback;

            scanner.Clear();
            scanner->enable = 1;
            scanner->filter_dup = SCAN_FILTER_DUPLICATES;

            if ((Exchange(MAX_ACTION_TIMEOUT, scanner, scanner) == Core::ERROR_NONE) && (scanner.Response() == 0)) {

                _state.SetState(static_cast<state>(_state.GetState() | SCANNING));

                // Now lets wait for the scanning period..
                _state.Unlock();

                _state.WaitState(ABORT, scanTime * 1000);

                _state.Lock();

                _callback = nullptr;

                scanner->enable = 0;
                Exchange(MAX_ACTION_TIMEOUT, scanner, scanner);

                _state.SetState(static_cast<state>(_state.GetState() & (~(ABORT | SCANNING))));
            }
        }
    }

    _state.Unlock();
}

uint32_t HCISocket::Pair(const Address& remote, const uint8_t type = BDADDR_BREDR, const capabilities cap = NO_INPUT_NO_OUTPUT)
{
    uint32_t result = Core::ERROR_INPROGRESS;

    _state.Lock();

    if ((_state & ACTION_MASK) == 0) {

        _state.SetState(static_cast<state>(_state.GetState() | PAIRING));

        _state.Unlock();

        Management::Pair command(0);

        command.Clear();
        command->addr.bdaddr = *remote.Data();
        command->addr.type = type;
        command->io_cap = cap;

        result = Core::ERROR_NONE;

        result = Exchange(MAX_ACTION_TIMEOUT, command);

        _state.Lock();

        _state.SetState(static_cast<state>(_state.GetState() & (~(ABORT | PAIRING))));
    }

    _state.Unlock();

    return (result);
}

uint32_t HCISocket::Unpair(const Address& remote, const uint8_t type = BDADDR_BREDR)
{
    uint32_t result = Core::ERROR_INPROGRESS;

    _state.Lock();

    if ((_state & ACTION_MASK) == 0) {

        _state.SetState(static_cast<state>(_state.GetState() | PAIRING));

        _state.Unlock();

        Management::Unpair command(0);
        command.Clear();
        command->addr.bdaddr = *remote.Data();
        command->addr.type = type;
        command->disconnect = 1;

        result = Exchange(MAX_ACTION_TIMEOUT, command);

        _state.SetState(static_cast<state>(_state.GetState() & (~(ABORT | PAIRING))));
    }

    _state.Unlock();

    return (result);
}

void HCISocket::Abort()
{
    _state.Lock();
            
    if ((_state & ACTION_MASK) != 0) {
        // TODO: Find if we can actually abort a IOCTL:HCIINQUIRY !!
        _state.SetState(static_cast<state>(_state.GetState() | ABORT));
    }

    _state.Unlock();
}

/* virtual */ void HCISocket::StateChange() 
{
    Core::SynchronousChannelType<Core::SocketPort>::StateChange();
    if (IsOpen() == true) {
        hci_filter_clear(&_filter);
        hci_filter_set_ptype(HCI_EVENT_PKT, &_filter);
        hci_filter_set_event(EVT_CMD_STATUS, &_filter);
        hci_filter_set_event(EVT_CMD_COMPLETE, &_filter);
        hci_filter_set_event(EVT_LE_META_EVENT, &_filter);

        // Interesting why this needs to be set.... I hope not!!
        // hci_filter_set_opcode(0, &_filter);

        setsockopt(Handle(), SOL_HCI, HCI_FILTER, &_filter, sizeof(_filter));
    }
}

/* virtual */ uint16_t HCISocket::Deserialize(const uint8_t* dataFrame, const uint16_t availableData)
{
    const hci_event_hdr* hdr = reinterpret_cast<const hci_event_hdr*>(&(dataFrame[1]));
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&(dataFrame[1 + HCI_EVENT_HDR_SIZE]));

    switch (hdr->evt) {
        case EVT_CMD_STATUS: {
             const evt_cmd_status* cs = reinterpret_cast<const evt_cmd_status*>(ptr);
             TRACE(Trace::Information, (_T("==EVT_CMD_STATUS: %X-%03X status: %d"), (((cs->opcode >> 10) & 0xF), (cs->opcode & 0x3FF), cs->status)));
             break;
        }
        case EVT_CMD_COMPLETE: {
             const evt_cmd_complete* cc = reinterpret_cast<const evt_cmd_complete*>(ptr);
             TRACE(Trace::Information, (_T("==EVT_CMD_COMPLETE: %X-%03X"), (((cc->opcode >> 10) & 0xF), (cc->opcode & 0x3FF))));
             break;
        }
        case EVT_LE_META_EVENT: {
             const evt_le_meta_event* eventMetaData = reinterpret_cast<const evt_le_meta_event*>(ptr);

             if (eventMetaData->subevent == EVT_LE_CONN_COMPLETE) {
                 TRACE(Trace::Information, (_T("==EVT_LE_CONN_COMPLETE: unexpected")));
             } else if (eventMetaData->subevent == EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE) {
                 TRACE(Trace::Information, (_T("==EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE: unexpected")));
             } else if (eventMetaData->subevent == EVT_DISCONNECT_PHYSICAL_LINK_COMPLETE) {
                 TRACE(Trace::Information, (_T("==EVT_DISCONNECT_PHYSICAL_LINK_COMPLETE: unexpected")));
             } else if (eventMetaData->subevent == EVT_LE_ADVERTISING_REPORT) {
                 string shortName;
                 string longName;
                 const le_advertising_info* advertisingInfo = reinterpret_cast<const le_advertising_info*>(&(eventMetaData->data[1]));
                 uint16_t offset = 0;
                 const uint8_t* buffer = advertisingInfo->data;
                 const char* name = nullptr;
                 uint8_t pos = 0;

                 while (((offset + buffer[offset]) <= advertisingInfo->length) && (buffer[offset] != 0)) {

                     if (((buffer[offset + 1] == EIR_NAME_SHORT) && (name == nullptr)) || (buffer[offset + 1] == EIR_NAME_COMPLETE)) {
                         name = reinterpret_cast<const char*>(&(buffer[offset + 2]));
                         pos = buffer[offset] - 1;
                     }
                     offset += (buffer[offset] + 1);
                 }

                 if ((name == nullptr) || (pos == 0)) {
                     TRACE_L1("Entry[%s] has no name. Do not report it.", Address(advertisingInfo->bdaddr).ToString());
                 } else {
                     _state.Lock();
                     if (_callback != nullptr) {
                         _callback->DiscoveredDevice(true, Address(advertisingInfo->bdaddr), string(name, pos));
                     }
                     _state.Unlock();
                 }
             } else {
                 TRACE(Trace::Information, (_T("==EVT_LE_META_EVENT: unexpected subevent: %d"), eventMetaData->subevent));
             }
             break;
        }
        case 0:
            break;
        default:
            TRACE(Trace::Information, (_T("==UNKNOWN: event %X"), hdr->evt));
            break;
    }

    return (availableData);
}

void HCISocket::SetOpcode(const uint16_t opcode)
{
    hci_filter_set_opcode(opcode, &_filter);
    setsockopt(Handle(), SOL_HCI, HCI_FILTER, &_filter, sizeof(_filter));
}

} // namespace Bluetooth

} // namespace WPEFramework
