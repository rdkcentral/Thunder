#pragma once

#include "Module.h"

namespace WPEFramework {

namespace Bluetooth {

    class Address {
    public:
        Address()
            : _length(0)
        {
        }
        Address(const int handle)
            : _length(0)
        {
            if (handle > 0) {
                if (hci_devba(handle, &_address) >= 0) {
                    _length = sizeof(_address);
                }
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

        static constexpr uint8_t BREDR_ADDRESS = 0x00;
        static constexpr uint8_t LE_PUBLIC_ADDRESS = 0x01;
        static constexpr uint8_t LE_RANDOM_ADDRESS = 0x02;

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
        bool Default()
        {
            _length = 0;
            int deviceId = hci_get_route(nullptr);
            if ((deviceId >= 0) && (hci_devba(deviceId, &_address) >= 0)) {
                _length = sizeof(_address);
            }
            return (IsValid());
        }
        Address AnyInterface() const
        {
            static bdaddr_t g_anyAddress;
            return (Address(g_anyAddress));
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

    class HCISocket : public Core::SynchronousChannelType<Core::SocketPort> {
    private:
        static constexpr int      SCAN_TIMEOUT = 1000;
        static constexpr uint8_t  SCAN_TYPE = 0x01;
        static constexpr uint8_t  SCAN_FILTER_POLICY = 0x00;
        static constexpr uint8_t  SCAN_FILTER_DUPLICATES = 0x01;
        static constexpr uint8_t  EIR_NAME_SHORT = 0x08;
        static constexpr uint8_t  EIR_NAME_COMPLETE = 0x09;
        static constexpr uint32_t MAX_ACTION_TIMEOUT = 2000; /* 2 Seconds for commands to complete ? */
        static constexpr uint16_t ACTION_MASK = 0x3FFF;

    public:
        enum capabilities {
            DISPLAY_ONLY = 0x00,
            DISPLAY_YES_NO = 0x01,
            KEYBOARD_ONLY = 0x02,
            NO_INPUT_NO_OUTPUT = 0x03,
            KEYBOARD_DISPLAY = 0x04,
            INVALID = 0xFF
        };

        struct IScanning {
            virtual ~IScanning() {}

            virtual void DiscoveredDevice(const bool lowEnergy, const Address&, const string& name) = 0;
        };

    public:
        enum state : uint16_t {
            IDLE        = 0x0000,
            SCANNING    = 0x0001,
            PAIRING     = 0x0002,
            ADVERTISING = 0x4000,
            ABORT       = 0x8000
        };

    public:
        HCISocket&(const HCISocket&) = delete;
        HCISocket& operator=(const HCISocket&) = delete;

        HCISocket()
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::RAW, Core::NodeId(), Core::NodeId(), 256, 256)
            , _state(IDLE)
        {
        }
        HCISocket(const Core::NodeId& sourceNode)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::RAW, sourceNode, Core::NodeId(), 256, 256)
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
        void Scan(IScanning* callback, const uint16_t scanTime, const uint32_t type, const uint8_t flags);
        void Scan(IScanning* callback, const uint16_t scanTime, const bool limited, const bool passive);
        uint32_t Pair(const Address& remote, const uint8_t type = BDADDR_BREDR, const capabilities cap = NO_INPUT_NO_OUTPUT);
        uint32_t Unpair(const Address& remote, const uint8_t type = BDADDR_BREDR);
        void Abort();

    protected:
        virtual void StateChange() override;
        virtual uint16_t HCISocket::Deserialize(const uint8_t* dataFrame, const uint16_t availableData) override;

    private:
        void SetOpcode(const uint16_t opcode);

    private:
        Core::StateTrigger<state> _state;
        IScanning* _callback;
        struct hci_filter _filter;
    };

} // namespace Bluetooth

} // namespace WPEFramework
