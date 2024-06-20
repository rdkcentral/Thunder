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
 
#ifndef __SOCKETSERVER_H
#define __SOCKETSERVER_H

#include "Module.h"
#include "Portability.h"
#include "Proxy.h"
#include "SocketPort.h"

namespace WPEFramework {
namespace Core {
    template <typename CLIENT>
    class SocketServerType {
    private:
        typedef std::map<uint32_t, Core::ProxyType<CLIENT>> ClientMap;

    public:
        template <typename HANDLECLIENT>
        class IteratorType {
        public:
            IteratorType()
                : _atHead(true)
                , _clients()
                , _iterator(_clients.begin())
            {
            }
            IteratorType(const ClientMap& container)
                : _atHead(true)
                , _clients()
                , _iterator()
            {

                typename ClientMap::const_iterator index(container.begin());

                while (index != container.end()) {
                    _clients.push_back(index->second);
                    index++;
                }
                _iterator = _clients.begin();
            }
            IteratorType(const IteratorType<HANDLECLIENT>& copy)
                : _atHead(true)
                , _clients(copy._clients)
                , _iterator(_clients.begin())
            {
            }
            ~IteratorType()
            {
            }

            IteratorType& operator=(const IteratorType<HANDLECLIENT>& RHS)
            {

                _atHead = RHS._atHead;
                _clients = RHS._clients;
                _iterator = RHS._iterator;
            }

        public:
            inline bool IsValid() const
            {
                return ((_atHead == false) && (_iterator != _clients.end()));
            }
            inline void Reset()
            {
                _atHead = true;
                _iterator = _clients->begin();
            }
            inline bool Next()
            {

                if (_atHead == true) {
                    _atHead = false;
                } else if (_iterator != _clients.end()) {
                    _iterator++;
                }

                return (_iterator != _clients.end());
            }
            inline uint32_t Count() const
            {
                return (_clients.size());
            }
            HANDLECLIENT Client()
            {

                ASSERT(IsValid() == true);

                return (*_iterator);
            }

        private:
            bool _atHead;
            typename std::list<HANDLECLIENT> _clients;
            typename std::list<HANDLECLIENT>::iterator _iterator;
        };

        typedef IteratorType<ProxyType<CLIENT>> Iterator;

    private:
        template <typename HANDLECLIENT>
        class SocketHandler : public SocketListner {
        private:
            SocketHandler() = delete;
            SocketHandler(const SocketHandler<HANDLECLIENT>&) = delete;
            SocketHandler<HANDLECLIENT>& operator=(const SocketHandler<HANDLECLIENT>&) = delete;

        public:
            SocketHandler(SocketServerType<CLIENT>* parent)
                : SocketListner()
                , _nextClient(1)
                , _lock()
                , _clients()
                , _parent(*parent)
            {

                ASSERT(parent != nullptr);
            }
            SocketHandler(const NodeId& listenNode, SocketServerType<CLIENT>* parent)
                : SocketListner(listenNode)
                , _nextClient(1)
                , _lock()
                , _clients()
                , _parent(*parent)
            {

                ASSERT(parent != nullptr);
            }
            ~SocketHandler()
            {
                SocketListner::Close(Core::infinite);
                CloseClients(0);

                _lock.Lock();

                while (_clients.size() > 0) {
                    ProxyType<HANDLECLIENT> client = _clients.begin()->second;

                    while (client->IsClosed() == false) {
                        SleepMs(10);
                    }

                    _clients.erase(_clients.begin());

                    client.Release();
                }

                _lock.Unlock();
            }

        public:
            inline uint32_t Count() const
            {
                return (static_cast<uint32_t>(_clients.size()));
            }
            template <typename PACKAGE>
            uint32_t Submit(const uint32_t ID, PACKAGE package)
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                _lock.Lock();

                typename ClientMap::iterator index = _clients.find(ID);

                if (index == _clients.end()) {
                    _lock.Unlock();
                }
                else {
                    // Oke connection still exists, send the message..
                    Core::ProxyType<HANDLECLIENT> client (index->second);
                    _lock.Unlock();

                    client->Submit(package);
                    client.Release();

                    result = Core::ERROR_NONE;
                }

                return (result);
            }
            inline Iterator Clients() const
            {
                _lock.Lock();
                Iterator result(_clients);
                _lock.Unlock();

                return (result);
            }
            inline void LocalNode(const Core::NodeId& localNode)
            {
                SocketListner::LocalNode(localNode);
            }
            inline Core::ProxyType<HANDLECLIENT> Client(const uint32_t ID)
            {
                Core::ProxyType<HANDLECLIENT> result;

                _lock.Lock();

                typename ClientMap::iterator index = _clients.find(ID);

                if (index != _clients.end()) {
                    // Oke connection still exists, send the message..
                    result = index->second;
                }

                _lock.Unlock();

                return (result);
            }
            inline void Suspend(const uint32_t ID)
            {
                _lock.Lock();

                typename ClientMap::iterator index = _clients.find(ID);

                if (index != _clients.end()) {
                    // Oke connection still exists, send the message..
                    index->second->Close(0);
                }

                _lock.Unlock();
            }
            inline void CloseClients(const uint32_t waiTime)
            {
                _lock.Lock();

                typename ClientMap::iterator index = _clients.begin();

                while (index != _clients.end()) {
                    // Oke connection still exists, send the message..
                    index->second->Close(waiTime);
                    ++index;
                }

                _lock.Unlock();
            }
            void Cleanup()
            {
                _lock.Lock();

                // Check if we can remove closed clients.
                typename ClientMap::iterator index = _clients.begin();

                while (index != _clients.end()) {
                    // Do not change the Close() duration to a value >0. We should just test, but not wait for a statechange.
                    // Waiting for a Statwchange might require, in the SocketPort imlementation of Close, WaitForCloseure with
                    // parameter Core::infinite in case we have a faulthy socket. This call will than only return if the 
                    // ResourceMonitor thread does report on CLosure of the socket. However, the ResourceMonitor thread might
                    // also be calling into here for an Accept. 
                    // In that case, the Accept will block on the _lock from this object as it is taken by this Cleanup call
                    // running on a different thread but also this lock will not be freed as this cleanup thread is waiting 
                    // now on the WaitForClosure that needs attention from the ResourceMonitor thread, which is currently 
                    // blocked by the Accpet, waiting for this lock ;-)
                    // By setting the Close wait time to 0, it wil never require the ReourceMonitor thread to participate 
                    // in the evaluatio of this socket state and thus in due time, the Server lock is *always* released.
                    if ((index->second->IsClosed() == true) || ((index->second->IsSuspended() == true) && (index->second->Close(0) == Core::ERROR_NONE))) {
                        // Step forward but remember where we were and delete that one....
                        index = _clients.erase(index);
                    }
                    else {
                        index++;
                    }
                }

                _lock.Unlock();
            }
            virtual void Accept(SOCKET& newClient, const NodeId& remoteId)
            {
                ProxyType<HANDLECLIENT> client = ProxyType<HANDLECLIENT>::Create(newClient, remoteId, &_parent);

                ASSERT(client.IsValid() == true);

                // What is left, is opening up the socket, make sure the administration is coorect :-)
                if (client->Open(0) == ERROR_NONE) {

                    _lock.Lock();

                    // Check if we can remove closed clients.
                    typename ClientMap::iterator index = _clients.begin();

                    while (index != _clients.end()) {
                        if (index->second->IsClosed() == true) {
                            index = _clients.erase(index);
                        }
                        else {
                            ++index;
                        }
                    }

                    // If the CLient has a method to receive it's Id pass it on..
                    __Id(*client, _nextClient);

                    // A new connection is available, open up a new client
                    _clients.insert(std::pair<uint32_t, ProxyType<HANDLECLIENT>>(_nextClient++, client));

                    _lock.Unlock();
                }
            }
			void Lock() 
			{
                _lock.Lock();
			}
            void Unlock()
            {
                _lock.Unlock();
            }

        private:
            // -----------------------------------------------------
            // Check for Id  method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE(Id, hasId);

            template <typename SUBJECT=HANDLECLIENT>
            inline typename Core::TypeTraits::enable_if<hasId<SUBJECT, void, uint32_t>::value, void>::type
            __Id(HANDLECLIENT& object, const uint32_t id)
            {
                object.Id(id);
            }

            template <typename SUBJECT=HANDLECLIENT>
            inline typename Core::TypeTraits::enable_if<!hasId<SUBJECT, void, uint32_t>::value, void>::type
            __Id(HANDLECLIENT&, const uint32_t)
            {
            }

        private:
            uint32_t _nextClient;
            mutable Core::CriticalSection _lock;
            std::map<uint32_t, ProxyType<HANDLECLIENT>> _clients;
            SocketServerType<CLIENT>& _parent;
        };

        SocketServerType(const SocketServerType<CLIENT>&) = delete;
        SocketServerType<CLIENT>& operator=(const SocketServerType<CLIENT>&) = delete;

    public:
PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        SocketServerType()
            : _handler(this)
        {
        }
        SocketServerType(const NodeId& listeningNode)
            : _handler(listeningNode, this)
        {
        }
POP_WARNING()
        ~SocketServerType()
        {
        }

    public:
        inline uint32_t Open(const uint32_t waitTime)
        {
            return (_handler.Open(waitTime));
        }
        inline uint32_t Close(const uint32_t waitTime)
        {
            uint32_t result = _handler.Close(waitTime);
            _handler.CloseClients(waitTime);
            return (result);
        }
        inline void Cleanup()
        {
            _handler.Cleanup();
        }
        inline Core::ProxyType<CLIENT> Client(const uint32_t ID)
        {
            return (_handler.Client(ID));
        }
        inline void Suspend(const uint32_t ID)
        {
            return (_handler.Suspend(ID));
        }
        inline void LocalNode(const Core::NodeId& localNode)
        {
            _handler.LocalNode(localNode);
        }
        template <typename PACKAGE>
        inline uint32_t Submit(const uint32_t ID, PACKAGE package)
        {
            return (_handler.Submit(ID, package));
        }
        inline Iterator Clients() const
        {
            return (_handler.Clients());
        }
        inline uint32_t Count() const
        {
            return (_handler.Count());
        }
        void Lock()
        {
            _handler.Lock();
        }
        void Unlock()
        {
            _handler.Unlock();
        }

    private:
        SocketHandler<CLIENT> _handler;
    };
}
} // namespace Core

#endif // __SOCKETSERVER_H
