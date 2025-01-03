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

#include <string>

#include <openssl/ssl.h>

#include "Module.h"

namespace Thunder {
namespace Crypto {

    class EXTERNAL Certificate {
    public:
        Certificate() = delete;
        Certificate& operator=(Certificate&&) = delete;
        Certificate& operator=(const Certificate&) = delete;

        Certificate(const std::string& fileName);
        Certificate(Certificate&& move) noexcept;
        Certificate(const Certificate& copy);
        ~Certificate();

    public:
        const std::string Issuer() const;
        const std::string Subject() const;
        Core::Time ValidFrom() const;
        Core::Time ValidTill() const;
        bool ValidHostname(const std::string& expectedHostname) const;

    protected:
        Certificate(const void* certificate);
        operator const void* () const;

    private:
        mutable void* _certificate;
    };

    class EXTERNAL Key {
    public:
        Key() = delete;
        Key& operator=(Key&&) = delete;
        Key& operator=(const Key&) = delete;

        Key(const string& fileName);
        Key(const string& fileName, const string& password);
        Key(Key&& move) noexcept;
        Key(const Key& copy);
        ~Key();

    protected:
        Key(const EVP_PKEY* key);
        operator const EVP_PKEY* () const;

    private:
        mutable EVP_PKEY* _key;
    };
 
    class EXTERNAL CertificateStore {
    public:
        CertificateStore() = delete;
        CertificateStore& operator=(CertificateStore&&) = delete;
        CertificateStore& operator=(const CertificateStore&) = delete;

        CertificateStore(bool defaultStore);
        CertificateStore(CertificateStore&&) noexcept;
        CertificateStore(const CertificateStore&);
        ~CertificateStore();

    public:
        uint32_t Add(const Certificate& certificate);
        uint32_t Remove(const Certificate& certificate);

        bool IsDefaultStore() const;

    protected:

        operator X509_STORE* () const;
        operator STACK_OF(X509_NAME)* () const;

    private:
        uint32_t CreateDefaultStore();

        // (Extra) added certificates
        std::vector<Crypto::Certificate> _list;

        const bool _defaultStore;
    };

    class EXTERNAL SecureSocketPort : public Core::IResource {
    public:
        struct IValidate {
            virtual ~IValidate() = default;

            virtual bool Validate(const Certificate& certificate) const = 0;
        };

    private:
        class EXTERNAL Handler : public Core::SocketPort {
        private:
            enum state : uint8_t {
                ACCEPTING,
                CONNECTING,
                EXCHANGE,
                CONNECTED,
                ERROR
            };

        public:
            Handler(Handler&&) = delete;
            Handler(const Handler&) = delete;
            Handler& operator=(const Handler&) = delete;
            Handler& operator=(Handler&&) = delete;

            template <typename... Args>
            Handler(SecureSocketPort& parent, bool isClientSocketType, bool requestCert, Args&&... args)
                : Core::SocketPort(std::forward<Args>(args)...)
                , _parent(parent)
                , _context(nullptr)
                , _ssl(nullptr)
                , _callback(nullptr)
                , _handShaking{isClientSocketType ? CONNECTING : ACCEPTING}
                , _certificate{std::string{""}}
                , _privateKey{std::string{""}}
                , _requestCertificate{requestCert}
                , _waitTime{0}
                , _store{ true }
            {}
            ~Handler();

        public:
            uint32_t Initialize() override;

            int32_t Read(uint8_t buffer[], const uint16_t length) const override;
            int32_t Write(const uint8_t buffer[], const uint16_t length) override;

            uint32_t Open(const uint32_t waitTime);
            uint32_t Close(const uint32_t waitTime);

            // Methods to extract and insert data into the socket buffers
            uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override {
                return (_parent.SendData(dataFrame, maxSendSize));
            }

            uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override {
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }

            // Signal a state change, Opened, Closed or Accepted
            void StateChange() override {
                Update();
            }
            inline uint32_t Callback(IValidate* callback) {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                Core::SocketPort::Lock();

                ASSERT((callback == nullptr) || (_callback == nullptr));

                if ((callback == nullptr) || (_callback == nullptr)) {
                    _callback = callback;
                    result = Core::ERROR_NONE;
                }
                Core::SocketPort::Unlock();

                return (result);
            }
            uint32_t Certificate(const Crypto::Certificate& certificate, const Crypto::Key& key);
            uint32_t CustomStore( const CertificateStore& store);

        private:
            void Update();
            void ValidateHandShake();
            uint32_t EnableClientCertificateRequest();
 
        private:
            SecureSocketPort& _parent;
            void* _context;
            void* _ssl;
            IValidate* _callback;
            mutable state _handShaking;
            mutable Crypto::Certificate _certificate; // (PEM formatted ccertificate (chain)
            mutable Crypto::Key _privateKey; // (PEM formatted) private key
            const bool _requestCertificate;
            mutable uint32_t _waitTime; // Extracted from Open for use in I/O blocking operations
            CertificateStore _store;
        };

    public:
        SecureSocketPort(SecureSocketPort&&) = delete;
        SecureSocketPort(const SecureSocketPort&) = delete;
        SecureSocketPort& operator=(const SecureSocketPort&) = delete;

    protected:
        // Operational context
        enum class context_t : uint8_t {
              CLIENT_CONTEXT
            , SERVER_CONTEXT
        };

        template <typename... Args>
        SecureSocketPort(context_t context, Args&&... args)
            : SecureSocketPort(context, false, std::forward<Args>(args)...)
        {}

        template <typename... Args>
        SecureSocketPort(context_t context, bool requestPeerCert, Args&&... args)
            : _handler(*this, context == context_t::CLIENT_CONTEXT, requestPeerCert && context == context_t::SERVER_CONTEXT, std::forward<Args>(args)...)
        {}

    public:
        template <typename... Args>
        SecureSocketPort(Args&&... args)
            : SecureSocketPort(context_t::CLIENT_CONTEXT, std::forward<Args>(args)...)
        {}
        ~SecureSocketPort() override {
        }

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
        inline uint32_t Callback(IValidate* callback) {
            return (_handler.Callback(callback));
        }
        inline uint32_t Certificate(const Crypto::Certificate& certificate, const Crypto::Key& key) {
            return (_handler.Certificate(certificate, key));
        }
        inline uint32_t CustomStore(const CertificateStore& store) {
            return (_handler.CustomStore(store));
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
}
}
