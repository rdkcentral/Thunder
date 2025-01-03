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
#include <openssl/x509.h>
#include <openssl/crypto.h>
#include <openssl/err.h>

#include <unordered_map>

#include <core/Time.h>

#ifndef _FALLTHROUGH
#define _FALLTHROUGH while(false){};
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

class X509Certificate : public Certificate {
public:
    X509Certificate(const Certificate& certificate)
        : Certificate{ certificate }
    {}

    X509Certificate(const X509* certificate)
        : Certificate { certificate }
    {}

    operator const X509* () const
    {
        return static_cast<const X509*>(Certificate::operator const void* ());
    }
};

class X509Key : public Key {
public :
    X509Key(const Key& key)
        : Key{ key}
    {}

    X509Key(const EVP_PKEY* key)
        : Key{ key }
    {}

    operator const EVP_PKEY* () const
    {
        return static_cast<const EVP_PKEY*>(Key::operator const void* ());
    }
};


class X509CertificateStore : public CertificateStore {
public:
    X509CertificateStore(const CertificateStore& store)
        : CertificateStore{ store }
    {}

    operator X509_STORE* () const
    {
        const std::vector<Certificate>* list { static_cast<const std::vector<Certificate>*>(CertificateStore::operator const void* ()) };

        X509_STORE* store{ list != nullptr ? X509_STORE_new() : nullptr };

        if (store != nullptr) {
            for (auto head = list->begin(), index = head, tail = list->end(); index != tail; index++) {
                // Do not X509_free
                X509* certificate = const_cast<X509*>(X509Certificate{ *index }.operator const X509*());

                if (   certificate == nullptr
                    || X509_STORE_add_cert(store, certificate) != 1
                   ) {
                    X509_STORE_free(store);

                    store = nullptr;

                    break;
                }
            }
        }

        return (store);
    }

    operator STACK_OF(X509_NAME)* () const
    {
        const std::vector<Certificate>* list { static_cast<const std::vector<Certificate>*>(CertificateStore::operator const void* ()) };

        STACK_OF(X509_NAME)* store{ list != nullptr ? sk_X509_NAME_new_null() : nullptr };

        if (store != nullptr) {
            for (auto head = list->begin(), index = head, tail = list->end(); index != tail; index++) {
                // Do not X509_free
                X509* certificate = const_cast<X509*>(X509Certificate{ *index }.operator const X509*());

                // name must not be freed
                X509_NAME* name = certificate != nullptr ? X509_get_subject_name(certificate) : nullptr;

                if (   certificate == nullptr
                    || sk_X509_NAME_push(store, name) == 0
                   ) {
                    sk_X509_NAME_free(store);

                    store = nullptr;

                    break;
                }
            }
        }

        return (store);
    }
};

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

        result = _map.insert({ssl, index}).second ? index : -1;

        return result;
    }

    int Index(const SSL* const ssl) const
    {
        ASSERT(ssl != nullptr);

        int result = -1;

        auto it = _map.find(ssl);

        result = it != _map.end() ? it->second : -1;

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
Certificate::Certificate(const void* certificate)
    : _certificate{ const_cast<void*>(certificate) }
{
    if (certificate != nullptr) {
        VARIABLE_IS_NOT_USED int result = X509_up_ref(static_cast<X509*>(_certificate));
        ASSERT(result == 1);
    }
}

Certificate::Certificate(const string& fileName)
    : _certificate{ nullptr }
{
    BIO* bioFile{ !fileName.empty() ? BIO_new_file(fileName.c_str(), "r") : nullptr };

    if (bioFile != nullptr) {
        _certificate = PEM_read_bio_X509(bioFile, nullptr, nullptr, nullptr);

        BIO_free(bioFile);
    }
}

Certificate::Certificate(Certificate&& certificate) noexcept
    : _certificate{ certificate._certificate }
{
    certificate._certificate = nullptr;
}

Certificate::Certificate(const Certificate& certificate)
    : _certificate{ certificate._certificate }
{
    if (_certificate != nullptr) {
        VARIABLE_IS_NOT_USED int result = X509_up_ref(static_cast<X509*>(_certificate));
        ASSERT(result == 1);
    }
}

Certificate::~Certificate()
{
    if (_certificate != nullptr) {
        X509_free(static_cast<X509*>(_certificate));
    }
}

const std::string Certificate::Issuer() const
{
    ASSERT(_certificate != nullptr);

    std::string result;
    char* buffer = nullptr; 

    // Do not free
    X509_NAME* name = X509_get_issuer_name(static_cast<X509*>(_certificate));

    if ((buffer = X509_NAME_oneline(name, nullptr, 0)) != nullptr) {
        result = buffer;

        free(buffer);
    }

    return (result);
}

const std::string Certificate::Subject() const
{
    ASSERT(_certificate != nullptr);

    std::string result;
    char* buffer = nullptr; 

    // Do not free
    X509_NAME* name = X509_get_subject_name(static_cast<X509*>(_certificate));

    if ((buffer = X509_NAME_oneline(name, nullptr, 0)) != nullptr) {
        result = buffer;

        free(buffer);
    }

    return (result);
}

Core::Time Certificate::ValidFrom() const
{
    ASSERT(_certificate != nullptr);

    // Do not free
    return (ASN1_ToTime(X509_get0_notBefore(static_cast<X509*>(_certificate))));
}

Core::Time Certificate::ValidTill() const
{
    ASSERT(_certificate != nullptr);

    // Do not free
    return (ASN1_ToTime(X509_get0_notAfter(static_cast<X509*>(_certificate))));
}

bool Certificate::ValidHostname(const std::string& expectedHostname) const
{
    ASSERT(_certificate != nullptr);

    return (X509_check_host(static_cast<X509*>(_certificate), expectedHostname.c_str(), expectedHostname.size(), 0, nullptr) == 1);
}

Certificate::operator const void* () const
{
    if (_certificate != nullptr) {
        VARIABLE_IS_NOT_USED int result = X509_up_ref(static_cast<X509*>(_certificate));
        ASSERT(result == 1);
    }

    return (_certificate);
}

// -----------------------------------------------------------------------------
// class Key
// -----------------------------------------------------------------------------
Key::Key(const void* key)
    : _key{ const_cast<void*>(key) }
{
    if (_key != nullptr) {
        VARIABLE_IS_NOT_USED int result = EVP_PKEY_up_ref(static_cast<EVP_PKEY*>(_key));
        ASSERT(result == 1);
    }
}

Key::Key(Key&& key) noexcept
    : _key{ key._key }
{
    key._key = nullptr;
}

Key::Key(const Key& key)
    : _key{ key._key }
{
    if (_key != nullptr) {
        VARIABLE_IS_NOT_USED int result = EVP_PKEY_up_ref(static_cast<EVP_PKEY*>(_key));
        ASSERT(result == 1);
    }
}

Key::Key(const string& fileName)
    : _key{ nullptr }
{
    BIO* bioFile{ !fileName.empty() ? BIO_new_file(fileName.c_str(), "r") : nullptr };

    if (bioFile != nullptr) {
        _key = PEM_read_bio_PrivateKey(bioFile, nullptr, nullptr, nullptr);

        BIO_free(bioFile);
    }
}

extern "C"
{
int PasswdCallback(char* buffer, int size, VARIABLE_IS_NOT_USED int rw, void* password)
{
    // if rw == 0 then request to supply passphrase
    // if rw == 1, something else, possibly verify the supplied passphrase in a second prompt
    ASSERT(rw == 0);

    ASSERT(size < 0);

    size_t copied = std::min(strlen(static_cast<const char*>(password)), static_cast<size_t>(size));
    /* void* */ memcpy(buffer, password, copied);

//    using common_t = std::common_type<int, size_t>::type;
//    static_assert(static_cast<common_t>(std::numeric_limits<int>::max()) <= static_cast<size_t>(std::numeric_limits<size_t>::max()));

    // 0 - error, 0> - number of characaters in passphrase
    return static_cast<int>(copied);
}
}

Key::Key(const string& fileName, const string& password) 
    : _key(nullptr)
{
    BIO* bioFile{ !fileName.empty() ? BIO_new_file(fileName.c_str(), "r") : nullptr };

    if (bioFile != nullptr) {
        _key = PEM_read_bio_PrivateKey(bioFile, nullptr, PasswdCallback, const_cast<char*>(password.c_str()));

        BIO_free(bioFile);
    }
}

Key::~Key()
{
    if (_key != nullptr) {
        EVP_PKEY_free(static_cast<EVP_PKEY*>(_key));
    }
}

Key::operator const void* () const
{
    return (_key);
}

// -----------------------------------------------------------------------------
// class CertificateStore
// -----------------------------------------------------------------------------
CertificateStore::CertificateStore(bool defaultStore) 
    : _list{}
    , _defaultStore{ defaultStore && CreateDefaultStore() }
{}

CertificateStore::CertificateStore(CertificateStore&& move) noexcept
    : _list { move._list }
    , _defaultStore{ move._defaultStore }
{}

CertificateStore::CertificateStore(const CertificateStore& copy)
    : _list { copy._list }
    , _defaultStore{ copy._defaultStore }
{}

CertificateStore::~CertificateStore()
{}

uint32_t CertificateStore::Add(const Certificate& certificate)
{
    uint32_t result = Core::ERROR_GENERAL;

    const X509* x509Certificate = X509Certificate{ certificate };

    if (   x509Certificate != nullptr
        && std::find_if(_list.begin(), _list.end(), [&x509Certificate](const X509Certificate item){
                                                                                                   const X509* x509 = item;
                                                                                                   return X509_cmp(x509Certificate, x509) == 0;
                                                                                                  }
                       ) == _list.end()
       ) {
        _list.push_back(certificate);

        result = Core::ERROR_NONE;
    }

    return result;
}

uint32_t CertificateStore::Remove(const Certificate& certificate)
{
    uint32_t result = Core::ERROR_GENERAL;

    std::vector<Certificate>::iterator it;

    const X509* x509Certificate = X509Certificate{ certificate };

    if ((it = std::find_if(_list.begin(), _list.end(), [&x509Certificate](const X509Certificate item){
                                                                                                      const X509* x509 = item;
                                                                                                      return X509_cmp(x509Certificate, x509) == 0;
                                                                                                     }
                       )) == _list.end()
       ) {
        size_t position = std::distance(_list.begin(), it);

        static_assert(  !std::has_virtual_destructor<Crypto::Certificate>::value
                      , "new placement without support for virtual base classes"
        );

        if (_list.size() > 1) {
            Certificate& item = _list[position];
            item.~Certificate();
            new (&(item)) Crypto::Certificate(_list.back());
        }

        _list.pop_back();

        result = Core::ERROR_NONE;
    }

    return (result);
}

uint32_t CertificateStore::CreateDefaultStore()
{
    // Intended use only at object construction

    uint32_t result = Core::ERROR_NONE;

    const char* dir = getenv(X509_get_default_cert_dir_env());

    if (dir == nullptr) {
        dir = X509_get_default_cert_dir();
    }

    const std::string paths = dir;

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

        Core::Directory path{ paths.substr(head, tail != std::string::npos ? tail - 1 : tail).c_str() };

        while (path.Next()) {
            const Certificate certificate{ Certificate{ path.Current() } };

            if (X509Certificate { certificate }.operator const X509* () != nullptr) {
                result = Add(certificate);
            }

            if (result != Core::ERROR_NONE) {
                tail = std::string::npos;
                break;
            }
        }

        head = tail == std::string::npos ? tail : tail + 1;
    }

    return (result);
}

CertificateStore::operator const void* () const
{
    return &_list;
}

bool CertificateStore::IsDefaultStore() const
{
    return _defaultStore;
}

SecureSocketPort::Handler::~Handler() {
    ASSERT(IsClosed() == true);
    Close(0);
}

uint32_t SecureSocketPort::Handler::Initialize() {
    ASSERT(_context == nullptr);

    uint32_t success = Core::ERROR_NONE;

    // Client and server use
    _context = SSL_CTX_new(TLS_method());

    ASSERT(_context != nullptr);

    constexpr uint64_t options = SSL_OP_ALL | SSL_OP_NO_SSLv2;

    VARIABLE_IS_NOT_USED uint64_t bitmask = SSL_CTX_set_options(static_cast<SSL_CTX*>(_context), options);

    ASSERT((bitmask & options) == options);

    int exDataIndex = -1;

    // Do not X509_free
    X509* x509Certificate = const_cast<X509*>(X509Certificate{ _certificate }.operator const X509*());
    // Do not EVP_PKEY_free
    EVP_PKEY* x509Key = const_cast<EVP_PKEY*>(X509Key{ _privateKey }.operator const EVP_PKEY*());

    if ( ( (    (_handShaking == ACCEPTING)
              // Load server certificate and private key
             && x509Certificate != nullptr
             && (SSL_CTX_use_certificate(static_cast<SSL_CTX*>(_context), x509Certificate) == 1)
             && x509Key != nullptr
             && (SSL_CTX_use_PrivateKey(static_cast<SSL_CTX*>(_context), x509Key) == 1)
             && (SSL_CTX_check_private_key(static_cast<SSL_CTX*>(_context)) == 1)
             && (EnableClientCertificateRequest() == Core::ERROR_NONE)
           )
           ||
              // Load client certificate and private key if present
           (    (_handShaking == CONNECTING)
             && (
                  x509Certificate == nullptr
                  ||
                  (    x509Certificate != nullptr
                    && (SSL_CTX_use_certificate(static_cast<SSL_CTX*>(_context), x509Certificate) == 1)
                  )
                )
             && (
                  x509Key == nullptr
                  ||
                  (   x509Key != nullptr
                   && (SSL_CTX_use_PrivateKey(static_cast<SSL_CTX*>(_context), x509Key) == 1)
                   && (SSL_CTX_check_private_key(static_cast<SSL_CTX*>(_context)) == 1)
                  )
                )
           )
         )
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

    ASSERT(success == 0);

    return success;
}

int32_t SecureSocketPort::Handler::Read(uint8_t buffer[], const uint16_t length) const {
    ASSERT(_handShaking == CONNECTED);
    ASSERT(_ssl != nullptr);
    ASSERT(length > 0);

    int fd = SSL_get_fd(static_cast<SSL*>(_ssl));

    ASSERT(fd >= 0);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    using common_time_t = std::common_type<time_t, uint32_t>::type;
    using common_sec_t = std::common_type<suseconds_t, uint32_t>::type;
#ifdef _STATIC_CAST_TIME
    static_assert(static_cast<common_time_t>(std::numeric_limits<time_t>::max()) >= static_cast<common_time_t>(std::numeric_limits<uint32_t>::max()), "Possible narrowing");
    static_assert(static_cast<common_sec_t>(std::numeric_limits<suseconds_t>::max()) >= static_cast<common_sec_t>(std::numeric_limits<uint32_t>::max()), "Possible narrowing");
#else
    ASSERT(static_cast<common_time_t>(std::numeric_limits<time_t>::max()) >= static_cast<common_time_t>(_waitTime));
    ASSERT(static_cast<common_sec_t>(std::numeric_limits<suseconds_t>::max()) >= static_cast<common_sec_t>(_waitTime));
#endif

    struct timeval tv {
      static_cast<time_t>(_waitTime / (Core::Time::MilliSecondsPerSecond))
    , static_cast<suseconds_t>((_waitTime % (Core::Time::MilliSecondsPerSecond)) * (Core::Time::MilliSecondsPerSecond / Core::Time::MicroSecondsPerSecond))
    };

    int result = -1;

    switch (SSL_want(static_cast<SSL*>(_ssl))) {
    case SSL_NOTHING            :   // No data to be written or read
                                    result = SSL_read(static_cast<SSL*>(_ssl), buffer, length);
                                    break;
    case SSL_WRITING            :   // More data to be written to complete
                                    result = (select(fd + 1, &fds, nullptr, nullptr, &tv) > 0) && FD_ISSET(fd, &fds) ? SSL_read(static_cast<SSL*>(_ssl), buffer, length) : -1;
                                    break;
    case SSL_READING            :   // More data to be read to complete
                                    result = (select(fd + 1, nullptr, &fds, nullptr, &tv) > 0) && FD_ISSET(fd, &fds) ? SSL_read(static_cast<SSL*>(_ssl), buffer, length) : -1;
                                    break;
    case SSL_X509_LOOKUP        :   _FALLTHROUGH; // Callback should be called again, see SSL_CTX_set_client_cert_cb()
    case SSL_RETRY_VERIFY       :   _FALLTHROUGH; // Callback should be called again, see SSL_set_retry_verify()
    case SSL_ASYNC_PAUSED       :   _FALLTHROUGH; // Asynchronous operation partially completed and paused, see SSL_get_all_async_fds
    case SSL_ASYNC_NO_JOBS      :   _FALLTHROUGH; // Asynchronous jobs could not be started, bone available, see ASYNC_init_thread()
    case SSL_CLIENT_HELLO_CB    :   _FALLTHROUGH; // Operation did not complete, callback has to be called again. see SSL_CTX_set_client_hello_cb()
    default                     :   // Error not processed
                                    result = -1;
    }

    return (result > 0 ? result : /* error */ -1);
}

int32_t SecureSocketPort::Handler::Write(const uint8_t buffer[], const uint16_t length) {
    ASSERT(_handShaking == CONNECTED);
    ASSERT(_ssl != nullptr);
    ASSERT(length > 0);

    int fd = SSL_get_fd(static_cast<SSL*>(_ssl));

    ASSERT(fd >= 0);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    using common_time_t = std::common_type<time_t, uint32_t>::type;
    using common_sec_t = std::common_type<suseconds_t, uint32_t>::type;
#ifdef _STATIC_CAST_TIME
    static_assert(static_cast<common_time_t>(std::numeric_limits<time_t>::max()) >= static_cast<common_time_t>(std::numeric_limits<uint32_t>::max()), "Possible narrowing");
    static_assert(static_cast<common_sec_t>(std::numeric_limits<suseconds_t>::max()) >= static_cast<common_sec_t>(std::numeric_limits<uint32_t>::max()), "Possible narrowing");
#else
    ASSERT(static_cast<common_time_t>(std::numeric_limits<time_t>::max()) >= static_cast<common_time_t>(_waitTime));
    ASSERT(static_cast<common_sec_t>(std::numeric_limits<suseconds_t>::max()) >= static_cast<common_sec_t>(_waitTime));
#endif

    struct timeval tv {
      static_cast<time_t>(_waitTime / (Core::Time::MilliSecondsPerSecond))
    , static_cast<suseconds_t>((_waitTime % (Core::Time::MilliSecondsPerSecond)) * (Core::Time::MilliSecondsPerSecond / Core::Time::MicroSecondsPerSecond))
    };

    int result = -1;

    switch (SSL_want(static_cast<SSL*>(_ssl))) {
    case SSL_NOTHING            :   // No data to be written or read
                                    result = SSL_write(static_cast<SSL*>(_ssl), buffer, length);
                                    break;
    case SSL_WRITING            :   // More data to be written to complete
                                    result = (select(fd + 1, &fds, nullptr, nullptr, &tv) > 0) && FD_ISSET(fd, &fds) ? SSL_write(static_cast<SSL*>(_ssl), buffer, length) : -1;
                                    break;
    case SSL_READING            :   // More data to be read to complete
                                    result = (select(fd + 1, nullptr, &fds, nullptr, &tv) > 0) && FD_ISSET(fd, &fds) ? SSL_write(static_cast<SSL*>(_ssl), buffer, length) : -1;
                                    break;
    case SSL_X509_LOOKUP        :   _FALLTHROUGH; // Callback should be called again, see SSL_CTX_set_client_cert_cb()
    case SSL_RETRY_VERIFY       :   _FALLTHROUGH; // Callback should be called again, see SSL_set_retry_verify()
    case SSL_ASYNC_PAUSED       :   _FALLTHROUGH; // Asynchronous operation partially completed and paused, see SSL_get_all_async_fds
    case SSL_ASYNC_NO_JOBS      :   _FALLTHROUGH; // Asynchronous jobs could not be started, bone available, see ASYNC_init_thread()
    case SSL_CLIENT_HELLO_CB    :   _FALLTHROUGH; // Operation did not complete, callback has to be called again. see SSL_CTX_set_client_hello_cb()
    default                     :   // Error not processed
                                    result = -1;
    }

    return (result > 0 ? result : /* error */ -1);
}

uint32_t SecureSocketPort::Handler::Open(const uint32_t waitTime) {
    _waitTime = waitTime;

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

uint32_t SecureSocketPort::Handler::Certificate(const Crypto::Certificate& certificate, const Crypto::Key& key) {
    // Load server / client certificate and private key

    uint32_t result = Core::ERROR_BAD_REQUEST;

    // Do not free
    const X509* x509Certificate = X509Certificate{ certificate };
    // Do not free
    const EVP_PKEY* x509Key = X509Key{ key };

    if (   x509Certificate != nullptr
        && x509Key != nullptr
    ) 
    {
        static_assert(   !std::has_virtual_destructor<Crypto::Certificate>::value
                      && !std::has_virtual_destructor<Crypto::Key>::value
                      , "new placement without support for virtual base classes"
        );

        this->_certificate.~Certificate();
        new (&(this->_certificate)) Crypto::Certificate(certificate);

        this->_privateKey.~Key();
        new (&(this->_privateKey)) Crypto::Key(key);

        // Verification of the pair at Initialize()

        result = Core::ERROR_NONE;
    }

    return (result);
}

uint32_t SecureSocketPort::Handler::CustomStore(const CertificateStore& certStore)
{
    static_assert(!std::has_virtual_destructor<CertificateStore>::value, "new placement without support for virtual base classes");

    this->_store.~CertificateStore();
    new (&(this->_store)) CertificateStore(certStore);

    return (Core::ERROR_NONE);
}

void SecureSocketPort::Handler::ValidateHandShake() {
    ASSERT(_ssl != nullptr && _context != nullptr);

    // Internal (partial) validation result if no callback is set
    if (SSL_get_verify_result(static_cast<SSL*>(_ssl)) != X509_V_OK) {
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
                    SecureSocketPort::IValidate* validator = nullptr;

                    // Retrieve and call the registered callback

                    if (   ctx != nullptr
                         && (exDataIndex = SSL_get_ex_data_X509_STORE_CTX_idx()) != -1
                         && (ssl = static_cast<SSL*>(X509_STORE_CTX_get_ex_data(ctx, exDataIndex))) != nullptr
                         && (exDataIndex = ApplicationData::Instance().Index(static_cast<SSL*>(ssl))) != -1
                         && (validator =  static_cast<SecureSocketPort::IValidate*>(SSL_get_ex_data(ssl, exDataIndex))) != nullptr
                         && (x509Cert = X509_STORE_CTX_get_current_cert(ctx)) != nullptr
                       ) {
                        X509_up_ref(x509Cert);

                        X509Certificate certificate(x509Cert);

                        result = validator->Validate(certificate);

                        // Reflect the verification result in the error member of X509_STORE_CTX
                        if (result) {
                            X509_STORE_CTX_set_error(ctx, X509_V_OK);

                           ASSERT(X509_STORE_CTX_get_error(ctx) == X509_V_OK);
                        }

                        X509_free(x509Cert);
                    }

                    break;
                }
    case 1  :   // No error
                break;
    default :   ASSERT(false); // Not within set of defined values
    }

    return result; // 0 - Failure, 1 - OK
}

int PeerCertificateCallbackWrapper(VARIABLE_IS_NOT_USED X509_STORE_CTX* ctx, VARIABLE_IS_NOT_USED void* arg)
{ // This is called if the complete certificate validation procedure has its custom implementation
  // This is typically not what is intended or required, and, a complex process to do correct

    ASSERT(false);

    return 0; // 0 - Failurre, 1 - OK
}

uint32_t SecureSocketPort::Handler::EnableClientCertificateRequest()
{
    uint32_t result{Core::ERROR_NONE};

    if (_requestCertificate) {
        STACK_OF(X509_NAME)* nameList = X509CertificateStore{ _store };

        if (nameList != nullptr) {
            // Takes ownership of nameList
            // CA list to send to the client against the client's certificate is vlidated
            SSL_CTX_set_client_CA_list(static_cast<SSL_CTX*>(_context), nameList);

            // Callback is triggered if certificates have errors
            SSL_CTX_set_verify(static_cast<SSL_CTX*>(_context), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, VerifyCallbackWrapper);

            // Typically, SSL_CTX_set_cert_verify_callback is the most complex as it should replace the complete verification process procedure
//            SSL_CTX_set_cert_verify_callback(static_cast<SSL_CTX*>(_context), PeerCertificateCallbackWrapper, static_cast<void*>(_ssl));
        } else {
            result = Core::ERROR_GENERAL;
        }
    }

    return result;
}

void SecureSocketPort::Handler::Update() {
    if (IsOpen() == true) {
        ASSERT(_ssl != nullptr);
        ASSERT(_context != nullptr);

        if (_handShaking != CONNECTED) {
            // The old store is automatically X509_store_free()ed
            /* void */ SSL_CTX_set_cert_store(static_cast<SSL_CTX*>(_context), X509CertificateStore{ _store });
        }

//        const char* file = nullptr, *func = nullptr, *data = nullptr;
//        int line = 0, flag = 0;

//        ASSERT(ERR_get_error_all(&file, &line, &func, &data, &flag) == 0);

        errno = 0;

        switch (_handShaking) {
        case CONNECTING :   // Client
                            SSL_set_connect_state(static_cast<SSL*>(_ssl));
                            break;
        case ACCEPTING  :  // Server
                            SSL_set_accept_state(static_cast<SSL*>(_ssl));
                            break;
        case EXCHANGE   :   // Re-initialie a previous session
                            break;
        default         :   ASSERT(false);
        }

        const int fd = SSL_get_fd(static_cast<SSL*>(_ssl));

        ASSERT(fd >= 0);

        fd_set rfds, wfds;
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_SET(fd, &rfds);
        FD_SET(fd, &wfds);

    using common_time_t = std::common_type<time_t, uint32_t>::type;
    using common_sec_t = std::common_type<suseconds_t, uint32_t>::type;
#ifdef _STATIC_CAST_TIME
    static_assert(static_cast<common_time_t>(std::numeric_limits<time_t>::max()) >= static_cast<common_time_t>(std::numeric_limits<uint32_t>::max()), "Possible narrowing");
    static_assert(static_cast<common_sec_t>(std::numeric_limits<suseconds_t>::max()) >= static_cast<common_sec_t>(std::numeric_limits<uint32_t>::max()), "Possible narrowing");
#else
    ASSERT(static_cast<common_time_t>(std::numeric_limits<time_t>::max()) >= static_cast<common_time_t>(_waitTime));
    ASSERT(static_cast<common_sec_t>(std::numeric_limits<suseconds_t>::max()) >= static_cast<common_sec_t>(_waitTime));
#endif

        struct timeval tv {
          static_cast<time_t>(_waitTime / (Core::Time::MilliSecondsPerSecond))
        , static_cast<suseconds_t>((_waitTime % (Core::Time::MilliSecondsPerSecond)) * (Core::Time::MilliSecondsPerSecond / Core::Time::MicroSecondsPerSecond))
        };

        do {
            int result = SSL_do_handshake(static_cast<SSL*>(_ssl));

            switch ((result = SSL_get_error(static_cast<SSL*>(_ssl), result))) {
            case SSL_ERROR_NONE                 :   _handShaking = CONNECTED;
                                                    break;
            case SSL_ERROR_SYSCALL              :   // Some syscall failed
                                                    if (errno != EAGAIN) {
                                                        // Last error without removing it from the queue
#ifdef VERBOSE
                                                       unsigned long result = 0;

                                                        if ((result = ERR_peek_last_error_all(&file, &line, &func, &data, &flag)) != 0) {
                                                            int lib = ERR_GET_LIB(result);
                                                            int reason = ERR_GET_REASON(result);

                                                            TRACE_L1(_T("ERROR: OpenSSL SYSCALL error in LIB %d with REASON %d"), lib, reason);
                                                        }
#endif
                                                    }
                                                    // Fallthrough for select
            case SSL_ERROR_WANT_READ            :   _FALLTHROUGH; // Wait until ready to read
            case SSL_ERROR_WANT_WRITE           :   _FALLTHROUGH; // Wait until ready to write
            case SSL_ERROR_WANT_CONNECT         :   _FALLTHROUGH; // Operation did not complete. Redo if the connection has been established
            case SSL_ERROR_WANT_ACCEPT          :   // Idem
                                                    switch (select(fd + 1, &rfds, &wfds, nullptr, &tv)) {
                                                    default :   if (   !FD_ISSET(fd, &rfds)
                                                                    && !FD_ISSET(fd, &wfds)
                                                                   ) {
                                                                    // Only file descriptors in the set should be set
                                                                    _handShaking = ERROR;
            ASSERT(_handShaking != ERROR);
                                                                    break;
                                                                }
                                                    case 0  :   // Timeout
// TODO: redo SSL_do_handshake or consider it an error?
                                                                continue;
                                                    case -1 :   // Select failed, consider it an error
                                                                _handShaking = ERROR;
            ASSERT(_handShaking != ERROR);
                                                    }
                                                    break;
            case SSL_ERROR_ZERO_RETURN          :   _FALLTHROUGH; 
            case SSL_ERROR_WANT_ASYNC           :   _FALLTHROUGH; 
            case SSL_ERROR_WANT_ASYNC_JOB       :   _FALLTHROUGH; 
            case SSL_ERROR_WANT_CLIENT_HELLO_CB :   _FALLTHROUGH;
            case SSL_ERROR_SSL                  :   _FALLTHROUGH;   // Unrecoverable error
            default                             :   // Error
                                                    _handShaking = ERROR;
            }

        } while (_handShaking != CONNECTED);

        // If server has sent a certificate, and, we want to do 'our' own check
        // or
        // Client has sent a certificate (optional, on request only)

        // Only if a callback has not been set
        if (   /*(_callback == nullptr)
            &&*/ (SSL_get_verify_callback(static_cast<SSL*>(_ssl)) != nullptr)
            && (SSL_get_verify_callback(static_cast<SSL*>(_ssl)) != &VerifyCallbackWrapper)
            && (SSL_CTX_get_verify_callback(static_cast<SSL_CTX*>(_context)) != nullptr)
            && (SSL_CTX_get_verify_callback(static_cast<SSL_CTX*>(_context)) != &VerifyCallbackWrapper)
        ) {
            ValidateHandShake();
        }
            ASSERT(_handShaking != ERROR);

        _parent.StateChange();
    }
}

}

} // namespace Thunder::Crypto

#ifdef _FALLTHROUGH
#undef _FALLTHROUGH
#endif
