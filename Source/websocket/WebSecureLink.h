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

#pragma once

#include "Module.h"

#include <openssl/ssl.h>

namespace WPEFramework {
namespace Web {

    class SecureSocketPort : public Core::IResource {
    private:
        class Handler : public Core::SocketPort {
        public:
            Handler(const Handler&) = delete;
            Handler& operator=(const Handler&) = delete;

            template <typename... Args>
            Handler(SecureSocketPort& parent, Args&&... args)
                : Core::SocketPort(args...) 
                , _parent(parent)
                , _context(SSL_CTX_new(TLSv1_2_client_method()))
                , _ssl(nullptr) {
            }
            ~Handler() {
                if(_ssl != nullptr) {
                    SSL_free(_ssl);
                }
                if(_context != nullptr) {
                    SSL_CTX_free(_context);
                }
            }

        public:
            bool Initialize() override {
                if(_ssl != nullptr) {
                    SSL_free(_ssl);
                    _ssl = nullptr;
                }
                return (_context != nullptr);
            }
            int32_t Read(uint8_t buffer[], const uint16_t length) const override {
                return (SSL_read(_ssl, buffer, length));
            }
            int32_t Write(const uint8_t buffer[], const uint16_t length) override {
                return (SSL_write(_ssl, buffer, length));
            }

            // Methods to extract and insert data into the socket buffers
            uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override {
                return (_parent.SendData(dataFrame, maxSendSize));
            }

            uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override {
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }

            // Signal a state change, Opened, Closed or Accepted
            void StateChange() override {

                ASSERT (_context != nullptr);

                if (IsOpen() == true) {

                    if (_ssl == nullptr) {
                        _ssl = SSL_new(_context);
                        SSL_set_fd(_ssl, static_cast<Core::IResource&>(*this).Descriptor());
                    }
                    if (SSL_connect(_ssl) == -1) {
                        Close(0);
                    }
                }
                else if (_ssl != nullptr) {
                    SSL_shutdown(_ssl);
                }

                _parent.StateChange();
            }

        private:
            SecureSocketPort& _parent;
            SSL_CTX* _context;
            SSL* _ssl;
        };

    public:
        SecureSocketPort(const SecureSocketPort&);
        SecureSocketPort& operator=(const SecureSocketPort&);

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
