#ifndef __WEBSERIALIZER_H
#define __WEBSERIALIZER_H

#include "Module.h"
#include "WebRequest.h"
#include "WebResponse.h"

namespace WPEFramework {
namespace Web {

    class EXTERNAL TextBody : public string, public IBody {
    private:
        TextBody(const TextBody&) = delete;
        TextBody& operator=(const TextBody&) = delete;

    public:
        TextBody()
        {
        }
        virtual ~TextBody()
        {
        }

        TextBody& operator=(const string& newText)
        {
            string::operator=(newText);

            return (*this);
        }

    public:
        inline void Clear()
        {
            string::clear();
        }

    protected:
        virtual uint32_t Serialize() const override
        {
            _lastPosition = 0;
            return (string::length() * sizeof(TCHAR));
        }
        virtual uint32_t Deserialize() override
        {
            string::clear();
            return (static_cast<uint32_t>(~0));
        }
        virtual void Serialize(uint8_t stream[], const uint16_t maxLength) const override
        {
            uint16_t size = static_cast<uint16_t>(maxLength > (string::length() * sizeof(TCHAR)) ? (string::length() * sizeof(TCHAR)) : (sizeof(TCHAR) == 1 ? maxLength : (maxLength & 0xFFFE)));

            if (size > 0) {
                ::memcpy(stream, &(reinterpret_cast<const uint8_t*>(string::c_str())[_lastPosition]), size);
                _lastPosition += size;
            }
        }
        virtual void Deserialize(const uint8_t stream[], const uint16_t maxLength) override
        {
            uint16_t index = 0;

            while (index < maxLength) {
                string::operator+=(stream[index]);
                index++;
            }
        }
        virtual void End() const override
        {
        }
        mutable uint32_t _lastPosition;
    };

    template <typename HASHALGORITHM>
    class SignedTextBodyType : public TextBody {
    private:
        SignedTextBodyType(const SignedTextBodyType<HASHALGORITHM>&);
        SignedTextBodyType<HASHALGORITHM>& operator=(const SignedTextBodyType<HASHALGORITHM>&);

    public:
        SignedTextBodyType()
            : TextBody()
            , _hash()
        {
            _hash.Reset();
        }
        SignedTextBodyType(const string& HMACKey)
            : TextBody()
            , _hash(HMACKey)
        {
            _hash.Reset();
        }
        virtual ~SignedTextBodyType()
        {
        }

    public:
        inline static Crypto::EnumHashType HashType()
        {
            return (HASHALGORITHM::Type);
        }
        inline uint8_t* HashValue() const
        {
            return (_hash.Result());
        }

    protected:
        virtual uint32_t Deserialize() override
        {
            _hash.Reset();
            return (TextBody::Deserialize());
        }
        virtual void Deserialize(const uint8_t stream[], const uint16_t maxLength) override
        {
            TextBody::Deserialize(stream, maxLength);

            // Also pass it through our hashing algorithm.
            _hash.Input(stream, maxLength);
        }

        HASHALGORITHM _hash;
    };

    class EXTERNAL FileBody : public Core::File, public IBody {
    private:
        FileBody(const FileBody&) = delete;
        FileBody& operator=(const FileBody&) = delete;

    public:
        FileBody()
            : Core::File()
            , _opened(false)
            , _startPosition(0)
        {
        }

        FileBody(const string& path, const bool sharable)
            : Core::File(path, sharable)
            , _opened(false)
            , _startPosition(0)
        {
        }

        virtual ~FileBody()
        {
        }

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
            _startPosition = Core::File::Position();

            return (*this);
        }

    protected:
        virtual uint32_t Serialize() const override
        {
            _opened = (Core::File::IsOpen() == false);

            if (_opened == false) {
                const_cast<FileBody*>(this)->LoadFileInfo();
                const_cast<FileBody*>(this)->Position(false, _startPosition);
            }
            return (((_opened == false) || (Core::File::Open() == true)) ? static_cast<uint32_t>(Core::File::Size() - _startPosition) : 0);
        }
        virtual uint32_t Deserialize() override
        {
            _opened = (Core::File::IsOpen() == false);
            return (((_opened == false) || (Core::File::Create() == true)) ? static_cast<uint32_t>(~0) : 0);
        }
        virtual void Serialize(uint8_t stream[], const uint16_t maxLength) const override
        {

            Core::File::Read(stream, maxLength);
        }
        virtual void Deserialize(const uint8_t stream[], const uint16_t maxLength) override
        {
            Core::File::Write(stream, maxLength);
        }
        virtual void End() const override
        {
            if (Core::File::IsOpen() == true) {
                if (_opened == true) {
                    Core::File::Close();
                }
                else {
                    const_cast<FileBody*>(this)->Position(false, _startPosition);
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
        SignedFileBodyType(const SignedFileBodyType<HASHALGORITHM>&) = delete;
        SignedFileBodyType<HASHALGORITHM>& operator=(const SignedFileBodyType<HASHALGORITHM>&) = delete;

    public:
        SignedFileBodyType()
            : FileBody()
            , _hash()
        {
        }
        SignedFileBodyType(const string& HMACKey)
            : FileBody()
            , _hash(HMACKey)
        {
        }
        virtual ~SignedFileBodyType()
        {
        }

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
        inline static Crypto::EnumHashType HashType()
        {
            return (HASHALGORITHM::Type);
        }
        inline const uint8_t* SerializedHashValue() const
        {
            _hash.Reset();

            // Read all Data
            uint32_t length = FileBody::Serialize();

            uint8_t buffer[64];

            while (length > 0) {
                uint16_t size = (length > 64 ? 64 : length);
                FileBody::Serialize(buffer, size);
                _hash.Input(buffer, size);
                length -= size;
            }

            FileBody::End();

            return (_hash.Result());
        }
        inline const uint8_t* DeserializedHashValue() const
        {
            return (_hash.Result());
        }

    protected:
        virtual uint32_t Deserialize() override
        {
            _hash.Reset();

            return (FileBody::Deserialize());
        }
        virtual void Deserialize(const uint8_t stream[], const uint16_t maxLength) override
        {
            FileBody::Deserialize(stream, maxLength);

            // Also pass it through our hashing algorithm.
            _hash.Input(stream, maxLength);
        }

        mutable HASHALGORITHM _hash;
    };

    template <typename JSONOBJECT>
    class JSONBodyType : public JSONOBJECT, public IBody {
    private:
        typedef JSONBodyType<JSONOBJECT> ThisClass;

        class JSONDeserializer : public Core::JSON::IElement::Deserializer {
        private:
            JSONDeserializer();
            JSONDeserializer(const JSONDeserializer&);
            JSONDeserializer& operator=(const JSONDeserializer&);

        public:
            JSONDeserializer(JSONOBJECT& jsonElement)
                : Core::JSON::IElement::Deserializer()
                , _element(jsonElement)
            {
            }
            ~JSONDeserializer()
            {
            }

        public:
            virtual Core::JSON::IElement* Element(const string& identifier) override
            {
                return &_element;
            }
            virtual void Deserialized(Core::JSON::IElement& /* element */) override
            {
            }

        private:
            JSONOBJECT& _element;
        };

    private:
        JSONBodyType(const JSONBodyType<JSONOBJECT>&);
        JSONBodyType<JSONOBJECT>& operator=(const JSONBodyType<JSONOBJECT>&);

    public:
		#ifdef __WIN32__
		#pragma warning( disable : 4355 )
		#endif
        JSONBodyType()
            : JSONOBJECT()
            , _deserializer(*this)
        {
        }
		#ifdef __WIN32__
		#pragma warning( default : 4355 )
		#endif
        virtual ~JSONBodyType()
        {
        }

        JSONBodyType<JSONOBJECT>& operator=(const JSONOBJECT& RHS)
        {
            JSONOBJECT::operator=(RHS);

            return (*this);
        }

    public:
        /* JIRA: BRIDGE-165
         *
         * Bad code but if you remove it without fixing the duplicated json issue for json arrays you
         * buy us apple pie :-)
         *
         * cppcheck will give a warning on this code, but if we remove this the json output is not cleared properly
         * Pierre will think out a solution on how to fix this warning for the future, because cppcheck is right
         * about this one.
         */
        inline void Clear()
        {
            JSONOBJECT::Clear();
        }

    protected:
        virtual uint32_t Serialize() const override
        {
            _lastPosition = 0;
            _body.clear();

            JSONOBJECT::ToString(_body);

            if (_body.length() <= 2) {
                _body.clear();
            }

            return (_body.length() * sizeof(TCHAR));
        }
        virtual uint32_t Deserialize() override
        {
            return (static_cast<uint32_t>(~0));
        }
        virtual void Serialize(uint8_t stream[], const uint16_t maxLength) const override
        {
            uint16_t size = static_cast<uint16_t>(maxLength > (_body.length() * sizeof(TCHAR)) ? (_body.length() * sizeof(TCHAR)) : (sizeof(TCHAR) == 1 ? maxLength : (maxLength & 0xFFFE)));

            if (size > 0) {
                ::memcpy(stream, &(reinterpret_cast<const uint8_t*>(_body.c_str())[_lastPosition]), size);
                _lastPosition += size;
            }
        }
        virtual void Deserialize(const uint8_t stream[], const uint16_t maxLength) override
        {
            _deserializer.Deserialize(stream, maxLength);
        }
        virtual void End() const override
        {
        }

    private:
        JSONDeserializer _deserializer;
        mutable uint32_t _lastPosition;
        mutable string _body;
    };

    template <typename JSONOBJECT, typename HASHALGORITHM>
    class SignedJSONBodyType : public JSONBodyType<JSONOBJECT> {
    private:
        SignedJSONBodyType(const SignedJSONBodyType<JSONOBJECT, HASHALGORITHM>&);
        SignedJSONBodyType<JSONOBJECT, HASHALGORITHM>& operator=(const SignedJSONBodyType<JSONOBJECT, HASHALGORITHM>&);

    public:
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
        virtual ~SignedJSONBodyType()
        {
        }

    public:
        inline static Crypto::EnumHashType HashType()
        {
            return (HASHALGORITHM::Type);
        }
        inline const uint8_t* HashValue() const
        {
            return (const_cast<HASHALGORITHM&>(_hash).Result());
        }

    protected:
        virtual uint32_t Deserialize() override
        {
            _hash.Reset();

            return (JSONBodyType<JSONOBJECT>::Deserialize());
        }
        virtual void Deserialize(const uint8_t stream[], const uint16_t maxLength) override
        {
            JSONBodyType<JSONOBJECT>::Deserialize(stream, maxLength);

            // Also pass it through our hashing algorithm.
            _hash.Input(stream, maxLength);
        }

        HASHALGORITHM _hash;
    };
}
}

#endif // __WEBSERIALIZER_H|
