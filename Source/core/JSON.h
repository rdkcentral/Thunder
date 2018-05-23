#ifndef __JSON_H
#define __JSON_H

#include <map>

#include "Enumerate.h"
#include "FileSystem.h"
#include "Number.h"
#include "Portability.h"
#include "Proxy.h"
#include "Serialization.h"
#include "TextFragment.h"
#include "TypeTraits.h"

#define JSONTYPEID(JSONNAME) \
    \
public:                      \
    static const char* Id() { return (#JSONNAME); }

namespace WPEFramework {
namespace Core {
    namespace JSON {

        bool EXTERNAL ContainsNull(const string& value);

        struct IIterator;
        class String;

        typedef enum {
            PARSE_CONTAINER,
            PARSE_DIRECT,
            PARSE_BUFFERED

        } ParserType;

        struct EXTERNAL IDirect {
            virtual ~IDirect() {}

            // If this should be serialized/deserialized, it is indicated by a MinSize > 0)
            virtual uint16_t Serialize(char Stream[], const uint16_t MaxLength, uint16_t& offset) const = 0;
            virtual uint16_t Deserialize(const char Stream[], const uint16_t MaxLength, uint16_t& offset) = 0;
        };

        struct EXTERNAL IBuffered {
            virtual ~IBuffered() {}

            // If this should be serialized/deserialized, it is indicated by a MinSize > 0)
            virtual void Serialize(std::string& buffer) const = 0;
            virtual void Deserialize(const std::string& buffer) = 0;
        };

        struct EXTERNAL IElement {
            virtual ~IElement() {}

            class EXTERNAL Serializer {
            private:
                typedef enum {
                    STATE_OPEN,
                    STATE_LABEL,
                    STATE_DELIMITER,
                    STATE_VALUE,
                    STATE_CLOSE,
                    STATE_PREFIX,
                    STATE_REPORT

                } State;

            private:
                Serializer(const Serializer&);
                Serializer& operator=(const Serializer&);

            public:
                Serializer()
                    : _root(nullptr)
                    , _handleStack()
                    , _state(STATE_OPEN)
                    , _offset(0)
                    , _bufferUsed(0)
                    , _adminLock()
                    , _buffer()
                    , _prefix()
                {
                }
                virtual ~Serializer()
                {
                }

            public:
                virtual void Serialized(const IElement& element) = 0;

                void Flush()
                {
                    _adminLock.Lock();
                    _handleStack.clear();
                    if (_root != nullptr) {
                        const IElement* element = _root;
                        _root = nullptr;

                        Serialized(*element);
                    }
                    _adminLock.Unlock();
                }
                void Submit(const IElement& element)
                {
                    // If we are in progress, do not offer a new one. Wait till we are ready !!
                    ASSERT(_root == nullptr);

                    _adminLock.Lock();

                    if (element.Label() != nullptr) {
                        Core::ToString(element.Label(), _prefix);

                        ASSERT(_prefix.empty() == false);

                        _state = STATE_PREFIX;
                        _root = const_cast<IElement*>(&element);
                        _handleStack.push_front(_root->ElementIterator());
                        _offset = 0;
                    } else {
                        _prefix.clear();
                        _state = STATE_OPEN;
                        _root = const_cast<IElement*>(&element);
                        _handleStack.push_front(_root->ElementIterator());
                        _offset = 0;
                    }

                    _adminLock.Unlock();
                }
                uint16_t Serialize(uint8_t stream[], const uint16_t maxLength);

            private:
                IElement* _root;
                std::list<JSON::IIterator*> _handleStack;
                State _state;
                uint16_t _offset;
                uint16_t _bufferUsed;
                Core::CriticalSection _adminLock;
                std::string _buffer;
                std::string _prefix;
            };

            class EXTERNAL Deserializer {
            private:
                typedef enum {
                    STATE_OPEN,
                    STATE_LABEL,
                    STATE_DELIMITER,
                    STATE_VALUE,
                    STATE_CLOSE,
                    STATE_SKIP,
                    STATE_START,
                    STATE_PREFIX,
                    STATE_HANDLED

                } State;

                typedef enum {
                    QUOTED_OFF,
                    QUOTED_ON,
                    QUOTED_ESCAPED
                } Quoted;

            private:
                Deserializer(const Deserializer&);
                Deserializer& operator=(const Deserializer&);

            public:
                Deserializer()
                    : _handling(nullptr)
                    , _handleStack()
                    , _state(STATE_HANDLED)
                    , _offset(0)
                    , _bufferUsed(0)
                    , _quoted(QUOTED_OFF)
                    , _levelSkip(0)
                    , _adminLock()
                    , _buffer()
                {
                }
                virtual ~Deserializer()
                {
                }

            public:
                virtual void Deserialized(IElement& element) = 0;

                void Flush()
                {
                    _adminLock.Lock();
                    _handleStack.clear();
                    if (_handling != nullptr) {
                        Deserialized(*_handling);
                        _handling = nullptr;
                        _handlingName.clear();
                    }
                    _state = STATE_HANDLED;
                    _adminLock.Unlock();
                }

                inline const string& Label() const
                {
                    return (_handlingName);
                }

                uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength);

            private:
                virtual IElement* Element(const string& identifer) = 0;

            private:
                IElement* _handling;
                string _handlingName;
                std::list<JSON::IIterator*> _handleStack;
                State _state;
                uint16_t _offset;
                uint16_t _bufferUsed;
                Quoted _quoted;
                uint16_t _levelSkip;
                Core::CriticalSection _adminLock;
                std::string _buffer;
            };

            template <typename INSTANCEOBJECT>
            static void ToString(const INSTANCEOBJECT& realObject, string& text)
            {
                uint16_t fillCount = 0;
                bool ready = false;
                class SerializerImpl : public Serializer {
                public:
                    SerializerImpl(bool& readyFlag)
                        : INSTANCEOBJECT::Serializer()
                        , _ready(readyFlag)
                    {
                    }
                    virtual ~SerializerImpl()
                    {
                    }

                public:
                    virtual void Serialized(const Core::JSON::IElement& /* element */)
                    {
                        _ready = true;
                    }

                private:
                    bool& _ready;
                } serializer(ready);

                // Request an object to e serialized..
                serializer.Submit(realObject);

                // Serialize object
                while (ready == false) {
                    uint8_t buffer[1024];
                    uint16_t loaded = serializer.Serialize(buffer, sizeof(buffer));

                    ASSERT(loaded <= sizeof(buffer));

                    fillCount += loaded;

                    text += string(reinterpret_cast<char*>(&buffer[0]), loaded);
                }
            }

            template <typename INSTANCEOBJECT>
            static bool FromString(const string& text, INSTANCEOBJECT& realObject)
            {
                bool ready = false;
                class DeserializerImpl : public Deserializer {
                public:
                    DeserializerImpl(bool& ready)
                        : INSTANCEOBJECT::Deserializer()
                        , _element(nullptr)
                        , _ready(ready)
                    {
                    }
                    ~DeserializerImpl()
                    {
                    }

                public:
                    inline void SetElement(Core::JSON::IElement* element)
                    {
                        ASSERT(_element == nullptr);
                        _element = element;
                    }
                    virtual Core::JSON::IElement* Element(const string& identifier)
                    {
                        DEBUG_VARIABLE(identifier);
                        ASSERT(identifier.empty() == true);
                        return (_element);
                    }
                    virtual void Deserialized(Core::JSON::IElement& element)
                    {
                        DEBUG_VARIABLE(element);
                        ASSERT(&element == _element);
                        _element = nullptr;
                        _ready = true;
                    }

                private:
                    Core::JSON::IElement* _element;
                    bool& _ready;
                } deserializer(ready);

                uint16_t fillCount = 0;
                realObject.Clear();

                deserializer.SetElement(&realObject);

                // Serialize object
                while ((ready == false) && (fillCount < text.size())) {
                    uint8_t buffer[1024];
                    uint16_t size = ((text.size() - fillCount) < sizeof(buffer) ? (static_cast<uint16_t>(text.size()) - fillCount) : sizeof(buffer));

                    // Prepare the deserialize buffer
                    memcpy(buffer, &(text.data()[fillCount]), size);

                    fillCount += size;

                    deserializer.Deserialize(buffer, size);
                }

                return (ready == true);
            }

            void ToString(string& text) const
            {
                Core::JSON::IElement::ToString(*this, text);
            }
            bool FromString(const string& text)
            {
                return (Core::JSON::IElement::FromString(text, *this));
            }

            template <typename INSTANCEOBJECT>
            static bool ToFile(Core::File& fileObject, const INSTANCEOBJECT& realObject)
            {
                bool ready = false;

                if (fileObject.IsOpen()) {
                    uint32_t missedBytes = 0;

                    class SerializerImpl : public Serializer {
                    public:
                        SerializerImpl(bool& readyFlag)
                            : INSTANCEOBJECT::Serializer()
                            , _ready(readyFlag)
                        {
                        }
                        virtual ~SerializerImpl()
                        {
                        }

                    public:
                        virtual void Serialized(const Core::JSON::IElement& /* element */)
                        {
                            _ready = true;
                        }

                    private:
                        bool& _ready;
                    } serializer(ready);

                    // Request an object to e serialized..
                    serializer.Submit(realObject);

                    // Serialize object
                    while ((ready == false) && (missedBytes == 0)) {
                        uint8_t buffer[1024];
                        uint16_t bytes = serializer.Serialize(buffer, sizeof(buffer));

                        missedBytes = (fileObject.Write(buffer, bytes) - bytes);
                    }
                }

                return (ready);
            }

            template <typename INSTANCEOBJECT>
            static bool FromFile(Core::File& fileObject, INSTANCEOBJECT& realObject)
            {
                bool ready = false;

                if (fileObject.IsOpen() == true) {
                    uint16_t unusedBytes = 0;
                    uint16_t readBytes = static_cast<uint16_t>(~0);

                    class DeserializerImpl : public Deserializer {
                    public:
                        DeserializerImpl(bool& ready)
                            : INSTANCEOBJECT::Deserializer()
                            , _element(nullptr)
                            , _ready(ready)
                        {
                        }
                        ~DeserializerImpl()
                        {
                        }

                    public:
                        inline void SetElement(Core::JSON::IElement* element)
                        {
                            ASSERT(_element == nullptr);
                            _element = element;
                        }
                        virtual Core::JSON::IElement* Element(const string& identifier)
                        {
                            DEBUG_VARIABLE(identifier);
                            ASSERT(identifier.empty() == true);
                            return (_element);
                        }
                        virtual void Deserialized(Core::JSON::IElement& element)
                        {
                            DEBUG_VARIABLE(element);
                            ASSERT(&element == _element);
                            _element = nullptr;
                            _ready = true;
                        }

                    private:
                        Core::JSON::IElement* _element;
                        bool& _ready;
                    } deserializer(ready);

                    realObject.Clear();

                    deserializer.SetElement(&realObject);

                    while ((ready == false) && (unusedBytes == 0) && (readBytes != 0)) {
                        uint8_t buffer[1024];

                        readBytes = static_cast<uint16_t>(fileObject.Read(buffer, sizeof(buffer)));

                        if (readBytes != 0) {
                            // Deserialize object
                            uint16_t usedBytes = deserializer.Deserialize(buffer, readBytes);

                            unusedBytes = (readBytes - usedBytes);
                        }
                    }

                    if (unusedBytes != 0) {
                        fileObject.Position(true, -unusedBytes);
                    }
                }

                return (ready == true);
            }

            bool ToFile(Core::File& fileObject) const
            {
                return (Core::JSON::IElement::ToFile(fileObject, *this));
            }
            bool FromFile(Core::File& fileObject)
            {
                return (Core::JSON::IElement::FromFile(fileObject, *this));
            }

            virtual const char* Label() const
            {
                return nullptr;
            }
            static const char* Id()
            {
                return nullptr;
            }

            virtual ParserType Type() const = 0;
            virtual bool IsSet() const = 0;
            virtual void Clear() = 0;

            virtual IBuffered* BufferParser() = 0;
            virtual IDirect* DirectParser() = 0;
            virtual IIterator* ElementIterator() = 0;
        };

        struct IIterator {
            virtual ~IIterator() {}

            // Just move the iterator to the next point in the map.
            virtual bool Next() = 0;

            // Return the element, we are currently pointing at.
            virtual IElement* Element() = 0;
        };

        struct ILabelIterator : public IIterator {
            virtual ~ILabelIterator() {}

            // Move the iterator to the element that is compliant with the label.
            virtual const char* Label() const = 0;
            virtual bool Find(const char label[]) = 0;
        };

        struct IArrayIterator : public IIterator {
            virtual ~IArrayIterator() {}

            virtual void AddElement() = 0;
        };

        template <class TYPE, bool SIGNED, const NumberBase BASETYPE>
        class NumberType : public IElement, public IBuffered {
        public:
            NumberType()
                : _set(false)
                , _value()
                , _default(0)
            {
            }
            NumberType(const TYPE Value, const bool set = false)
                : _set(set)
                , _value(set == false ? 0 : Value)
                , _default(Value)
            {
            }
            NumberType(const NumberType<TYPE, SIGNED, BASETYPE>& copy)
                : _set(copy._set)
                , _value(copy._value)
                , _default(copy._default)
            {
            }
            virtual ~NumberType()
            {
            }

            NumberType<TYPE, SIGNED, BASETYPE>& operator=(const NumberType<TYPE, SIGNED, BASETYPE>& RHS)
            {
                _value = RHS._value;
                _set = RHS._set;

                return (*this);
            }
            NumberType<TYPE, SIGNED, BASETYPE>& operator=(const TYPE& RHS)
            {
                _value = RHS;
                _set = true;

                return (*this);
            }

            inline const TYPE Default() const
            {
                return _default;
            }
            inline const TYPE Value() const
            {
                return (_set == true ? _value.Value() : _default);
            }
            inline operator const TYPE() const
            {
                return Value();
            }

            // IElement interface methods
            virtual bool IsSet() const override
            {
                return (_set == true);
            }
            virtual void Clear() override
            {
                _set = false;
            }

        private:
            // IElement interface methods (private)
            virtual ParserType Type() const override
            {
                return (PARSE_BUFFERED);
            }
            virtual IBuffered* BufferParser() override
            {
                return this;
            }

            virtual IDirect* DirectParser() override
            {
                return nullptr;
            }

            virtual IIterator* ElementIterator() override
            {
                return nullptr;
            }

            // IBuffered interface methods (private)
            virtual void Serialize(std::string& buffer) const override
            {
                _value.Serialize(buffer);

                if (BASETYPE != BASE_DECIMAL) {
                    // JSON does not support octal and hexadecimal format as number,
                    // make it string between double quotes
                    buffer = '"' + buffer + '"';
                }
            }
            virtual void Deserialize(const std::string& buffer) override
            {
                _set = !ContainsNull(buffer);

                if (_set == true) {

                    _value.Deserialize(buffer);
                }
            }

        private:
            bool _set;
            Core::NumberType<TYPE, SIGNED, BASETYPE> _value;
            TYPE _default;
        };

        typedef NumberType<uint8_t, false, BASE_DECIMAL> DecUInt8;
        typedef NumberType<int8_t, true, BASE_DECIMAL> DecSInt8;
        typedef NumberType<uint16_t, false, BASE_DECIMAL> DecUInt16;
        typedef NumberType<int16_t, true, BASE_DECIMAL> DecSInt16;
        typedef NumberType<uint32_t, false, BASE_DECIMAL> DecUInt32;
        typedef NumberType<int32_t, true, BASE_DECIMAL> DecSInt32;
        typedef NumberType<uint64_t, false, BASE_DECIMAL> DecUInt64;
        typedef NumberType<int64_t, true, BASE_DECIMAL> DecSInt64;
        typedef NumberType<uint8_t, false, BASE_HEXADECIMAL> HexUInt8;
        typedef NumberType<int8_t, true, BASE_HEXADECIMAL> HexSInt8;
        typedef NumberType<uint16_t, false, BASE_HEXADECIMAL> HexUInt16;
        typedef NumberType<int16_t, true, BASE_HEXADECIMAL> HexSInt16;
        typedef NumberType<uint32_t, false, BASE_HEXADECIMAL> HexUInt32;
        typedef NumberType<int32_t, true, BASE_HEXADECIMAL> HexSInt32;
        typedef NumberType<uint64_t, false, BASE_HEXADECIMAL> HexUInt64;
        typedef NumberType<int64_t, true, BASE_HEXADECIMAL> HexSInt64;
        typedef NumberType<uint8_t, false, BASE_OCTAL> OctUInt8;
        typedef NumberType<int8_t, true, BASE_OCTAL> OctSInt8;
        typedef NumberType<uint16_t, false, BASE_OCTAL> OctUInt16;
        typedef NumberType<int16_t, true, BASE_OCTAL> OctSInt16;
        typedef NumberType<uint32_t, false, BASE_OCTAL> OctUInt32;
        typedef NumberType<int32_t, true, BASE_OCTAL> OctSInt32;
        typedef NumberType<uint64_t, false, BASE_OCTAL> OctUInt64;
        typedef NumberType<int64_t, true, BASE_OCTAL> OctSInt64;

        template <typename ENUMERATE>
        class EnumType : public IElement, public IBuffered {
        public:
            EnumType()
                : _value()
            {
            }
            EnumType(const ENUMERATE Value)
                : _value()
                , _default(Value)
            {
            }
            EnumType(const EnumType<ENUMERATE>& copy)
                : _value(copy._value)
                , _default(copy._default)
            {
            }
            virtual ~EnumType()
            {
            }

            EnumType<ENUMERATE>& operator=(const EnumType<ENUMERATE>& RHS)
            {
                _value = RHS._value;

                return (*this);
            }
            EnumType<ENUMERATE>& operator=(const ENUMERATE& RHS)
            {
                _value = RHS;

                return (*this);
            }

            inline const ENUMERATE Default() const
            {
                return (_default);
            }
            inline const ENUMERATE Value() const
            {
                return (_value.IsSet() == true ? _value.Value() : _default);
            }
            inline operator const ENUMERATE() const
            {
                return Value();
            }
            const TCHAR* Data() const
            {
                return (_value.Data());
            }

            // IElement interface methods
            virtual bool IsSet() const override
            {
                return (_value.IsSet() == true);
            }
            virtual void Clear() override
            {
                _value.Clear();
            }

        private:
            // IElement interface methods (private)
            virtual ParserType Type() const override
            {
                return (PARSE_BUFFERED);
            }
            virtual IBuffered* BufferParser() override
            {
                return this;
            }

            virtual IDirect* DirectParser() override
            {
                return nullptr;
            }

            virtual IIterator* ElementIterator() override
            {
                return nullptr;
            }

            // IBuffered interface methods (private)
            virtual void Serialize(std::string& buffer) const override
            {
                uint16_t index = 0;

                // We always start with a quote...
                buffer = '\"';

                const TCHAR* enumText = _value.Data();

                while (enumText[index] != '\0') {
                    buffer += enumText[index++];
                }

                // and end with a quote...
                buffer += '\"';
            }
            virtual void Deserialize(const std::string& buffer) override
            {
                if (ContainsNull(buffer)) {
                    _value.Clear();
                } else {
                    _value.Assignment(true, buffer.c_str());
                }
            }

        private:
            Core::EnumerateType<ENUMERATE> _value;
            ENUMERATE _default;
        };

        class EXTERNAL Boolean : public IElement, public IBuffered {
        private:
            static constexpr uint8_t None = 0x00;
            static constexpr uint8_t ValueBit = 0x01;
            static constexpr uint8_t DefaultBit = 0x02;
            static constexpr uint8_t SetBit = 0x04;

        public:
            Boolean()
                : _value(None)
            {
            }
            Boolean(const bool Value)
                : _value(Value ? DefaultBit : None)
            {
            }
            Boolean(const Boolean& copy)
                : _value(copy._value)
            {
            }
            ~Boolean()
            {
            }

            Boolean& operator=(const Boolean& RHS)
            {
                // Do not overwrite the default, if not set...copy if set
                _value = (RHS._value & (SetBit | ValueBit)) | ((RHS._value & (SetBit)) ? (RHS._value & DefaultBit) : (_value & DefaultBit));

                return (*this);
            }
            Boolean& operator=(const bool& RHS)
            {
                // Do not overwrite the default
                _value = (RHS ? (SetBit | ValueBit) : SetBit) | (_value & DefaultBit);

                return (*this);
            }

            inline bool Value() const
            {
                return (IsSet() ? (_value & ValueBit) != None : (_value & DefaultBit) != None);
            }
            inline bool Default() const
            {
                return (_value & DefaultBit) != None;
            }
            inline operator bool() const
            {
                return Value();
            }

            // IElement interface methods
            virtual bool IsSet() const override
            {
                return ((_value & SetBit) != None);
            }
            virtual void Clear() override
            {
                _value = (_value & DefaultBit);
            }

        private:
            // IElement interface methods (private)
            virtual ParserType Type() const override
            {
                return (PARSE_BUFFERED);
            }
            virtual IBuffered* BufferParser() override
            {
                return this;
            }

            virtual IDirect* DirectParser() override
            {
                return nullptr;
            }

            virtual IIterator* ElementIterator() override
            {
                return nullptr;
            }

            // IBuffered interface methods (private)
            virtual void Serialize(std::string& buffer) const override
            {
                if (Value() == true) {
                    buffer = "true";
                } else {
                    buffer = "false";
                }
            }

            virtual void Deserialize(const std::string& buffer) override
            {
                if (buffer.length() == 1) {
                    if ((buffer[0] == '1') || (toupper(buffer[0]) == 'T')) {
                        _value = (SetBit | ValueBit) | (_value & DefaultBit);
                    } else if ((buffer[0] == '0') || (toupper(buffer[0]) == 'F')) {
                        _value = SetBit | (_value & DefaultBit);
                    }
                } else if ((buffer.length() >= 4) && (toupper(buffer[0]) == 'T') && (toupper(buffer[1]) == 'R') && (toupper(buffer[2]) == 'U') && (toupper(buffer[3]) == 'E')) {
                    _value = (SetBit | ValueBit) | (_value & DefaultBit);
                } else if ((buffer.length() >= 5) && (toupper(buffer[0]) == 'F') && (toupper(buffer[1]) == 'A') && (toupper(buffer[2]) == 'L') && (toupper(buffer[3]) == 'S') && (toupper(buffer[4]) == 'E')) {
                    _value = SetBit | (_value & DefaultBit);
                }
            }

        private:
            uint8_t _value;
        };

        class EXTERNAL String : public IElement, public IDirect {
        private:
            static constexpr uint32_t None = 0x00000000;
            static constexpr uint32_t ScopeMask = 0x1FFFFFFF;
            static constexpr uint32_t QuotedSerializeBit = 0x80000000;
            static constexpr uint32_t SetBit = 0x40000000;
            static constexpr uint32_t QuoteFoundBit = 0x20000000;

        public:
            String(const bool quoted = true)
                : _default()
                , _scopeCount(quoted ? QuotedSerializeBit : None)
                , _value()
            {
            }
            explicit String(const string& Value, const bool quoted = true)
                : _default()
                , _scopeCount(quoted ? QuotedSerializeBit : None)
                , _value()
            {
                Core::ToString(Value.c_str(), _default);
            }
            explicit String(const char Value[], const bool quoted = true)
                : _default()
                , _scopeCount(quoted ? QuotedSerializeBit : None)
                , _value()
            {
                Core::ToString(Value, _default);
            }
#ifndef __NO_WCHAR_SUPPORT__
            explicit String(const wchar_t Value[], const bool quoted = true)
                : _default()
                , _scopeCount(quoted ? QuotedSerializeBit : None)
                , _value()
            {
                Core::ToString(Value, _default);
            }
#endif // __NO_WCHAR_SUPPORT__
            String(const String& copy)
                : _default(copy._default)
                , _scopeCount(copy._scopeCount & (QuotedSerializeBit | SetBit))
                , _value(copy._value)
            {
            }
            virtual ~String()
            {
            }

            String& operator=(const string& RHS)
            {
                Core::ToString(RHS.c_str(), _value);
                _scopeCount |= SetBit;

                return (*this);
            }
            String& operator=(const char RHS[])
            {
                Core::ToString(RHS, _value);
                _scopeCount |= SetBit;

                return (*this);
            }
#ifndef __NO_WCHAR_SUPPORT__
            String& operator=(const wchar_t RHS[])
            {
                Core::ToString(RHS, _value);
                _scopeCount |= SetBit;

                return (*this);
            }
#endif // __NO_WCHAR_SUPPORT__
            String& operator=(const String& RHS)
            {
                _default = RHS._default;
                _value = RHS._value;
                _scopeCount = RHS._scopeCount & (QuotedSerializeBit | SetBit);

                return (*this);
            }
            inline bool operator==(const String& RHS) const
            {
                return (Value() == RHS.Value());
            }
            inline bool operator!=(const String& RHS) const
            {
                return (!operator==(RHS));
            }
            inline bool operator==(const char RHS[]) const
            {
                return (Value() == RHS);
            }

            inline bool operator!=(const char RHS[]) const
            {
                return (!operator==(RHS));
            }
#ifndef __NO_WCHAR_SUPPORT__
            inline bool operator==(const wchar_t RHS[]) const
            {
                std::string comparator;
                Core::ToString(RHS, comparator);
                return (Value() == comparator);
            }

            inline bool operator!=(const wchar_t RHS[]) const
            {
                return (!operator==(RHS));
            }

#endif // __NO_WCHAR_SUPPORT__
            inline bool operator<(const String& RHS) const
            {
                return (Value() < RHS.Value());
            }
            inline bool operator>(const String& RHS) const
            {
                return (Value() > RHS.Value());
            }
            inline bool operator>=(const String& RHS) const
            {
                return (!operator<(RHS));
            }
            inline bool operator<=(const String& RHS) const
            {
                return (!operator>(RHS));
            }

            inline const string Value() const
            {
                return (((_scopeCount & SetBit) != 0) ? Core::ToString(_value.c_str()) : _default);
            }
            inline const string& Default() const
            {
                return (_default);
            }
            inline operator const string() const
            {
                return (((_scopeCount & SetBit) != 0) ? Core::ToString(_value.c_str()) : _default);
            }

            // IElement interface methods
            virtual bool IsSet() const override
            {
                return (_scopeCount & SetBit) != 0;
            }
            virtual void Clear() override
            {
                _scopeCount &= QuotedSerializeBit;
            }

        private:
            // IElement interface methods (private)
            virtual ParserType Type() const override
            {
                return (PARSE_DIRECT);
            }
            virtual IBuffered* BufferParser() override
            {
                return nullptr;
            }
            virtual IDirect* DirectParser() override
            {
                return this;
            }
            virtual IIterator* ElementIterator() override
            {
                return nullptr;
            }

            inline bool UseQuotes() const
            {
                return (_scopeCount & QuotedSerializeBit) != 0;
            }
            inline bool MatchLastCharacter(const string& str, char ch) const
            {
                return (str.length() > 0) && (str[str.length() - 1] == ch);
            }
            inline bool EnterScope(char ch) const
            {
                return (ch == '{') || (ch == '[');
            }
            inline bool ExitScope(char ch) const
            {
                return (ch == '}') || (ch == ']');
            }
            inline bool EndOfQuotedString(char ch) const
            {
                return ((_scopeCount & (QuoteFoundBit | 1)) == (QuoteFoundBit | 1)) && (ch == '\"');
            }
            inline bool OutsideQuotedString() const
            {
                return (_scopeCount & QuoteFoundBit) == 0;
            }

            // IDirect interface methods (private)
            virtual uint16_t Serialize(char stream[], const uint16_t maxLength, uint16_t& offset) const override
            {
                uint16_t result = 0;

                ASSERT(maxLength > 0);

                if (offset == 0) {
                    if (UseQuotes() == true) {
                        // We always start with a quote or Block marker
                        stream[result++] = '\"';
                    }
                    offset++;
                }
                if (result < maxLength) {
#ifdef __WIN32__
#pragma warning(disable : 4996)
#endif

                    // Write the amount we possibly can..
                    uint16_t written = static_cast<uint16_t>(_value.copy(&(stream[result]), maxLength - result, offset - 1));

#ifdef __WIN32__
#pragma warning(default : 4996)
#endif

                    offset += written;
                    result += written;
                }
                if (result < maxLength) {
                    if (UseQuotes() == true) {
                        // And we close with a quote..
                        stream[result++] = '\"';
                    }
                    offset = 0;
                }

                return (result);
            }

            virtual uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint16_t& offset) override
            {
                bool finished = false;
                uint16_t result = 0;
                ASSERT(maxLength > 0);

                if (offset == 0) {
                    // We got a quote, start recording..
                    _value.clear();
                    _scopeCount &= QuotedSerializeBit;
                    if (stream[result] == '\"') {
                        result++;
                        _scopeCount |= (QuoteFoundBit | 1);
                    }
                }

                bool escapedSequence = MatchLastCharacter(_value, '\\');

                // Might be that the last character we added was a
                while ((result < maxLength) && (finished == false)) {
                    if (escapedSequence == false) {
                        if (EnterScope(stream[result])) {
                            _scopeCount++;
                        } else if (ExitScope(stream[result]) || EndOfQuotedString(stream[result])) {
                            _scopeCount--;
                        }

                        finished = (((_scopeCount & ScopeMask) == 0) && ((stream[result] == '\"') || (stream[result] == '}') || (stream[result] == ']') || (stream[result] == ',') || (stream[result] == ' ') || (stream[result] == '\t')));
                    }

                    if ((finished == false) || ExitScope(stream[result])) {
                        escapedSequence = (stream[result] == '\\');
                        // Write the amount we possibly can..
                        _value += stream[result];
                        // Move on to the next position
                        result++;
                    } else if (stream[result] != ',') {
                        result++;
                    }
                }

                if ((result < maxLength) && (finished == false) && OutsideQuotedString())
                    finished = true;

                if (finished == false) {
                    offset =static_cast<uint16_t>( _value.length() + (((_scopeCount & ScopeMask) != 0) ? 1 : 0));
                } else {
                    offset = 0;
                    _scopeCount |= (ContainsNull(_value) ? None : SetBit);
                }

                return (result);
            }

        private:
            std::string _default;
            uint32_t _scopeCount;
            std::string _value;
        };

        class EXTERNAL Container : public IElement {
        private:
            Container(const Container& copy) = delete;
            Container& operator=(const Container& RHS) = delete;

            typedef std::pair<const char*, IElement*> JSONLabelValue;
            typedef std::list<JSONLabelValue> JSONElementList;

            class Iterator : public ILabelIterator {
            private:
                enum State {
                    AT_BEGINNING,
                    AT_ELEMENT,
                    AT_END
                };

            private:
                Iterator();
                Iterator(const Iterator& copy);
                Iterator& operator=(const Iterator& RHS);

            public:
                Iterator(JSONElementList& container)
                    : _container(container)
                    , _iterator(container.begin())
                    , _state(AT_BEGINNING)
                {
                }
                virtual ~Iterator()
                {
                }

            public:
                void Reset()
                {
                    _iterator = _container.begin();
                    _state = AT_BEGINNING;
                }
                virtual bool Next()
                {
                    if (_state != AT_END) {
                        if (_state != AT_BEGINNING) {
                            _iterator++;
                        }

                        _state = (_iterator != _container.end() ? AT_ELEMENT : AT_END);
                    }
                    return (_state == AT_ELEMENT);
                }
                virtual const char* Label() const
                {
                    ASSERT(_state == AT_ELEMENT);

                    return (*_iterator).first;
                }
                virtual IElement* Element()
                {
                    ASSERT(_state == AT_ELEMENT);
                    ASSERT((*_iterator).second != nullptr);

                    return ((*_iterator).second);
                }
                virtual bool Find(const char label[])
                {
                    JSONElementList::iterator index = _container.begin();

                    while ((index != _container.end()) && (strcmp(label, index->first) != 0)) {
                        index++;
                    }

                    _iterator = index;

                    if (index != _container.end()) {
                        _state = AT_ELEMENT;
                    } else {
                        _state = AT_END;
                    }

                    return (_state == AT_ELEMENT);
                }

            private:
                JSONElementList& _container;
                JSONElementList::iterator _iterator;
                State _state;
            };

        public:
            Container()
                : _data()
                , _iterator(_data)
            {
            }
            virtual ~Container()
            {
            }

        public:
            bool HasLabel(const string& label) const
            {

                JSONElementList::const_iterator index(_data.begin());

                while ((index != _data.end()) && (index->first != label)) {
                    index++;
                }

                return (index != _data.end());
            }

            // IElement interface methods
            virtual bool IsSet() const override
            {
                JSONElementList::const_iterator index = _data.begin();

                // As long as we did not find a set element, continue..
                while ((index != _data.end()) && (index->second->IsSet() == false)) {
                    index++;
                }

                return (index != _data.end());
            }
            virtual void Clear() override
            {
                JSONElementList::const_iterator index = _data.begin();

                // As long as we did not find a set element, continue..
                while (index != _data.end()) {
                    index->second->Clear();
                    index++;
                }
            }

        private:
            // IElement interface methods (private)
            virtual ParserType Type() const override
            {
                return (PARSE_CONTAINER);
            }
            virtual IBuffered* BufferParser() override
            {
                return nullptr;
            }
            virtual IDirect* DirectParser() override
            {
                return nullptr;
            }
            virtual IIterator* ElementIterator() override
            {
                _iterator.Reset();

                return (&_iterator);
            }

        public:
            void Add(const TCHAR label[], IElement* element)
            {
                _data.push_back(JSONLabelValue(label, element));
            }

            void Remove(const TCHAR label[])
            {
                JSONElementList::iterator index(_data.begin());

                while ((index != _data.end()) && (index->first != label)) {
                    index++;
                }

                if (index != _data.end()) {
                    _data.erase(index);
                }
            }

        private:
            JSONElementList _data;
            Container::Iterator _iterator;
        };

        template <typename ELEMENTSELECTOR>
        class ChoiceType : public IElement {
        public:
        private:
            ELEMENTSELECTOR _selector;
        };

        class Iterator : public ILabelIterator {
        private:
            Iterator();
            Iterator(const Iterator& copy);
            Iterator& operator=(const Iterator& RHS);

        public:
            Iterator(IElement* element)
                : _element(element)
                , _label()
                , _index(0)
            {
            }
            Iterator(IElement* element, const char label[])
                : _element(element)
                , _label()
                , _index(0)
            {
                ToString(label, _label);
            }
#ifndef __NO_WCHAR_SUPPORT__
            Iterator(IElement* element, const wchar_t label[])
                : _element(element)
                , _label()
                , _index(0)
            {
                ToString(label, _label);
            }
#endif
            ~Iterator()
            {
            }

        public:
            void Reset()
            {
                _index = 0;
            }

            // Just move the iterator to the next point in the map.
            virtual bool Next()
            {
                if (_index != 2) {
                    _index++;
                }
                return (_index == 1);
            }

            // Return the element, we are currently pointing at.
            virtual IElement* Element()
            {
                ASSERT(_index == 1);

                return (_element);
            }

            virtual const char* Label() const
            {
                ASSERT(_index == 1);

                return _label.c_str();
            }
            virtual bool Find(const char label[])
            {
                if (strcmp(_label.c_str(), label) == 0) {
                    _index = 1;
                } else {
                    _index = 2;
                }
                return (_index == 1);
            }

        private:
            IElement* _element;
            std::string _label;
            uint8_t _index;
        };

        template <typename ELEMENT>
        class ArrayType : public IElement {
        public:
            template <typename ARRAYELEMENT>
            class ConstIteratorType {
            private:
                typedef std::list<ARRAYELEMENT> ArrayContainer;
                enum State {
                    AT_BEGINNING,
                    AT_ELEMENT,
                    AT_END
                };

            public:
                ConstIteratorType()
                    : _container(nullptr)
                    , _iterator()
                    , _state(AT_BEGINNING)
                {
                }
                ConstIteratorType(const ArrayContainer& container)
                    : _container(&container)
                    , _iterator(container.begin())
                    , _state(AT_BEGINNING)
                {
                }
                ConstIteratorType(const ConstIteratorType<ARRAYELEMENT>& copy)
                    : _container(copy._container)
                    , _iterator(copy._iterator)
                    , _state(copy._state)
                {
                }
                ~ConstIteratorType()
                {
                }

                ConstIteratorType<ARRAYELEMENT>& operator=(const ConstIteratorType<ARRAYELEMENT>& RHS)
                {
                    _container = RHS._container;
                    _iterator = RHS._iterator;
                    _state = RHS._state;

                    return (*this);
                }

            public:
                inline bool IsValid() const
                {
                    return (_state == AT_ELEMENT);
                }
                void Reset()
                {
                    if (_container != nullptr) {
                        _iterator = _container->begin();
                    }
                    _state = AT_BEGINNING;
                }
                virtual bool Next()
                {
                    if (_container != nullptr) {
                        if (_state != AT_END) {
                            if (_state != AT_BEGINNING) {
                                _iterator++;
                            }

                            while ((_iterator != _container->end()) && (_iterator->IsSet() == false)) {
                                _iterator++;
                            }

                            _state = (_iterator != _container->end() ? AT_ELEMENT : AT_END);
                        }
                    } else {
                        _state = AT_END;
                    }
                    return (_state == AT_ELEMENT);
                }
                const ARRAYELEMENT& Current() const
                {
                    ASSERT(_state == AT_ELEMENT);

                    return (*_iterator);
                }
				inline uint32_t Count() const {
					return (_container == nullptr ? 0 : _container->size());
				}

            private:
                const ArrayContainer* _container;
                typename ArrayContainer::const_iterator _iterator;
                State _state;
            };
            template <typename ARRAYELEMENT>
            class IteratorType : public IArrayIterator {
            private:
                typedef std::list<ARRAYELEMENT> ArrayContainer;
                enum State {
                    AT_BEGINNING,
                    AT_ELEMENT,
                    AT_END
                };

            public:
                IteratorType()
                    : _container(nullptr)
                    , _iterator()
                    , _state(AT_BEGINNING)
                {
                }
                IteratorType(ArrayContainer& container)
                    : _container(&container)
                    , _iterator(container.begin())
                    , _state(AT_BEGINNING)
                {
                }
                IteratorType(const IteratorType<ARRAYELEMENT>& copy)
                    : _container(copy._container)
                    , _iterator(copy._iterator)
                    , _state(copy._state)
                {
                }
                virtual ~IteratorType()
                {
                }

                IteratorType<ARRAYELEMENT>& operator=(const IteratorType<ARRAYELEMENT>& RHS)
                {
                    _container = RHS._container;
                    _iterator = RHS._iterator;
                    _state = RHS._state;

                    return (*this);
                }

            public:
                inline bool IsValid() const
                {
                    return (_state == AT_ELEMENT);
                }
                void Reset()
                {
                    if (_container != nullptr) {
                        _iterator = _container->begin();
                    }
                    _state = AT_BEGINNING;
                }
                virtual bool Next()
                {
                    if (_container != nullptr) {
                        if (_state != AT_END) {
                            if (_state != AT_BEGINNING) {
                                _iterator++;
                            }

                            while ((_iterator != _container->end()) && (_iterator->IsSet() == false)) {
                                _iterator++;
                            }

                            _state = (_iterator != _container->end() ? AT_ELEMENT : AT_END);
                        }
                    } else {
                        _state = AT_END;
                    }
                    return (_state == AT_ELEMENT);
                }
                virtual IElement* Element()
                {
                    ASSERT(_state == AT_ELEMENT);

                    return (&(*_iterator));
                }
                virtual void AddElement()
                {
                    // This can only be called if there is a container..
                    ASSERT(_container != nullptr);

                    if ((_container->size() == 0) || (_container->back().IsSet() == true)) {
                        _container->push_back(ARRAYELEMENT());
                        _state = AT_ELEMENT;
                        _iterator = _container->end();
                        _iterator--;
                    }
                }
                ARRAYELEMENT& Current()
                {
                    ASSERT(_state == AT_ELEMENT);

                    return (*_iterator);
                }
				inline uint32_t Count() const {
					return (_container == nullptr ? 0 : _container->size());
				}

            private:
                ArrayContainer* _container;
                typename ArrayContainer::iterator _iterator;
                State _state;
            };

            typedef IteratorType<ELEMENT> Iterator;
            typedef ConstIteratorType<ELEMENT> ConstIterator;


        public:
            ArrayType()
                : _data()
                , _iterator(_data)
            {
            }
            ArrayType(const ArrayType<ELEMENT>& copy)
                : _data(copy._data)
                , _iterator(_data)
            {
            }
            virtual ~ArrayType()
            {
            }

            // IElement interface methods
            virtual bool IsSet() const override
            {
                return (Length() > 0);
            }
            virtual void Clear() override
            {
                _data.clear();
            }

            ArrayType<ELEMENT>& operator=(const ArrayType<ELEMENT>& RHS) {

                _data = RHS._data;
                _iterator = IteratorType<ELEMENT>(_data);

                return (*this);
            }

        private:
            // IElement interface methods (private)
            virtual ParserType Type() const override
            {
                return (PARSE_CONTAINER);
            }
            virtual IBuffered* BufferParser() override
            {
                return nullptr;
            }

            virtual IDirect* DirectParser() override
            {
                return nullptr;
            }
            virtual IIterator* ElementIterator() override
            {
                _iterator.Reset();

                return (&_iterator);
            }

        public:
            inline uint16_t Length() const
            {
                return static_cast<uint16_t>(_data.size());
            }
            ELEMENT& operator[](const uint32_t index)
            {
                uint32_t skip = index;
                ASSERT(index < Length());

                typename std::list<ELEMENT>::iterator locator = _data.begin();

                while (skip != 0) {
                    locator++;
                    skip--;
                }

                ASSERT(locator != _data.end());

                return (*locator);
            }
            const ELEMENT& operator[](const uint32_t index) const
            {
                uint32_t skip = index;
                ASSERT(index < Length());

                typename std::list<ELEMENT>::const_iterator locator = _data.begin();

                while (skip != 0) {
                    locator++;
                    skip--;
                }

                ASSERT(locator != _data.end());

                return (*locator);
            }
            inline ELEMENT& Add()
            {
                _data.push_back(ELEMENT());

                return (_data.back());
            }
			inline ELEMENT& Add(const ELEMENT& element)
			{
				_data.push_back(element);

				return (_data.back());
			}
			inline Iterator Elements()
            {
                return (Iterator(_data));
            }
            inline ConstIterator Elements() const
            {
                return (ConstIterator(_data));
            }

        private:
            std::list<ELEMENT> _data;
            IteratorType<ELEMENT> _iterator;
        };

        template <typename JSONOBJECT>
        class LabelType : public JSONOBJECT {
        private:
            LabelType(const LabelType<JSONOBJECT>&) = delete;
            LabelType<JSONOBJECT>& operator=(const LabelType<JSONOBJECT>) = delete;

        public:
            LabelType()
                : JSONOBJECT()
            {
            }
            virtual ~LabelType()
            {
            }

        public:
            static const char* Id()
            {
                return (__ID<JSONOBJECT>());
            }

            virtual const char* Label() const
            {
                return (LabelType<JSONOBJECT>::Id());
            }

        private:
            HAS_MEMBER(Id, hasID);

            typedef hasID<JSONOBJECT, const char* (JSONOBJECT::*)()> TraitID;

            template <typename TYPE>
            static inline typename Core::TypeTraits::enable_if<LabelType<TYPE>::TraitID::value, const char*>::type __ID()
            {
                return (JSONOBJECT::Id());
            }

            template <typename TYPE>
            static inline typename Core::TypeTraits::enable_if<!LabelType<TYPE>::TraitID::value, const char*>::type __ID()
            {
                static std::string className = (Core::ClassNameOnly(typeid(JSONOBJECT).name()).Text());

                return (className.c_str());
            }
        };

        template <uint16_t SIZE, typename INSTANCEOBJECT>
        class Tester {
        private:
            typedef Tester<SIZE, INSTANCEOBJECT> ThisClass;

            class SerializerImpl : public INSTANCEOBJECT::Serializer {
            private:
                SerializerImpl(const SerializerImpl&) = delete;
                SerializerImpl& operator=(const SerializerImpl&) = delete;

            public:
                SerializerImpl(bool& readyFlag)
                    : INSTANCEOBJECT::Serializer()
                    , _ready(readyFlag)
                {
                }
                ~SerializerImpl()
                {
                }

            public:
                virtual void Serialized(const INSTANCEOBJECT& /* element */)
                {
                    _ready = true;
                }
                virtual void Serialized(const Core::JSON::IElement& /* element */)
                {
                    _ready = true;
                }

            private:
                bool& _ready;
            };

            class DeserializerImpl : public INSTANCEOBJECT::Deserializer {
            private:
                DeserializerImpl() = delete;
                DeserializerImpl(const DeserializerImpl&) = delete;
                DeserializerImpl& operator=(const DeserializerImpl&) = delete;

            public:
                DeserializerImpl(ThisClass& parent, bool& ready)
                    : INSTANCEOBJECT::Deserializer()
                    , _element(nullptr)
                    , _parent(parent)
                    , _ready(ready)
                {
                }
                ~DeserializerImpl()
                {
                }

            public:
                inline void SetElement(Core::JSON::IElement* element)
                {
                    ASSERT(((element == nullptr) ^ (_element == nullptr)) || (element == _element));

                    _element = element;
                }

                virtual Core::JSON::IElement *Element(const string &identifier VARIABLE_IS_NOT_USED)
                {
                    return (_element);
                }
                virtual void Deserialized(INSTANCEOBJECT& element)
                {
                    ASSERT(&element == _element);
                    _element = nullptr;
                    _ready = true;
                }
                virtual void Deserialized(Core::JSON::IElement& element)
                {
                    ASSERT(&element == _element);
                    _element = nullptr;
                    _ready = true;
                }

            private:
                Core::JSON::IElement* _element;
                ThisClass& _parent;
                bool& _ready;
            };

        private:
            Tester(const Tester<SIZE, INSTANCEOBJECT>&) = delete;
            Tester<SIZE, INSTANCEOBJECT>& operator=(const Tester<SIZE, INSTANCEOBJECT>&) = delete;

        public:
            Tester()
                : _ready(false)
                , _serializer(_ready)
                , _deserializer(*this, _ready)
            {
            }
            ~Tester()
            {
            }

            bool FromString(const string& value, Core::ProxyType<INSTANCEOBJECT>& receptor)
            {
                uint16_t fillCount = 0;
                receptor->Clear();

                _ready = false;
                _deserializer.SetElement(&(*(receptor)));
                // Serialize object
                while ((_ready == false) && (fillCount < value.size())) {
                    uint16_t size = ((value.size() - fillCount) < SIZE ? (value.size() - fillCount) : SIZE);

                    // Prepare the deserialize buffer
                    memcpy(_buffer, &(value.data()[fillCount]), size);

                    fillCount += size;

    #ifdef __DEBUG__
                    uint16_t handled =
    #endif // __DEBUG__

                        _deserializer.Deserialize(_buffer, size);

                    ASSERT(handled <= size);
                }

                return (_ready == true);
            }

            void ToString(const Core::ProxyType<INSTANCEOBJECT>& receptor, string& value)
            {
                uint16_t fillCount = 0;
                _ready = false;

                // Request an object to e serialized..
                _serializer.Submit(*receptor);

                // Serialize object
                while (_ready == false) {
                    uint16_t loaded = _serializer.Serialize(_buffer, SIZE);

                    ASSERT(loaded <= SIZE);

                    fillCount += loaded;

                    value += string(reinterpret_cast<char*>(&_buffer[0]), loaded);
                }
            }

        private:
            bool _ready;
            SerializerImpl _serializer;
            DeserializerImpl _deserializer;
            uint8_t _buffer[SIZE];
        };
    } // namespace JSON
} // namespace Core
} // namespace WPEFramework

#endif // __JSON_H
