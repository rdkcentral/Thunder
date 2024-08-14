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

#include "HCISocket.h"

namespace Thunder {

ENUM_CONVERSION_BEGIN(Bluetooth::Address::type)
    { Bluetooth::Address::BREDR_ADDRESS, _TXT(_T("bredr")) },
    { Bluetooth::Address::LE_PUBLIC_ADDRESS, _TXT(_T("le_public")) },
    { Bluetooth::Address::LE_RANDOM_ADDRESS, _TXT(_T("le_random")) },
ENUM_CONVERSION_END(Bluetooth::Address::type)

namespace Bluetooth {

uint32_t HCISocket::Advertising(const bool enable, const uint8_t mode)
{
    uint32_t result = Core::ERROR_ILLEGAL_STATE;

    if (enable == true) {
        if (IsAdvertising() == false) {
            result = Core::ERROR_BAD_REQUEST;
            Command::AdvertisingParametersLE parameters;

            parameters->min_interval = htobs(0x0800);
            parameters->max_interval = htobs(0x0800);
            parameters->advtype = mode;
            parameters->chan_map = 7;

            if (Exchange(MAX_ACTION_TIMEOUT, parameters, parameters) == Core::ERROR_NONE) {
            // if ((temp == Core::ERROR_NONE) && (parameters.Response() == 0)) {

                Command::AdvertisingEnableLE advertising;

                advertising->enable = 1;

                if (Exchange(MAX_ACTION_TIMEOUT, advertising, advertising) == Core::ERROR_NONE) { //  && (advertising.Response() == 0)) {
                    _state.SetState(static_cast<state>(_state.GetState() | ADVERTISING));
                    result = Core::ERROR_NONE;
                }
            }
        }
    } else if (IsAdvertising() == true) {
        result = Core::ERROR_BAD_REQUEST;
        Command::AdvertisingEnableLE advertising;

        advertising->enable = 0;

        if ((Exchange(MAX_ACTION_TIMEOUT, advertising, advertising) == Core::ERROR_NONE) && (advertising.Response() == 0)) {
            _state.SetState(static_cast<state>(_state.GetState() & (~ADVERTISING)));
            result = Core::ERROR_NONE;
        }
    }
    return (result);
}

uint32_t HCISocket::Inquiry(const uint16_t scanTime, const bool limited)
{
    uint32_t result = Core::ERROR_ILLEGAL_STATE;

    _state.Lock();

    if (((_state & ACTION_MASK) == 0) || ((_state & ACTION_MASK) == SCANNING)) {
        /* The inquiry API limits the scan time to 326 seconds, so split it into mutliple inquiries if neccessary. */
        const uint16_t lapTime = 30; /* sec */
        uint16_t timeLeft = scanTime;

        Command::Inquiry inquiry;
        Command::InquiryCancel inquiryCancel;

        inquiry->num_rsp = 255;
        inquiry->length = 30; /* in 1.28s == 35s */

        ASSERT(((uint32_t)inquiry->length * 128 / 100) > lapTime + 1);

        inquiry->lap[0] = (limited == true? 0x00 : 0x33);
        inquiry->lap[1] = 0x8B;
        inquiry->lap[2] = 0x9e;

        while (timeLeft > 0) {
            if (Exchange(MAX_ACTION_TIMEOUT, inquiry, inquiry) == Core::ERROR_NONE) {
                if (inquiry.Response() == 0) {
                    _state.SetState(static_cast<state>(_state.GetState() | INQUIRING));

                    _state.Unlock();

                    // This is not super-precise, but it doesn't have to be.
                    uint16_t roundTime = (timeLeft > lapTime? lapTime : timeLeft);
                    if (_state.WaitState(ABORT_INQUIRING, (roundTime * 1000)) == true) {
                        roundTime = timeLeft; // essentially break
                    }
                    timeLeft -= roundTime;

                    _state.Lock();

                    Exchange(MAX_ACTION_TIMEOUT, inquiryCancel, inquiryCancel);
                    _state.SetState(static_cast<state>(_state.GetState() & (~(ABORT_INQUIRING | INQUIRING))));

                    result = Core::ERROR_NONE;
                } else {
                    TRACE(Trace::Error, (_T("Inquiry command failed [0x%02x]"), inquiry.Response()));
                    result = Core::ERROR_ASYNC_FAILED;
                    break;
                }
            } else {
                result = Core::ERROR_ASYNC_FAILED;
                break;
            }
        }
    } else {
        TRACE_L1("Busy, controller is now inquiring or pairing");
    }

    _state.Unlock();

    return (result);
}

uint32_t HCISocket::AbortInquiry()
{
    uint32_t result = Core::ERROR_ILLEGAL_STATE;

    _state.Lock();

    if ((_state & INQUIRING) != 0) {
        _state.SetState(static_cast<state>(_state.GetState() | ABORT_INQUIRING));
        result = Core::ERROR_NONE;
    }

    _state.Unlock();

    return (result);
}

uint32_t HCISocket::Scan(const uint16_t scanTime, const bool limited, const bool passive)
{
    uint32_t result = Core::ERROR_ILLEGAL_STATE;

    _state.Lock();

    if (((_state & ACTION_MASK) == 0) ||  ((_state & ACTION_MASK) == INQUIRING)) {
        Command::ScanParametersLE parameters;
        const uint16_t window = (limited? 0x12 : 0x10 /* 10ms */);
        // Make sure window is smaller than interval, so the link layer has time for other Bluetooth operations during scanning.
        parameters->type = (passive? SCAN_TYPE_PASSIVE : SCAN_TYPE_ACTIVE);
        parameters->window = htobs(window);
        parameters->interval = htobs(passive? (8 * window) : (4 * window));
        parameters->own_bdaddr_type = LE_PUBLIC_ADDRESS;
        parameters->filter = SCAN_FILTER_POLICY_ALL;

        if (Exchange(MAX_ACTION_TIMEOUT, parameters, parameters) == Core::ERROR_NONE) {
            if (parameters.Response() == 0) {
                Command::ScanEnableLE scanner;
                scanner->enable = 1;
                scanner->filter_dup = SCAN_FILTER_DUPLICATES_ENABLE;

                if (Exchange(MAX_ACTION_TIMEOUT, scanner, scanner) == Core::ERROR_NONE) {
                    if (scanner.Response() == 0) {
                        _state.SetState(static_cast<state>(_state.GetState() | SCANNING));

                        // Now lets wait for the scanning period..
                        _state.Unlock();

                        _state.WaitState(ABORT_SCANNING, scanTime * 1000);

                        _state.Lock();

                        scanner->enable = 0;
                        Exchange(MAX_ACTION_TIMEOUT, scanner, scanner);

                        _state.SetState(static_cast<state>(_state.GetState() & (~(ABORT_SCANNING | SCANNING))));
                        result = Core::ERROR_NONE;
                    } else {
                        TRACE(Trace::Error, (_T("ScanEnableLE command failed [0x%02x]"), scanner.Response()));
                        result = Core::ERROR_ASYNC_FAILED;
                    }
                }
            } else {
                TRACE(Trace::Error, (_T("ScanParametersLE command failed [0x%02x]"), parameters.Response()));
                result = Core::ERROR_ASYNC_FAILED;
            }
        }
    } else {
        TRACE_L1("Busy, controller is now scanning or pairing");
    }

    _state.Unlock();

    return (result);
}

uint32_t HCISocket::AbortScan()
{
    uint32_t result = Core::ERROR_ILLEGAL_STATE;

    _state.Lock();

    if ((_state & SCANNING) != 0) {
        _state.SetState(static_cast<state>(_state.GetState() | ABORT_SCANNING));
        result = Core::ERROR_NONE;
    }

    _state.Unlock();

    return (result);
}

uint32_t HCISocket::Discovery(const bool enable)
{
    uint32_t result = Core::ERROR_ILLEGAL_STATE;

    _state.Lock();

    if ((_state & ACTION_MASK) == 0) {
        if (((enable == true) && (IsDiscovering() == true)) || ((enable == false) && (IsDiscovering() == false))) {
            TRACE_L1("Target LE discovery mode already set...");
        } else {
            if (enable == true) {
                Command::ScanParametersLE parameters;
                const uint16_t window = 0x10; // 10ms
                parameters->type = SCAN_TYPE_PASSIVE;
                // Make sure window is smaller than interval, so the link layer has time for other Bluetooth operations during scanning.
                parameters->interval = htobs(8 * window);
                parameters->window = htobs(window);
                parameters->own_bdaddr_type = LE_PUBLIC_ADDRESS;
                parameters->filter = SCAN_FILTER_POLICY_ALL;

                uint32_t rv = Exchange(MAX_ACTION_TIMEOUT, parameters, parameters);
                if (rv == Core::ERROR_NONE) {
                    if (parameters.Response() == 0) {
                        result = Core::ERROR_NONE;
                    } else  {
                        TRACE(Trace::Error, (_T("ScanParametersLE command failed [0x%02x]"), parameters.Response()));
                        result = Core::ERROR_ASYNC_FAILED;
                    }
                }
            }

            if ((result == Core::ERROR_NONE) || (enable == false)) {
                Command::ScanEnableLE scanner;
                scanner->enable = enable;
                scanner->filter_dup = SCAN_FILTER_DUPLICATES_ENABLE;

                if (Exchange(MAX_ACTION_TIMEOUT, scanner, scanner) == Core::ERROR_NONE) {
                    if (scanner.Response() == 0) {
                        _state.SetState(static_cast<state>(enable? (_state.GetState() | DISCOVERING) : (_state.GetState() & (~DISCOVERING))));
                        result = Core::ERROR_NONE;
                    } else {
                        TRACE(Trace::Error, (_T("ScanEnableLE command failed [0x%02x]"), scanner.Response()));
                        result = Core::ERROR_ASYNC_FAILED;
                    }
                }
            }
        }
    } else {
        TRACE_L1("Busy, controller is now scanning or pairing");
    }

    _state.Unlock();

    return (result);
}

uint32_t HCISocket::ReadStoredLinkKeys(const Address adr, const bool all, LinkKeys& list VARIABLE_IS_NOT_USED)
{
    Command::ReadStoredLinkKey parameters;

    ::memcpy(&(parameters->bdaddr), adr.Data(), sizeof(parameters->bdaddr));
    parameters->read_all = (all ? 0x1 : 0x0);

    return (Exchange(MAX_ACTION_TIMEOUT, parameters, parameters));
}

/* virtual */ void HCISocket::StateChange()
{
    Core::SynchronousChannelType<Core::SocketPort>::StateChange();
    if (IsOpen() == true) {
        BtUtilsHciFilterClear(&_filter);
        BtUtilsHciFilterSetPtype(HCI_EVENT_PKT, &_filter);
        BtUtilsHciFilterSetEvent(EVT_LE_META_EVENT, &_filter);
        BtUtilsHciFilterSetEvent(EVT_CMD_STATUS, &_filter);
        BtUtilsHciFilterSetEvent(EVT_CMD_COMPLETE, &_filter);
        BtUtilsHciFilterSetEvent(EVT_PIN_CODE_REQ, &_filter);
        BtUtilsHciFilterSetEvent(EVT_LINK_KEY_REQ, &_filter);
        BtUtilsHciFilterSetEvent(EVT_LINK_KEY_NOTIFY, &_filter);
        BtUtilsHciFilterSetEvent(EVT_RETURN_LINK_KEYS, &_filter);
        BtUtilsHciFilterSetEvent(EVT_IO_CAPABILITY_REQUEST, &_filter);
        BtUtilsHciFilterSetEvent(EVT_IO_CAPABILITY_RESPONSE, &_filter);
        BtUtilsHciFilterSetEvent(EVT_USER_CONFIRM_REQUEST, &_filter);
        BtUtilsHciFilterSetEvent(EVT_USER_PASSKEY_REQUEST, &_filter);
        BtUtilsHciFilterSetEvent(EVT_REMOTE_OOB_DATA_REQUEST, &_filter);
        BtUtilsHciFilterSetEvent(EVT_USER_PASSKEY_NOTIFY, &_filter);
        BtUtilsHciFilterSetEvent(EVT_KEYPRESS_NOTIFY, &_filter);
        BtUtilsHciFilterSetEvent(EVT_SIMPLE_PAIRING_COMPLETE, &_filter);
        BtUtilsHciFilterSetEvent(EVT_AUTH_COMPLETE, &_filter);
        BtUtilsHciFilterSetEvent(EVT_REMOTE_NAME_REQ_COMPLETE, &_filter);
        BtUtilsHciFilterSetEvent(EVT_READ_REMOTE_VERSION_COMPLETE, &_filter);
        BtUtilsHciFilterSetEvent(EVT_READ_REMOTE_FEATURES_COMPLETE, &_filter);
        BtUtilsHciFilterSetEvent(EVT_REMOTE_HOST_FEATURES_NOTIFY, &_filter);
        BtUtilsHciFilterSetEvent(EVT_INQUIRY_COMPLETE, &_filter);
        BtUtilsHciFilterSetEvent(EVT_INQUIRY_RESULT, &_filter);
        BtUtilsHciFilterSetEvent(EVT_INQUIRY_RESULT_WITH_RSSI, &_filter);
        BtUtilsHciFilterSetEvent(EVT_EXTENDED_INQUIRY_RESULT, &_filter);
        BtUtilsHciFilterSetEvent(EVT_CONN_REQUEST, &_filter);
        BtUtilsHciFilterSetEvent(EVT_CONN_COMPLETE, &_filter);
        BtUtilsHciFilterSetEvent(EVT_DISCONN_COMPLETE, &_filter);

	    if (setsockopt(Handle(), SOL_HCI, HCI_FILTER, &_filter, sizeof(_filter)) < 0) {
            TRACE(Trace::Error, (_T("Can't set HCI filter: %s (%d)"), strerror(errno), errno));
        } else {
            TRACE(Trace::Information, (_T("HCI Filter set!")));
        }
    }
}

template<typename EVENT> void HCISocket::DeserializeScanResponse(const uint8_t* data)
{
    const uint8_t* segment = data;
    uint8_t entries = *segment++;

    for (uint8_t loop = 0; loop < entries; loop++) {
        const EVENT* info = reinterpret_cast<const EVENT*>(segment);
        Update(*info);
        segment += sizeof(EVENT);
    }
}

template<> void HCISocket::DeserializeScanResponse<le_advertising_info>(const uint8_t* data)
{
    const uint8_t* segment = data;
    uint8_t entries = *segment++;

    for (uint8_t loop = 0; loop < entries; loop++) {
        const le_advertising_info* info = reinterpret_cast<const le_advertising_info*>(segment);
        Update(*info);
        segment += (sizeof(le_advertising_info) + info->length);
    }
}

/* virtual */ uint16_t HCISocket::Deserialize(const uint8_t* dataFrame, const uint16_t availableData)
{
    CMD_DUMP("HCI event received", dataFrame, availableData);

    uint16_t result = 0;
    const hci_event_hdr* hdr = reinterpret_cast<const hci_event_hdr*>(&(dataFrame[1]));

    if ( (availableData > sizeof(hci_event_hdr)) && (availableData > (sizeof(hci_event_hdr) + hdr->plen)) ) {
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&(dataFrame[1 + sizeof(hci_event_hdr)]));

        result = 1 + sizeof(hci_event_hdr) + hdr->plen;

        // Deserialize scan response events
        if ((hdr->evt == EVT_LE_META_EVENT) && (reinterpret_cast<const evt_le_meta_event*>(ptr)->subevent == EVT_LE_ADVERTISING_REPORT)) {
            DeserializeScanResponse<le_advertising_info>(reinterpret_cast<const evt_le_meta_event*>(ptr)->data);
        } else if (hdr->evt == EVT_INQUIRY_RESULT) {
            DeserializeScanResponse<inquiry_info>(ptr);
        } else if (hdr->evt == EVT_INQUIRY_RESULT_WITH_RSSI) {
            DeserializeScanResponse<inquiry_info_with_rssi>(ptr);
        } else if (hdr->evt == EVT_EXTENDED_INQUIRY_RESULT) {
            DeserializeScanResponse<extended_inquiry_info>(ptr);
        } else {
            // All other events
            Update(*hdr);
        }
    }
    else {
        TRACE_L1("EVT_HCI: Message too short => (hci_event_hdr)");
    }

    return (result);
}

/* virtual */ void HCISocket::Update(const hci_event_hdr&)
{
}

/* virtual */ void HCISocket::Update(const inquiry_info&)
{
}

/* virtual */ void HCISocket::Update(const inquiry_info_with_rssi&)
{
}

/* virtual */ void HCISocket::Update(const extended_inquiry_info&)
{
}

/* virtual */ void HCISocket::Update(const le_advertising_info&)
{
}

void EIR::Ingest(const uint8_t buffer[], const uint16_t bufferLength)
{
    std::string store;
    uint8_t offset = 0;

    while (((offset + buffer[offset]) <= bufferLength) && (buffer[offset+1] != 0)) {
        const uint8_t length = (buffer[offset] - 1);
        if (length == 0) {
            break;
        }

        const uint8_t type = buffer[offset + 1];
        const uint8_t* const data = &buffer[offset + 2];

        if (type == EIR_NAME_SHORT) {
            _shortName = std::string(reinterpret_cast<const char*>(data), length);
        } else if (type == EIR_NAME_COMPLETE) {
            _completeName = std::string(reinterpret_cast<const char*>(data), length);
        } else if (type == EIR_CLASS_OF_DEV) {
            _class = (data[0] | (data[1] << 8) | (data[2] << 16));
        } else if ((type == EIR_UUID16_SOME) || (type == EIR_UUID16_ALL)) {
            for (uint8_t i = 0; i < (length / 2); i++) {
                _UUIDs.emplace_back(btohs(*reinterpret_cast<const short*>(data + (i * 2))));
            }
        } else if ((type == EIR_UUID128_SOME) || (type == EIR_UUID128_ALL)) {
            for (uint8_t i = 0; i < (length / 16); i++) {
                _UUIDs.emplace_back(data + (i * 16));
            }
        }

        offset += (1 /* length */ + 1 /* type */ + length);
    }
}

// --------------------------------------------------------------------------------------------------
// ManagementSocket !!!
// --------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------
// Create definitions for the Management commands
// ------------------------------------------------------------------------
// https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/mgmt-api.txt
// ------------------------------------------------------------------------

template <const uint16_t OPCODE, typename OUTBOUND, typename INBOUND>
class ManagementType : public Core::IOutbound, public Core::IInbound {
protected:
    ManagementType(uint8_t buffer[]) : _buffer(buffer) {
    }

public:
    ManagementType() = delete;
    ManagementType(const ManagementType<OPCODE, OUTBOUND, INBOUND>&) = delete;
    ManagementType<OPCODE, OUTBOUND, INBOUND>& operator=(const ManagementType<OPCODE, OUTBOUND, INBOUND>&) = delete;
    ManagementType(const uint16_t size, uint8_t buffer[], const uint16_t adapterIndex)
        : _size(size), _buffer(buffer), _offset(_size), _error(~0), _inboundSize(0)
    {
        _buffer[0] = (OPCODE & 0xFF);
        _buffer[1] = (OPCODE >> 8) & 0xFF;
        _buffer[2] = (adapterIndex & 0xFF);
        _buffer[3] = ((adapterIndex >> 8) & 0xFF);
        _buffer[4] = static_cast<uint8_t>((size - sizeof(mgmt_hdr)) & 0xFF);
        _buffer[5] = static_cast<uint8_t>((size >> 8) & 0xFF);
    }
    virtual ~ManagementType()
    {
    }

public:
    void Clear()
    {
        ::memset(&(_buffer[sizeof(mgmt_hdr)]), 0, _size - sizeof(mgmt_hdr));
    }
    OUTBOUND* operator->()
    {
        return (reinterpret_cast<OUTBOUND*>(&(_buffer[sizeof(mgmt_hdr)])));
    }
    uint16_t Result() const
    {
        return (_error);
    }
    const INBOUND& Response()
    {
        return (_inbound);
    }
    uint16_t Loaded () const
    {
        return (_inboundSize);
    }

private:
    virtual void Reload() const override
    {
        _error = ~0;
        _offset = 0;
        _inboundSize = 0;
    }
    virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const override
    {
        uint16_t result = std::min(static_cast<uint16_t>(_size - _offset), length);
        if (result > 0) {

            ::memcpy(stream, &(_buffer[_offset]), result);
            _offset += result;

            CMD_DUMP("MGMT sent", stream, result);
        }
        return (result);
    }
    virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override
    {
        CMD_DUMP("MGMT received", stream, length);

        uint16_t result = 0;
        if (length >= sizeof(mgmt_hdr)) {

            const mgmt_hdr* hdr = reinterpret_cast<const mgmt_hdr*>(stream);
            uint16_t opCode = btohs(hdr->opcode);
            uint16_t payload = btohs(hdr->len);

            if (opCode == MGMT_EV_CMD_STATUS) {
                uint16_t len = (length - sizeof(mgmt_hdr));
                const mgmt_ev_cmd_status* data = reinterpret_cast<const mgmt_ev_cmd_status*>(&(stream[sizeof(mgmt_hdr)]));
                if (btohs(data->opcode) == OPCODE) {
                    if (len < sizeof(mgmt_ev_cmd_status)) {
                       TRACE_L1("MGMT_EV_CMD_STATUS: Message too short; opcode=%04X", data->opcode);
                        _error  = Core::ERROR_GENERAL;
                    } else {
                        TRACE_L2("MGMT_EV_CMD_STATUS: opcode=0x%04X, status=%d", data->opcode, data->status);
                        _error = data->status;
                    }
                    result = length;
                }
            }
            else if (opCode == MGMT_EV_CMD_COMPLETE) {
                const mgmt_ev_cmd_complete* data = reinterpret_cast<const mgmt_ev_cmd_complete*>(&(stream[sizeof(mgmt_hdr)]));
                if (btohs(data->opcode) == OPCODE) {
                    uint16_t len = (length - sizeof(mgmt_hdr));
                    if (len < sizeof(mgmt_ev_cmd_complete)) {
                        TRACE_L1("MGMT_EV_CMD_COMPLETE: Message too short; opcode=%04X", data->opcode);
                        _error = Core::ERROR_GENERAL;
                    } else {
                        _inboundSize = std::min(
                            static_cast<uint16_t>(sizeof(INBOUND)),
                            static_cast<uint16_t>(payload - sizeof(mgmt_ev_cmd_complete)));

                        _inboundSize = std::min(_inboundSize, static_cast<uint16_t>(len - sizeof(mgmt_ev_cmd_complete)));
                        ::memcpy(reinterpret_cast<uint8_t*>(&_inbound), data->data, _inboundSize);

                        TRACE_L2("MGMT_EV_CMD_COMPLETE: opcode=0x%04X, status=%d", data->opcode, data->status);
                        _error = data->status;
                    }
                    result = length;
                }
            }
        }
        return (result);
    }
    virtual state IsCompleted() const override
    {
        return (_error != static_cast<uint16_t>(~0) ? state::COMPLETED : state::INPROGRESS);
    }

private:
    uint16_t _size;
    uint8_t* _buffer;
    mutable uint16_t _offset;
    mutable uint16_t _error;
    mutable uint16_t _inboundSize;
    INBOUND _inbound;
};

template <const uint16_t OPCODE, typename OUTBOUND, typename INBOUND>
class ManagementFixedType : public ManagementType<OPCODE,OUTBOUND,INBOUND> {
public:
    ManagementFixedType() = delete;
    ManagementFixedType(const ManagementFixedType<OPCODE, OUTBOUND, INBOUND>&) = delete;
    ManagementFixedType<OPCODE, OUTBOUND, INBOUND>& operator=(const ManagementFixedType<OPCODE, OUTBOUND, INBOUND>&) = delete;

    ManagementFixedType(const uint16_t adapterIndex)
        : ManagementType<OPCODE,OUTBOUND,INBOUND> (sizeof(_buffer), _buffer, adapterIndex) {
        this->Clear();
    }
    ~ManagementFixedType() {
    }

private:
    uint8_t _buffer[sizeof(mgmt_hdr) + (std::is_same<OUTBOUND, Core::Void>::value ? 0 : sizeof(OUTBOUND))];
};

template <const uint16_t OPCODE, typename OUTBOUND, typename INBOUND, typename LISTTYPE>
class ManagementListType : public ManagementType<OPCODE,OUTBOUND,INBOUND> {
private:
    ManagementListType(const uint16_t size, uint8_t buffer[], const uint16_t adapterIndex)
        : ManagementType<OPCODE,OUTBOUND,INBOUND> (size, buffer, adapterIndex), _buffer(buffer) {
    }
public:
    ManagementListType() = delete;
    ManagementListType(const ManagementListType<OPCODE, OUTBOUND, INBOUND, LISTTYPE>&) = delete;
    ManagementListType<OPCODE, OUTBOUND, INBOUND, LISTTYPE>& operator=(const ManagementListType<OPCODE, OUTBOUND, INBOUND, LISTTYPE>&) = delete;

    ManagementListType(ManagementListType<OPCODE, OUTBOUND, INBOUND, LISTTYPE>&& copy)
        : ManagementType<OPCODE,OUTBOUND,INBOUND> (copy._buffer){
    }

    static ManagementListType<OPCODE, OUTBOUND, INBOUND, LISTTYPE> Instance(const uint16_t adapterIndex, const LISTTYPE& list) {
        uint16_t listLength = (list.Entries() * LISTTYPE::Length());
        uint16_t length = sizeof(mgmt_hdr) + sizeof(OUTBOUND) + listLength;
        uint8_t* buffer = new uint8_t[length];
        ASSERT(buffer != nullptr);

        ManagementListType<OPCODE, OUTBOUND, INBOUND, LISTTYPE> result(length, buffer, adapterIndex);
        result.Clear();
        list.Clone(listLength, &(buffer[sizeof(mgmt_hdr) + sizeof(OUTBOUND)]));
        return (result);
    }

    ~ManagementListType() {
        if (_buffer != nullptr) {
            delete [] _buffer;
        }
        _buffer = nullptr;
    }

private:
    uint8_t* _buffer;
};

/* 500 ms to execute a management command. Should be enough for a kernel message exchange. */
static uint32_t MANAGMENT_TIMEOUT = 500;

static constexpr uint8_t DISABLE_MODE = 0x00;
static constexpr uint8_t ENABLE_MODE  = 0x01;
static constexpr uint8_t LIMITED_MODE  = 0x02;

namespace Management {

    typedef ManagementFixedType<MGMT_OP_READ_INFO, Core::Void, mgmt_rp_read_info> Settings;

    typedef ManagementFixedType<MGMT_OP_SET_POWERED, mgmt_mode, uint32_t> Power;

    typedef ManagementFixedType<MGMT_OP_SET_CONNECTABLE, mgmt_mode, uint32_t> Connectable;
    typedef ManagementFixedType<MGMT_SETTING_FAST_CONNECTABLE, mgmt_mode, uint32_t> FastConnectable;
    typedef ManagementFixedType<MGMT_OP_SET_BONDABLE, mgmt_mode, uint32_t> Bondable;
    typedef ManagementFixedType<MGMT_OP_SET_SSP, mgmt_mode, uint32_t> SimplePairing;
    typedef ManagementFixedType<MGMT_OP_SET_SECURE_CONN, mgmt_mode, uint32_t> SecureConnection;
    typedef ManagementFixedType<MGMT_OP_SET_HS, mgmt_mode, uint32_t> HighSpeed;
    typedef ManagementFixedType<MGMT_OP_SET_LINK_SECURITY, mgmt_mode, uint32_t> SecureLink;
    typedef ManagementFixedType<MGMT_OP_SET_LE, mgmt_mode, uint32_t> LowEnergy;
    typedef ManagementFixedType<MGMT_OP_SET_ADVERTISING, mgmt_mode, uint32_t> Advertising;
    typedef ManagementFixedType<MGMT_OP_SET_CONNECTABLE, mgmt_mode, uint32_t> Connectable;
    typedef ManagementFixedType<MGMT_OP_SET_DISCOVERABLE, mgmt_cp_set_discoverable, uint32_t> Discoverable;
    typedef ManagementFixedType<MGMT_OP_SET_PUBLIC_ADDRESS, mgmt_cp_set_public_address, uint32_t> PublicAddress;

    typedef ManagementFixedType<MGMT_OP_SET_LOCAL_NAME, mgmt_cp_set_local_name, mgmt_cp_set_local_name> DeviceName;
    typedef ManagementFixedType<MGMT_OP_SET_DEV_CLASS, mgmt_cp_set_dev_class, Core::Void> DeviceClass;
    typedef ManagementFixedType<MGMT_OP_ADD_UUID, mgmt_cp_add_uuid, uint8_t[3]> AddUUID;
    typedef ManagementFixedType<MGMT_OP_REMOVE_UUID, mgmt_cp_remove_uuid, uint8_t[3]> RemoveUUID;

    typedef ManagementFixedType<MGMT_OP_ADD_ADVERTISING, mgmt_cp_add_advertising , mgmt_rp_add_advertising> AddAdvertising;
    typedef ManagementFixedType<MGMT_OP_REMOVE_ADVERTISING, mgmt_cp_remove_advertising , mgmt_rp_remove_advertising> RemoveAdvertising;

    // Kernel-side discovery and auto-connection
    typedef ManagementFixedType<MGMT_OP_START_DISCOVERY, mgmt_cp_start_discovery, uint8_t> StartDiscovery;
    typedef ManagementFixedType<MGMT_OP_STOP_DISCOVERY, mgmt_cp_stop_discovery, uint8_t> StopDiscovery;
    typedef ManagementFixedType<MGMT_OP_BLOCK_DEVICE, mgmt_cp_block_device, mgmt_addr_info> Block;
    typedef ManagementFixedType<MGMT_OP_UNBLOCK_DEVICE, mgmt_cp_unblock_device, mgmt_addr_info> Unblock;
    typedef ManagementFixedType<MGMT_OP_ADD_DEVICE, mgmt_cp_add_device, mgmt_rp_add_device> AddDevice;
    typedef ManagementFixedType<MGMT_OP_REMOVE_DEVICE, mgmt_cp_remove_device, mgmt_rp_remove_device> RemoveDevice;

    typedef ManagementFixedType<MGMT_OP_PAIR_DEVICE, mgmt_cp_pair_device, mgmt_rp_pair_device> Pair;
    typedef ManagementFixedType<MGMT_OP_UNPAIR_DEVICE, mgmt_cp_unpair_device, mgmt_rp_unpair_device> Unpair;
    typedef ManagementFixedType<MGMT_OP_CANCEL_PAIR_DEVICE, mgmt_addr_info, Core::Void> PairAbort;

    typedef ManagementFixedType<MGMT_OP_PIN_CODE_REPLY, mgmt_cp_pin_code_reply, Core::Void> UserPINCodeReply;
    typedef ManagementFixedType<MGMT_OP_PIN_CODE_NEG_REPLY, mgmt_cp_pin_code_neg_reply, Core::Void> UserPINCodeNegReply;
    typedef ManagementFixedType<MGMT_OP_USER_CONFIRM_REPLY, mgmt_cp_user_confirm_reply, Core::Void> UserConfirmReply;
    typedef ManagementFixedType<MGMT_OP_USER_CONFIRM_NEG_REPLY, mgmt_cp_user_confirm_reply, Core::Void> UserConfirmNegReply;
    typedef ManagementFixedType<MGMT_OP_USER_PASSKEY_REPLY, mgmt_cp_user_passkey_reply, Core::Void> UserPasskeyReply;
    typedef ManagementFixedType<MGMT_OP_USER_PASSKEY_NEG_REPLY, mgmt_cp_user_passkey_reply, Core::Void> UserPasskeyNegReply;

    typedef ManagementFixedType<MGMT_OP_SET_PRIVACY, mgmt_cp_set_privacy, Core::Void> Privacy;
    typedef ManagementFixedType<MGMT_OP_READ_INDEX_LIST, Core::Void, uint16_t[33]> Indexes;
    typedef ManagementListType<MGMT_OP_LOAD_LINK_KEYS, mgmt_cp_load_link_keys, uint32_t, LinkKeys> LinkKeys;
    typedef ManagementListType<MGMT_OP_LOAD_LONG_TERM_KEYS, mgmt_cp_load_long_term_keys, uint32_t, LongTermKeys> LongTermKeys;
    typedef ManagementListType<MGMT_OP_LOAD_IRKS, mgmt_cp_load_irks, uint32_t, IdentityKeys> IdentityKeys;
}

/* static */ void ManagementSocket::Devices(std::list<uint16_t>& adapters)
{
    Management::Indexes message(HCI_DEV_NONE);
    ManagementSocket globalPort;

    if (globalPort.Exchange(MANAGMENT_TIMEOUT, message, message) == Core::ERROR_NONE) {
        if (message.Result() == Core::ERROR_NONE) {
            for (uint16_t index = 0; index < message.Response()[0]; index++) {
                adapters.push_back(message.Response()[index+1]);
            }
        } else {
            TRACE_GLOBAL(Trace::Error, (_T("ReadIndexList command failed [0x%02x]"), message.Result()));
        }
    }
}

uint32_t ManagementSocket::Name(const string& shortName, const string& longName)
{
    Management::DeviceName message(_deviceId);
    std::string shortName2(shortName.substr(0, sizeof(message->short_name) - 1));
    std::string longName2(longName.substr(0, sizeof(message->name) - 1));

    ::strcpy(reinterpret_cast<char*>(message->short_name), shortName2.c_str());
    ::strcpy(reinterpret_cast<char*>(message->name), longName2.c_str());

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetLocalName command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}


// COD value us composed out of major/minor from this method and OR'd values of
// service bits from each UUID added with AddUUID()
uint32_t ManagementSocket::DeviceClass(const uint8_t major, const uint8_t minor)
{
    Management::DeviceClass message(_deviceId);
    message->major = (major & 0x1F);
    message->minor = (minor << 2);

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetDevClass command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::AddUUID(const UUID& uuid, const uint8_t codServiceBits)
{
    ASSERT(uuid.IsValid() == true);

    Management::AddUUID message(_deviceId);
    ::memcpy(message->uuid, uuid.Full(), sizeof(message->uuid));
    message->svc_hint = codServiceBits;

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("AddUUID command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::RemoveUUID(const UUID& uuid)
{
    ASSERT(uuid.IsValid() == true);

    Management::RemoveUUID message(_deviceId);
    ::memcpy(message->uuid, uuid.Full(), sizeof(message->uuid));

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("RemoveUUID command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::AddAdvertising(uint8_t& instance, const bool limited, const bool connectable, const uint16_t duration)
{
    Management::AddAdvertising message(_deviceId);
    message->instance = 1; // only one advertising at a time
    message->flags = ((limited? 4 : 2) | (connectable? 1 : 0));
    message->duration = 0;
    message->timeout = duration;
    message->adv_data_len = 0;
    message->scan_rsp_len = 0;

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("AddAdvertising command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    } else {
        instance = message->instance;
    }

    return (result);
}

uint32_t ManagementSocket::RemoveAdvertising()
{
    Management::RemoveAdvertising message(_deviceId);
    message->instance = 0;

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("RemoveAdvertising command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::Discoverable(const bool enabled, const bool limited, const uint16_t duration)
{
    ASSERT((enabled == true) || (duration == 0));
    ASSERT((limited == false) || (duration > 0));

    Management::Discoverable message(_deviceId);
    message->val = (enabled ? (limited? LIMITED_MODE : ENABLE_MODE) : DISABLE_MODE);
    message->timeout = htobs(duration);

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetDiscoverable command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::Power(const bool enabled)
{
    Management::Power message(_deviceId);
    message->val = (enabled ? ENABLE_MODE : DISABLE_MODE);

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetPowered command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::Connectable(const bool enabled)
{
    Management::Connectable message(_deviceId);
    message->val = (enabled ? ENABLE_MODE : DISABLE_MODE);

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetConnectable command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::FastConnectable(const bool enabled)
{
    Management::FastConnectable message(_deviceId);
    message->val = (enabled ? ENABLE_MODE : DISABLE_MODE);

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetFastConnectable command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::Block(const Address& address, const Address::type type)
{
    Management::Block message(_deviceId);
    message->addr.type = type;
    ::memcpy(&(message->addr.bdaddr), address.Data(), sizeof(message->addr.bdaddr));

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("BlockDevice command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::Unblock(const Address& address, const Address::type type)
{
    Management::Unblock message(_deviceId);
    message->addr.type = type;
    ::memcpy(&(message->addr.bdaddr), address.Data(), sizeof(message->addr.bdaddr));

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("UnblockDevice command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::AddDevice(const Address& address, const Address::type type, const autoconnmode value)
{
    Management::AddDevice message(_deviceId);
    message->action = value;
    message->addr.type = type;
    ::memcpy(&(message->addr.bdaddr), address.Data(), sizeof(message->addr.bdaddr));

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("AddDevice command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::RemoveDevice(const Address& address, const Address::type type)
{
    Management::RemoveDevice message(_deviceId);
    message->addr.type = type;
    ::memcpy(&(message->addr.bdaddr), address.Data(), sizeof(message->addr.bdaddr));

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("RemoveDevice command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::Privacy(const uint8_t mode, const uint8_t identity[16])
{
    uint32_t result = Core::ERROR_NONE;

    ASSERT((mode == 0) || (identity != nullptr));

    Management::Privacy message(_deviceId);
    message->privacy = mode;
    if ((identity != nullptr) && (mode != 0)) {
        ::memcpy(message->irk, identity, sizeof(message->irk));
    }

    result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetPrivacy command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::Bondable(const bool enabled)
{
    Management::Bondable message(_deviceId);
    message->val = (enabled ? ENABLE_MODE : DISABLE_MODE);

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetBondable command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::Advertising(const bool enabled, const bool connectable)
{
    Management::Advertising message(_deviceId);
    message->val = (enabled ? (connectable? 2 : 1) : DISABLE_MODE);

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetAdvertising command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::SimplePairing(const bool enabled)
{
    Management::SimplePairing message(_deviceId);
    message->val = (enabled ? ENABLE_MODE : DISABLE_MODE);

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetSSP command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::HighSpeed(const bool enabled)
{
    Management::HighSpeed message(_deviceId);
    message->val = (enabled ? ENABLE_MODE : DISABLE_MODE);

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetHS command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::LowEnergy(const bool enabled)
{
    Management::LowEnergy message(_deviceId);
    message->val = (enabled ? ENABLE_MODE : DISABLE_MODE);

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetLE command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::SecureLink(const bool enabled)
{
    Management::SecureLink message(_deviceId);
    message->val = (enabled ? ENABLE_MODE : DISABLE_MODE);

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetSecureLink command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::SecureConnection(const bool enabled)
{
    Management::SecureConnection message(_deviceId);
    message->val = (enabled ? ENABLE_MODE : DISABLE_MODE);

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetSecureConn command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::PublicAddress(const Address& address)
{
    Management::PublicAddress message(_deviceId);
    ::memcpy(&(message->bdaddr), address.Data(), sizeof(message->bdaddr));

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetPublicAddress command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::LinkKey(const LinkKeys& keys, const bool debugKeys)
{
    Management::LinkKeys message = Management::LinkKeys::Instance(_deviceId, keys);
    message->key_count = htobs(keys.Entries());
    message->debug_keys = (debugKeys ? ENABLE_MODE : DISABLE_MODE);

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetLinkKey command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::LongTermKey(const LongTermKeys& keys)
{
    Management::LongTermKeys message = Management::LongTermKeys::Instance(_deviceId, keys);
    message->key_count = htobs(keys.Entries());

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetLongTermKey command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::IdentityKey(const IdentityKeys& keys)
{
    Management::IdentityKeys message = Management::IdentityKeys::Instance(_deviceId, keys);
    message->irk_count = htobs(keys.Entries());

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, message, message);
    if ((result == Core::ERROR_NONE) && (message.Result() != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("SetIdentityKey command failed [0x%02x]"), message.Result()));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::Discovering(const bool on, const bool regular, const bool lowEnergy)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;
    uint32_t commandResult = MGMT_STATUS_FAILED;
    const uint8_t mode = (regular ? 1 : 0) | (lowEnergy ? 6 : 0);

    // If you do not select any type, no use calling this method
    ASSERT (mode != 0);

    if (on == true) {
        Management::StartDiscovery message(_deviceId);
        message->type = mode;
        result = Exchange(MANAGMENT_TIMEOUT, message, message);
        commandResult = message.Result();
    } else {
        Management::StopDiscovery message(_deviceId);
        message->type = mode;
        result = Exchange(MANAGMENT_TIMEOUT, message, message);
        commandResult = message.Result();
    }

    if ((result == Core::ERROR_NONE) && (commandResult != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("StartDiscovery/StopDiscovery command failed [0x%02x]"), commandResult));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

ManagementSocket::Info ManagementSocket::Settings() const
{
    Info result;
    Management::Settings message(_deviceId);

    if (const_cast<ManagementSocket*>(this)->Exchange(MANAGMENT_TIMEOUT, message, message) == Core::ERROR_NONE) {
        result = Info(message.Response());
    }

    return (result);
}

uint32_t ManagementSocket::Pair(const Address& remote, const Address::type type, const capabilities cap)
{
    Management::Pair command(_deviceId);

    command->addr.bdaddr = *remote.Data();
    command->addr.type = type;
    command->io_cap = cap;

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, command, command);
    if (result == Core::ERROR_NONE) {
        switch (command.Result()) {
            case MGMT_STATUS_SUCCESS:
                break;
            case MGMT_STATUS_ALREADY_PAIRED:
                result = Core::ERROR_ALREADY_CONNECTED;
                break;
            default:
                TRACE(Trace::Error, (_T("Pair command failed [0x%02x]"), command.Result()));
                result = Core::ERROR_ASYNC_FAILED;
                break;
        }
    } else if (result == Core::ERROR_TIMEDOUT) {
        // OP_PAIR does not seem to send CMD_STATUS unfortunately...
        result = Core::ERROR_INPROGRESS;
    }

    return (result);
}

uint32_t ManagementSocket::Unpair(const Address& remote, const Address::type type)
{
    Management::Unpair command(_deviceId);

    command->addr.bdaddr = *remote.Data();
    command->addr.type = type;
    command->disconnect = 1;

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, command, command);
    if (result == Core::ERROR_NONE) {
        switch (command.Result()) {
            case MGMT_STATUS_SUCCESS:
                break;
            case MGMT_STATUS_NOT_PAIRED:
                result = Core::ERROR_ALREADY_RELEASED;
                break;
            default:
                TRACE(Trace::Error, (_T("Unpair command failed [0x%02x]"), command.Result()));
                result = Core::ERROR_ASYNC_FAILED;
                break;
        }
    }

    return (result);
}

uint32_t ManagementSocket::PairAbort(const Address& remote, const Address::type type)
{
    Management::PairAbort command(_deviceId);

    command->bdaddr = *remote.Data();
    command->type = type;

    uint32_t result = Exchange(MANAGMENT_TIMEOUT, command, command);
    if (result == Core::ERROR_NONE) {
        switch (command.Result()) {
            case MGMT_STATUS_SUCCESS:
                break;
            case MGMT_STATUS_INVALID_PARAMS:
                // Not currently pairing
                result = Core::ERROR_ILLEGAL_STATE;
                break;
            default:
                TRACE(Trace::Error, (_T("Pairing abort command failed [0x%02x]"), command.Result()));
                result = Core::ERROR_ASYNC_FAILED;
                break;
        }
    } else if (result == Core::ERROR_TIMEDOUT) {
        // Seems we need a bit more time... but it's fine.
        result = Core::ERROR_INPROGRESS;
    }

    return (result);
}

uint32_t ManagementSocket::UserPINCodeReply(const Address& remote, const Address::type type, const string& pinCode)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;
    uint32_t commandResult = MGMT_STATUS_FAILED;

    if (pinCode.empty() == false) {
        Management::UserPINCodeReply command(_deviceId);
        command->addr.bdaddr = *remote.Data();
        command->addr.type = type;
        command->pin_len = std::min(pinCode.length(), sizeof(command->pin_code));
        ::memcpy(command->pin_code, pinCode.c_str(), command->pin_len);
        result = Exchange(MANAGMENT_TIMEOUT, command, command);
        commandResult = command.Result();
    } else {
        Management::UserPINCodeNegReply command(_deviceId);
        command->addr.bdaddr = *remote.Data();
        command->addr.type = type;
        result = Exchange(MANAGMENT_TIMEOUT, command, command);
        commandResult = command.Result();
    }

    if ((result == Core::ERROR_NONE) && (commandResult != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("UserPINCodeReply/UserPINCodeNegReply command failed [0x%02x]"), commandResult));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::UserPasskeyReply(const Address& remote, const Address::type type, const uint32_t passkey)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;
    uint32_t commandResult = MGMT_STATUS_FAILED;

    if (passkey != static_cast<uint32_t>(~0)) {
        Management::UserPasskeyReply command(_deviceId);
        command->addr.bdaddr = *remote.Data();
        command->addr.type = type;
        command->passkey = passkey;
        result = Exchange(MANAGMENT_TIMEOUT, command, command);
        commandResult = command.Result();
    } else {
        Management::UserPasskeyNegReply command(_deviceId);
        command->addr.bdaddr = *remote.Data();
        command->addr.type = type;
        result = Exchange(MANAGMENT_TIMEOUT, command, command);
        commandResult = command.Result();
    }

    if ((result == Core::ERROR_NONE) && (commandResult != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("UserPassReply/UserPassNegReply command failed [0x%02x]"), commandResult));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::UserPasskeyConfirmReply(const Address& remote, const Address::type type, const bool confirm)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;
    uint32_t commandResult = MGMT_STATUS_FAILED;

    if (confirm == true) {
        Management::UserConfirmReply command(_deviceId);
        command->addr.bdaddr = *remote.Data();
        command->addr.type = type;
        result = Exchange(MANAGMENT_TIMEOUT, command, command);
        commandResult = command.Result();
    } else {
        Management::UserConfirmNegReply command(_deviceId);
        command->addr.bdaddr = *remote.Data();
        command->addr.type = type;
        result = Exchange(MANAGMENT_TIMEOUT, command, command);
        commandResult = command.Result();
    }

    if ((result == Core::ERROR_NONE) && (commandResult != MGMT_STATUS_SUCCESS)) {
        TRACE(Trace::Error, (_T("UserConfirmReply/UserConfirmNegReply command failed [0x%02x]"), commandResult));
        result = Core::ERROR_ASYNC_FAILED;
    }

    return (result);
}

uint32_t ManagementSocket::Notifications(const bool enabled)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    if (IsOpen() == true) {
        BtUtilsHciFilterClear(&_filter);

        if (enabled == true) {
            BtUtilsHciFilterSetPtype(HCI_EVENT_PKT, &_filter);
            BtUtilsHciFilterSetEvent(EVT_CMD_STATUS, &_filter);
            BtUtilsHciFilterSetEvent(EVT_CMD_COMPLETE, &_filter);
            BtUtilsHciFilterSetEvent(EVT_PIN_CODE_REQ, &_filter);
            BtUtilsHciFilterSetEvent(EVT_LINK_KEY_REQ, &_filter);
            BtUtilsHciFilterSetEvent(EVT_LINK_KEY_NOTIFY, &_filter);
            BtUtilsHciFilterSetEvent(EVT_RETURN_LINK_KEYS, &_filter);
            BtUtilsHciFilterSetEvent(EVT_IO_CAPABILITY_REQUEST, &_filter);
            BtUtilsHciFilterSetEvent(EVT_IO_CAPABILITY_RESPONSE, &_filter);
            BtUtilsHciFilterSetEvent(EVT_USER_CONFIRM_REQUEST, &_filter);
            BtUtilsHciFilterSetEvent(EVT_USER_PASSKEY_REQUEST, &_filter);
            BtUtilsHciFilterSetEvent(EVT_REMOTE_OOB_DATA_REQUEST, &_filter);
            BtUtilsHciFilterSetEvent(EVT_USER_PASSKEY_NOTIFY, &_filter);
            BtUtilsHciFilterSetEvent(EVT_KEYPRESS_NOTIFY, &_filter);
            BtUtilsHciFilterSetEvent(EVT_SIMPLE_PAIRING_COMPLETE, &_filter);
            BtUtilsHciFilterSetEvent(EVT_AUTH_COMPLETE, &_filter);
            BtUtilsHciFilterSetEvent(EVT_REMOTE_NAME_REQ_COMPLETE, &_filter);
            BtUtilsHciFilterSetEvent(EVT_READ_REMOTE_VERSION_COMPLETE, &_filter);
            BtUtilsHciFilterSetEvent(EVT_READ_REMOTE_FEATURES_COMPLETE, &_filter);
            BtUtilsHciFilterSetEvent(EVT_REMOTE_HOST_FEATURES_NOTIFY, &_filter);
            BtUtilsHciFilterSetEvent(EVT_INQUIRY_COMPLETE, &_filter);
            BtUtilsHciFilterSetEvent(EVT_INQUIRY_RESULT, &_filter);
            BtUtilsHciFilterSetEvent(EVT_INQUIRY_RESULT_WITH_RSSI, &_filter);
            BtUtilsHciFilterSetEvent(EVT_EXTENDED_INQUIRY_RESULT, &_filter);
            BtUtilsHciFilterSetEvent(EVT_CONN_REQUEST, &_filter);
            BtUtilsHciFilterSetEvent(EVT_CONN_COMPLETE, &_filter);
            BtUtilsHciFilterSetEvent(EVT_DISCONN_COMPLETE, &_filter);
        }

        if (setsockopt(Handle(), SOL_HCI, HCI_FILTER, &_filter, sizeof(_filter)) < 0) {
            TRACE(Trace::Error, (_T("Can't set MGMT filter: %s (%d)"), strerror(errno), errno));
            result = Core::ERROR_GENERAL;
        } else {
            TRACE(Trace::Information, (_T("MGMT Filter set!")));
            result = Core::ERROR_NONE;
        }
    }
    return (result);
}

/* virtual */ uint16_t ManagementSocket::Deserialize(const uint8_t* dataFrame, const uint16_t availableData)
{
    CMD_DUMP("MGMT event received", dataFrame, availableData);

    if (availableData >= sizeof(mgmt_hdr)) {
        const mgmt_hdr* hdr = reinterpret_cast<const mgmt_hdr*>(dataFrame);
        Update(*hdr);
    } else {
        TRACE_L1("EVT_MGMT: Message too short => (hci_event_hdr)");
    }

    return (availableData);
}

/* virtual */ void ManagementSocket::Update(const mgmt_hdr&)
{
}

} // namespace Bluetooth

}
