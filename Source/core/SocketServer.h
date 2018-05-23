#ifndef __SOCKETSERVER_H
#define __SOCKETSERVER_H

#include "Module.h"
#include "Portability.h"
#include "SocketPort.h"
#include "Proxy.h"

namespace WPEFramework {
namespace Core {
    template <typename CLIENT>
    class SocketServerType {
    private:
        typedef std::map<uint32_t, Core::ProxyType<CLIENT> > ClientMap;

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
                }
                else if (_iterator != _clients.end()) {
                    _iterator++;
                }

                return (_iterator != _clients.end());
            }
            inline uint32_t Count() const
            {
                return (_clients->size());
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

        typedef IteratorType<ProxyType<CLIENT> > Iterator;

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
                Close(Core::infinite);
                CloseClients();

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
                return (_clients.size());
            }
            template <typename PACKAGE>
            void Submit(const uint32_t ID, PACKAGE package)
            {
                _lock.Lock();

                typename ClientMap::iterator index = _clients.find(ID);

                if (index != _clients.end()) {
                    // Oke connection still exists, send the message..
                    index->second->Submit(package);
                }

                _lock.Unlock();
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
            inline void CloseClients()
            {
                _lock.Lock();

                typename ClientMap::iterator index = _clients.begin();

                while (index != _clients.end()) {
                    // Oke connection still exists, send the message..
                    index->second->Close(0);
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
                    if ((index->second->IsClosed() == true) || ((index->second->IsSuspended() == true) && (index->second->Close(100) == Core::ERROR_NONE))) {
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

                    // If the CLient has a method to receive it's Id pass it on..
                    __Id<HANDLECLIENT>(*client, _nextClient);

                    // A new connection is available, open up a new client
                    _clients.insert(std::pair<uint32_t, ProxyType<HANDLECLIENT> >(_nextClient++, client));

                    _lock.Unlock();
                }
            }

        private:
            // -----------------------------------------------------
            // Check for Id  method on Object
            // -----------------------------------------------------
            HAS_MEMBER(Id, hasId);

            typedef hasId<HANDLECLIENT, void (HANDLECLIENT::*)(uint32_t)> TraitId;

            template <typename SUBJECT>
            inline typename Core::TypeTraits::enable_if<SocketHandler<SUBJECT>::TraitId::value, void>::type
            __Id(HANDLECLIENT& object, const uint32_t id)
            {
                object.Id(id);
            }

            template <typename SUBJECT>
            inline typename Core::TypeTraits::enable_if<!SocketHandler<SUBJECT>::TraitId::value, void>::type
            __Id(HANDLECLIENT&, const uint32_t)
            {
            }

        private:
            uint32_t _nextClient;
            mutable Core::CriticalSection _lock;
            std::map<uint32_t, ProxyType<HANDLECLIENT> > _clients;
            SocketServerType<CLIENT>& _parent;
        };

        SocketServerType(const SocketServerType<CLIENT>&) = delete;
        SocketServerType<CLIENT>& operator=(const SocketServerType<CLIENT>&) = delete;

    public:
		#ifdef __WIN32__ 
		#pragma warning( disable : 4355 )
		#endif
        SocketServerType()
            : _handler(this)
        {
        }
        SocketServerType(const NodeId& listeningNode)
            : _handler(listeningNode, this)
        {
		}
		#ifdef __WIN32__ 
		#pragma warning( default : 4355 )
		#endif
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
            _handler.CloseClients();
            return (result);
        }
        inline void Cleanup()
        {
            _handler.Cleanup();
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
        inline void Submit(const uint32_t ID, PACKAGE package)
        {
            _handler.Submit(ID, package);
        }
        inline Iterator Clients() const
        {
            return (_handler.Clients());
        }
        inline uint32_t Count() const
        {
            return (_handler.Count());
        }

    private:
        SocketHandler<CLIENT> _handler;
    };
}
} // namespace Core

#endif // __SOCKETSERVER_H
