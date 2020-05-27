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

#include "GATTSocket.h"
#include "UUID.h"

namespace WPEFramework {

namespace Bluetooth {

uint16_t Attribute::Deserialize(const uint16_t size, const uint8_t stream[])
{
    uint16_t result = 0;

    if (size > 0) {
        if (_offset == 0) {
            _type = stream[0];
            if ((_type & 0x7) <= 4) {
                if (_type == 0) {
                    result = 1;

                    // It's a nil, nothing more required
                    _offset = static_cast<uint32_t>(~0);
                    _type = stream[0];
                } else {
                    uint8_t length = (1 << (_type & 0x07));
                    uint8_t copyLength = std::min(length, static_cast<uint8_t>(size - 1));
                    ::memcpy(_buffer, &stream[1], copyLength);
                    result = 1 + copyLength;
                    _offset = (copyLength == length ? ~0 : copyLength);
                }
            } else {
                uint8_t length = (1 << ((_type & 0x07) - 5));
                uint8_t copyLength = std::min(length, static_cast<uint8_t>(size - 1));
                ::memcpy(_buffer, &stream[1], copyLength);
                result = 1 + copyLength;
                _offset = copyLength;
            }
        }

        if ((result < size) && (IsLoaded() == false)) {
            // See if we need to set the length
            if (((_type & 0x7) > 4) && (_length == static_cast<uint32_t>(~0))) {
                uint8_t length = (1 << ((_type & 0x07) - 5));
                uint8_t copyLength = std::min(static_cast<uint8_t>(length - _offset), static_cast<uint8_t>(size - result));
                if (copyLength > 0) {
                    ::memcpy(&(_buffer[_offset]), &stream[result], copyLength);
                    result += copyLength;
                    _offset += copyLength;
                }

                if (_offset == length) {
                    _length = 0;

                    for (uint8_t index = 0; index < length; index++) {
                        _length = (length << 8) | _buffer[index];
                    }
                    _offset = (_length == 0 ? ~0 : 0);
                }
            }

            if (result < size) {
                // Normal payload loading...
                uint32_t copyLength = std::min(Length() - _offset, static_cast<uint32_t>(size - result));
                // TODO: This one might potentially get very big. Check if the buffer still fits..
                ::memcpy(&(_buffer[_offset]), &stream[result], copyLength);
                result += copyLength;
                _offset = (_offset + copyLength == Length() ? ~0 : _offset + copyLength);
            }
        }
    }
    return (result);
}

/* virtual */ uint16_t GATTSocket::Command::Deserialize(const uint8_t stream[], const uint16_t length)
{
    uint16_t result = 0;

    // See if we need to retrigger..
    if ((stream[0] != _id) && ((stream[0] != ATT_OP_ERROR) && (stream[1] == _id))) {
        TRACE_L1(_T("Unexpected L2CapSocket message. Expected: %d, got %d [%d]"), _id, stream[0], stream[1]);
    } else {
        result = length;

        // TRACE_L1(_T("L2CapSocket Receive [%d], Type: %02X"), length, stream[0]);
        // printf("L2CAP received [%d]: ", length);
        // for (uint8_t index = 0; index < (length - 1); index++) { printf("%02X:", stream[index]); } printf("%02X\n", stream[length - 1]);

        // This is what we are expecting, so process it...
        switch (stream[0]) {
        case ATT_OP_ERROR: {
             if ((stream[4] == ATT_ECODE_ATTR_NOT_FOUND) && (_frame.End() != 0) && (_response.Empty() == false)) {
                 _error = Core::ERROR_NONE;
             }
             else { 
                 _response._min = stream[4];
                 _response.Type(stream[0]);
                 _error = Core::ERROR_GENERAL;
             }
             break;
        }
        case ATT_OP_READ_BY_GROUP_RESP: {
             /* PDU must contain at least:
              * - Attribute length (1 octet)
              * - Attribute Data List (at least one entry):
              *   - Attribute Handle (2 octets)
              *   - End Group Handle (2 octets) 
              *   - Data (Attribute length - 4) */
             /* Minimum Attribute Data List size */
             if (stream[1] < 4) {
                 _error = Core::ERROR_BAD_REQUEST;
             }
             else {
                 uint16_t last = 0;
                 uint8_t entries = (length - 2) / stream[1];
                 for (uint8_t index = 0; index < entries; index++) {
                     uint16_t offset = 2 + (index * stream[1]);
                     uint16_t foundHandle = (stream[offset + 1] << 8) | stream[offset + 0];
                     uint16_t groupHandle = (stream[offset + 3] << 8) | stream[offset + 2];
                     _response.Add(foundHandle, groupHandle, (stream[1] - 4), &(stream[offset+4]));
                     if (last < groupHandle) {
                         last = groupHandle;
                     }
                 }

                 ASSERT(_frame.End() != 0);

                 if (last >= _frame.End()) {
                     _error = Core::ERROR_NONE;
                 }
                 else {
                     _frame.ReadByGroupType(last + 1);
                 }

                 _response.Type(stream[0]);
             }
             break;
        }
        case ATT_OP_FIND_BY_TYPE_RESP: {
             /* PDU must contain at least:
              * - Attribute Opcode (1 octet)
              * - Length (1 octet)
              * - Attribute Data List (at least one entry):
              *   - Attribute Handle (2 octets)
              *   - Attribute Value (at least 1 octet) */
             /* Minimum Attribute Data List size */
             if (stream[1] < 3) {
                 _error = Core::ERROR_BAD_REQUEST;
             }
             else {
                 uint16_t last = 0;
                 uint8_t entries = (length - 1) / 4;
                 for (uint8_t index = 0; index < entries; index++) {
                     uint16_t offset = 2 + (index * stream[1]);
                     uint16_t foundHandle = (stream[offset + 1] << 8) | stream[offset + 0];
                     uint16_t groupHandle = (stream[offset + 3] << 8) | stream[offset + 2];
                     _response.Add(foundHandle, groupHandle);

                     if (last < groupHandle) {
                         last = groupHandle;
                     }
                 }

                 ASSERT(_frame.End() != 0);

                 if (last >= _frame.End()) {
                     _error = Core::ERROR_NONE;
                 }
                 else {
                     _frame.FindByType(last + 1);
                 }

                 _response.Type(stream[0]);
             }
 
             break;
        }
        case ATT_OP_READ_BY_TYPE_RESP: {
             /* PDU must contain at least:
              * - Attribute Opcode (1 octet)
              * - Length (1 octet)
              * - Attribute Data List (at least one entry):
              *   - Attribute Handle (2 octets)
              *   - Attribute Value (at least 1 octet) */
             /* Minimum Attribute Data List size */
             if (stream[1] < 3) {
                 _error = Core::ERROR_BAD_REQUEST;
             }
             else {
                 uint16_t last = 0;
                 uint8_t entries = ((length - 2) / stream[1]);
                 for (uint8_t index = 0; index < entries; index++) {
                     uint16_t offset = 2 + (index * stream[1]);
                     uint16_t handle = (stream[offset + 1] << 8) | stream[offset + 0];

                     _response.Add(handle, stream[1] - 2, &(stream[offset + 2]));
                     if (last < handle) {
                         last = handle;
                     }
                 }

                 ASSERT(_frame.End() != 0);

                 if (last >= _frame.End()) {
                     _error = Core::ERROR_NONE;
                 }
                 else {
                     _frame.ReadByType(last + 1);
                 }

                 _response.Type(stream[0]);
             }
 
             break;
        }
        case ATT_OP_FIND_INFO_RESP: {
             if ((stream[1] != 1) && (stream[1] != 2)) {
                 _error = Core::ERROR_BAD_REQUEST;
             }
             else {
                 uint16_t last = 0;
                 uint8_t step = (stream[1] == 1 ? 2 : 16);
                 uint8_t entries = ((length - 2) / (2 + step));
                 for (uint8_t index = 0; index < entries; index++) {
                     uint16_t offset = 2 + (index * (2 + step));
                     uint16_t handle = (stream[offset + 1] << 8) | stream[offset + 0];

                     _response.Add(handle, step, &(stream[offset + 2]));
                     if (last < handle) {
                         last = handle;
                     }
                 }

                 ASSERT(_frame.End() != 0);

                 if (last >= _frame.End()) {
                     _error = Core::ERROR_NONE;
                 }
                 else {
                     _frame.FindInformation(last + 1);
                 }

                 _response.Type(stream[0]);
             }
 
             break;
        }
        case ATT_OP_WRITE_RESP: {
            // TRACE_L1(_T("We have written: %d"),length);
            _error = Core::ERROR_NONE;
            _response.Type(stream[0]);
            break;
        }
        case ATT_OP_READ_RESP: {
            _response.Add(_frame.Handle(), length - 1, &(stream[1]));
            if (length == _mtu) {
                _id = _frame.ReadBlob(_frame.Handle(), _response.Offset());
            }
            else {
                _error = Core::ERROR_NONE;
            }

            _response.Type(stream[0]);
            break;
        }
        case ATT_OP_READ_BLOB_RESP: {
            if (_response.Offset() == 0) {
                _response.Add(_frame.Handle(), length - 1, &(stream[1]));
            } else {
                _response.Extend(length - 1, &(stream[1]));
            }
            TRACE_L1(_T("Received a blob of length %d"), length);
            if (length == _mtu) {
                _id = _frame.ReadBlob(_frame.Handle(), _response.Offset());
            } else {
                _error = Core::ERROR_NONE;
            }
            _response.Type(ATT_OP_READ_RESP);
            break;
        }
        default:
            break;
        }
    }
    return (result);
}

bool GATTSocket::Security(const uint8_t level)
{
    bool result = true;

    int lm = 0;
    switch (level) {
    case BT_SECURITY_SDP:
        break;
    case BT_SECURITY_LOW:
        lm = L2CAP_LM_AUTH;
        break;
    case BT_SECURITY_MEDIUM:
        lm = L2CAP_LM_AUTH | L2CAP_LM_ENCRYPT;
        break;
    case BT_SECURITY_HIGH:
        lm = L2CAP_LM_AUTH | L2CAP_LM_ENCRYPT | L2CAP_LM_SECURE;
        break;
    default:
        TRACE_L1("Invalid security level");
        result = false;
    }

    if (result == true) {
        struct bt_security btSecurity = { .level = level, 0 };
        if (::setsockopt(Handle(), SOL_BLUETOOTH, BT_SECURITY, &btSecurity, sizeof(btSecurity))) {
            TRACE_L1("Failed to set Bluetooth Security level for device [%s], error: %d", RemoteId().c_str(), errno);
            result = false;
        } else if (::setsockopt(Handle(), SOL_L2CAP, L2CAP_LM, &lm, sizeof(lm)) < 0) {
            TRACE_L1("Error setting L2CAP Security for device [%s], error: %d", RemoteId().c_str(), errno);
            result = false;
        }
    }

    return (result);
}

/* virtual */ void GATTSocket::StateChange() 
{
    Core::SynchronousChannelType<Core::SocketPort>::StateChange();
 
    if (IsOpen() == true) {
        socklen_t len = sizeof(_connectionInfo);
        ::getsockopt(Handle(), SOL_L2CAP, L2CAP_CONNINFO, &_connectionInfo, &len);

        Send(CommunicationTimeOut, _sink, &_sink, &_sink);
    }
}

} // namespace Bluetooth

} // namespace WPEFramework
