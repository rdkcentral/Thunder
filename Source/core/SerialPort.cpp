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
 
#include "SerialPort.h"

#include "Serialization.h"
#include "Singleton.h"
#include "Sync.h"
#include "Thread.h"

#include <errno.h>

#ifdef __POSIX__
#include <fcntl.h>
#include <poll.h>
#ifdef __APPLE__
#include "NodeId.h"
#define SOCKET signed int
#define SOCKET_ERROR static_cast<signed int>(-1)
#define INVALID_SOCKET static_cast<SOCKET>(-1)
#define __ERRORRESULT__ errno
#endif

#include <sys/ioctl.h>
#define ERRORRESULT errno
#define ERROR_WOULDBLOCK EWOULDBLOCK
#define ERROR_AGAIN EAGAIN
#endif

#ifdef __WINDOWS__
#define ERRORRESULT ::GetLastError()
#define ERROR_WOULDBLOCK ERROR_IO_PENDING
#define ERROR_AGAIN ERROR_IO_PENDING
#endif

namespace Thunder {
namespace Core {

//////////////////////////////////////////////////////////////////////
// SerialPort::SerialMonitor
//////////////////////////////////////////////////////////////////////
#ifdef __WINDOWS__
        // Retrieve the system error message for the last-error code
    string LastError(const uint32_t errorCode) {
        LPVOID lpMsgBuf;
        DWORD dw = errorCode;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);

        // Display the error message and exit the process
        string result(reinterpret_cast<const TCHAR*>(lpMsgBuf));
        LocalFree(lpMsgBuf);
        return (result);
    }

    class SerialMonitor {
    private:
        class MonitorWorker : public Core::Thread {
        public:
            MonitorWorker(const MonitorWorker&) = delete;
            MonitorWorker(MonitorWorker&&) = delete;
            MonitorWorker& operator=(const MonitorWorker&) = delete;
            MonitorWorker& operator=(MonitorWorker&&) = delete;

        public:
            MonitorWorker(SerialMonitor& parent)
                : Core::Thread(Thread::DefaultStackSize(), _T("SerialPortMonitor"))
                , _parent(parent)
            {
                Thread::Init();
            }
            virtual ~MonitorWorker()
            {
                Wait(Thread::BLOCKED | Thread::STOPPED | Thread::INITIALIZED, Core::infinite);
            }

        public:
            virtual uint32_t Worker()
            {
                return (_parent.Worker());
            }

        private:
            SerialMonitor& _parent;
        };

        static const uint8_t SLOT_ALLOCATION = 8;
    public:
        SerialMonitor(const SerialMonitor&) = delete;
        SerialMonitor(SerialMonitor&&) = delete;
        SerialMonitor& operator=(const SerialMonitor&) = delete;
        SerialMonitor& operator=(SerialMonitor&&) = delete;

    public:
        SerialMonitor()
            : _ThreadInstance(nullptr)
            , _Admin()
            , _MonitoredPorts()
            , _MaxSlots(SLOT_ALLOCATION)
            , _Slots(static_cast<HANDLE*>(::malloc((sizeof(HANDLE) * 2 * _MaxSlots) + sizeof(HANDLE))))
            , _Break(CreateEvent(nullptr, TRUE, FALSE, nullptr))
        {
            _Slots[0] = _Break;
        }
        virtual ~SerialMonitor()
        {
            TRACE_L1("SerialPortMonitor: Closing [%d] ports", static_cast<uint32_t>(_MonitoredPorts.size()));

            // Unregister all serial ports (Close the port !!!) before closing the App !!!!
            ASSERT(_MonitoredPorts.size() == 0);

            if (_ThreadInstance != nullptr) {
                _Admin.Lock();

                _MonitoredPorts.clear();

                _ThreadInstance->Block();

                Break();

                _Admin.Unlock();

                delete _ThreadInstance;
            }

            ::free(_Slots);
            ::CloseHandle(_Break);
        }

    public:
        void Monitor(SerialPort& port)
        {
            _Admin.Lock();

// Make sure this entry does not exist, only register sockets once !!!
#ifdef __DEBUG__
            std::list<SerialPort*>::const_iterator index = _MonitoredPorts.begin();

            while ((index != _MonitoredPorts.end()) && (*index != &port)) {
                index++;
            }
            ASSERT(index == _MonitoredPorts.end());
#endif

            _MonitoredPorts.push_back(&port);

            // Start waiting for characters to come in...
            port.Read(0);

            Break();

            if (_MonitoredPorts.size() == 1) {
                if (_ThreadInstance == nullptr) {
                    _ThreadInstance = new MonitorWorker(*this);
                }
                _ThreadInstance->Run();
            }

            _Admin.Unlock();
        }
        inline void Break()
        {
            ::SetEvent(_Break);
        }

    private:
        uint32_t Worker()
        {
            uint32_t delay = 0;

            // Add entries not in the Array before we start !!!
            _Admin.Lock();

            // Do we have enough space to allocate all file descriptors ?
            if ((_MonitoredPorts.size() + 1) >= _MaxSlots) {
                _MaxSlots = ((((static_cast<uint32_t>(_MonitoredPorts.size()) + 1) / SLOT_ALLOCATION) + 1) * SLOT_ALLOCATION);

                ::free(_Slots);

                // Resize the array to fit..
                _Slots = static_cast<HANDLE*>(::malloc((sizeof(HANDLE) * 2 * _MaxSlots) + sizeof(HANDLE)));

                _Slots[0] = _Break;
            }

            int filledSlot = 1;
            std::list<SerialPort*>::iterator index = _MonitoredPorts.begin();

            // Fill in all entries required/updated..
            while (index != _MonitoredPorts.end()) {
                SerialPort* port = (*index);

                if (port->IsOpen() == false) {
                    index = _MonitoredPorts.erase(index);
                    port->Closed();
                } else {
                    if (port->IsOpen() == true) {
                        port->Opened();
                    }
                    _Slots[filledSlot++] = (*index)->_readInfo.hEvent;
                    _Slots[filledSlot++] = (*index)->_writeInfo.hEvent;
                    index++;
                }
            }

            if (filledSlot <= 1) {
                _ThreadInstance->Block();
                delay = Core::infinite;
            } else {
                _Admin.Unlock();

                ::WaitForMultipleObjects(filledSlot, _Slots, FALSE, Core::infinite);

                _Admin.Lock();

                ::ResetEvent(_Slots[0]);

                // We are only interested in events that were fired..
                index = _MonitoredPorts.begin();

                while (index != _MonitoredPorts.end()) {
                    DWORD info;
                    SerialPort* port = (*index);

                    if (port != nullptr) {
                        if (::WaitForSingleObject(port->_readInfo.hEvent, 0) == WAIT_OBJECT_0) {

                            ::ResetEvent(port->_readInfo.hEvent);

                            if (::GetOverlappedResult(port->Descriptor(), &(port->_readInfo), &info, FALSE)) {

                                port->Read(static_cast<uint16_t>(info));
                            }
                            else {
                                DWORD result = GetLastError();

                                TRACE_L1("Oopsie daisy, could not read Serial Port: Error: %s :-(", LastError(result).c_str());
                            }
                        }

                        if (::WaitForSingleObject(port->_writeInfo.hEvent, 0) == WAIT_OBJECT_0) {

                            ::ResetEvent(port->_writeInfo.hEvent);

                            if (::GetOverlappedResult(port->Descriptor(), &(port->_writeInfo), &info, FALSE)) {
                                port->Write(static_cast<uint16_t>(info));
                            }
                            #ifdef __DEBUG__
                            else {
                                DWORD result = GetLastError();

                                if (result != ERROR_IO_INCOMPLETE) {
                                    TRACE_L1("Oopsie daisy, could not write Serial Port: Error: 0x%X => %s :-(", result, LastError(result).c_str());
                                }
                            }
                            #endif
                        }
                    }

                    index++;
                }
            }

            _Admin.Unlock();

            return (delay);
        }

    private:
        MonitorWorker* _ThreadInstance;
        Core::CriticalSection _Admin;
        std::list<SerialPort*> _MonitoredPorts;
        uint32_t _MaxSlots;
        HANDLE* _Slots;
        HANDLE _Break;
    };

    static SerialMonitor& g_SerialPortMonitor = SingletonType<SerialMonitor>::Instance();
#endif

    //////////////////////////////////////////////////////////////////////
    // SerialPort::Trigger
    //////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////
    // SerialPort
    //////////////////////////////////////////////////////////////////////

    SerialPort::SerialPort()
        : _adminLock()
        , _portName()
        , _state(0)
        , _sendBufferSize(0)
        , _receiveBufferSize(0)
        , _sendBuffer(nullptr)
        , _receiveBuffer(nullptr)
        , _readBytes(0)
        , _sendOffset(0)
        , _sendBytes(0)
        , _descriptor(INVALID_HANDLE_VALUE)
        , _baudRate(BAUDRATE_57600)
        , _parity(NONE)
        , _dataBits(BITS_8)
        , _stopBits(BITS_1)
        , _flowControl(OFF)
    {
        Construct(DefaultSendBuffer, DefaultReceiveBuffer);
    }
    SerialPort::SerialPort(const string& port)
        : _adminLock()
        , _portName(port)
        , _state(0)
        , _sendBufferSize(0)
        , _receiveBufferSize(0)
        , _sendBuffer(nullptr)
        , _receiveBuffer(nullptr)
        , _readBytes(0)
        , _sendOffset(0)
        , _sendBytes(0)
        , _descriptor(INVALID_HANDLE_VALUE)
        , _baudRate(BAUDRATE_57600)
        , _parity(NONE)
        , _dataBits(BITS_8)
        , _stopBits(BITS_1)
        , _flowControl(OFF)
    {
        Construct(DefaultSendBuffer, DefaultReceiveBuffer);
    }
    SerialPort::SerialPort(
        const string& port,
        const BaudRate baudRate,
        const Parity parity,
        const DataBits dataBits,
        const StopBits stopBits,
        const FlowControl flowControl,
        const uint16_t sendBufferSize,
        const uint16_t receiveBufferSize)
        : _adminLock()
        , _portName(port)
        , _state(0)
        , _sendBufferSize(0)
        , _receiveBufferSize(0)
        , _sendBuffer(nullptr)
        , _receiveBuffer(nullptr)
        , _readBytes(0)
        , _sendOffset(0)
        , _sendBytes(0)
        , _descriptor(INVALID_HANDLE_VALUE)
        , _baudRate(BAUDRATE_57600)
        , _parity(NONE)
        , _dataBits(BITS_8)
        , _stopBits(BITS_1)
        , _flowControl(OFF)
    {
        Construct(sendBufferSize, receiveBufferSize);
        Configuration(port, baudRate, parity, dataBits, stopBits, flowControl);
    }

    /* virtual */ SerialPort::~SerialPort()
    {
        // Make sure the socket is closed before you destruct. Otherwise
        // the virtuals might be called, which are destructed at this point !!!!
        ASSERT(_descriptor == INVALID_HANDLE_VALUE);

        if (_sendBuffer != nullptr) {
            ::free(_sendBuffer);
        }

        #ifdef __WINDOWS__
        ::CloseHandle(_readInfo.hEvent);
        ::CloseHandle(_writeInfo.hEvent);
        #endif
    }

    /* static */ SerialPort::BaudRate SerialPort::Convert(const uint32_t speed) {
        if (speed <= 110) {
            return(BAUDRATE_110);
        }
        else if (speed <= 300) {
            return(BAUDRATE_300);
        }
        else if (speed <= 600) {
            return(BAUDRATE_600);
        }
        else if (speed <= 1200) {
            return(BAUDRATE_1200);
        }
        else if (speed <= 2400) {
            return(BAUDRATE_2400);
        }
        else if (speed <= 4800) {
            return(BAUDRATE_4800);
        }
        else if (speed <= 9600) {
            return(BAUDRATE_9600);
        }
        else if (speed <= 19200) {
            return(BAUDRATE_19200);
        }
        else if (speed <= 38400) {
            return(BAUDRATE_38400);
        }
        else if (speed <= 57600) {
            return(BAUDRATE_57600);
        }
        else if (speed <= 115200) {
            return(BAUDRATE_115200);
        }
#ifdef B230400
        else if (speed <= 230400) {
            return(BAUDRATE_230400);
        }
#endif
#ifdef B460800
        else if (speed <= 460800) {
            return(BAUDRATE_460800);
        }
#endif
        else if (speed <= 500000) {
            return(BAUDRATE_500000);
        }
#ifdef B576000
        else if (speed <= 576000) {
            return(BAUDRATE_576000);
        }
#endif
#ifdef B921600
        else if (speed <= 921600) {
            return(BAUDRATE_921600);
        }
#endif
        else if (speed <= 1000000) {
            return(BAUDRATE_1000000);
        }
        else if (speed <= 1152000) {
            return(BAUDRATE_1152000);
        }
        else if (speed <= 1500000) {
            return(BAUDRATE_1500000);
        }
        else if (speed <= 2000000) {
            return(BAUDRATE_2000000);
        }
        else if (speed <= 2500000) {
            return(BAUDRATE_2500000);
        }
        else if (speed <= 3000000) {
            return(BAUDRATE_3000000);
        }
        else if (speed <= 3500000) {
            return(BAUDRATE_3500000);
        }
#ifdef B3710000
        else if (speed <= 3710000) {
            return(BAUDRATE_3710000);
        }
#endif
        else if (speed <= 4000000) {
            return(BAUDRATE_4000000);
        }
        return(BAUDRATE_0);
    }

    uint32_t SerialPort::Configuration(
        const BaudRate baudRate,
        const FlowControl flowControl)
    {
        uint32_t result = Core::ERROR_NONE;

        _baudRate = baudRate;
        _flowControl = flowControl;

        if (_descriptor != INVALID_HANDLE_VALUE) {
            result = Settings();
        }

        return (result);
    }

    uint32_t SerialPort::Configuration(
        const string& port,
        const BaudRate baudRate,
        const Parity parity,
        const DataBits dataBits,
        const StopBits stopBits,
        const FlowControl flowControl)
    {
        uint32_t result = Core::ERROR_NONE;

        _portName = port;
        _baudRate = baudRate;
        _flowControl = flowControl;
        _parity = parity;
        _dataBits = dataBits;
        _stopBits = stopBits;

        if (_descriptor != INVALID_HANDLE_VALUE) {
            result = Settings();
        }

        return (result);
    }
    uint32_t SerialPort::Open(uint32_t /* waitTime */)
    {
        uint32_t result = 0;

        _adminLock.Lock();

        if (_descriptor == INVALID_HANDLE_VALUE) {

#ifdef __POSIX__
            std::string convertedPortName;
            Core::ToString(_portName.c_str(), convertedPortName);

            _descriptor = open(convertedPortName.c_str(), O_RDWR | O_NOCTTY);

            result = errno;

            if (_descriptor != -1) {
                int flags = fcntl(_descriptor, F_GETFL, 0) | O_NONBLOCK;

                if (fcntl(_descriptor, F_SETFL, flags) != 0) {
                    TRACE_L4("Error on port socket F_SETFL call. Error %d", ERRORRESULT);

                    result = errno;

                    close(_descriptor);

                    _descriptor = -1;
                } else {
                    result = Settings();

                    if (result != Core::ERROR_NONE) {
                        ::close(_descriptor);
                        _descriptor = INVALID_HANDLE_VALUE;
                    }
                    else {
                        _state = SerialPort::OPEN;
                        ResourceMonitor::Instance().Register(*this);
                    }
                }
            }
#endif

#ifdef __WINDOWS__
            _descriptor = ::CreateFile(_portName.c_str(),
                GENERIC_READ | GENERIC_WRITE, //access ( read and write)
                0, //(share) 0:cannot share the COM port
                0, //security  (None)
                OPEN_EXISTING, // creation : open_existing
                FILE_FLAG_OVERLAPPED, // we want overlapped operation
                0 // no templates file for COM port...
            );

            result = GetLastError();

            if (_descriptor != INVALID_HANDLE_VALUE) {
                result = Settings();

                if (result != Core::ERROR_NONE) {
                    ::CloseHandle(_descriptor);
                    _descriptor = INVALID_HANDLE_VALUE;
                }
                else {
                    _readBytes = 0;
                    _state = SerialPort::OPEN;

                    g_SerialPortMonitor.Monitor(*this);

                    result = Core::ERROR_NONE;
                }
            }
#endif

        }

        _adminLock.Unlock();

        return (result);
    }

    uint32_t SerialPort::Close(uint32_t waitTime)
    {
        _adminLock.Lock();

#ifdef __LINUX__

        if (_descriptor != -1) {
            // Before we delete the descriptor, get ride of the Trigger
            // subscribtion.
            _state |= SerialPort::EXCEPTION;
            _state &= ~SerialPort::OPEN;
            ResourceMonitor::Instance().Break();
        } 
#endif

#ifdef __WINDOWS__
            if (_descriptor != INVALID_HANDLE_VALUE) {

                _state |= SerialPort::EXCEPTION;
                _state &= ~SerialPort::OPEN;
                g_SerialPortMonitor.Break();
            } 
#endif

        WaitForClosure(waitTime);
        _adminLock.Unlock();

        return (Core::ERROR_NONE);
    }

        uint32_t SerialPort::WaitForClosure(const uint32_t time) const
        {
            // If we build in release, we do not want to "hang" forever, forcefull close after 20S waiting...
#ifdef __DEBUG__
            uint32_t waiting = time; // Expect time in MS
            uint32_t reportSlot = 0;
#else
    uint32_t waiting = (time == Core::infinite ? 20000 : time);
#endif

            // Right, a wait till connection is closed is requested..
            while ((waiting > 0) && (_state != 0)) {
                // Make sure we aren't in the monitor thread waiting for close completion.
                ASSERT(Core::Thread::ThreadId() != ResourceMonitor::Instance().Id());

                uint32_t sleepSlot = (waiting > SLEEPSLOT_POLLING_TIME ? SLEEPSLOT_POLLING_TIME : waiting);

                _adminLock.Unlock();

                // Right, lets sleep in slices of <= SLEEPSLOT_POLLING_TIME ms
                SleepMs(sleepSlot);

                _adminLock.Lock();

#ifdef __DEBUG__
                if ((++reportSlot & 0x1F) == 0) {
                    TRACE_L1("Currently waiting for Socket Closure. Current State [0x%X]", _state);
                }
                waiting -= (waiting == Core::infinite ? 0 : sleepSlot);
#else
        waiting -= sleepSlot;
#endif
            }
            return (_state == 0 ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
        }

        void SerialPort::Trigger()
        {
            _adminLock.Lock();

#ifdef __WINDOWS__
            if ((_state & (SerialPort::OPEN | SerialPort::EXCEPTION)) == SerialPort::OPEN) {
                ::SetEvent(_writeInfo.hEvent);
            }
#else
    if ((_state & (SerialPort::OPEN | SerialPort::EXCEPTION | SerialPort::WRITESLOT)) == SerialPort::OPEN) {
        _state |= SerialPort::WRITESLOT;
        ResourceMonitor::Instance().Break();
    }
#endif

            _adminLock.Unlock();
        }

#ifndef __WINDOWS__
        /* virtual */ uint16_t SerialPort::Events()
        {
            uint16_t result = POLLIN;
            if ((_state & SerialPort::OPEN) == 0) {
                result = 0;
                Closed();
            } else if ((_state & (SerialPort::OPEN | SerialPort::READ | SerialPort::WRITE)) == SerialPort::OPEN) {
                Opened();
                Write();
            } else if ((_state & SerialPort::WRITE) != 0) {
                result |= POLLOUT;
            }
            return (result);
        }

        /* virtual */ void SerialPort::Handle(const uint16_t flags)
        {

            bool breakIssued = ((_state & SerialPort::WRITESLOT) != 0);

            if (((flags != 0) || (breakIssued == true)) && ((_state & SerialPort::OPEN) != 0)) {

                if (((flags & POLLOUT) != 0) || (breakIssued == true)) {
                    Write();
                }
                if ((flags & POLLIN) != 0) {
                    Read();
                }
            }
            Read();
        }

#else
void SerialPort::Write(const uint16_t writtenBytes)
{
    _adminLock.Lock();

    _state &= (~(SerialPort::WRITE));

    ASSERT(((_sendBytes == 0) && (_sendOffset == 0)) || ((writtenBytes + _sendOffset) <= _sendBytes));

    if (_sendBytes != 0) {
        _sendOffset += writtenBytes;
    }

    do {
        if (_sendOffset == _sendBytes) {
            _sendBytes = SendData(_sendBuffer, _sendBufferSize);
            _sendOffset = 0;
        }

        if (_sendOffset < _sendBytes) {
            DWORD sendSize;

            // Sockets are non blocking the Send buffer size is equal to the buffer size. We only send
            // if the buffer free (SEND flag) is active, so the buffer should always fit.
            if (::WriteFile(_descriptor,
                    &_sendBuffer[_sendOffset],
                    _sendBytes - _sendOffset,
                    &sendSize, &_writeInfo)
                != FALSE) {
                _sendOffset += static_cast<uint16_t>(sendSize);
            } else {
                uint32_t result = ERRORRESULT;

                _state |= (SerialPort::WRITE);

                if ((result != ERROR_WOULDBLOCK) && (result != ERROR_AGAIN)) {
                    _state |= SerialPort::EXCEPTION;

                    StateChange();
                }
            }
        }
    } while (((_state & (SerialPort::WRITE | SerialPort::EXCEPTION)) == 0) && (_sendBytes != 0));

    _adminLock.Unlock();
}

void SerialPort::Read(const uint16_t readBytes)
{
    _adminLock.Lock();

    if (readBytes > 0) {
        uint16_t handledBytes = ReceiveData(_receiveBuffer, _readBytes + readBytes);

        ASSERT((_readBytes + readBytes) >= handledBytes);

        _readBytes += readBytes;
        _readBytes -= handledBytes;

        if ((_readBytes != 0) && (handledBytes != 0)) {
            // Oops not all data was consumed, Lets remove the read data
            ::memmove(_receiveBuffer, &_receiveBuffer[handledBytes], _readBytes);
        }
    }

    _state &= (~SerialPort::READ);

    while ((_state & (SerialPort::READ | SerialPort::EXCEPTION | SerialPort::OPEN)) == SerialPort::OPEN) {
        DWORD readBytes;
        uint32_t size = static_cast<uint32_t>(~0);

        ASSERT(_readBytes <= _receiveBufferSize);

        if (_readBytes == _receiveBufferSize) {
            _readBytes = 0;
        }

        // Read the actual data from the port.
        if (ReadFile(_descriptor, &_receiveBuffer[_readBytes], 1, &readBytes, &_readInfo) == 0) {
            uint32_t reason = ERRORRESULT;

            _state |= SerialPort::READ;

            if ((reason != ERROR_WOULDBLOCK) && (reason != ERROR_INPROGRESS)) {
                _state |= SerialPort::EXCEPTION;
                StateChange();
            }
        } else if (readBytes == 0) {
            // nothing to read wait on the next trigger..
            _state |= SerialPort::READ;
        } else {
            _readBytes += static_cast<uint16_t>(readBytes);
        }

        if (_readBytes != 0) {
            uint16_t handledBytes = ReceiveData(_receiveBuffer, _readBytes);

            ASSERT(_readBytes >= handledBytes);

            _readBytes -= handledBytes;

            if ((_readBytes != 0) && (handledBytes != 0)) {
                // Oops not all data was consumed, Lets remove the read data
                ::memcpy(_receiveBuffer, &_receiveBuffer[handledBytes], _readBytes);
            }
        }
    }

    _adminLock.Unlock();
}
#endif

#ifdef __POSIX__
        void SerialPort::Write()
        {
            _adminLock.Lock();

            _state &= (~(SerialPort::WRITE | SerialPort::WRITESLOT));

            do {
                if (_sendOffset == _sendBytes) {
                    _sendBytes = SendData(_sendBuffer, _sendBufferSize);
                    _sendOffset = 0;
                }

                if (_sendOffset < _sendBytes) {
                    uint32_t sendSize;

                    sendSize = write(_descriptor, reinterpret_cast<const char*>(&_sendBuffer[_sendOffset]),
                        _sendBytes - _sendOffset);

                    if (sendSize != static_cast<uint32_t>(-1)) {
                        _sendOffset += sendSize;
                    } else {
                        uint32_t result = ERRORRESULT;

                        if ((result == ERROR_WOULDBLOCK) || (result == ERROR_AGAIN)) {
                            _state |= (SerialPort::WRITE);
                        } else {
                            _state |= SerialPort::EXCEPTION;

                            StateChange();
                        }
                    }
                }
            } while (((_state & (SerialPort::WRITE | SerialPort::EXCEPTION)) == 0) && (_sendBytes != 0));

            _adminLock.Unlock();
        }

        void SerialPort::Read()
        {
            _adminLock.Lock();

            _state &= (~SerialPort::READ);

            while ((_state & (SerialPort::READ | SerialPort::EXCEPTION | SerialPort::OPEN)) == SerialPort::OPEN) {

                ASSERT(_readBytes <= _receiveBufferSize);

                if (_readBytes == _receiveBufferSize) {
                    _readBytes = 0;
                }

                // Read the actual data from the port.
                uint32_t size = ::read(_descriptor, reinterpret_cast<char*>(&_receiveBuffer[_readBytes]), _receiveBufferSize - _readBytes);

                if ((size != static_cast<uint32_t>(~0)) && (size != 0)) {
                    _readBytes += size;

                    if (_readBytes != 0) {
                        uint16_t handledBytes = ReceiveData(_receiveBuffer, _readBytes);

                        ASSERT(_readBytes >= handledBytes);

                        _readBytes -= handledBytes;

                        if ((_readBytes != 0) && (handledBytes != 0)) {
                            // Oops not all data was consumed, Lets remove the read data
                            ::memcpy(_receiveBuffer, &_receiveBuffer[handledBytes], _readBytes);
                        }
                    }
                } else {
                    uint32_t result = ERRORRESULT;

                    _state |= SerialPort::READ;

                    if ((result != ERROR_WOULDBLOCK) && (result != ERROR_INPROGRESS) && (result != 0)) {
                        _state |= SerialPort::EXCEPTION;
                        StateChange();
                    }
                }
            }

            _adminLock.Unlock();
        }
#endif

    void SerialPort::Construct(const uint16_t sendBufferSize, const uint16_t receiveBufferSize) {

        #ifdef __WINDOWS__
        ::memset(&_readInfo, 0, sizeof(OVERLAPPED));
        ::memset(&_writeInfo, 0, sizeof(OVERLAPPED));
        _readInfo.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
        _writeInfo.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
        #endif

        _sendBufferSize = sendBufferSize;
        _receiveBufferSize = receiveBufferSize;

        ASSERT((_sendBufferSize != 0) && (_receiveBufferSize != 0));

        uint8_t* allocatedMemory = static_cast<uint8_t*>(::calloc(_sendBufferSize + _receiveBufferSize, 1));

        _sendBuffer = allocatedMemory;
        _receiveBuffer = &(allocatedMemory[_sendBufferSize]);

    }

    uint32_t SerialPort::Settings()
    {
        uint32_t result = Core::ERROR_NONE;

        ASSERT(_descriptor != INVALID_HANDLE_VALUE);

#ifdef __LINUX__
        struct termios options;

        if (::tcgetattr(_descriptor, &options) != 0) {
            result = Core::ERROR_BAD_REQUEST;
        }
        else {
            if (cfsetospeed(&options, _baudRate) != 0) {
                _baudRate = static_cast<BaudRate>(cfgetospeed(&options));
            }

            // Set the input baurate equal to the output!
            cfsetispeed(&options, 0); 

            options.c_cflag &= ~(PARENB | PARODD | CSTOPB | CSIZE); // Clear all relevant bits
            options.c_cflag |= _parity | _stopBits | _dataBits; // Set the requested bits
            options.c_cflag |= (CLOCAL | CREAD);
            options.c_oflag &= ~OPOST;
            options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
            options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

            if (_flowControl == OFF) {
                options.c_cflag &= ~CRTSCTS;
                options.c_iflag &=  ~(IXON | IXOFF | IXANY);
            } else if (_flowControl == SOFTWARE) {
                options.c_cflag &= ~CRTSCTS;
                options.c_iflag |= (IXON | IXOFF);
            } else if (_flowControl == HARDWARE) {
                options.c_cflag |= CRTSCTS;
                options.c_iflag &=  ~(IXON | IXOFF | IXANY);
            }

            if (::tcsetattr(_descriptor, TCSANOW, &options) != 0) {
                result = errno;
            }
            else {
                ::tcflush(_descriptor, TCIOFLUSH);
            }
        }
#endif

#ifdef __WINDOWS__
        DCB options;

        ::GetCommState(_descriptor, &options);

        options.DCBlength = sizeof(DCB);
        options.BaudRate = _baudRate;
        options.ByteSize = _dataBits;
        options.Parity = _parity;
        options.StopBits = _stopBits;
        if (_flowControl == OFF) {
            options.fOutX = FALSE;
            options.fInX = FALSE;
            options.fDtrControl = DTR_CONTROL_DISABLE;
            options.fRtsControl = RTS_CONTROL_DISABLE;
            options.fOutxCtsFlow = FALSE;
            options.fOutxDsrFlow = FALSE;
        } else if (_flowControl == SOFTWARE) {
            options.fOutX = TRUE;
            options.fInX = TRUE;
            options.fDtrControl = DTR_CONTROL_DISABLE;
            options.fRtsControl = RTS_CONTROL_DISABLE;
            options.fOutxCtsFlow = FALSE;
            options.fOutxDsrFlow = FALSE;
        } else if (_flowControl == HARDWARE) {
            options.fOutX = FALSE;
            options.fInX = FALSE;
            options.fDtrControl = DTR_CONTROL_HANDSHAKE;
            options.fRtsControl = RTS_CONTROL_HANDSHAKE;
            options.fOutxCtsFlow = TRUE;
            options.fOutxDsrFlow = TRUE;
        }

        if (::SetCommState(_descriptor, &options) == 0) {
            result = GetLastError();
        }
#endif

        return (result);
    }
 
    }
} // namespace Solution::Core
