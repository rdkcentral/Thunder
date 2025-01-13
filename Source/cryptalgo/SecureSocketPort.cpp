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
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bn.h>

#ifdef __WINDOWS__
#include <wincrypt.h>
#include <cryptuiapi.h>
#include <iostream>
#include <tchar.h>

#pragma comment (lib, "crypt32.lib")
#pragma comment (lib, "cryptui.lib")
#endif

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

// -----------------------------------------------------------------------------
// class Certificate
// -----------------------------------------------------------------------------
Certificate::Certificate(const x509_st* certificate)
    : _certificate(certificate) {
    if (certificate != nullptr) {
        X509_up_ref(const_cast<x509_st*>(_certificate));
    }
}

Certificate::Certificate(const TCHAR fileName[]) {
    X509* cert = X509_new();
    BIO* bio_cert = BIO_new_file(fileName, "rb");
    PEM_read_bio_X509(bio_cert, &cert, NULL, NULL);
    _certificate = cert;
}

Certificate::Certificate(Certificate&& certificate) noexcept
    : _certificate(certificate._certificate) {
    certificate._certificate = nullptr;
}

Certificate::Certificate(const Certificate& certificate)
    : _certificate(certificate._certificate) {
    if (_certificate != nullptr) {
        X509_up_ref(const_cast<x509_st*>(_certificate));
    }
}

Certificate::~Certificate()
{
    if (_certificate != nullptr) {
        X509_free(const_cast<x509_st*>(_certificate));
    }
}

string Certificate::Issuer() const {
    char buffer[1024];
    buffer[0] = '\0';
    X509_NAME_oneline(X509_get_issuer_name(_certificate), buffer, sizeof(buffer));

    return (string(buffer));
}

string Certificate::Subject() const {
    char buffer[1024];
    buffer[0] = '\0';
    X509_NAME_oneline(X509_get_subject_name(_certificate), buffer, sizeof(buffer));

    return (string(buffer));
}

Core::Time Certificate::ValidFrom() const {
    return(ASN1_ToTime(X509_get0_notBefore(_certificate)));
}

Core::Time Certificate::ValidTill() const {
    return(ASN1_ToTime(X509_get0_notAfter(_certificate)));
}

bool Certificate::ValidHostname(const string& expectedHostname) const {
    return (X509_check_host(const_cast<struct x509_st*>(_certificate), expectedHostname.data(), expectedHostname.size(), 0, nullptr) == 1);
}

// -----------------------------------------------------------------------------
// class Key
// -----------------------------------------------------------------------------
Key::Key(const evp_pkey_st* key)
    : _key(key) {
    if (_key != nullptr) {
        EVP_PKEY_up_ref(const_cast<evp_pkey_st*>(_key));
    }
}

Key::Key(Key&& key) noexcept
    : _key(key._key) {
    key._key = nullptr;
}

Key::Key(const Key& key)
    : _key(key._key) {
    if (_key != nullptr) {
        EVP_PKEY_up_ref(const_cast<evp_pkey_st*>(_key));
    }
}

Key::Key(const string& fileName)
    : _key(nullptr) {

    BIO* bio_key = BIO_new_file(fileName.c_str(), "rb");
    
    if (bio_key != nullptr) {
        _key = PEM_read_bio_PUBKEY(bio_key, NULL, NULL, NULL);

        BIO_free(bio_key);
    }
}

static int passwd_callback(char* buffer, int size, int /* flags */, void* password)
{
    int copied = std::min(static_cast<int>(strlen(static_cast<const char*>(password))), size);
    memcpy(buffer, password, copied);
    return copied;
}

Key::Key(const string& fileName, const string& password) 
    : _key(nullptr) {
    BIO* bio_key = BIO_new_file(fileName.c_str(), "rb");

    if (bio_key != nullptr) {
        _key = PEM_read_bio_PrivateKey(bio_key, NULL, passwd_callback, const_cast<void*>(static_cast<const void*>(password.c_str())));
        BIO_free(bio_key);
    }
}

Key::~Key()
{
    if (_key != nullptr) {
        EVP_PKEY_free(const_cast<evp_pkey_st*>(_key));
    }
}

// -----------------------------------------------------------------------------
// class CertificateStore
// -----------------------------------------------------------------------------

#ifdef __WINDOWS__
static struct x509_store_st* CreateDefaultStore()
{
    HCERTSTORE hStore;
    PCCERT_CONTEXT pContext = nullptr;
    X509_STORE* store = X509_STORE_new();

    hStore = CertOpenSystemStore(NULL, _T("ROOT"));

    if (hStore != nullptr) {
        while (pContext = CertEnumCertificatesInStore(hStore, pContext)) {
            X509* x509 = d2i_X509(nullptr, (const unsigned char**)&pContext->pbCertEncoded, pContext->cbCertEncoded);

            if (x509 != nullptr) {
                X509_STORE_add_cert(store, x509);
                X509_free(x509);
            }
        }
        CertFreeCertificateContext(pContext);
        CertCloseStore(hStore, 0);
    }

    return (store);
}
#else
static struct x509_store_st* CreateDefaultStore()
{
    X509_STORE* store = X509_STORE_new();

    const char* dir = getenv(X509_get_default_cert_dir_env());

    if (dir == nullptr) {
        dir = X509_get_default_cert_dir();
    }

    X509_STORE_load_path(store, dir);

    return (store);
}
#endif

/* static */ struct x509_store_st* CertificateStore::_default = CreateDefaultStore();

CertificateStore::CertificateStore() 
    : _store(X509_STORE_new()) {
}

CertificateStore::CertificateStore(CertificateStore&& move) noexcept
    : _store(move._store) {
    move._store = nullptr;
}

CertificateStore::CertificateStore(const CertificateStore& copy)
    : _store(copy._store) {
    if (_store != nullptr) {
        X509_STORE_up_ref(const_cast<x509_store_st*>(_store));
    }
}

CertificateStore::CertificateStore(struct x509_store_st* store) 
    : _store(store) {
    if (_store != nullptr) {
        X509_STORE_up_ref(const_cast<x509_store_st*>(_store));
    }
}

CertificateStore::~CertificateStore() {
    if (_store != nullptr) {
        X509_STORE_free(const_cast<x509_store_st*>(_store));
    }
}

void CertificateStore::Add(const Certificate& certificate) {
    const struct x509_st* cert = certificate;
    X509_STORE_add_cert(_store, const_cast<x509_st*>(cert));
}

// -----------------------------------------------------------------------------
// class SecureSocketPort::Handler
// -----------------------------------------------------------------------------
SecureSocketPort::Handler::Handler(SecureSocketPort& parent,
    const enumType socketType,
    const Core::NodeId& localNode,
    const Core::NodeId& remoteNode,
    const uint16_t sendBufferSize,
    const uint16_t receiveBufferSize,
    const uint32_t socketSendBufferSize,
    const uint32_t socketReceiveBufferSize)
    : SocketPort(socketType, localNode, remoteNode, sendBufferSize, receiveBufferSize, socketSendBufferSize, socketReceiveBufferSize)
    , _parent(parent)
    , _callback(nullptr)
    , _handShaking(EXCHANGE) {
    CreateContext(TLS_method());
}

SecureSocketPort::Handler::Handler(SecureSocketPort& parent,
    const enumType socketType,
    const SOCKET& connector,
    const Core::NodeId& remoteNode,
    const uint16_t sendBufferSize,
    const uint16_t receiveBufferSize,
    const uint32_t socketSendBufferSize,
    const uint32_t socketReceiveBufferSize)
    : SocketPort(socketType, connector, remoteNode, sendBufferSize, receiveBufferSize, socketSendBufferSize, socketReceiveBufferSize)
    , _parent(parent)
    , _callback(nullptr)
    , _handShaking(EXCHANGE) {
    CreateContext(TLS_server_method());
}

SecureSocketPort::Handler::~Handler() {
    ASSERT(IsClosed() == true);
    Close(0);

    if (_ssl != nullptr) {
        SSL_free(_ssl);
        _ssl = nullptr;
    }
    if (_context != nullptr) {
        SSL_CTX_free(_context);
        _context = nullptr;
    }
}

void SecureSocketPort::Handler::CreateContext(const struct ssl_method_st* method) {
    _context = SSL_CTX_new(method);
    if (_context != nullptr) {
        _ssl = SSL_new(_context);

        if (_ssl == nullptr) {
            SSL_CTX_free(_context);
            _context = nullptr;
        }
        else {
            constexpr unsigned long options = SSL_OP_ALL | SSL_OP_NO_SSLv2;

            VARIABLE_IS_NOT_USED unsigned long bitmask = SSL_CTX_set_options(_context, options);

            ASSERT((bitmask & options) == options);
        }
    }
}

uint32_t SecureSocketPort::Handler::Initialize() {
    bool initialized = false;

    ASSERT(_context != nullptr);
    ASSERT(_ssl != nullptr);

    if (SSL_set_fd(_ssl, static_cast<Core::IResource&>(*this).Descriptor()) == 1) {
        SSL_set_tlsext_host_name(_ssl, RemoteNode().HostName().c_str());
        if (IsOpen() == true) {
            SSL_set_accept_state(_ssl);
        }
        else {
            SSL_set_connect_state(_ssl);
        }

        initialized =  Core::SocketPort::Initialize();
    }

    return (initialized);
}

int32_t SecureSocketPort::Handler::Read(uint8_t buffer[], const uint16_t length) const {

    ASSERT(_handShaking != ERROR);

    if (_handShaking != OPEN) {
        const_cast<Handler&>(*this).Update();
    }
 
    return (SSL_read(_ssl, buffer, length));
}

int32_t SecureSocketPort::Handler::Write(const uint8_t buffer[], const uint16_t length) {

    ASSERT(_handShaking != ERROR);

    uint32_t result = SSL_write(_ssl, buffer, length);

    if (_handShaking != OPEN) {
        Update();
    }

    return (result);
}

uint32_t SecureSocketPort::Handler::Open(const uint32_t waitTime) {
    return (Core::SocketPort::Open(waitTime));
}

uint32_t SecureSocketPort::Handler::Close(const uint32_t waitTime) {
    ASSERT(_ssl != nullptr);
    SSL_shutdown(_ssl);

    return(Core::SocketPort::Close(waitTime));
}

uint32_t SecureSocketPort::Handler::Certificate(const Crypto::Certificate& certificate, const Crypto::Key& key) {
    // Load server certificate and private key
    const struct x509_st* cert = certificate;
    const struct evp_pkey_st* base_key = key;
    uint32_t result = Core::ERROR_BAD_REQUEST;

    if (SSL_CTX_use_certificate(_context, const_cast<struct x509_st*>(cert)) == 1) {
        result = Core::ERROR_UNKNOWN_KEY;
        if (SSL_CTX_use_PrivateKey(_context, const_cast<EVP_PKEY*>(base_key)) == 1) {
            result = Core::ERROR_NONE;
        }
    }

    return (result);
}

uint32_t SecureSocketPort::Handler::Root(const CertificateStore& certStore) {
    const struct x509_store_st* store = certStore;

    SSL_CTX_set_cert_store(_context, const_cast<struct x509_store_st*>(store));

    return (Core::ERROR_NONE);
}

void SecureSocketPort::Handler::ValidateHandShake() {
    // Step 1: verify a certificate was presented during the negotiation
    X509* x509cert = SSL_get_peer_certificate(_ssl);

    if (x509cert == nullptr) {
        _handShaking = ERROR;
        SetError();
        _parent.StateChange();
    }
    else {
        long error;
        string validationError;
        Crypto::Certificate certificate(x509cert);

        // Step 2: Validate certificate - use custom IValidator instance if available or if self signed
        // certificates are needed :-)
        if (_callback != nullptr) {
            if (_callback->Validate(certificate) == false) {
                _handShaking = ERROR;
                SetError();
                _parent.StateChange();
            }
            else {
                _handShaking = OPEN;
                _parent.StateChange();
            }
        }
        // SSL handshake does an implicit verification, its result is:
        else if ((error = SSL_get_verify_result(_ssl)) != X509_V_OK) {
            // string errorMsg = X509_verify_cert_error_string(error);
            _handShaking = ERROR;
            SetError();
            _parent.StateChange();
        }
        else {
            _handShaking = OPEN;
            _parent.StateChange();
        }

        X509_free(x509cert);
    }   
}

void SecureSocketPort::Handler::Update() {

    if (IsOpen() == true) {
        int result = 1;

        ASSERT(_ssl != nullptr);

        if (_handShaking == EXCHANGE) {
            if ((result = SSL_do_handshake(_ssl)) == 1) {
                ValidateHandShake();
            }
            else {
                result = SSL_get_error(_ssl, result);

                if (result == SSL_ERROR_WANT_WRITE) {
                    Trigger();
                }
                else if (result != SSL_ERROR_WANT_READ) {
                    _handShaking = ERROR;
                }
            }
        }
    }
    else {
        _handShaking = EXCHANGE;
        _parent.StateChange();
    }
}

SecureSocketPort::~SecureSocketPort() {
}

} } // namespace Thunder::Crypto
