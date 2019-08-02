#define _TRACE_LEVEL 2

#include "SerialDriver.h"

namespace WPEFramework {

namespace Bluetooth {

    class Broadcom43XX : public SerialDriver {
    private:
        Broadcom43XX() = delete;
        Broadcom43XX(const Broadcom43XX&) = delete;
        Broadcom43XX& operator=(const Broadcom43XX&) = delete;

        static constexpr uint8_t BCM43XX_CLOCK_48 = 1;
        static constexpr uint8_t BCM43XX_CLOCK_24 = 2;
        static constexpr uint8_t CMD_SUCCESS = 0;

    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , Port(_T("/dev/ttyAMA0"))
                , Firmware(_T("/etc/firmware/"))
                , SetupRate(115200)
                , BaudRate(921600)
                , MACAddress()
                , Break(false)
            {
                Add(_T("port"), &Port);
                Add(_T("firmware"), &Firmware);
                Add(_T("baudrate"), &BaudRate);
                Add(_T("setup"), &SetupRate);
                Add(_T("address"), &MACAddress);
                Add(_T("break"), &Break);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Port;
            Core::JSON::String Firmware;
            Core::JSON::DecUInt32 SetupRate;
            Core::JSON::DecUInt32 BaudRate;
            Core::JSON::String MACAddress;
            Core::JSON::Boolean Break;
        };

    public:
        Broadcom43XX(const Config& config)
            : SerialDriver(config.Port.Value(), config.SetupRate.Value(), Core::SerialPort::OFF, config.Break.Value())
            , _directory(config.Firmware.Value())
            , _name()
            , _MACLength(0)
            , _setupRate(config.SetupRate.Value())
            , _baudRate(config.BaudRate.Value())
        {
        }
        virtual ~Broadcom43XX()
        {
        }

    public:
        const char* Initialize()
        {
            const char* result = nullptr;
            
            if (Reset() != Core::ERROR_NONE) {
                ::SleepMs(500);
                SetBaudRate(_baudRate);
                result = "Initial reset failed!!!";
            }
 
            if ((result != nullptr) && (Reset() != Core::ERROR_NONE)) {
                result = "Could not reset the chip to a defined state";
            }
            else if (LoadName() != Core::ERROR_NONE) {
                result = "Could not load the drivers name.";
            }
            else if (SetSpeed(_baudRate) != Core::ERROR_NONE) {
                result = "Could not set the BaudRate (first time)";
            }
            else {
                uint16_t index = 0;
                while (isalnum(_name[index++])) /* INTENTIONALLY LEFT EMPTY */ ;
                uint32_t loaded = Firmware(_directory, _name.substr(0, index - 1));

                result = nullptr;

                // It has been observed that once the firmware is loaded the name of
                // the device changes from BCM43430A1 to BCM43438A1, this is due to
                // a previous load, that is not nessecarely an issue :-)
                if (loaded == Core::ERROR_NONE) {
                    // Controller speed has been reset to default speed!!!
                    SetBaudRate(_setupRate);

                    if (Reset() != Core::ERROR_NONE) {
                        result = "Could not reset the device after the firmware upload";
                    }
                    else if (SetSpeed(_baudRate) != Core::ERROR_NONE) {
                        result = "Could not set the BaudRate (second time)";
                    }
                } else if (loaded != Core::ERROR_ALREADY_CONNECTED) {
                    result = "Could not upload firmware.";
                }

                if ((result == nullptr) && (_MACLength > 0)) {
                    if (MACAddress(_MACLength, _MACAddress) != Core::ERROR_NONE) {
                        result = "Could not set the MAC Address specified.";
                    }
                }

                if ( (result == nullptr) && (SerialDriver::Setup(0, HCI_UART_H4) != Core::ERROR_NONE) ) {
                    result = "Could not set up the driver in H4 mode.";
                }
            }

            return (result);
        }
        uint32_t Reset()
        {
            Exchange::Response response(Exchange::COMMAND_PKT, 0x030C);
            uint32_t result = Exchange(Exchange::Request(Exchange::COMMAND_PKT, 0x030C, 0, nullptr), response, 1000);

            if ((result == Core::ERROR_NONE) && (response[3] != CMD_SUCCESS)) {
                TRACE_L1("Failed to reset chip, command failure\n");
                result = Core::ERROR_GENERAL;
            }

            return result;
        }
 
    private:
        string FindFirmware(const string& directory, const string& chipName)
        {
            string result;
            Core::Directory index(directory.c_str(), "*.hcd");

            while ((result.empty() == true) && (index.Next() == true)) {

                if (index.Name() == chipName) {
                    result = index.Current();
                } else if ((index.IsDirectory() == true) && (index.Name() != _T(".")) && (index.Name() != _T(".."))) {

                    result = FindFirmware(index.Current(), chipName);
                }
            }

            return (result);
        }
       uint32_t LoadName()
        {
            Exchange::Response response(Exchange::COMMAND_PKT, 0x140C);
            uint32_t result = Exchange(Exchange::Request(Exchange::COMMAND_PKT, 0x140C, 0, nullptr), response, 500);

            if ((result == Core::ERROR_NONE) && (response[3] != CMD_SUCCESS)) {
                TRACE_L1("Failed to read local name, command failure\n");
                result = Core::ERROR_GENERAL;
            }
            if (result == Core::ERROR_NONE) {
                _name = string(reinterpret_cast<const TCHAR*>(&(response[4])));
            }
            return (result);
        }
        uint32_t SetClock(const uint8_t clock)
        {
            Exchange::Response response(Exchange::COMMAND_PKT, 0x45fc);
            uint32_t result = Exchange(Exchange::Request(Exchange::COMMAND_PKT, 0x45fc, 1, &clock), response, 500);

            if ((result == Core::ERROR_NONE) && (response[3] != CMD_SUCCESS)) {
                TRACE_L1("Failed to read local name, command failure\n");
                result = Core::ERROR_GENERAL;
            }

            return (result);
        }
        uint32_t SetSpeed(const uint32_t baudrate)
        {
            uint32_t result = Core::ERROR_NONE;

            if (baudrate > 3000000) {
                result = SetClock(BCM43XX_CLOCK_48);
                Flush();
            }

            if (result == Core::ERROR_NONE) {
                uint8_t data[6];

                data[0] = 0x00;
                data[1] = 0x00;
                data[2] = static_cast<uint8_t>(baudrate & 0xFF);
                data[3] = static_cast<uint8_t>((baudrate >> 8) & 0xFF);
                data[4] = static_cast<uint8_t>((baudrate >> 16) & 0xFF);
                data[5] = static_cast<uint8_t>((baudrate >> 24) & 0xFF);

                Exchange::Response response(Exchange::COMMAND_PKT, 0x18fc);
                uint32_t result = Exchange(Exchange::Request(Exchange::COMMAND_PKT, 0x18fc, sizeof(data), data), response, 500);

                if ((result == Core::ERROR_NONE) && (response[3] != CMD_SUCCESS)) {
                    TRACE_L1("Failed to read local name, command failure\n");
                    result = Core::ERROR_GENERAL;
                } else {
                    SetBaudRate(baudrate);
                }
            }

            return (result);
        }
        uint32_t MACAddress(const uint8_t length, const uint8_t address[])
        {
            uint8_t data[6];
            ::memcpy(data, address, std::min(length, static_cast<uint8_t>(sizeof(data))));
            ::memset(&(data[length]), 0, sizeof(data) - std::min(length, static_cast<uint8_t>(sizeof(data))));

            Exchange::Response response(Exchange::COMMAND_PKT, 0x01fc);
            uint32_t result = Exchange(Exchange::Request(Exchange::COMMAND_PKT, 0x01fc, sizeof(data), address), response, 500);

            if ((result == Core::ERROR_NONE) && (response[3] != CMD_SUCCESS)) {
                TRACE_L1("Failed to set the MAC address\n");
                result = Core::ERROR_GENERAL;
            }

            return (result);
        }
        uint32_t Firmware(const string& directory, const string& name)
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;
            string searchPath = Core::Directory::Normalize(directory);
            string firmwareName = FindFirmware(searchPath, name + ".hcd");

            if (firmwareName.empty() == true) {
                // It has been observed that once the firmware is loaded the name of
                // the device changes from BCM43430A1 to BCM43438A1, this is due to
                // a previous load, that is not nessecarely an issue :-)
                result = Core::ERROR_ALREADY_CONNECTED;
            } else {
                int fd = open(firmwareName.c_str(), O_RDONLY);

                if (fd >= 0) {
                    Exchange::Response response(Exchange::COMMAND_PKT, 0x2efc);
                    result = Exchange(Exchange::Request(Exchange::COMMAND_PKT, 0x2efc, 0, nullptr), response, 500);

                    Flush();

                    if ((result == Core::ERROR_NONE) && (response[3] != CMD_SUCCESS)) {
                        TRACE_L1("Failed to set chip to download firmware, code: %d", response[3]);
                        result = Core::ERROR_GENERAL;
                    }

                    if (result == Core::ERROR_NONE) {
                        int loaded = 0;
                        uint8_t tx_buf[255];

                        /* Wait 50ms to let the firmware placed in download mode */
                        SleepMs(50);
                        Flush();

                        while ((result == Core::ERROR_NONE) && ((loaded = read(fd, tx_buf, 3)) > 0)) {
                            uint16_t code = (tx_buf[0] << 8) | tx_buf[1];
                            uint8_t len = tx_buf[2];

                            if (read(fd, tx_buf, len) < 0) {
                                result = Core::ERROR_READ_ERROR;
                            } else {
                                Exchange::Response response(Exchange::COMMAND_PKT, code);
                                result = Exchange(Exchange::Request(Exchange::COMMAND_PKT, code, len, tx_buf), response, 500);
                                Flush();
                            }
                        }

                        if ((loaded != 0) && (result == Core::ERROR_NONE)) {
                            result = Core::ERROR_NEGATIVE_ACKNOWLEDGE;
                        }

                        /* Wait for firmware ready */
                        SleepMs(2000);
                    }

                    close(fd);
                }
            }

            return (result);
        }

    private:
        const string _directory;
        string _name;
        uint8_t _MACLength;
        uint8_t _MACAddress[6];
        uint32_t _setupRate;
        uint32_t _baudRate;
    };
}
} // namespace WPEFramework::Bluetooth


#ifdef __cplusplus
extern "C" {
#endif

WPEFramework::Bluetooth::Broadcom43XX* g_driver = nullptr;

const char* construct_bluetooth_driver(const char* input) {
    const char* result = "Driver already loaded.";
    
    if (g_driver == nullptr) {
        WPEFramework::Bluetooth::Broadcom43XX::Config config;
        config.FromString(input);
        WPEFramework::Bluetooth::Broadcom43XX* driver = new WPEFramework::Bluetooth::Broadcom43XX(config);


        result = driver->Initialize();

        if (result == nullptr) {
            g_driver = driver;
        }
        else {
            delete driver;
        }
    }
    return (result);
}

void destruct_bluetooth_driver() {
    if (g_driver != nullptr) {
        delete g_driver;
        g_driver = nullptr;
    }
}


#ifdef __cplusplus
}
#endif


