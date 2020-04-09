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

#include <core/core.h>
#include <string>
#include <cstdint>

namespace WPEFramework {

namespace Cryptography {

struct INetflixSecurity : public Core::IUnknown
{
    enum { ID = 0x00001200 };

    virtual ~INetflixSecurity() { }

    /* Retrieve the ESN */
    virtual std::string ESN() const = 0;

    /* Retrieve the pre-shared encryption key */
    virtual uint32_t EncryptionKey() const = 0;

    /* Retrieve the pre-shared HMAC key */
    virtual uint32_t HMACKey() const = 0;

    /* Retrieve the pre-shared wrapping key */
    virtual uint32_t WrappingKey() const = 0;

    /* Derive encryption keys based on an authenticated Diffie-Hellman procedure */
    virtual uint32_t DeriveKeys(const uint32_t privateDhKeyId, const uint32_t peerPublicDhKeyId, const uint32_t derivationKeyId,
                                uint32_t& encryptionKeyId /* @out */, uint32_t& hmacKeyId /* @out */, uint32_t& wrappingKeyId /* @out */) = 0;

    static INetflixSecurity* Instance();
};

} // namespace Cryptography

}
