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
#include <map>


namespace Implementation {

class Vault {
public:
    static Vault& NetflixInstance();
    static Vault& PlatformInstance();

private:
    using Callback = std::function<void(Vault&)>;

    Vault(const string key, const Callback& ctor = nullptr, const Callback& dtor = nullptr);
    ~Vault();

public:
    Vault(Vault const&) = delete;
    void operator=(Vault const&) = delete;

public:
    class Element : public WPEFramework::Core::DataStore {
    public:
        Element(bool exportable, const uint16_t size)
            : WPEFramework::Core::DataStore(size)
            , _exportable(exportable)
        {
        }

        Element(bool exportable, const uint16_t size, const uint8_t* buffer)
            : WPEFramework::Core::DataStore(size)
            , _exportable(exportable)
        {
            Copy(buffer, size);
        }

        bool IsExportable() const
        {
            return _exportable;
        }

    private:
        bool _exportable;
    };

public:
    uint16_t Size(const uint32_t id, bool allowSealed = false) const;
    uint32_t Import(const uint16_t size, const uint8_t blob[], bool exportable = false);
    uint16_t Export(const uint32_t id, const uint16_t size, uint8_t blob[], bool allowSealed = false) const;
    uint32_t Put(const uint16_t size, const uint8_t blob[]);
    uint16_t Get(const uint32_t id, const uint16_t size, uint8_t blob[]) const;
    bool Delete(const uint32_t id);

private:
    uint16_t Cipher(bool encrypt, const uint16_t inSize, const uint8_t input[], const uint16_t maxOutSize, uint8_t output[]) const;

private:
    mutable WPEFramework::Core::CriticalSection _lock;
    std::map<uint32_t, Element> _items;
    uint32_t _lastHandle;
    string _vaultKey;
    Callback _dtor;
};

} // namespace Implementation
