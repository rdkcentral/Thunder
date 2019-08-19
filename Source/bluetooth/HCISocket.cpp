#include "HCISocket.h"

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
            Core::IOutbound& OpCode(const uint16_t opCode, const OUTBOUND& value)
            {
                _buffer[0] = (opCode & 0xFF);
                _buffer[1] = ((opCode >> 8) & 0xFF);
                ::memcpy(reinterpret_cast<OUTBOUND*>(&(_buffer[6])), &value, sizeof(OUTBOUND));
                return (*this);
            }

        private:
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

// ------------------------------------------------------------------------
// Create definitions for the Management commands
// ------------------------------------------------------------------------

static constexpr mgmt_mode DISABLE_MODE = { 0x00 };
static constexpr mgmt_mode ENABLE_MODE  = { 0x01 };

struct Management {
    typedef ManagementType<~0, mgmt_mode> OperationalMode;
    typedef ManagementType<MGMT_OP_PAIR_DEVICE, mgmt_cp_pair_device> Pair;
    typedef ManagementType<MGMT_OP_UNPAIR_DEVICE, mgmt_cp_unpair_device> Unpair;
};


uint32_t HCISocket::Config(const bool powered, const bool bondable, const bool advertising, const bool simplePairing, const bool lowEnergy, const bool secure)
{
    uint32_t result = Core::ERROR_GENERAL;

    Management::OperationalMode command(SocketPort::LocalNode().PortNumber());

    ASSERT (IsOpen() == true);

    if (Exchange(500, command.OpCode(MGMT_OP_SET_POWERED, powered ? ENABLE_MODE : DISABLE_MODE)) != Core::ERROR_NONE) {
        TRACE_L1("Failed to power on bluetooth adaptor");
    }
    // Enable/Disable Bondable on adaptor.
    else if (Exchange(500, command.OpCode(MGMT_OP_SET_BONDABLE, bondable ? ENABLE_MODE : DISABLE_MODE)) != Core::ERROR_NONE) {
        TRACE_L1("Failed to enable Bondable");
    }
    // Enable/Disable Simple Secure Simple Pairing.
    else if (Exchange(500, command.OpCode(MGMT_OP_SET_SSP, simplePairing ? ENABLE_MODE : DISABLE_MODE)) != Core::ERROR_NONE) {
        TRACE_L1("Failed to enable Simple Secure Simple Pairing");
    }
    // Enable/Disable Low Energy
    else if (Exchange(500, command.OpCode(MGMT_OP_SET_LE, lowEnergy ? ENABLE_MODE : DISABLE_MODE)) != Core::ERROR_NONE) {
        TRACE_L1("Failed to enable Low Energy");
    }
    // Enable/Disable Secure Connections
    else if (Exchange(500, command.OpCode(MGMT_OP_SET_SECURE_CONN, secure ? ENABLE_MODE : DISABLE_MODE)) != Core::ERROR_NONE) {
        TRACE_L1("Failed to enable Secure Connections");
    }
    // Enable/Disable Advertising
    else if (Exchange(500, command.OpCode(MGMT_OP_SET_ADVERTISING, advertising ? ENABLE_MODE : DISABLE_MODE)) != Core::ERROR_NONE) {
        TRACE_L1("Failed to enable Advertising");
    }
    else {
        result = Core::ERROR_NONE;
    }

    return (result);
}


uint32_t HCISocket::Advertising(const bool enable, const uint8_t mode)
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

void HCISocket::Scan(const uint16_t scanTime, const uint32_t type, const uint8_t flags)
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
                    Discovered(false, newSource, _T("[Unknown]"));
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

void HCISocket::Scan(const uint16_t scanTime, const bool limited, const bool passive)
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
            scanner.Clear();
            scanner->enable = 1;
            scanner->filter_dup = SCAN_FILTER_DUPLICATES;

            if ((Exchange(MAX_ACTION_TIMEOUT, scanner, scanner) == Core::ERROR_NONE) && (scanner.Response() == 0)) {

                _state.SetState(static_cast<state>(_state.GetState() | SCANNING));

                // Now lets wait for the scanning period..
                _state.Unlock();

                _state.WaitState(ABORT, scanTime * 1000);

                _state.Lock();

                scanner->enable = 0;
                Exchange(MAX_ACTION_TIMEOUT, scanner, scanner);

                _state.SetState(static_cast<state>(_state.GetState() & (~(ABORT | SCANNING))));
            }
        }
    }

    _state.Unlock();
}

uint32_t HCISocket::Pair(const Address& remote, const uint8_t type, const capabilities cap)
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

uint32_t HCISocket::Unpair(const Address& remote, const uint8_t type)
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

    if ( (hdr->evt != EVT_LE_META_EVENT) || (reinterpret_cast<const evt_le_meta_event*>(ptr)->subevent != EVT_LE_ADVERTISING_REPORT) ) {
        Update(*hdr);
    }
    else {
        const le_advertising_info* advertisingInfo = reinterpret_cast<const le_advertising_info*>(&(reinterpret_cast<const evt_le_meta_event*>(ptr)->data[1]));
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
            TRACE_L1("Entry[%s] has no name.", Address(advertisingInfo->bdaddr).ToString().c_str());
            Discovered(false, Address(advertisingInfo->bdaddr), _T("[Unknown]"));
        } else {
            Discovered(true, Address(advertisingInfo->bdaddr), string(name, pos));
        }
    }

    return (availableData);
}

/* virtual */ void HCISocket::Update(const hci_event_hdr& eventData)
{
}

/* virtual */ void HCISocket::Discovered(const bool lowEnergy, const Bluetooth::Address& address, const string& name)
{
}
void HCISocket::SetOpcode(const uint16_t opcode)
{
    hci_filter_set_opcode(opcode, &_filter);
    setsockopt(Handle(), SOL_HCI, HCI_FILTER, &_filter, sizeof(_filter));
}

} // namespace Bluetooth

} // namespace WPEFramework
