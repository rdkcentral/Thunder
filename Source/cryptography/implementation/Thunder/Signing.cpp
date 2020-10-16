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

#include "../../Module.h"

#include <ICryptographic.h>
#include <signing_implementation.h>

#include <core/core.h>
#include <cryptalgo/cryptalgo.h>

#include "Vault.h"


struct SigningImplementation {
    virtual void Ingest(const uint16_t length, const uint8_t data[]) = 0;
    virtual uint8_t Calculate(const uint8_t maxLength, uint8_t data[]) = 0;

    virtual ~SigningImplementation() { }
};


namespace Implementation {

template <typename HASH>
class SigningType : public SigningImplementation {
public:
    SigningType(const SigningType<HASH>&) = delete;
    SigningType<HASH>& operator=(const SigningType) = delete;

    SigningType()
        : _hash(nullptr)
    {
        _hash = new HASH();
        ASSERT(_hash != nullptr);
    }

    SigningType(const uint32_t secretId, const uint16_t secretLength)
        : _hash(nullptr)
    {
        ASSERT(secretId != 0);
        ASSERT(secretLength != 0);

        uint8_t* secret = reinterpret_cast<uint8_t*>(ALLOCA(secretLength));
        ASSERT(secret != nullptr);

        uint16_t secretLen = Implementation::Vault::Instance().Export(secretId, secretLength, secret, true);
        ASSERT(secretLen != 0);

        if (secretLen != 0) {
            _hash = new HASH(std::string(reinterpret_cast<const char*>(secret), secretLen));
            ASSERT(_hash != nullptr);
        }

        ::memset(secret, 0xFF, secretLen);
    }

    ~SigningType() override
    {
        if (_hash != nullptr) {
            delete _hash;
        }
    }

public:
    void Ingest(const uint16_t length, const uint8_t data[]) override
    {
        ASSERT(data != nullptr);

        if (_hash != nullptr) {
            _hash->Input(data, length);
        }
    }

    uint8_t Calculate(const uint8_t maxLength, uint8_t data[]) override
    {
        uint8_t length = 0;

        if (_hash != nullptr) {
            if (maxLength >= HASH::Length) {
                const uint8_t* result = _hash->Result();
                ::memcpy(data, result, HASH::Length);
                length = HASH::Length;
            } else {
                TRACE_L1(_T("Output buffer too small for digest, need %i bytes"), HASH::Length);
            }
        }

        return (length);
    }

private:
    HASH* _hash;
};

} // namespace Implementation


extern "C" {

SigningImplementation* signing_create_hash(const hash_type type)
{
    SigningImplementation* implementation = nullptr;

    switch (type) {
    case hash_type::HASH_SHA1:
        implementation = new Implementation::SigningType<WPEFramework::Crypto::SHA1>();
        break;
    case hash_type::HASH_SHA224:
        implementation = new Implementation::SigningType<WPEFramework::Crypto::SHA224>();
        break;
    case hash_type::HASH_SHA256:
        implementation = new Implementation::SigningType<WPEFramework::Crypto::SHA256>();
        break;
    case hash_type::HASH_SHA384:
        implementation = new Implementation::SigningType<WPEFramework::Crypto::SHA384>();
        break;
    case hash_type::HASH_SHA512:
        implementation = new Implementation::SigningType<WPEFramework::Crypto::SHA512>();
        break;
    default:
        TRACE_L1(_T("Hashing algorithm %i not supported"), type);
        break;
    }

    return (implementation);
};

SigningImplementation* signing_create_hmac(const hash_type type, const uint32_t secret_id)
{
    SigningImplementation* implementation = nullptr;

    uint16_t secretLength = Implementation::Vault::Instance().Size(secret_id, true);
    if (secretLength == 0) {
        TRACE_L1(_T("Failed to retrieve secret id 0x%08x"), secret_id);
    } else {
        switch (type) {
        case hash_type::HASH_SHA1:
            implementation = new Implementation::SigningType<WPEFramework::Crypto::HMACType<WPEFramework::Crypto::SHA1>>(secret_id, secretLength);
            break;
        case hash_type::HASH_SHA224:
            implementation = new Implementation::SigningType<WPEFramework::Crypto::HMACType<WPEFramework::Crypto::SHA224>>(secret_id, secretLength);
            break;
        case hash_type::HASH_SHA256:
            implementation = new Implementation::SigningType<WPEFramework::Crypto::HMACType<WPEFramework::Crypto::SHA256>>(secret_id, secretLength);
            break;
        case hash_type::HASH_SHA384:
            implementation = new Implementation::SigningType<WPEFramework::Crypto::HMACType<WPEFramework::Crypto::SHA384>>(secret_id, secretLength);
            break;
        case hash_type::HASH_SHA512:
            implementation = new Implementation::SigningType<WPEFramework::Crypto::HMACType<WPEFramework::Crypto::SHA512>>(secret_id, secretLength);
            break;
        default:
            TRACE_L1(_T("Hashing algorithm %i not supported for HMAC"), type);
            break;
        }
    }

    return (implementation);
}

void signing_destroy(SigningImplementation* signing)
{
    ASSERT(signing != nullptr);

    delete signing;
}

void signing_ingest(SigningImplementation* signing, const uint16_t length, const uint8_t data[])
{
    ASSERT(signing != nullptr);

    signing->Ingest(length, data);
}

uint8_t signing_calculate(SigningImplementation* signing, const uint8_t max_length, uint8_t data[])
{
    ASSERT(signing != nullptr);

    return (signing->Calculate(max_length, data));
}

} // extern "C"
