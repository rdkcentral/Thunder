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

namespace WPEFramework {
namespace Core {

#ifdef __WIN32__
    class EXTERNAL SerialPort {
#else
    class EXTERNAL SerialPort : public IResource {
#endif
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
            ODD = ODDPARITY
        };

        enum DataBits {
            BITS_5 = CS5,
            BITS_6 = CS6,
            BITS_7 = CS7,
            BITS_8 = CS8
        };

        enum StopBits {
            BITS_1 = ONESTOPBIT,
            BITS_2 = TWOSTOPBITS
        };

#ifdef __WIN32__
        typedef enum {
            READ = 0x0001,
            WRITE = 0x0002,
            EXCEPTION = 0x0200,
            OPEN = 0x0400,

        } enumState;

#endif

#ifdef __LINUX__
        typedef enum {
            READ = POLLIN,
            WRITE = POLLOUT,
            WRITESLOT = 0x0100,
            EXCEPTION = 0x0200,
            OPEN = 0x0400
        } enumState;
#endif
        // -------------------------------------------------------------------------
        // This object should not be copied, assigned or created with a default
        // constructor. Prevent them from being used, generatoed by the compiler.
        // define them but do not implement them. Compile error and/or link error.
        // -------------------------------------------------------------------------
    private:
        SerialPort(const SerialPort& a_RHS);
        SerialPort& operator=(const SerialPort& a_RHS);

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

    public:
        inline bool operator==(const SerialPort& RHS) const
        {
            return (m_Descriptor == RHS.m_Descriptor);
        }
        inline bool operator!=(const SerialPort& RHS) const
        {
            return (!operator==(RHS));
        }
        inline bool IsOpen() const
        {
            return ((m_State & SerialPort::OPEN) == SerialPort::OPEN);
        }
        inline bool IsClosed() const
        {
            return (!IsOpen());
        }
        inline bool HasError() const
        {
            return ((m_State & SerialPort::EXCEPTION) != 0);
        }
        inline void Flush()
        {
            m_syncAdmin.Lock();
            m_ReadBytes = 0;
            m_SendOffset = 0;
            m_SendBytes = 0;
#ifndef __WIN32__
            tcflush(m_Descriptor, TCIOFLUSH);
#endif

            m_syncAdmin.Unlock();
        }
        inline const string& LocalId() const
        {
            return (m_PortName);
        }
        inline const string& RemoteId() const
        {
            return (m_PortName);
        }

        uint32_t Open(uint32_t waitTime);
        uint32_t Close(uint32_t waitTime);
        void Trigger();

        // Methods to extract and insert data into the socket buffers
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) = 0;
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) = 0;
        virtual void StateChange() = 0;

        bool Configuration(
            const string& port,
            const BaudRate baudRate,
            const Parity parity,
            const DataBits dataBits,
            const StopBits stopBits,
            const FlowControl flowControl,
            const uint16_t sendBufferSize,
            const uint16_t receiveBufferSize);

        bool Configuration(
            const BaudRate baudRate,
            const FlowControl flowControl,
            const uint16_t sendBufferSize,
            const uint16_t receiveBufferSize);

        void SetBaudRate(const BaudRate baudrate)
        {
#ifdef __WIN32__
            m_PortSettings.BaudRate = baudrate;
            if (m_Descriptor != INVALID_HANDLE_VALUE) {
                // TODO implementa on the fly changes..
                ASSERT(false);
            }
#else
            cfsetospeed(&m_PortSettings, baudrate);
            cfsetispeed(&m_PortSettings, baudrate);
            if (m_Descriptor != -1) {

                if (tcsetattr(m_Descriptor, TCSANOW, &m_PortSettings) < 0) {
                    TRACE_L1("Error setting a new speed: %d", -errno);
                }
            }
#endif
        }
        void SendBreak()
        {
#ifdef __WIN32__
            // TODO: Implement a windows variant..
            ASSERT(false);
#else
            if (m_Descriptor != -1) {
                tcsendbreak(m_Descriptor, 0);
            }
#endif
        }

    private:
        void Opened()
        {
            m_State = SerialPort::OPEN | SerialPort::READ | SerialPort::WRITE;
            StateChange();
        }
        void Closed()
        {
            StateChange();
            m_State = 0;
        }
        uint32_t WaitForClosure(const uint32_t time) const;
#ifdef __WIN32__
        void Write(const uint16_t writtenBytes);
        void Read(const uint16_t readBytes);
#endif
#ifdef __LINUX__
        void Write();
        void Read();
        virtual IResource::handle Descriptor() const override
        {
            return (static_cast<IResource::handle>(m_Descriptor));
        }
        virtual uint16_t Events() override;
        virtual void Handle(const uint16_t events) override;
#endif

#ifdef __LINUX__
        void BufferAlignment(int descriptor);
#endif
#ifdef __WIN32__
        void BufferAlignment(HANDLE descriptor);
#endif
#ifdef __WIN32__
        inline HANDLE Descriptor()
        {
            return (m_Descriptor);
        }
#endif
    private:
        mutable CriticalSection m_syncAdmin;
        string m_PortName;
        volatile uint16_t m_State;
        uint16_t m_SendBufferSize;
        uint16_t m_ReceiveBufferSize;
        uint8_t* m_SendBuffer;
        uint8_t* m_ReceiveBuffer;
        uint16_t m_ReadBytes;
        uint16_t m_WriteBytes;
        uint16_t m_SendOffset;
        uint16_t m_SendBytes;

#ifdef __WIN32__
        HANDLE m_Descriptor;
        DCB m_PortSettings;
        uint8_t m_CharBuffer;
        mutable OVERLAPPED m_WriteInfo;
        mutable OVERLAPPED m_ReadInfo;
#endif

#ifdef __LINUX__
        int m_Descriptor;
        struct termios m_PortSettings;
#endif
    };
}
} // namespace Core

#endif // __SERIALPORT_H
