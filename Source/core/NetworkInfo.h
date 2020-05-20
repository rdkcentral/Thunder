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
 
#ifndef __NETWORKINFO_H
#define __NETWORKINFO_H

#include "Module.h"
#include "Portability.h"
#include "Netlink.h"
#include "NodeId.h"
#include "Portability.h"
#include "SocketPort.h"

namespace WPEFramework {
namespace Core {
    class EXTERNAL IPV4AddressIterator {
    public:
        inline IPV4AddressIterator()
            : _adapter(static_cast<uint16_t>(~0))
            , _index(static_cast<uint16_t>(~0))
#ifdef __WINDOWS__
            , _section1(0)
            , _section2(0)
            , _section3(0)
#else
            , _count(0)
#endif
        {
        }
        IPV4AddressIterator(const uint16_t adapter);
        inline IPV4AddressIterator(const IPV4AddressIterator& copy)
            : _adapter(copy._adapter)
            , _index(copy._index)
#ifdef __WINDOWS__
            , _section1(copy._section1)
            , _section2(copy._section2)
            , _section3(copy._section3)
#else
            , _count(copy._count)
#endif
        {
        }
        inline ~IPV4AddressIterator()
        {
        }

        inline IPV4AddressIterator& operator=(const IPV4AddressIterator& RHS)
        {
            _adapter = RHS._adapter;
            _index = RHS._index;
#ifdef __WINDOWS__
            _section1 = RHS._section1;
            _section2 = RHS._section2;
            _section3 = RHS._section3;
#else
            _count = RHS._count;
#endif
            return (*this);
        }

    public:
        inline bool IsValid() const
        {
            return (_index < Count());
        }
        inline void Reset()
        {
            _index = static_cast<uint16_t>(~0);
        }
        inline bool Next()
        {
            if (_index == static_cast<uint16_t>(~0)) {
                _index = 0;
            } else if (_index < Count()) {
                _index++;
            }

            return (IsValid());
        }
        inline uint16_t Count() const
        {
#ifdef __WINDOWS__
            return (_section3);
#else
            return (_count);
#endif
        }
        IPNode Address() const;

    private:
        uint16_t _adapter;
        uint16_t _index;
#ifdef __WINDOWS__
        uint16_t _section1;
        uint16_t _section2;
        uint16_t _section3;
#else
        uint16_t _count;
#endif
    };

    class EXTERNAL IPV6AddressIterator {
    public:
        inline IPV6AddressIterator()
            : _adapter(static_cast<uint16_t>(~0))
            , _index(static_cast<uint16_t>(~0))
#ifdef __WINDOWS__
            , _section1(0)
            , _section2(0)
            , _section3(0)
#else
            , _count(0)
#endif
        {
        }
        IPV6AddressIterator(const uint16_t adapter);
        inline IPV6AddressIterator(const IPV6AddressIterator& copy)
            : _adapter(copy._adapter)
            , _index(copy._index)
#ifdef __WINDOWS__
            , _section1(copy._section1)
            , _section2(copy._section2)
            , _section3(copy._section3)
#else
            , _count(copy._count)
#endif
        {
        }
        inline ~IPV6AddressIterator()
        {
        }

        inline IPV6AddressIterator& operator=(const IPV6AddressIterator& RHS)
        {
            _adapter = RHS._adapter;
            _index = RHS._index;
#ifdef __WINDOWS__
            _section1 = RHS._section1;
            _section2 = RHS._section2;
            _section3 = RHS._section3;
#else
            _count = RHS._count;
#endif
            return (*this);
        }

    public:
        inline bool IsValid() const
        {
            return (_index < Count());
        }
        inline void Reset()
        {
            _index = static_cast<uint16_t>(~0);
        }
        inline bool Next()
        {
            if (_index == static_cast<uint16_t>(~0)) {
                _index = 0;
            } else if (_index < Count()) {
                _index++;
            }

            return (IsValid());
        }
        inline uint16_t Count() const
        {
#ifdef __WINDOWS__
            return (_section3);
#else
            return (_count);
#endif
        }
        IPNode Address() const;

    private:
        uint16_t _adapter;
        uint16_t _index;
#ifdef __WINDOWS__
        uint16_t _section1;
        uint16_t _section2;
        uint16_t _section3;
#else
        uint16_t _count;
#endif
    };

    class EXTERNAL AdapterIterator {
    public:
        inline AdapterIterator()
            : _index(static_cast<uint16_t>(~0))
        {
        }
        inline AdapterIterator(const string& name)
            : _index(static_cast<uint16_t>(~0))
        {
            while ((Next() == true) && (Name() != name)) /* intentionally left blank */
                ;

            if (IsValid() == false) {
                Reset();
            }
        }
        inline AdapterIterator(const AdapterIterator& copy)
            : _index(copy._index)
        {
        }
        inline ~AdapterIterator()
        {
        }

        inline AdapterIterator& operator=(const AdapterIterator& RHS)
        {
            _index = RHS._index;
            return (*this);
        }

    public:
        inline uint16_t Index() const
        {
            return (_index);
        }
        inline bool IsValid() const
        {
            return (_index < Count());
        }
        inline void Reset()
        {
            _index = static_cast<uint16_t>(~0);
        }
        inline bool Next()
        {
            if (_index == static_cast<uint16_t>(~0)) {
                _index = 0;
            } else if (_index < Count()) {
                _index++;
            }

            return (IsValid());
        }
        inline IPV4AddressIterator IPV4Addresses() const
        {
            return (IPV4AddressIterator(_index));
        }
        inline IPV6AddressIterator IPV6Addresses() const
        {
            return (IPV6AddressIterator(_index));
        }
        bool IsUp() const;
        bool IsRunning() const;
        uint32_t Up(const bool enabled);

        static void Flush();
        uint16_t Count() const;
        string Name() const;

        string MACAddress(const char delimiter) const;
        void MACAddress(uint8_t buffer[], const uint8_t length) const;

        uint32_t Add(const IPNode& address);
        uint32_t Delete(const IPNode& address);
        uint32_t Gateway(const IPNode& network, const NodeId& gateway);
        uint32_t Broadcast(const Core::NodeId& address);

    private:
        uint16_t _index;
    };

    struct INetworkNotification {
        virtual ~INetworkNotification() {}

        virtual void Event(const string& interface) = 0;
    };

    class EXTERNAL AdapterObserver {
    private:
        AdapterObserver() = delete;
        AdapterObserver(const AdapterObserver&) = delete;
        AdapterObserver& operator=(const AdapterObserver&) = delete;

    public:
        AdapterObserver(INetworkNotification* callback);
        ~AdapterObserver();

    private:
        INetworkNotification* _callback;
    };
}
}

#endif // __NETWORKINFO_H
