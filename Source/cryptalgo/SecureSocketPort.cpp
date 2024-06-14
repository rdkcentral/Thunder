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
#include <openssl/x509v3.h>

#ifndef __WINDOWS__
namespace {

    class OpenSSL {
    public:
        OpenSSL(OpenSSL&&) = delete;
        OpenSSL(const OpenSSL&) = delete;
        OpenSSL& operator=(OpenSSL&&) = delete;
        OpenSSL& operator=(const OpenSSL&) = delete;

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

namespace Thunder {

namespace Crypto {

    static Core::Time ASN1_ToTime(const ASN1_TIME* input)
    {
        Core::Time result;

        if (input != nullptr) {
            uint16_t year = 0;
            const char* textVersion = reinterpret_cast<const char*>(input->data);

            if (input->type == V_ASN1_UTCTIME)
            {
                year = (textVersion[0] - '0') * 10 + (textVersion[1] - '0');
                year += (year < 70 ? 2000 : 1900);
                textVersion = &textVersion[2];
            }
            else if (input->type == V_ASN1_GENERALIZEDTIME)
            {
                year = (textVersion[0] - '0') * 1000 + (textVersion[1] - '0') * 100 + (textVersion[2] - '0') * 10 + (textVersion[3] - '0');
                textVersion = &textVersion[4];
            }
            uint8_t month = ((textVersion[0] - '0') * 10 + (textVersion[1] - '0')) - 1;
            uint8_t day = (textVersion[2] - '0') * 10 + (textVersion[3] - '0');
            uint8_t hour = (textVersion[4] - '0') * 10 + (textVersion[5] - '0');
            uint8_t minutes = (textVersion[6] - '0') * 10 + (textVersion[7] - '0');
            uint8_t seconds = (textVersion[8] - '0') * 10 + (textVersion[9] - '0');

            /* Note: we did not adjust the time based on time zone information */
            result = Core::Time(year, month, day, hour, minutes, seconds, 0, false);
        }
        return (result);
    }

string SecureSocketPort::Certificate::Issuer() const {
    char buffer[1024];
    buffer[0] = '\0';
    X509_NAME_oneline(X509_get_issuer_name(_certificate), buffer, sizeof(buffer));

    return (string(buffer));
}

string SecureSocketPort::Certificate::Subject() const {
    char buffer[1024];
    buffer[0] = '\0';
    X509_NAME_oneline(X509_get_subject_name(_certificate), buffer, sizeof(buffer));

    return (string(buffer));
}

Core::Time SecureSocketPort::Certificate::ValidFrom() const {
    return(ASN1_ToTime(X509_get0_notBefore(_certificate)));
}

Core::Time SecureSocketPort::Certificate::ValidTill() const {
    return(ASN1_ToTime(X509_get0_notAfter(_certificate)));
}

bool SecureSocketPort::Certificate::ValidHostname(const string& expectedHostname) const {
    return (X509_check_host(_certificate, expectedHostname.data(), expectedHostname.size(), 0, nullptr) == 1);
}

bool SecureSocketPort::Certificate::Verify(string& errorMsg) const {
    long error = SSL_get_verify_result(_context);

    if (error != X509_V_OK) {
        errorMsg = X509_verify_cert_error_string(error);
    }

    return error == X509_V_OK;
}


SecureSocketPort::Handler::~Handler() {
    if(_ssl != nullptr) {
        SSL_free(static_cast<SSL*>(_ssl));
    }
    if(_context != nullptr) {
        SSL_CTX_free(static_cast<SSL_CTX*>(_context));
    }
}

uint32_t SecureSocketPort::Handler::Initialize() {
    uint32_t success = Core::ERROR_NONE;

    _context = SSL_CTX_new(TLS_method());

    _ssl = SSL_new(static_cast<SSL_CTX*>(_context));
    SSL_set_fd(static_cast<SSL*>(_ssl), static_cast<Core::IResource&>(*this).Descriptor());
    SSL_CTX_set_options(static_cast<SSL_CTX*>(_context), SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

    // Trust the same certificates as any other application
    if (SSL_CTX_set_default_verify_paths(static_cast<SSL_CTX*>(_context)) == 1) {
        success = Core::SocketPort::Initialize();
    } else {
        TRACE_L1("OpenSSL failed to load certificate store");
        success = Core::ERROR_GENERAL;
    }

    return success;
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


uint32_t SecureSocketPort::Handler::Open(const uint32_t waitTime) {
    return (Core::SocketPort::Open(waitTime));
}

uint32_t SecureSocketPort::Handler::Close(const uint32_t waitTime) {
    if (_ssl != nullptr) {
        SSL_shutdown(static_cast<SSL*>(_ssl));
    }
    _handShaking = IDLE;
    return(Core::SocketPort::Close(waitTime));
}

void SecureSocketPort::Handler::ValidateHandShake() {
    // Step 1: verify a server certificate was presented during the negotiation
    X509* x509cert = SSL_get_peer_certificate(static_cast<SSL*>(_ssl));
    if (x509cert != nullptr) {
        Core::SocketPort::Lock();

        Certificate certificate(x509cert, static_cast<SSL*>(_ssl));

        // Step 2: Validate certificate - use custom IValidator instance if available or if self signed
        // certificates are needed :-)
        string validationError;
        if (_callback && _callback->Validate(certificate) == true) {
            _handShaking = CONNECTED;
            Core::SocketPort::Unlock();
            _parent.StateChange();
        } else if (certificate.Verify(validationError) && certificate.ValidHostname(RemoteNode().HostName())) {
            _handShaking = CONNECTED;
            Core::SocketPort::Unlock();\
            _parent.StateChange();
        } else {
            if (!validationError.empty()) {
                TRACE_L1("OpenSSL certificate validation error for %s: %s", certificate.Subject().c_str(), validationError.c_str());
            }
            _handShaking = IDLE;
            Core::SocketPort::Unlock();
            SetError();
        }

        X509_free(x509cert);
    } else {
        _handShaking = IDLE;
        SetError();
    }
}

void SecureSocketPort::Handler::Update() {
    if (IsOpen() == true) {
        int result;

        if (_handShaking == IDLE) {
            SSL_set_tlsext_host_name(static_cast<SSL*>(_ssl), RemoteNode().HostName().c_str());
            result = SSL_connect(static_cast<SSL*>(_ssl));
            if (result == 1) {
                ValidateHandShake();
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
                ValidateHandShake();
            }
        }
    }
    else if (_ssl != nullptr) {
        _handShaking = IDLE;
        _parent.StateChange();
    }
}

} } // namespace Thunder::Crypto
