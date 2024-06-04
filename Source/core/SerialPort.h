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
 
#ifndef __SERIALPORT_H
#define __SERIALPORT_H

#include "Module.h"
#include "Portability.h"
#include "Sync.h"

#ifdef __UNIX__
#include <errno.h>
#include <poll.h>
#include <stdarg.h>
#include <sys/types.h>
#endif
#include "Portability.h"
#include "ResourceMonitor.h"

namespace Thunder {
namespace Core {

#ifdef __WINDOWS__
    class EXTERNAL SerialPort {
#else
    class EXTERNAL SerialPort : public IResource {
#endif
    private:
        static constexpr uint16_t DefaultSendBuffer = 64;
        static constexpr uint16_t DefaultReceiveBuffer = 64;

        friend class SerialMonitor;

    public:
        enum FlowControl {
            OFF,
            SOFTWARE,
            HARDWARE
        };

        enum BaudRate {
            BAUDRATE_0 = B0, /* HANG UP */

            BAUDRATE_110 = B110,
            BAUDRATE_300 = B300,
            BAUDRATE_600 = B600,
            BAUDRATE_1200 = B1200,
            BAUDRATE_2400 = B2400,
            BAUDRATE_4800 = B4800,
            BAUDRATE_9600 = B9600,
            BAUDRATE_19200 = B19200,
            BAUDRATE_38400 = B38400,
            BAUDRATE_57600 = B57600,
            BAUDRATE_115200 = B115200,
#ifdef B230400
            BAUDRATE_230400 = B230400,
#endif
#ifdef B460800
            BAUDRATE_460800 = B460800,
#endif
            BAUDRATE_500000 = B500000,
#ifdef B576000
            BAUDRATE_576000 = B576000,
#endif
#ifdef B921600
            BAUDRATE_921600 = B921600,
#endif
            BAUDRATE_1000000 = B1000000,
            BAUDRATE_1152000 = B1152000,
            BAUDRATE_1500000 = B1500000,
            BAUDRATE_2000000 = B2000000,
            BAUDRATE_2500000 = B2500000,
            BAUDRATE_3000000 = B3000000,
            BAUDRATE_3500000 = B3500000,
#ifdef B3710000
            BAUDRATE_3710000 = B3710000,
#endif
            BAUDRATE_4000000 = B4000000
        };

        enum Parity {
            NONE = NOPARITY,
            EVEN = EVENPARITY,
            ODD = ODDPARITY,
            MARK = MARKPARITY,
            SPACE = SPACEPARITY
        };

        enum DataBits {
            BITS_5 = CS5,
            BITS_6 = CS6,
            BITS_7 = CS7,
            BITS_8 = CS8
        };

        enum StopBits {
            BITS_1 = ONESTOPBIT,
            BITS_2 = TWOSTOPBITS,
            BITS_15 = ONE5STOPBITS
        };

#ifdef __WINDOWS__
        enum enumState {
            READ = 0x0001,
            WRITE = 0x0002,
            EXCEPTION = 0x0200,
            OPEN = 0x0400,
        };

#endif

#ifdef __LINUX__
        enum enumState {
            READ = POLLIN,
            WRITE = POLLOUT,
            WRITESLOT = 0x0100,
            EXCEPTION = 0x0200,
            OPEN = 0x0400
        };
#endif
        // -------------------------------------------------------------------------
        // This object should not be copied, assigned or created with a default
        // constructor. Prevent them from being used, generatoed by the compiler.
        // define them but do not implement them. Compile error and/or link error.
        // -------------------------------------------------------------------------
    public:
        SerialPort(SerialPort&& move) = delete;
        SerialPort(const SerialPort& a_RHS) = delete;
        SerialPort& operator=(SerialPort&& move) = delete;
        SerialPort& operator=(const SerialPort& a_RHS) = delete;

    public:
        SerialPort();
        SerialPort(const string& port);
        SerialPort(
            const string& port,
            const BaudRate baudRate,
            const Parity parityE,
            const DataBits dataBits,
            const StopBits stopBits,
            const FlowControl flowControl,
            const uint16_t sendBufferSize,
            const uint16_t receiveBufferSize);

        virtual ~SerialPort();

        static BaudRate Convert(const uint32_t speed);

    public:
        inline bool operator==(const SerialPort& RHS) const
        {
            return (_descriptor == RHS._descriptor);
        }
        inline bool operator!=(const SerialPort& RHS) const
        {
            return (!operator==(RHS));
        }
        inline bool IsOpen() const
        {
            return ((_state & SerialPort::OPEN) == SerialPort::OPEN);
        }
        inline bool IsClosed() const
        {
            return (!IsOpen());
        }
        inline bool HasError() const
        {
            return ((_state & SerialPort::EXCEPTION) != 0);
        }
        inline void Flush()
        {
            _adminLock.Lock();
            _readBytes = 0;
            _sendOffset = 0;
            _sendBytes = 0;
#ifndef __WINDOWS__
            tcflush(_descriptor, TCIOFLUSH);
#endif

            _adminLock.Unlock();
        }
        inline const string& LocalId() const
        {
            return (_portName);
        }
        inline const string& RemoteId() const
        {
            return (_portName);
        }

        uint32_t Open(uint32_t waitTime);
        uint32_t Close(uint32_t waitTime);
        void Trigger();

        // Methods to extract and insert data into the socket buffers
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) = 0;
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) = 0;
        virtual void StateChange() = 0;

        uint32_t Configuration(
            const string& port,
            const BaudRate baudRate,
            const Parity parity,
            const DataBits dataBits,
            const StopBits stopBits,
            const FlowControl flowControl);

        uint32_t Configuration(
            const BaudRate baudRate,
            const FlowControl flowControl);

        uint32_t SetBaudRate(const BaudRate baudrate)
        {
            uint32_t result = Core::ERROR_NONE;

            _adminLock.Lock();

            _baudRate = baudrate;

            if (_descriptor != INVALID_HANDLE_VALUE) {
                result = Settings();
            }
            _adminLock.Unlock();

            return (result);
        }
        void SendBreak()
        {
            if (_descriptor != INVALID_HANDLE_VALUE) {
#ifdef __WINDOWS__
                // TODO: Implement a windows variant..
                ASSERT(false);
#else
                tcsendbreak(_descriptor, 0);
#endif
            }
        }

    private:
        void Construct(const uint16_t sendBufferSize, const uint16_t receiveBufferSize);
        uint32_t Settings();

        void Opened()
        {
            _state = SerialPort::OPEN | SerialPort::READ | SerialPort::WRITE;
            StateChange();
        }
        void Closed()
        {
            StateChange();

#ifdef __LINUX__
            close(_descriptor);
#endif

#ifdef __WINDOWS__
            ::CloseHandle(_descriptor);
#endif

            _descriptor = INVALID_HANDLE_VALUE;
            _state = 0;
        }
        uint32_t WaitForClosure(const uint32_t time) const;
#ifdef __WINDOWS__
        void Write(const uint16_t writtenBytes);
        void Read(const uint16_t readBytes);
#endif
#ifdef __LINUX__
        void Write();
        void Read();
        IResource::handle Descriptor() const override
        {
            return (static_cast<IResource::handle>(_descriptor));
        }
        uint16_t Events() override;
        void Handle(const uint16_t events) override;
#endif

#ifdef __LINUX__
        void BufferAlignment(int descriptor);
#endif
#ifdef __WINDOWS__
        void BufferAlignment(HANDLE descriptor);
#endif
#ifdef __WINDOWS__
        inline HANDLE Descriptor()
        {
            return (_descriptor);
        }
#endif
    private:
        mutable CriticalSection _adminLock;
        string _portName;
        volatile uint16_t _state;
        uint16_t _sendBufferSize;
        uint16_t _receiveBufferSize;
        uint8_t* _sendBuffer;
        uint8_t* _receiveBuffer;
        uint16_t _readBytes;
        uint16_t _sendOffset;
        uint16_t _sendBytes;

#ifdef __WINDOWS__
        HANDLE _descriptor;
        mutable OVERLAPPED _writeInfo;
        mutable OVERLAPPED _readInfo;
#endif

#ifdef __LINUX__
        int _descriptor;
#endif

        BaudRate _baudRate;
        Parity _parity;
        DataBits _dataBits;
        StopBits _stopBits;
        FlowControl _flowControl;
    };
}
} // namespace Core

#endif // __SERIALPORT_H
