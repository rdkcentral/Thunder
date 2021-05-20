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

#include "../Module.h"

namespace WPEFramework {

namespace Bluetooth {

// #define DUMP_FRAMES 1

#define HCIUARTSETPROTO _IOW('U', 200, int)
#define HCIUARTGETPROTO _IOR('U', 201, int)
#define HCIUARTGETDEVICE _IOR('U', 202, int)
#define HCIUARTSETFLAGS _IOW('U', 203, int)
#define HCIUARTGETFLAGS _IOR('U', 204, int)

#define HCI_UART_H4 0
#define HCI_UART_BCSP 1
#define HCI_UART_3WIRE 2
#define HCI_UART_H4DS 3
#define HCI_UART_LL 4
#define HCI_UART_ATH3K 5
#define HCI_UART_INTEL 6
#define HCI_UART_BCM 7
#define HCI_UART_QCA 8
#define HCI_UART_AG6XX 9
#define HCI_UART_NOKIA 10
#define HCI_UART_MRVL 11

    class EXTERNAL SerialDriver {
    private:
        SerialDriver() = delete;
        SerialDriver(const SerialDriver&) = delete;
        SerialDriver& operator=(const SerialDriver&) = delete;

    public:
        //
        // Exchange holds a definitions of a request and a response for the communication with the
        // HCI. See WPEFramework/Source/core/StreamTypeKeyValue.h for more details.
        //
        struct Exchange {

            enum command : uint8_t {
                COMMAND_PKT = 0x01,
                EVENT_PKT = 0x04
            };

            static constexpr uint32_t BUFFERSIZE = 255;

#ifdef DUMP_FRAMES
            static void DumpFrame(const char header[], const uint8_t length, const uint8_t stream[])
            {
                printf("%s ", header);
                for (uint8_t index = 0; index < length; index++) {
                    if (index != 0) {
                        printf(":");
                    }
                    printf("%02X", stream[index]);
                }
                printf("\n");
            }
#else
#define DumpFrame(X, Y, Z)
#endif

            class Request {
            private:
                Request() = delete;
                Request(const Request& copy) = delete;
                Request& operator=(const Request& copy) = delete;

            protected:
                inline Request(const uint8_t* value)
                    : _command(EVENT_PKT)
                    , _sequence(0)
                    , _length(0)
                    , _offset(0)
                    , _value(value)
                {
                    ASSERT(value != nullptr);
                }
                inline Request(const command& cmd, const uint16_t sequence, const uint8_t* value)
                    : _command(cmd)
                    , _sequence(sequence)
                    , _length(0)
                    , _offset(0)
                    , _value(value)
                {
                    ASSERT(value != nullptr);
                }

            public:
                inline Request(const command& cmd, const uint16_t sequence, const uint8_t length, const uint8_t* value)
                    : _command(cmd)
                    , _sequence(sequence)
                    , _length(length)
                    , _offset(0)
                    , _value(value)
                {
                    ASSERT((value != nullptr) || (length == 0));
                }
                inline ~Request()
                {
                }

            public:
                inline void Reset()
                {
                    Request::Offset(0);
                }
                inline command Command() const
                {
                    return (_command);
                }
                inline uint16_t Sequence() const
                {
                    return (_sequence);
                }
                inline uint8_t Length() const
                {
                    return (_length);
                }
                inline const uint8_t* Value() const
                {
                    return (_value);
                }
                inline uint16_t Acknowledge() const
                {
                    return (_command != EVENT_PKT ? _sequence : (_length > 2 ? (_value[1] | (_value[2] << 8)) : ~0));
                }
                inline command Response() const
                {
                    return (_command != EVENT_PKT ? _command : static_cast<command>(_length > 0 ? _value[0] : ~0));
                }
                inline uint8_t DataLength() const
                {
                    return (_length - EventOffset());
                }
                inline const uint8_t& operator[](const uint8_t index) const
                {
                    ASSERT(index < (_length - EventOffset()));
                    return (_value[index - EventOffset()]);
                }
                inline uint16_t Serialize(uint8_t stream[], const uint16_t length) const
                {
                    uint16_t result = 0;

                    if (_offset == 0) {
                        _offset++;
                        stream[result++] = _command;
                    }
                    if ((_offset == 1) && ((length - result) > 0)) {
                        _offset++;
                        stream[result++] = static_cast<uint8_t>((_sequence) & 0xFF);
                    }
                    if ((_offset == 2) && ((length - result) > 0)) {
                        _offset++;
                        if (_command != EVENT_PKT) {
                            stream[result++] = static_cast<uint8_t>((_sequence >> 8) & 0xFF);
                        }
                    }
                    if ((_offset == 3) && ((length - result) > 0)) {
                        _offset++;
                        stream[result++] = _length;
                    }
                    if ((length - result) > 0) {
                        uint16_t copyLength = ((_length - (_offset - 4)) > (length - result) ? (length - result) : (_length - (_offset - 4)));
                        if (copyLength > 0) {
                            ::memcpy(&(stream[result]), &(_value[_offset - 4]), copyLength);
                            _offset += copyLength;
                            result += copyLength;
                        }
                    }

                    DumpFrame("OUT:", result, stream);

                    return (result);
                }

            protected:
                inline uint8_t EventOffset() const
                {
                    return (_command != EVENT_PKT ? 0 : (_length > 2 ? 3 : _length));
                }
                inline void Command(const command cmd)
                {
                    _command = cmd;
                }
                inline void Sequence(const uint16_t sequence)
                {
                    _sequence = sequence;
                }
                inline void Length(const uint8_t length)
                {
                    _length = length;
                }
                inline void Offset(const uint16_t offset)
                {
                    _offset = offset;
                }
                inline uint16_t Offset() const
                {
                    return (_offset);
                }

            private:
                command _command;
                uint16_t _sequence;
                uint8_t _length;
                mutable uint16_t _offset;
                const uint8_t* _value;
            };
            class Response : public Request {
            private:
                Response() = delete;
                Response(const Response& copy) = delete;
                Response& operator=(const Response& copy) = delete;

            public:
                inline Response(const command cmd, const uint16_t sequence)
                    : Request(cmd, sequence, _value)
                {
                }
                inline ~Response()
                {
                }

            public:
                bool Copy(const Request& buffer)
                {
                    bool result = false;

                    if ((buffer.Response() == Command()) && (buffer.Acknowledge() == Sequence())) {
                        uint8_t copyLength = static_cast<uint8_t>(buffer.Length() < BUFFERSIZE ? buffer.Length() : BUFFERSIZE);
                        ::memcpy(_value, buffer.Value(), copyLength);
                        Length(copyLength);
                        result = true;
                    }

                    return (result);
                }

            private:
                uint8_t _value[BUFFERSIZE];
            };
            class Buffer : public Request {
            private:
                Buffer(const Buffer& copy) = delete;
                Buffer& operator=(const Buffer& copy) = delete;

            public:
                inline Buffer()
                    : Request(_value)
                    , _used(0)
                {
                }
                inline ~Buffer()
                {
                }

            public:
                inline void Flush() {
                    _used = 0;
                    Length(0);
                    Offset(0);
                }
                inline bool Next()
                {
                    bool result = false;

                    // Clear the current selected block, move on to the nex block, return true, if
                    // we have a next block..
                    if ((Request::Offset() > 4) && ((Request::Offset() - 4) == Request::Length())) {
                        Request::Offset(0);
                        if (_used > 0) {
                            uint8_t length = _used;
                            _used = 0;
                            ::memcpy(&_value[0], &(_value[Request::Length()]), length);
                            result = Deserialize(_value, length);
                        }
                    }
                    return (result);
                }
                inline bool Deserialize(const uint8_t stream[], const uint16_t length)
                {
                    uint16_t result = 0;
                    if (Offset() < 1) {
                        Command(static_cast<command>(stream[result++]));
                        Offset(1);
                    }
                    if ((Offset() < 2) && ((length - result) > 0)) {
                        Sequence(stream[result++]);
                        Offset(2);
                    }
                    if ((Offset() < 3) && ((length - result) > 0)) {
                        if (Command() != EVENT_PKT) {
                            uint16_t sequence = (stream[result++] << 8) | Sequence();
                            Sequence(sequence);
                        }
                        Offset(3);
                    }
                    if ((Offset() < 4) && ((length - result) > 0)) {
                        Length(stream[result++]);
                        Offset(4);
                    }
                    if ((length - result) > 0) {
                        uint16_t copyLength = std::min(Length() - (Offset() - 4), length - result);
                        if (copyLength > 0) {
                            ::memcpy(&(_value[Offset() - 4]), &(stream[result]), copyLength);
                            Offset(Offset() + copyLength);
                            result += copyLength;
                        }
                    }
                    DumpFrame("IN: ", result, stream);
                    if (result < length) {
                        // TODO: This needs furhter investigation in case of failures ......
                        uint16_t copyLength = std::min(static_cast<uint16_t>((2 * BUFFERSIZE) - (Offset() - 4) - _used), static_cast<uint16_t>(length - result));
                        ::memcpy(&(_value[Offset() - 4 + _used]), &stream[result], copyLength);
                        _used += copyLength;
                        printf("Bytes in store: %d\n", _used);
                    }

                    return ((Offset() >= 4) && ((Offset() - 4) == Length()));
                }

            private:
                uint16_t _used;
                uint8_t _value[2 * BUFFERSIZE];
            };
        };

    private:
        class Channel : public Core::MessageExchangeType<Core::SerialPort, Exchange> {
        private:
            Channel() = delete;
            Channel(const Channel&) = delete;
            Channel& operator=(const Channel&) = delete;

            typedef MessageExchangeType<Core::SerialPort, SerialDriver::Exchange> BaseClass;

        public:
            Channel(SerialDriver& parent, const string& name)
                : BaseClass(name)
                , _parent(parent)
            {
            }
            virtual ~Channel()
            {
            }

        private:
            virtual void Received(const Exchange::Request& element) override
            {
                _parent.Received(element);
            }

        private:
            SerialDriver& _parent;
        };

    protected:
        SerialDriver(const string& port, const uint32_t baudRate, const Core::SerialPort::FlowControl flowControl, const bool sendBreak)
            : _port(*this, port)
            , _flowControl(flowControl)
        {
            if (_port.Open(100) == Core::ERROR_NONE) {

                ToTerminal();
                _port.Link().Configuration(Convert(baudRate), flowControl, 64, 64);
                _port.Flush();

                if (sendBreak == true) {
                    _port.Link().SendBreak();
                    SleepMs(500);
                }
            }
            else {
                printf("Could not open serialport.\n");
            }
        }

    public:
        virtual ~SerialDriver()
        {
            if (_port.IsOpen() == true) {
                ToTerminal();
                _port.Close(Core::infinite);
            }
        }

    public:
        uint32_t Setup(const unsigned long flags, const int protocol)
        {
            _port.Flush();

            int ttyValue = N_HCI;
            if (::ioctl(static_cast<Core::IResource&>(_port.Link()).Descriptor(), TIOCSETD, &ttyValue) < 0) {
                TRACE_L1("Failed direct IOCTL to TIOCSETD, %d", errno);
            } else if (::ioctl(static_cast<Core::IResource&>(_port.Link()).Descriptor(), HCIUARTSETFLAGS, flags) < 0) {
                TRACE_L1("Failed HCIUARTSETFLAGS. [flags:%lu]", flags);
            } else if (::ioctl(static_cast<Core::IResource&>(_port.Link()).Descriptor(), HCIUARTSETPROTO, protocol) < 0) {
                TRACE_L1("Failed HCIUARTSETPROTO. [protocol:%d]", protocol);
            } else {
                return (Core::ERROR_NONE);
            }

            return (Core::ERROR_GENERAL);
        }
        uint32_t Exchange(const Exchange::Request& request, Exchange::Response& response, const uint32_t allowedTime)
        {
            return (_port.Exchange(request, response, allowedTime));
        }
        void Reconfigure(const uint32_t baudRate)
        {
            if (_port.IsOpen() == true) {
                ToTerminal();
                _port.Close(Core::infinite);
            }
            if (_port.Open(100) == Core::ERROR_NONE) {

                ToTerminal();
                _port.Link().Configuration(Convert(baudRate), _flowControl, 64, 64);
                _port.Flush();
            }
            else {
                printf("Could not open serialport.\n");
            }
        }
        void SetBaudRate(const uint32_t baudRate)
        {
            _port.Link().SetBaudRate(Convert(baudRate));
            _port.Flush();
        }
        void Flush() 
        {
            _port.Flush();
        }
        virtual void Received(const Exchange::Request& element)
        {
        }

    private:
        inline void ToTerminal()
        {
            int ttyValue = N_TTY;
            _port.Link().Flush();
            if (::ioctl(static_cast<Core::IResource&>(_port.Link()).Descriptor(), TIOCSETD, &ttyValue) < 0) {
                printf("Failed direct IOCTL to TIOCSETD, %d\n", errno);
            }
        }
        Core::SerialPort::BaudRate Convert(const uint32_t baudRate)
        {
            if (baudRate <= 110)
                return (Core::SerialPort::BAUDRATE_110);
            if (baudRate <= 300)
                return (Core::SerialPort::BAUDRATE_300);
            if (baudRate <= 600)
                return (Core::SerialPort::BAUDRATE_600);
            if (baudRate <= 1200)
                return (Core::SerialPort::BAUDRATE_1200);
            if (baudRate <= 2400)
                return (Core::SerialPort::BAUDRATE_2400);
            if (baudRate <= 4800)
                return (Core::SerialPort::BAUDRATE_4800);
            if (baudRate <= 9600)
                return (Core::SerialPort::BAUDRATE_9600);
            if (baudRate <= 19200)
                return (Core::SerialPort::BAUDRATE_19200);
            if (baudRate <= 38400)
                return (Core::SerialPort::BAUDRATE_38400);
            if (baudRate <= 57600)
                return (Core::SerialPort::BAUDRATE_57600);
            if (baudRate <= 115200)
                return (Core::SerialPort::BAUDRATE_115200);
            if (baudRate <= 230400)
                return (Core::SerialPort::BAUDRATE_230400);
            if (baudRate <= 460800)
                return (Core::SerialPort::BAUDRATE_460800);
            if (baudRate <= 500000)
                return (Core::SerialPort::BAUDRATE_500000);
            if (baudRate <= 576000)
                return (Core::SerialPort::BAUDRATE_576000);
            if (baudRate <= 921600)
                return (Core::SerialPort::BAUDRATE_921600);
            if (baudRate <= 1000000)
                return (Core::SerialPort::BAUDRATE_1000000);
            if (baudRate <= 1152000)
                return (Core::SerialPort::BAUDRATE_1152000);
            if (baudRate <= 1500000)
                return (Core::SerialPort::BAUDRATE_1500000);
            if (baudRate <= 2000000)
                return (Core::SerialPort::BAUDRATE_2000000);
            if (baudRate <= 2500000)
                return (Core::SerialPort::BAUDRATE_2500000);
            if (baudRate <= 3000000)
                return (Core::SerialPort::BAUDRATE_3000000);
            if (baudRate <= 3500000)
                return (Core::SerialPort::BAUDRATE_3500000);
#ifdef B3710000
            if (baudRate <= 3710000)
                return (Core::SerialPort::BAUDRATE_3710000);
#endif
            if (baudRate <= 4000000)
                return (Core::SerialPort::BAUDRATE_4000000);
            return (Core::SerialPort::BAUDRATE_9600);
        }

    private:
        Channel _port;
        Core::SerialPort::FlowControl _flowControl;
    };
}
} // namespace WPEFramework::Bluetooth
