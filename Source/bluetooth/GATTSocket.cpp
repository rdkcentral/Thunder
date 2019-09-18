#include "GATTSocket.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Bluetooth::Profile::Service::type)

    { Bluetooth::Profile::Service::GenericAccess,                  _TXT("Generic Access")                },
    { Bluetooth::Profile::Service::AlertNotificationService,       _TXT("Alert Notification Service")    },
    { Bluetooth::Profile::Service::AutomationIO,                   _TXT("Automation IO")                 },
    { Bluetooth::Profile::Service::BatteryService,                 _TXT("Battery Service")               },
    { Bluetooth::Profile::Service::BinarySensor,                   _TXT("Binary Sensor")                 },
    { Bluetooth::Profile::Service::BloodPressure,                  _TXT("Blood Pressure")                },
    { Bluetooth::Profile::Service::BodyComposition,                _TXT("Body Composition")              },
    { Bluetooth::Profile::Service::BondManagementService,          _TXT("Bond Management Service")       },
    { Bluetooth::Profile::Service::ContinuousGlucoseMonitoring,    _TXT("Continuous Glucose Monitoring") },
    { Bluetooth::Profile::Service::CurrentTimeService,             _TXT("Current Time Service")          },
    { Bluetooth::Profile::Service::CyclingPower,                   _TXT("Cycling Power")                 },
    { Bluetooth::Profile::Service::CyclingSpeedAndCadence,         _TXT("Cycling Speed and Cadence")     },
    { Bluetooth::Profile::Service::DeviceInformation,              _TXT("Device Information")            },
    { Bluetooth::Profile::Service::EmergencyConfiguration,         _TXT("Emergency Configuration")       },
    { Bluetooth::Profile::Service::EnvironmentalSensing,           _TXT("Environmental Sensing")         },
    { Bluetooth::Profile::Service::FitnessMachine,                 _TXT("Fitness Machine")               },
    { Bluetooth::Profile::Service::GenericAttribute,               _TXT("Generic Attribute")             },
    { Bluetooth::Profile::Service::Glucose,                        _TXT("Glucose")                       },
    { Bluetooth::Profile::Service::HealthThermometer,              _TXT("Health Thermometer")            },
    { Bluetooth::Profile::Service::HeartRate,                      _TXT("Heart Rate")                    },
    { Bluetooth::Profile::Service::HTTPProxy,                      _TXT("HTTP Proxy")                    },
    { Bluetooth::Profile::Service::HumanInterfaceDevice,           _TXT("Human Interface Device")        },
    { Bluetooth::Profile::Service::ImmediateAlert,                 _TXT("Immediate Alert")               },
    { Bluetooth::Profile::Service::IndoorPositioning,              _TXT("Indoor Positioning")            },
    { Bluetooth::Profile::Service::InsulinDelivery,                _TXT("Insulin Delivery")              },
    { Bluetooth::Profile::Service::InternetProtocolSupport,        _TXT("Internet Protocol Support")     },
    { Bluetooth::Profile::Service::LinkLoss,                       _TXT("Link Loss")                     },
    { Bluetooth::Profile::Service::LocationAndNavigation,          _TXT("Location and Navigation")       },
    { Bluetooth::Profile::Service::MeshProvisioningService,        _TXT("Mesh Provisioning Service")     },
    { Bluetooth::Profile::Service::MeshProxyService,               _TXT("Mesh Proxy Service")            },
    { Bluetooth::Profile::Service::NextDSTChangeService,           _TXT("Next DST Change Service")       },
    { Bluetooth::Profile::Service::ObjectTransferService,          _TXT("ObjectTransferService")         },
    { Bluetooth::Profile::Service::PhoneAlertStatusService,        _TXT("Phone Alert Status Service")    },
    { Bluetooth::Profile::Service::PulseOximeterService,           _TXT("Pulse Oximeter Service")        },
    { Bluetooth::Profile::Service::ReconnectionConfiguration,      _TXT("Reconnection Configuration")    },
    { Bluetooth::Profile::Service::ReferenceTimeUpdateService,     _TXT("Reference Time Update Service") },
    { Bluetooth::Profile::Service::RunningSpeedAndCadence,         _TXT("Running Speed and Cadence")     },
    { Bluetooth::Profile::Service::ScanParameters,                 _TXT("Scan Parameters")               },
    { Bluetooth::Profile::Service::TransportDiscovery,             _TXT("Transport Discovery")           },
    { Bluetooth::Profile::Service::TxPower,                        _TXT("Tx Power")                      },
    { Bluetooth::Profile::Service::UserData,                       _TXT("User Data")                     },
    { Bluetooth::Profile::Service::WeightScale,                    _TXT("Weight Scale")                  },

ENUM_CONVERSION_END(Bluetooth::Profile::Service::type)

namespace Bluetooth {

/* static */ const uint8_t UUID::BASE[] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

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

        TRACE_L1(_T("L2CapSocket Receive [%d], Type: %02X"), length, stream[0]);
        printf("L2CAP received [%d]: ", length);
        for (uint8_t index = 0; index < (length - 1); index++) { printf("%02X:", stream[index]); } printf("%02X\n", stream[length - 1]);

        // This is what we are expecting, so process it...
        switch (stream[0]) {
        case ATT_OP_ERROR: {
            TRACE_L1(_T("Houston we got an error... %d"), stream[4]);
            _error = stream[4];
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
            TRACE_L1(_T("L2CapSocket Read By Group Type"));
             if (stream[1] >= 4) {
                uint8_t entries = (length - 2) / stream[1];
                for (uint8_t index = 0; index < entries; index++) {
                    uint16_t offset = 2 + (index * stream[1]);
                    uint16_t foundHandle = (stream[offset + 1] << 8) | stream[offset + 0];
                    uint16_t groupHandle = (stream[offset + 3] << 8) | stream[offset + 2];
                    _response.Add(foundHandle, groupHandle, (stream[1] - 4), &(stream[offset+4]));
                }
            }
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
            TRACE_L1(_T("We have written: %d"),length);
            _error = Core::ERROR_NONE;
            break;
        }
        case ATT_OP_FIND_INFO_RESP: {
            _error = Core::ERROR_NONE;
            break;
        }
        case ATT_OP_READ_RESP: {
            _error = Core::ERROR_NONE;
            _response.Add(0, length, &(stream[1]));
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
                TRACE_L1(_T("Now we need to send another send...."));
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
        TRACE_L1("Failed to set L2CAP Security level for Device [%s], error: %d", RemoteId().c_str(), errno);
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
