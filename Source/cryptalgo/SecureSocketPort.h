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

#pragma once

#include "Module.h"

struct ssl_st;
struct ssl_ctx_st;
struct ssl_method_st;
struct x509_st;
struct evp_pkey_st;
struct x509_store_st;

namespace Thunder {
namespace Crypto {

    class EXTERNAL Certificate {
    public:
        Certificate() = delete;
        Certificate& operator=(Certificate&&) = delete;
        Certificate& operator=(const Certificate&) = delete;

        Certificate(const x509_st* certificate);
        Certificate(const TCHAR fileName[]);
        Certificate(Certificate&& move) noexcept;
        Certificate(const Certificate& copy);
        ~Certificate();

    public:
        string Issuer() const;
        string Subject() const;
        Core::Time ValidFrom() const;
        Core::Time ValidTill() const;
        bool ValidHostname(const string& expectedHostname) const;

        inline operator const struct x509_st* () const {
            return (_certificate);
        }

    private:
        const x509_st* _certificate;
    };
    class EXTERNAL Key {
    public:
        Key() = delete;
        Key& operator=(Key&&) = delete;
        Key& operator=(const Key&) = delete;

        Key(const evp_pkey_st* key);
        Key(const string& fileName);
        Key(const string& fileName, const string& password);
        Key(Key&& move) noexcept;
        Key(const Key& copy);
        ~Key();

    public:
        inline operator const evp_pkey_st* () const {
            return (_key);
        }

    private:
        const evp_pkey_st* _key;
    };
    class EXTERNAL CertificateStore {
    public:
        CertificateStore& operator=(CertificateStore&&) = delete;
        CertificateStore& operator=(const CertificateStore&) = delete;

        CertificateStore();
        CertificateStore(CertificateStore&&) noexcept;
        CertificateStore(const CertificateStore&);
        CertificateStore(struct x509_store_st*);
        ~CertificateStore();

    public:
        static CertificateStore Default() {
            return (CertificateStore(_default));
        }
        void Add(const Certificate& cert);
        inline operator const x509_store_st* () const {
            return (_store);
        }


    private:
        struct x509_store_st* _store;
        static struct x509_store_st* _default;
    };
    class EXTERNAL SecureSocketPort : public Core::IResource {
    public:
        struct EXTERNAL IValidate {
            virtual ~IValidate() = default;

            // Client part, override custom validation
            virtual bool Validate(const Certificate&) const = 0;
        };

    private:
        class EXTERNAL Handler : public Core::SocketPort {
        private:
            enum state : uint8_t {
                EXCHANGE,
                OPEN,
                ERROR
            };

        public:
            Handler(Handler&&) = delete;
            Handler(const Handler&) = delete;
            Handler& operator=(Handler&&) = delete;
            Handler& operator=(const Handler&) = delete;

            Handler(SecureSocketPort& parent,
                const enumType socketType,
                const Core::NodeId& localNode,
                const Core::NodeId& remoteNode,
                const uint16_t sendBufferSize,
                const uint16_t receiveBufferSize,
                const uint32_t socketSendBufferSize,
                const uint32_t socketReceiveBufferSize);
            Handler(SecureSocketPort& parent,
                const enumType socketType,
                const SOCKET& connector,
                const Core::NodeId& remoteNode,
                const uint16_t sendBufferSize,
                const uint16_t receiveBufferSize,
                const uint32_t socketSendBufferSize,
                const uint32_t socketReceiveBufferSize);
            ~Handler();

        public:
            uint32_t Initialize() override;

            int32_t Read(uint8_t buffer[], const uint16_t length) const override;
            int32_t Write(const uint8_t buffer[], const uint16_t length) override;

            uint32_t Open(const uint32_t waitTime);
            uint32_t Close(const uint32_t waitTime);

            // Methods to extract and insert data into the socket buffers
            inline uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override {
                return (_parent.SendData(dataFrame, maxSendSize));
            }

            inline uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override {
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }

            // Signal a state change, Opened, Closed or Accepted
            inline void StateChange() override {
                Update();
            };
            inline void Validate(const IValidate*  callback) {
                Core::SocketPort::Lock();

                ASSERT((callback == nullptr) ^ (_callback == nullptr));

                _callback = callback;
                Core::SocketPort::Unlock();
            }
            uint32_t Certificate(const Crypto::Certificate& certificate, const Crypto::Key& key);
            uint32_t Root(const CertificateStore& store);

        private:
            void Update();
            void ValidateHandShake();
            void CreateContext(const struct ssl_method_st* method);
 
        private:
            SecureSocketPort& _parent;
            struct ssl_ctx_st* _context;
            struct ssl_st* _ssl;
            const IValidate* _callback;
            mutable state _handShaking;
        };

    public:
        SecureSocketPort(SecureSocketPort&&) = delete;
        SecureSocketPort(const SecureSocketPort&) = delete;
        SecureSocketPort& operator=(SecureSocketPort&&) = delete;
        SecureSocketPort& operator=(const SecureSocketPort&) = delete;

        SecureSocketPort(
            const Core::SocketPort::enumType socketType,
            const Core::NodeId& localNode,
            const Core::NodeId& remoteNode,
            const uint16_t sendBufferSize,
            const uint16_t receiveBufferSize)
            : _handler(*this, socketType, localNode, remoteNode, sendBufferSize, receiveBufferSize, sendBufferSize, receiveBufferSize) {
        }
        SecureSocketPort(
            const Core::SocketPort::enumType socketType,
            const Core::NodeId& localNode,
            const Core::NodeId& remoteNode,
            const uint16_t sendBufferSize,
            const uint16_t receiveBufferSize,
            const uint32_t socketSendBufferSize,
            const uint32_t socketReceiveBufferSize)
            : _handler(*this, socketType, localNode, remoteNode, sendBufferSize, receiveBufferSize, socketSendBufferSize, socketReceiveBufferSize) {
        }
        SecureSocketPort(
            const Core::SocketPort::enumType socketType,
            const SOCKET& connector,
            const Core::NodeId& remoteNode,
            const uint16_t sendBufferSize,
            const uint16_t receiveBufferSize)
            : _handler(*this, socketType, connector, remoteNode, sendBufferSize, receiveBufferSize, sendBufferSize, receiveBufferSize) {
        }
        SecureSocketPort(
            const Core::SocketPort::enumType socketType,
            const SOCKET& connector,
            const Core::NodeId& remoteNode,
            const uint16_t sendBufferSize,
            const uint16_t receiveBufferSize,
            const uint32_t socketSendBufferSize,
            const uint32_t socketReceiveBufferSize)
            : _handler(*this, socketType, connector, remoteNode, sendBufferSize, receiveBufferSize, socketSendBufferSize, socketReceiveBufferSize) {
        }

        ~SecureSocketPort() override;

    public:
        inline bool IsOpen() const
        {
            return (_handler.IsOpen());
        }
        inline bool IsClosed() const
        {
            return (_handler.IsClosed());
        }
        inline bool IsSuspended() const {
            return (_handler.IsSuspended());
        }
        inline bool HasError() const
        {
            return (_handler.HasError());
        }
        inline string LocalId() const
        {
            return (_handler.LocalId());
        }
        inline string RemoteId() const
        {
            return (_handler.RemoteId());
        }
        inline void LocalNode(const Core::NodeId& node)
        {
            _handler.LocalNode(node);
        }
        inline void RemoteNode(const Core::NodeId& node)
        {
            _handler.RemoteNode(node);
        }
        inline const Core::NodeId& RemoteNode() const
        {
            return (_handler.RemoteNode());
        }

        inline uint32_t Open(const uint32_t waitTime) {
            return(_handler.Open(waitTime));
        }
        inline uint32_t Close(const uint32_t waitTime) {
            return(_handler.Close(waitTime));
        }
        inline void Trigger() {
            _handler.Trigger();
        }
        inline void Validate(const IValidate* callback) {
            _handler.Validate(callback);
        }
        inline uint32_t Certificate(const Crypto::Certificate& certificate, const Crypto::Key& key) {
            return (_handler.Certificate(certificate, key));
        }
        inline uint32_t Root(const CertificateStore& store) {
            return (_handler.Root(store));
        }


        //
        // Core::IResource interface
        // ------------------------------------------------------------------------
        IResource::handle Descriptor() const override
        {
            return (static_cast<const Core::IResource&>(_handler).Descriptor());
        }
        uint16_t Events() override
        {
            return (static_cast<Core::IResource&>(_handler).Events());
        }
        void Handle(const uint16_t events) override
        {
            static_cast<Core::IResource&>(_handler).Handle(events);
        }

        // Methods to extract and insert data into the socket buffers
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) = 0;
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) = 0;

        // Signal a state change, Opened, Closed or Accepted
        virtual void StateChange() = 0;

    private:
        Handler _handler;
    };

    template <typename CLIENT>
    class SecureSocketServerType : public Core::SocketServerType<CLIENT> {
    public:
        SecureSocketServerType() = delete;
        SecureSocketServerType(SecureSocketServerType<CLIENT>&&) = delete;
        SecureSocketServerType(const SecureSocketServerType<CLIENT>&) = delete;
        SecureSocketServerType& operator=(SecureSocketServerType<CLIENT>&&) = delete;
        SecureSocketServerType& operator=(const SecureSocketServerType<CLIENT>&) = delete;

        SecureSocketServerType(const Certificate& certificate, const Key& key)
            : Core::SocketServerType<CLIENT>()
            , _certificate(certificate)
            , _key(key) {
        }
        SecureSocketServerType(const Certificate& certificate, const Key& key, const Core::NodeId& serverNode)
            : Core::SocketServerType<CLIENT>(serverNode)
            , _certificate(certificate)
            , _key(key) {
        }
        ~SecureSocketServerType() = default;

    public:
        const Crypto::Certificate& Certificate() const {
            return (_certificate);
        }
        const Crypto::Key& Key() const {
            return (_key);
        }

    private:
        Crypto::Certificate _certificate;
        Crypto::Key _key;
    };
}
}
