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
#include <openssl/crypto.h>

#include <unordered_map>

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


// Placeholder for custom data 'appended' to SSL structures and sharing within the OpenSSL API
class ApplicationData {
public :

    ApplicationData(const ApplicationData&) = delete;
    ApplicationData( ApplicationData&&) = delete;

    ApplicationData& operator=(const ApplicationData&) = delete;
    ApplicationData& operator=(ApplicationData&&) = delete;

    static ApplicationData& Instance()
    {
        static ApplicationData data;
        return data;
    }

    int Index(const SSL* const ssl, int index)
    {
        ASSERT(index != -1 && ssl != nullptr);

        int result = -1;

        _lock.Lock();

        result = _map.insert({ssl, index}).second ? index : -1;

        _lock.Unlock();

        return result;
    }

    int Index(const SSL* const ssl) const
    {
        ASSERT(ssl != nullptr);

        int result = -1;

        _lock.Lock();

        auto it = _map.find(ssl);

        result = it != _map.end() ? it->second : -1;

        _lock.Unlock();

        return result;
    }

    bool Reset(const SSL* const ssl)
    {
        ASSERT(ssl != nullptr);

        bool result = false;

        _lock.Lock();

        result = _map.erase(ssl) == 1;

        _lock.Unlock();

        return result;
    }

private :

    ApplicationData()
    {}
    ~ApplicationData() = default;

    std::unordered_map<const SSL*, int> _map;

    mutable Core::CriticalSection _lock;
};

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
    ASSERT(IsClosed() == true);
    Close(0);
}

uint32_t SecureSocketPort::Handler::Initialize() {
    uint32_t success = Core::ERROR_NONE;

    // Client and server use
    _context = SSL_CTX_new(TLS_method());

    ASSERT(_context != nullptr);

    constexpr uint64_t options = SSL_OP_ALL | SSL_OP_NO_SSLv2;

    VARIABLE_IS_NOT_USED uint64_t bitmask = SSL_CTX_set_options(static_cast<SSL_CTX*>(_context), options);

    ASSERT((bitmask & options) == options);

    int exDataIndex = -1;

    if ( ( (    (_handShaking == ACCEPTING)
              // Load server certifiate and private key
             && !_certificatePath.empty()
             && (SSL_CTX_use_certificate_chain_file(static_cast<SSL_CTX*>(_context), _certificatePath.c_str()) == 1)
             && !_privateKeyPath.empty()
             && (SSL_CTX_use_PrivateKey_file(static_cast<SSL_CTX*>(_context), _privateKeyPath.c_str(), SSL_FILETYPE_PEM) == 1)
             && (EnableClientCertificateRequest() == Core::ERROR_NONE)
           )
           || 
           (    (_handShaking == CONNECTING)
             && (
                  _certificatePath.empty()
                  ||
                  (    !_certificatePath.empty()
                    && (SSL_CTX_use_certificate_file(static_cast<SSL_CTX*>(_context), _certificatePath.c_str(), SSL_FILETYPE_PEM) == 1)
                  ) 
                )
             && (
                  _privateKeyPath.empty()
                  ||
                  (   !_privateKeyPath.empty()
                   && (SSL_CTX_use_PrivateKey_file(static_cast<SSL_CTX*>(_context), _privateKeyPath.c_str(), SSL_FILETYPE_PEM) == 1)
                  )
                )
           )
         )
            // Default location from which CA certificates are loaded
         && (SSL_CTX_set_default_verify_paths(static_cast<SSL_CTX*>(_context)) == 1)
            // Create a new SSL connection structure for storing the custom certification method for use in the available callback mechanism
         && ((_ssl = SSL_new(static_cast<SSL_CTX*>(_context))) != nullptr)
         && (SSL_set_fd(static_cast<SSL*>(_ssl), static_cast<Core::IResource&>(*this).Descriptor()) == 1)
         && ((exDataIndex = SSL_get_ex_new_index(/* number of callback arguments */ 0, /* pointer to callback arguments */ nullptr, /* allocation and initialization of exdata */ nullptr, /* duplication of exdata in copy operations */ nullptr, /* deallocaton of exdata */ nullptr)) != -1)
            // The custom method to do validation of certificates with issues
         && (SSL_set_ex_data(static_cast<SSL*>(_ssl), exDataIndex, _callback) == 1)
       ) {
            // Placeholder to refer to the validation method within the context of SSL connection
            ApplicationData::Instance().Index(static_cast<SSL*>(_ssl), exDataIndex);

            success = Core::SocketPort::Initialize();
    } else {
        TRACE_L1("OpenSSL failed to initialize: ssl structure / certificate store");
        success = Core::ERROR_GENERAL;
    }

    return success;
}

int32_t SecureSocketPort::Handler::Read(uint8_t buffer[], const uint16_t length) const {
    ASSERT(_handShaking == CONNECTED);

    ASSERT(_ssl != nullptr);
    int result = SSL_read(static_cast<SSL*>(_ssl), buffer, length);

    return (result > 0 ? result : /* error */ -1);
}

int32_t SecureSocketPort::Handler::Write(const uint8_t buffer[], const uint16_t length) {
    ASSERT(_handShaking == CONNECTED);

    ASSERT(_ssl != nullptr);
    int result = SSL_write(static_cast<SSL*>(_ssl), buffer, length);

    return (result > 0 ? result : /* error */ -1);
}


uint32_t SecureSocketPort::Handler::Open(const uint32_t waitTime) {
    return (Core::SocketPort::Open(waitTime));
}

uint32_t SecureSocketPort::Handler::Close(const uint32_t waitTime) {
    if (_ssl != nullptr) {
        SSL_shutdown(static_cast<SSL*>(_ssl));
        SSL_free(static_cast<SSL*>(_ssl));

        /* bool */ ApplicationData::Instance().Reset(static_cast<SSL*>(_ssl));

        _ssl = nullptr;
    }
    if (_context != nullptr) {
        SSL_CTX_free(static_cast<SSL_CTX*>(_context));
        _context = nullptr;
    }

    return(Core::SocketPort::Close(waitTime));
}

void SecureSocketPort::Handler::ValidateHandShake() {
    ASSERT(_ssl != nullptr && _context != nullptr);

    // SSL handshake does an implicit verification, its result is:
    if (SSL_get_verify_result(static_cast<SSL*>(_ssl)) != X509_V_OK) { // this is what remains because our callback can also return, actually it is the smae as verify_cb!
        _handShaking = ERROR;
        SetError();
    }
}

extern "C"
{
int VerifyCallbackWrapper(int checkOK, X509_STORE_CTX* ctx);
int PeerCertificateCallbackWrapper(X509_STORE_CTX* ctx, void* arg);
}

int VerifyCallbackWrapper(int verifyStatus, X509_STORE_CTX* ctx)
{ // This is callled for certificates with issues to allow a custom validation
    int result { verifyStatus };

    switch(verifyStatus) {
    case 0  :   {
                    X509* x509Cert = nullptr;
                    int exDataIndex = -1;
                    SSL* ssl = nullptr;
                    SecureSocketPort::IValidator* validator = nullptr;

                    // Retireve and call the registered callback

                    if (   ctx != nullptr
                         && (exDataIndex = SSL_get_ex_data_X509_STORE_CTX_idx()) != -1
                         && (ssl = static_cast<SSL*>(X509_STORE_CTX_get_ex_data(ctx, exDataIndex))) != nullptr
                         && (exDataIndex = ApplicationData::Instance().Index(static_cast<SSL*>(ssl))) != -1
                         && (validator =  static_cast<SecureSocketPort::IValidator*>(SSL_get_ex_data(ssl, exDataIndex))) != nullptr
                         && (x509Cert = X509_STORE_CTX_get_current_cert(ctx)) != nullptr
                       ) {
                        X509_up_ref(x509Cert);

                        SecureSocketPort::Certificate certificate(x509Cert, static_cast<SSL*>(ssl));

                        result = validator->Validate(certificate);

                        X509_free(x509Cert);
                    }

                    break;
                }
    case 1  :   // No error
                break;
    default :   ASSERT(false); // Not within set of defined values
    }

    return result; // 0 - Failurre, 1 - OK
}

int PeerCertificateCallbackWrapper(X509_STORE_CTX* ctx, void* arg)
{ // This is called if the complete certificate validation procedure has its custom implementation
  // This is typically not what is intended or required, and, a complex to do right

    ASSERT(false);

    return 0; // 0 - Failurre, 1 - OK
}

uint32_t SecureSocketPort::Handler::EnableClientCertificateRequest()
{
    uint32_t result{Core::ERROR_NONE};

    if (_requestCertificate) {
        STACK_OF(X509_NAME)* nameList = _certificatePath.empty() ? nullptr : SSL_load_client_CA_file(_certificatePath.c_str());
        const std::string paths = X509_get_default_cert_dir();

        std::string::size_type head = paths.empty() ? std::string::npos : 0;

        // OPENSSL_info requires at least version 3.0

        static_assert(   ((OPENSSL_VERSION_NUMBER >> 28) & 0xF) >= 3  // Major
                      && ((OPENSSL_VERSION_NUMBER >> 20) & 0xFF) >= 0 // Minor
                      && ((OPENSSL_VERSION_NUMBER >> 4) & 0xF) >= 0   // Patch
                      && ((OPENSSL_VERSION_NUMBER) & 0xF) >= 0        // Pre-release
                      , "OpenSSL version unsupported. Expected version 3.0.0.0 and higher"
                     );

        const char* separator = OPENSSL_info(OPENSSL_INFO_LIST_SEPARATOR);

        while (head != std::string::npos && separator != nullptr) {
            std::string::size_type tail = paths.find(separator[0], head);

            std::string path = paths.substr(head, tail != std::string::npos ? tail - 1 : tail);

            if ( !(   nameList != nullptr
                   && !path.empty()
                   && (SSL_add_dir_cert_subjects_to_stack(nameList, path.c_str()) == 1)
               ) )
            {
                result = Core::ERROR_GENERAL;
                break;
            }

            head = tail == std::string::npos ? tail : tail + 1;
        }

        if (nameList != nullptr && result == Core::ERROR_NONE) {
            // Takes ownership of nameList
            SSL_CTX_set_client_CA_list(static_cast<SSL_CTX*>(_context), nameList);

            // Callback is triggered if certiifcates have errors
            SSL_CTX_set_verify(static_cast<SSL_CTX*>(_context), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, VerifyCallbackWrapper);

            // Typically, SSL_CTX_set_cert_verify_callback is the most complex as it should replace the complete verification process procedure
//            SSL_CTX_set_cert_verify_callback(static_cast<SSL_CTX*>(_context), PeerCertificateCallbackWrapper, static_cast<void*>(_ssl));
        } else if (nameList != nullptr) {
            sk_X509_NAME_pop_free(nameList, X509_NAME_free);
        }
    }

    return result;
}

void SecureSocketPort::Handler::Update() {
    if (IsOpen() == true) {
        int result = 1;

        ASSERT(_ssl != nullptr);

        // Client
        if (_handShaking == CONNECTING) {
            if ((result = SSL_connect(static_cast<SSL*>(_ssl))) == 1) {
                _handShaking = CONNECTED;
                // If server has sent a certificate, and, we want to do 'our' own check
                ValidateHandShake();
            }
        } // Server
        else if (_handShaking == ACCEPTING) {
            if ((result = SSL_accept(static_cast<SSL*>(_ssl))) == 1) {
                _handShaking = CONNECTED;
                // Client has sent a certificate (optional, on request only)
                ValidateHandShake();
            }
        } // Re-initialie a previous session
        else if (_handShaking == EXCHANGE) {
            if ((result = SSL_do_handshake(static_cast<SSL*>(_ssl))) == 1) {
                _handShaking = CONNECTED;
                ValidateHandShake();
            }
        }

        if (result != 1) {
            result = SSL_get_error(static_cast<SSL*>(_ssl), result);

            if ((result == SSL_ERROR_WANT_READ) || (result == SSL_ERROR_WANT_WRITE)) {
                // Non-blocking I/O: select may be used to check if condition has changed
                _handShaking = CONNECTED;
                ValidateHandShake();
            }
            else {
                _handShaking = ERROR;
            }
        }
    }

    _parent.StateChange();
}

} } // namespace Thunder::Crypto
