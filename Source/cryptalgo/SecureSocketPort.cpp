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

#include "SecureSocketPort.h"

#include <openssl/ssl.h>

#ifndef __WINDOWS__
namespace {

    class OpenSSL {
    public:
        OpenSSL(const OpenSSL&) = delete;
        OpenSSL& operator= (const OpenSSL&) = delete;

        OpenSSL()
        {
            SSL_library_init();
            OpenSSL_add_all_algorithms();
            SSL_load_error_strings();
        }
        ~OpenSSL()
        {
        }
    };

    static OpenSSL _initialization;
}
#endif

namespace WPEFramework {

namespace Crypto {

SecureSocketPort::Handler::~Handler() {
    if(_ssl != nullptr) {
        SSL_free(static_cast<SSL*>(_ssl));
    }
    if(_context != nullptr) {
        SSL_CTX_free(static_cast<SSL_CTX*>(_context));
    }
}

bool SecureSocketPort::Handler::Initialize() {
    _context = SSL_CTX_new(SSLv23_method());
    
    _ssl = SSL_new(static_cast<SSL_CTX*>(_context));
    SSL_set_fd(static_cast<SSL*>(_ssl), static_cast<Core::IResource&>(*this).Descriptor());

    // Enable SNI by default (Server Name Indication) in case there's a host name set in the remote
    // node, so that the TLS "Client Hello" message contains a "server_name" extension with the host
    // name, this way allowing a better interoperability with TLS servers.
    if (!RemoteNode().RemoteHostName().empty()) {
        SSL_set_tlsext_host_name(static_cast<SSL*>(_ssl), RemoteNode().RemoteHostName().c_str());
    }

    return (Core::SocketPort::Initialize());
}

int32_t SecureSocketPort::Handler::Read(uint8_t buffer[], const uint16_t length) const {
    int32_t result = SSL_read(static_cast<SSL*>(_ssl), buffer, length);

    if (_handShaking != CONNECTED) {
        const_cast<Handler&>(*this).Update();
    }
    return (result);
}

int32_t SecureSocketPort::Handler::Write(const uint8_t buffer[], const uint16_t length) {
    return (SSL_write(static_cast<SSL*>(_ssl), buffer, length));
}

uint32_t SecureSocketPort::Handler::Close(const uint32_t waitTime) {
    SSL_shutdown(static_cast<SSL*>(_ssl));
    return(Core::SocketPort::Close(waitTime));
}

void SecureSocketPort::Handler::Update() {
    if (IsOpen() == true) {
        int result;

        if (_handShaking == IDLE) {
            result = SSL_connect(static_cast<SSL*>(_ssl));
            if (result == 1) {
                _handShaking = CONNECTED;
                _parent.StateChange();
            }
            else {
                result = SSL_get_error(static_cast<SSL*>(_ssl), result);
                if ((result == SSL_ERROR_WANT_READ) || (result == SSL_ERROR_WANT_WRITE)) {
                    _handShaking = EXCHANGE;
                }
            }
        }
        else if (_handShaking == EXCHANGE) {
            if (SSL_do_handshake(static_cast<SSL*>(_ssl)) == 1) {
                _handShaking = CONNECTED;
                _parent.StateChange();
            }
        }
    }
    else if (_ssl != nullptr) {
        _handShaking = IDLE;
        _parent.StateChange();
    }
}

} } // namespace WPEFramework::Crypto
