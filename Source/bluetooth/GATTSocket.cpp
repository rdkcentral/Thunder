#include "GATTSocket.h"

namespace WPEFramework {

namespace Bluetooth {

uint16_t GATTSocket::Attribute::Deserialize(const uint8_t stream[], const uint16_t size)
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
        TRACE(Trace::Error, (_T("Unexpected L2CapSocket message. Expected: %d, got %d [%d]"), _id, stream[0], stream[1]));
    } else {
        result = length;

        TRACE(Trace::Information, (_T("L2CapSocket Receive [%d], Type: %02X"), length, stream[0]));

        // This is what we are expecting, so process it...
        switch (stream[0]) {
        case ATT_OP_ERROR: {
            TRACE(Trace::Error, (_T("Houston we got an error... %d"), stream[4]));
            _error = stream[4];
            break;
        }
        case ATT_OP_READ_BY_GROUP_RESP: {
            TRACE(Trace::Information, (_T("L2CapSocket Read By Group Type")));
            _error = Core::ERROR_NONE;
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
            if (stream[1] >= 3) {
                uint8_t entries = (length - 1) / 4;
                for (uint8_t index = 0; index < entries; index++) {
                    uint16_t offset = 2 + (index * stream[1]);
                    uint16_t foundHandle = (stream[offset + 1] << 8) | stream[offset + 0];
                    uint16_t groupHandle = (stream[offset + 3] << 8) | stream[offset + 2];
                    _response.Add(foundHandle, groupHandle);
                }
            }
            _error = Core::ERROR_NONE;
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
            if (stream[1] >= 3) {
                uint8_t entries = ((length - 2) / stream[1]);
                for (uint8_t index = 0; index < entries; index++) {
                    uint16_t offset = 2 + (index * stream[1]);
                    uint16_t handle = (stream[offset + 1] << 8) | stream[offset + 0];

                    _response.Add(handle, stream[1] - 2, &(stream[offset + 2]));
                }
            }
            _error = Core::ERROR_NONE;
            break;
        }
        case ATT_OP_WRITE_RESP: {
            TRACE(Trace::Information, (_T("We have written: %d"),length));
            _error = Core::ERROR_NONE;
            break;
        }
        case ATT_OP_FIND_INFO_RESP: {
            _error = Core::ERROR_NONE;
            break;
        }
        case ATT_OP_READ_RESP: {
            _error = Core::ERROR_NONE;
            break;
        }
        case ATT_OP_READ_BLOB_RESP: {
            if (_response.Offset() == 0) {
                _response.Add(_frame.Handle(), length - 1, &(stream[1]));
            } else {
                _response.Extend(length - 1, &(stream[1]));
            }
            TRACE(Trace::Information, (_T("Received a blob of length %d"), length));
            if (length == _mtu) {
                _id = _frame.ReadBlob(_frame.Handle(), _response.Offset());
                TRACE(Trace::Information, (_T("Now we need to send another send....")));
            } else {
                _error = Core::ERROR_NONE;
            }
            break;
        }
        default:
            break;
        }
    }
    return (result);
}

bool GATTSocket::Security(const uint8_t level, const uint8_t keySize)
{
    bool result = true;
    struct bt_security btSecurity;
    memset(&btSecurity, 0, sizeof(btSecurity));
    btSecurity.level = level;
    btSecurity.key_size = keySize;
    if (::setsockopt(Handle(), SOL_BLUETOOTH, BT_SECURITY, &btSecurity, sizeof(btSecurity))) {
        TRACE(Trace::Error, ("Failed to set L2CAP Security level for Device [%s]", RemoteId().c_str()));
        result = false;
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
