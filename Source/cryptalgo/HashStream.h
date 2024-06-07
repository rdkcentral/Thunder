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
 
#ifndef __HASHSTREAM_H
#define __HASHSTREAM_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "HMAC.h"
#include "Hash.h"
#include "Module.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----
namespace Thunder {
namespace Crypto {
    struct IHashStream {
        virtual ~IHashStream() = default;

        virtual EnumHashType Type() const = 0;
        virtual void Reset() = 0;
        virtual uint8_t* Result() = 0;
        virtual uint8_t Length() const = 0;
        virtual void Input(const uint8_t block, const uint16_t length) = 0;
    };

    template <typename HASHALGORITHM, const enum EnumHashType TYPE>
    class HashStreamType {
    private :
        HashStreamType(HashStreamType<HASHALGORITHM, TYPE>&&);
        HashStreamType(const HashStreamType<HASHALGORITHM, TYPE>&);
        HashStreamType<HASHALGORITHM, TYPE>& operator=(HashStreamType<HASHALGORITHM, TYPE>&&);
        HashStreamType<HASHALGORITHM, TYPE>& operator=(const HashStreamType<HASHALGORITHM, TYPE>&);

        public :
            // For Hash streaming
            HashStreamType();

        // For HMAC streaming
        HashStreamType(const Core::TextFragment& key);

        public :
            virtual void Reset(){
                _hash.Reset(); }
    virtual uint8_t* Result()
    {
        return (_hash.Result());
    }
    virtual uint8_t Length() const
    {
        return (HASHALGORITHM::Length());
    }
    virtual void Input(const uint8_t block[], const uint16_t length)
    {
        _hash.Input(block, length);
    }
    virtual EnumHashType Type() const
    {
        return (TYPE);
    }

private:
    HASHALGORITHM _hash;
};
}
}

#endif // __HASHSTREAM_H
