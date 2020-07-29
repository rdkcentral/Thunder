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

#include <map>

#include "Platform.h"


namespace Implementation {

template<typename FACE, typename ... Args>
struct IFactoryType {
    virtual ~IFactoryType() { }

    virtual FACE* Create(Args ... args) = 0;
};

template<typename FACE, cryptographyvault VaultID, typename IMPL, typename ... Args>
class FactoryType : public IFactoryType<FACE, Args ...> {
public:
    static_assert(VaultID != CRYPTOGRAPHY_VAULT_DEFAULT, "Vault ID must no be DEFAULT");
    static_assert(std::is_base_of<FACE, IMPL>::value, "Require interface implementation");

    FactoryType(const FactoryType&) = delete;
    FactoryType& operator=(const FactoryType&) = delete;
    FactoryType() = default;
    ~FactoryType() = default;

public:
    static constexpr cryptographyvault ID = VaultID;
};

template<cryptographyvault VaultID, typename IMPL>
using HashFactoryType = FactoryType<Platform::IHashImplementation, VaultID, IMPL, const hash_type>;
using IHashFactory = IFactoryType<Platform::IHashImplementation, const hash_type>;

template<cryptographyvault VaultID, typename IMPL>
using VaultFactoryType = FactoryType<Platform::IVaultImplementation, VaultID, IMPL, const cryptographyvault>;
using IVaultFactory = IFactoryType<Platform::IVaultImplementation, const cryptographyvault>;

template<cryptographyvault VaultID, typename IMPL>
using HMACFactoryType = FactoryType<Platform::IHashImplementation, VaultID, IMPL, const hash_type, const uint32_t /* secret id */>;
using IHMACFactory = IFactoryType<Platform::IHashImplementation, const hash_type, const uint32_t /* secret id */>;

template<cryptographyvault VaultID, typename IMPL>
using AESFactoryType = FactoryType<Platform::ICipherImplementation, VaultID, IMPL, const aes_mode, const uint32_t /* secret id */>;
using IAESFactory = IFactoryType<Platform::ICipherImplementation, const aes_mode, const uint32_t /* secret id */>;

template<cryptographyvault VaultID, typename IMPL>
using DHFactoryType = FactoryType<Platform::IDiffieHellmanImplementation, VaultID, IMPL>;
using IDHFactory = IFactoryType<Platform::IDiffieHellmanImplementation>;


class Administrator {
private:
    Administrator()
        : _adminLock()
        , _factories()
        , _defaultId(CRYPTOGRAPHY_VAULT_PLATFORM)
    { }

    ~Administrator() = default;

public:
    Administrator(const Administrator&) = delete;
    Administrator& operator=(const Administrator&) = delete;

    static Administrator& Instance() {
        static Administrator instance;
        return (instance);
    }

public:
    void Default(const cryptographyvault id)
    {
        ASSERT(id != CRYPTOGRAPHY_VAULT_DEFAULT);
        if (id != CRYPTOGRAPHY_VAULT_DEFAULT) {
            _defaultId = id;
        }
    }

    template<typename FACTORY>
    void Announce(const cryptographyvault id, FACTORY* factory)
    {
        ASSERT(id != CRYPTOGRAPHY_VAULT_DEFAULT);
        _adminLock.Lock();
        FactoryMap<FACTORY>& map = _factories;
        map.emplace(id, factory);
        _adminLock.Unlock();
    }

    template<typename FACTORY>
    void Revoke(const cryptographyvault id)
    {
        ASSERT(id != CRYPTOGRAPHY_VAULT_DEFAULT);
        _adminLock.Lock();
        FactoryMap<FACTORY>& map = _factories;
        map.erase(id);
        _adminLock.Unlock();
    }

    template<typename FACTORY>
    FACTORY* Factory(const cryptographyvault id)
    {
        FACTORY* factory = nullptr;
        cryptographyvault lookupId = (id == CRYPTOGRAPHY_VAULT_DEFAULT? _defaultId : id);
        _adminLock.Lock();
        FactoryMap<FACTORY>& map = _factories;
        auto it = map.find(lookupId);
        if (it != map.end()) {
            factory = (*it).second;
        }
        _adminLock.Unlock();
        return (factory);
    }

private:
    template<typename FACTORY>
    using FactoryMap = std::map<cryptographyvault, FACTORY*>;

    WPEFramework::Core::CriticalSection _adminLock;

    struct Factories
        : FactoryMap<IHashFactory>
        , FactoryMap<IVaultFactory>
        , FactoryMap<IHMACFactory>
        , FactoryMap<IAESFactory>
        , FactoryMap<IDHFactory> { } _factories;

    cryptographyvault _defaultId;
};

template<typename FACTORY, typename IMPL>
class PlatformRegistrationType {
public:
    PlatformRegistrationType(const PlatformRegistrationType<FACTORY, IMPL>&) = delete;
    PlatformRegistrationType& operator=(const PlatformRegistrationType<FACTORY, IMPL>&) = delete;

    PlatformRegistrationType(const bool defaultId = false)
    {
        FACTORY* factory = new IMPL();
        ASSERT(factory != nullptr);
        if (factory) {
            Administrator::Instance().Announce(IMPL::ID, factory);
            if (defaultId == true) {
                Administrator::Instance().Default(IMPL::ID);
            }
        }
    }

    ~PlatformRegistrationType()
    {
        FACTORY* factory = Administrator::Instance().Factory<FACTORY>(IMPL::ID);
        ASSERT(factory != nullptr);
        Administrator::Instance().Revoke<FACTORY>(IMPL::ID);
        delete factory;
    }
};

} // namespace Implementation
