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

struct x509_store_ctx_st;
struct x509_st;
struct ssl_st;

namespace Thunder {
namespace Crypto {

    class EXTERNAL SecureSocketPort : public Core::IResource {
    public:
        class EXTERNAL Certificate {
        public:
            Certificate() = delete;
            Certificate(Certificate&&) = delete;
            Certificate(const Certificate&) = delete;

            Certificate(x509_st* certificate, const ssl_st* context)
                : _certificate(certificate)
                , _context(context) {
            }
            ~Certificate() = default;

        public:
            string Issuer() const;
            string Subject() const;
            Core::Time ValidFrom() const;
            Core::Time ValidTill() const;
            bool ValidHostname(const string& expectedHostname) const;
            bool Verify(string& errorMsg) const;

        private:
            x509_st* _certificate;
            const ssl_st* _context;
        };
        struct IValidator {
            virtual ~IValidator() = default;

            virtual bool Validate(const Certificate& certificate) const = 0;
        };

    private:
        class EXTERNAL Handler : public Core::SocketPort {
        private:
            enum state : uint8_t {
                IDLE,
                EXCHANGE,
                CONNECTED
            };

        public:
            Handler(Handler&&) = delete;
            Handler(const Handler&) = delete;
            Handler& operator=(const Handler&) = delete;

            template <typename... Args>
            Handler(SecureSocketPort& parent, Args&&... args)
                : Core::SocketPort(args...)
                , _parent(parent)
                , _context(nullptr)
                , _ssl(nullptr)
                , _callback(nullptr)
                , _handShaking(IDLE) {
            }
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

                ASSERT(_context != nullptr);
                Update();
            };
            inline uint32_t Callback(IValidator* callback) {
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

        private:
            void Update();
            void ValidateHandShake();
 
        private:
            SecureSocketPort& _parent;
            void* _context;
            void* _ssl;
            IValidator* _callback;
            mutable state _handShaking;
        };

    public:
        SecureSocketPort(SecureSocketPort&&) = delete;
        SecureSocketPort(const SecureSocketPort&) = delete;
        SecureSocketPort& operator=(const SecureSocketPort&) = delete;

        template <typename... Args>
        SecureSocketPort(Args&&... args)
            : _handler(*this, args...) {
        }
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
        inline uint32_t Callback(IValidator* callback) {
            return (_handler.Callback(callback));
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
