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

#pragma once

#include "Module.h"
#include "WebRequest.h"
#include "WebResponse.h"

namespace Thunder {
namespace Web {

    class EXTERNAL TextBody : public string, public IBody {
    public:
        TextBody(const TextBody&) = delete;
        TextBody& operator=(const TextBody&) = delete;

        TextBody()
            : _lastPosition(0)
        {
        }
        ~TextBody() override = default;

    public:
        TextBody& operator=(const string& newText)
        {
            string::operator=(newText);

            return (*this);
        }
        inline void Clear()
        {
            string::clear();
        }

    protected:
        uint32_t Serialize() const override
        {
            _lastPosition = 0;
            return (static_cast<uint32_t>(string::length() * sizeof(TCHAR)));
        }
        uint32_t Deserialize() override
        {
            string::clear();
            return (static_cast<uint32_t>(~0));
        }
        uint16_t Serialize(uint8_t stream[], const uint16_t maxLength) const override
        {
            uint16_t size = static_cast<uint16_t>(maxLength > (string::length() * sizeof(TCHAR)) ? (string::length() * sizeof(TCHAR)) : (sizeof(TCHAR) == 1 ? maxLength : (maxLength & 0xFFFE)));

            if (size > 0) {
                ::memcpy(stream, &(reinterpret_cast<const uint8_t*>(string::c_str())[_lastPosition]), size);
                _lastPosition += size;
            }

            return size;
        }
        uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength) override
        {
            uint16_t index = 0;

            while (index < maxLength) {
                string::operator+=(stream[index]);
                index++;
            }

            return index;
        }
        void End() const override
        {
        }

    private:
        mutable uint32_t _lastPosition;
    };

    template <typename HASHALGORITHM>
    class SignedTextBodyType : public TextBody {
    public:
        using HashType = HASHALGORITHM;

        SignedTextBodyType(const SignedTextBodyType<HASHALGORITHM>&) = delete;
        SignedTextBodyType<HASHALGORITHM>& operator=(const SignedTextBodyType<HASHALGORITHM>&) = delete;

        SignedTextBodyType()
            : TextBody()
            , _hash()
        {
        }
        SignedTextBodyType(const string& HMACKey)
            : TextBody()
            , _hash(HMACKey)
        {
        }
        ~SignedTextBodyType() override = default;

    public:
        inline HashType& Hash()
        {
            return (_hash);
        }
        inline const HashType& Hash() const
        {
            return (_hash);
        }

    protected:
        uint32_t Deserialize() override
        {
            _hash.Reset();
            return (TextBody::Deserialize());
        }
        uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength) override
        {
            uint16_t deserialized = TextBody::Deserialize(stream, maxLength);

            // Also pass it through our hashing algorithm.
            if (deserialized) {
                _hash.Input(stream, maxLength);
            }
            return deserialized;
        }

    private:
        HashType _hash;
    };

    class EXTERNAL FileBody : public Core::File, public IBody {
    public:
        FileBody(const FileBody&) = delete;
        FileBody& operator=(const FileBody&) = delete;

        FileBody()
            : Core::File()
            , _opened(false)
            , _startPosition(0)
        {
        }
        FileBody(const string& path)
            : Core::File(path)
            , _opened(false)
            , _startPosition(0)
        {
        }
        ~FileBody() override = default;

    public:
        inline FileBody& operator=(const string& location)
        {
            Core::File::operator=(location);
            _startPosition = 0;

            return (*this);
        }
        inline FileBody& operator=(const File& RHS)
        {
            Core::File::operator=(RHS);
            _startPosition = static_cast<int32_t>(Core::File::Position());

            return (*this);
        }

    protected:
        uint32_t Serialize() const override
        {
            // Are we opening the file ?
            _opened = (Core::File::IsOpen() == false);

            if (_opened == false) {
                const_cast<FileBody*>(this)->LoadFileInfo();
                const_cast<FileBody*>(this)->Position(false, _startPosition);
            }
            return (((_opened == false) || (Core::File::Open() == true)) ? static_cast<uint32_t>(Core::File::Size() - _startPosition) : 0);
        }
        uint32_t Deserialize() override
        {
            _opened = (Core::File::IsOpen() == false);
            return (((_opened == false) || (Core::File::Create() == true)) ? static_cast<uint32_t>(~0) : 0);
        }
        uint16_t Serialize(uint8_t stream[], const uint16_t maxLength) const override
        {
            return Core::File::Read(stream, maxLength);
        }
        uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength) override
        {
            uint16_t write = Core::File::Write(stream, maxLength);
            if (!write) {
                _startPosition = Core::NumberType<int32_t>::Max();
            }

            return write;
        }
        void End() const override
        {
            if (Core::File::IsOpen() == true) {
                if (_opened == true) {
                    Core::File::Close();
                } else {
                    if (_startPosition == Core::NumberType<int32_t>::Max()) {
                        const_cast<FileBody*>(this)->SetSize(0);
                    } else {
                        const_cast<FileBody*>(this)->Position(false, _startPosition);
                    }
                }
            }
        }

    private:
        mutable bool _opened;
        mutable int32_t _startPosition;
    };

    template <typename HASHALGORITHM>
    class SignedFileBodyType : public FileBody {
    private:
        static int16_t constexpr BlockSize = 1024;
    public:
        using HashType = HASHALGORITHM;

        SignedFileBodyType(const SignedFileBodyType<HASHALGORITHM>&) = delete;
        SignedFileBodyType<HASHALGORITHM>& operator=(const SignedFileBodyType<HASHALGORITHM>&) = delete;

        SignedFileBodyType()
            : FileBody()
            , _hash()
        {
            _hash.Reset();
        }
        SignedFileBodyType(const string& HMACKey)
            : FileBody()
            , _hash(HMACKey)
        {
            _hash.Reset();
        }
        ~SignedFileBodyType() override = default;

    public:
        inline SignedFileBodyType<HASHALGORITHM>& operator=(const string& location)
        {
            FileBody::operator=(location);

            return (*this);
        }
        inline SignedFileBodyType<HASHALGORITHM>& operator=(const File& RHS)
        {
            FileBody::operator=(RHS);

            return (*this);
        }

    public:
        inline HashType& Hash()
        {
            return (_hash);
        }
        inline const HashType& Hash() const
        {
            return (_hash);
        }
        uint16_t Serialize(uint8_t stream[], const uint16_t maxLength) const override
        {
            return Core::File::Read(stream, maxLength);
        }

    protected:
        uint32_t Deserialize() override
        {
            return (FileBody::Deserialize());
        }
        uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength) override
        {
            uint16_t deserialized = FileBody::Deserialize(stream, maxLength);

            // Also pass it through our hashing algorithm.
            if (deserialized == maxLength) {
                _hash.Input(stream, maxLength);
            }

            return deserialized;
        }

    private:
        HashType _hash;
    };

    template <typename JSONOBJECT>
    class JSONBodyType : public JSONOBJECT, public IBody {
    public:
        JSONBodyType(JSONBodyType<JSONOBJECT>&&) = delete;
        JSONBodyType(const JSONBodyType<JSONOBJECT>&) = delete;
        JSONBodyType<JSONOBJECT>& operator=(JSONBodyType<JSONOBJECT>&&) = delete;
        JSONBodyType<JSONOBJECT>& operator=(const JSONBodyType<JSONOBJECT>&) = delete;

        JSONBodyType()
            : JSONOBJECT()
            , _lastPosition(0)
            , _offset(0)
            , _error()
        {
        }
        ~JSONBodyType() override = default;

    public:
        inline JSONBodyType<JSONOBJECT>& operator=(const JSONOBJECT& RHS)
        {
            JSONOBJECT::operator=(RHS);
            return (*this);
        }
        void Clear() override
        {
            // Before we clear this, 
            JSONOBJECT::Clear();
            _offset = 0;
        }
        const Core::OptionalType<Core::JSON::Error>& Report() const {
            return (_error);
        }

    protected:
        uint32_t Serialize() const override
        {
            _lastPosition = 0;
            _body.clear();

            // We need to transfer this to a string as we need to send the length 
            // upfront (in the headers)
            JSONOBJECT::ToString(_body);

            if (_body.length() <= 2) {
                _body.clear();
            }

            return (static_cast<uint32_t>(_body.length() * sizeof(TCHAR)));
        }
        uint32_t Deserialize() override
        {
            _error.Clear();
            return (static_cast<uint32_t>(~0));
        }
        uint16_t Serialize(uint8_t stream[], const uint16_t maxLength) const override
        {
            uint16_t size = static_cast<uint16_t>(maxLength > (_body.length() * sizeof(TCHAR)) ? (_body.length() * sizeof(TCHAR)) : (sizeof(TCHAR) == 1 ? maxLength : (maxLength & 0xFFFE)));

            if (size > 0) {
                ::memcpy(stream, &(reinterpret_cast<const uint8_t*>(_body.c_str())[_lastPosition]), size);
                _lastPosition += size;
            }
            return size;
        }
        uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength) override
        {
            return static_cast<Core::JSON::IElement&>(*this).Deserialize(reinterpret_cast<const char*>(stream), maxLength, _offset, _error);
        }
        void End() const override
        {
        }

    private:
        mutable uint32_t _lastPosition;
        mutable string _body;
        uint32_t _offset;
        Core::OptionalType<Core::JSON::Error> _error;
    };

    template <typename JSONOBJECT, typename HASHALGORITHM>
    class SignedJSONBodyType : public JSONBodyType<JSONOBJECT> {
    public:
        using HashType = HASHALGORITHM;

        SignedJSONBodyType(const SignedJSONBodyType<JSONOBJECT, HASHALGORITHM>&) = delete;
        SignedJSONBodyType<JSONOBJECT, HASHALGORITHM>& operator=(const SignedJSONBodyType<JSONOBJECT, HASHALGORITHM>&) = delete;

        SignedJSONBodyType()
            : JSONBodyType<JSONOBJECT>()
            , _hash()
        {
        }
        SignedJSONBodyType(const string& HMACKey)
            : JSONBodyType<JSONOBJECT>()
            , _hash(HMACKey)
        {
        }
        ~SignedJSONBodyType() override = default;

    public:
        inline HashType& Hash()
        {
            return (_hash);
        }
        inline const HashType& Hash() const
        {
            return (_hash);
        }

    protected:
        uint32_t Deserialize() override
        {
            _hash.Reset();

            return (JSONBodyType<JSONOBJECT>::Deserialize());
        }
        uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength) override
        {
            uint16_t deserialized = JSONBodyType<JSONOBJECT>::Deserialize(stream, maxLength);

            // Also pass it through our hashing algorithm.
            if (deserialized == maxLength) {
                _hash.Input(stream, maxLength);
            }

            return deserialized;
        }

    private:
        HashType _hash;
    };

    namespace JSONRPC {

        class Body : public JSONBodyType<Core::JSONRPC::Message> {
        public:
            Body(Body&&) = delete;
            Body(const Body&) = delete;
            Body& operator=(Body&&) = delete;
            Body& operator=(const Body&) = delete;

            Body() = default;
            ~Body() override = default;

        public:
            void Clear() override {
                // If by an error this gets cleared, at least remember the Id, if it was set...
                if (Core::JSONRPC::Message::Id.IsSet() == true) {
                    _id = Core::JSONRPC::Message::Id.Value();
                }
                JSONBodyType<Core::JSONRPC::Message>::Clear();
            }
            uint32_t Deserialize() override
            {
                _id.Clear();
                return (JSONBodyType<Core::JSONRPC::Message>::Deserialize());
            }
            const Core::OptionalType<uint32_t>& Recorded() const {
                return (_id);
            }

        private:
            Core::OptionalType<uint32_t> _id;
        };

    }
}
}
