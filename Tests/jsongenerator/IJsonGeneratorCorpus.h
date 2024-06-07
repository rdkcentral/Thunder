/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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


// JsonGenerator stress test interfaces

#pragma once

#include "Module.h"

// @insert <com/IIteratorType.h>

namespace Thunder {

namespace Exchange {

// @json
struct EXTERNAL IJsonGeneratorCorpus : virtual public Core::IUnknown {

    enum { ID = 0xF0000000 };

    enum {
        ID_STRINGITERATOR = ID + 1,
        ID_VALUEITERATOR,
        ID_ENUMITERATOR,
        ID_COMPOUNDITERATOR
    };

    enum anenum : uint8_t {
        ENUM_ONE,
        ENUM_TWO,
        ENUM_THREE
    };

    struct Compound {
        uint32_t integerElem;
        string stringElem;
        bool boolElem;
        anenum enumElem;
    };

    struct NestedCompound {
        string stringElem;
        Compound complexElem;
    };

    using IStringIterator = RPC::IIteratorType<string, ID_STRINGITERATOR>;
    using IValueIterator = RPC::IIteratorType<uint32_t, ID_VALUEITERATOR>;
    using IEnumIterator = RPC::IIteratorType<anenum, ID_ENUMITERATOR>;
    using ICompoundIterator = RPC::IIteratorType<Compound, ID_COMPOUNDITERATOR>;


/////////// NOTIFICATIONS

    // @event
    class INotification {
        enum { ID = IJsonGeneratorCorpus::ID + 10 };

#if 1
        virtual void NotificationSingleFundamental(const uint32_t argument) = 0;
        virtual void NotificationSingleString(const string& argument) = 0;
        virtual void NotificationSingleEnum(const anenum argument) = 0;
        virtual void NotificationSingleCompound(const Compound& argument) = 0;

        virtual void NotificationMultipleFundamental(const uint32_t argumentOne, const uint32_t argumentTwo) = 0;
        virtual void NotificationMultipleString(const string& argumentOne, const string& argumentTwo) = 0;
        virtual void NotificationMultipleEnum(const anenum argumentOne, const anenum argumentTwo) = 0;
        virtual void NotificationMultipleCompound(const Compound& argumentOne, const Compound& argumentTwo) = 0;
#endif

#if 1
        virtual void NotificationSingleFundamentalWithSendif(const string& id /* @index */, const uint32_t argument /* @index */) = 0;
        virtual void NotificationSingleStringWithSendif(const string& id /* @index */, const string& argument) = 0;
        virtual void NotificationSingleEnumWithSendif(const string& id /* @index */, const anenum argument) = 0;
        virtual void NotificationSingleCompoundWithSendif(const string& id /* @index */, const Compound& argument) = 0;

        virtual void NotificationMultipleFundamentalWithSendif(const string& id /* @index */, const uint32_t argumentOne, const uint32_t argumentTwo) = 0;
        virtual void NotificationMultipleStringWithSendif(const string& id /* @index */, const string& argumentOne, const string& argumentTwo) = 0;
        virtual void NotificationMultipleEnumWithSendif(const string& id /* @index */, const anenum argumentOne, const anenum argumentTwo) = 0;
        virtual void NotificationMultipleCompoundWithSendif(const string& id /* @index */, const Compound& argumentOne, const Compound& argumentTwo) = 0;
#endif

#if 0
        virtual void NotificationMultipleFundamentalWithSendifInteger(const uint8_t& id /* @index */, const uint32_t argumentOne, const uint32_t argumentTwo) = 0;
        virtual void NotificationMultipleStringWithSendifInteger(const uint8_t& id /* @index */, const string& argumentOne, const string& argumentTwo) = 0;
        virtual void NotificationMultipleEnumWithSendifInteger(const uint8_t& id /* @index */, const anenum argumentOne, const anenum argumentTwo) = 0;
        virtual void NotificationMultipleCompoundWithSendifInteger(const uint8_t& id /* @index */, const Compound& argumentOne, const Compound& argumentTwo) = 0;
#endif

#if 1
        virtual void NotificationMultipleFundamentalWithSendifEnum(const anenum& id /* @index */, const uint32_t argumentOne, const uint32_t argumentTwo) = 0;
        virtual void NotificationMultipleStringWithSendifEnum(const anenum& id /* @index */, const string& argumentOne, const string& argumentTwo) = 0;
        virtual void NotificationMultipleEnumWithSendifEnum(const anenum& id /* @index */, const anenum argumentOne, const anenum argumentTwo) = 0;
        virtual void NotificationMultipleCompoundWithSendifEnum(const anenum& id /* @index */, const Compound& argumentOne, const Compound& argumentTwo) = 0;
#endif
    };

#if 1
    uint32_t Register(INotification* event);
    uint32_t Unregister(INotification* event);
#endif


/////////// METHODS

#if 1
    virtual uint32_t MethodSingleFundmentalIn(const uint8_t argument) = 0;
    virtual uint32_t MethodSingleFundmentalOut(uint8_t& argument /* @out */) const = 0;
    virtual uint32_t MethodSingleFundmentalInOut(uint8_t& argument /* @inout */) = 0;

    virtual uint32_t MethodSingleStringIn(const string& argument) = 0;
    virtual uint32_t MethodSingleStringOut(string& argument /* @out */) const = 0;
    virtual uint32_t MethodSingleStringInOut(string& argument /* @inout */) = 0;

    virtual uint32_t MethodSingleEnumIn(const anenum argument) = 0;
    virtual uint32_t MethodSingleEnumOut(anenum& argument /* @out */) const = 0;
    virtual uint32_t MethodSingleEnumInOut(anenum& argument /* @inout */) = 0;
#endif

#if 1
    virtual uint32_t MethodSingleBufferInSizeAfter(const uint8_t argument[] /* @length:size */, const uint16_t size) = 0;
    virtual uint32_t MethodSingleBufferOutSizeAfter(uint8_t argument[] /* @out @length:size */, const uint16_t size) const = 0;
    virtual uint32_t MethodSingleBufferInOutSizeAfter(uint8_t argument[] /* @inout @length:size */, const uint16_t size) = 0;
    virtual uint32_t MethodSingleBufferInSizeBefore(const uint16_t size, const uint8_t argument[] /* @length:size */) = 0;
    virtual uint32_t MethodSingleBufferOutSizeBefore(const uint16_t size, uint8_t argument[] /* @out @length:size */) const = 0;
    virtual uint32_t MethodSingleBufferInOutSizeBefore(const uint16_t size, uint8_t argument[] /* @inout @length:size */) = 0;
    virtual uint32_t MethodSingleBufferInSizeBeforeInOut(uint16_t& size /* @inout */, const uint8_t argument[] /* @length:size */) = 0;
    virtual uint32_t MethodSingleBufferOutSizeBeforeInOut(uint16_t& size /* @inout */, uint8_t argument[] /* @out @length:size */) const = 0;
    virtual uint32_t MethodSingleBufferInOutSizeBeforeInOut(uint16_t& size /* @inout */, uint8_t argument[] /* @inout @length:size */) = 0;
#endif

#if 1
    virtual uint32_t MethodSingleStringIteratorIn(IStringIterator* const list) = 0;
    virtual uint32_t MethodSingleStringIteratorOut(IStringIterator*& list /* @out */) const = 0;

    virtual uint32_t MethodSingleValueIteratorIn(IValueIterator* const list) = 0;
    virtual uint32_t MethodSingleValueIteratorOut(IValueIterator*& list /* @out */) const = 0;

    virtual uint32_t MethodSingleEnumIteratorIn(IEnumIterator* const list) = 0;
    virtual uint32_t MethodSingleEnumIteratorOut(IEnumIterator*& list /* @out */) const = 0;

    virtual uint32_t MethodSingleCompoundIteratorIn(ICompoundIterator* const list) = 0;
    virtual uint32_t MethodSingleCompoundIteratorOut(ICompoundIterator*& list /* @out */) const = 0;
#endif

#if 1
    virtual uint32_t MethodSingleCompoundIn(const Compound argument) = 0;
    virtual uint32_t MethodSingleCompoundOut(Compound& argument /* @out */) const = 0;
    virtual uint32_t MethodSingleCompoundInOut(Compound& argument /* @inout */) = 0;
#endif

#if 0
    // Nested compound currently not supported by ProxyStubGenerator
    virtual uint32_t MethodSingleNestedCompoundIn(const NestedCompound& argumentParam) = 0;
    virtual uint32_t MethodSingleNestedCompoundOut(NestedCompound& argumentParam /* @out */) const = 0;
    virtual uint32_t MethodSingleNestedCompoundInOut(NestedCompound& argumentParam /* @inout */) = 0;
#endif

#if 1
    virtual uint32_t MethodMultipleFundmentalIn(const uint8_t argumentOne, const uint8_t argumentTwo) = 0;
    virtual uint32_t MethodMultipleFundmentalOut(uint8_t& argumentOne /* @out */, uint8_t& argumentTwo /* @out */) const = 0;
    virtual uint32_t MethodMultipleFundmentalInOut(uint8_t& argumentOne /* @inout */, uint8_t& argumentTwo /* @inout */)  = 0;
    virtual uint32_t MethodMultipleFundmentalInAndOut(const uint8_t& argumentOne, uint8_t& argumentTwo /* @out */) = 0;
    virtual uint32_t MethodMultipleFundmentalOutAndIn(uint8_t& argumentOne /* @out */, const uint8_t argumentTwo) const = 0;
    virtual uint32_t MethodMultipleFundmentalInAndInOut(const uint8_t& argumentOne, uint8_t& argumentTwo /* @inout */) = 0;
    virtual uint32_t MethodMultipleFundmentalOutAndInOut(uint8_t& argumentOne /* @out */, uint8_t& argumentTwo /* @inout */) = 0;
#endif

#if 1
    virtual uint32_t MethodMultipleCompoundIn(const Compound& argumentOne, const Compound& argumentTwo) = 0;
    virtual uint32_t MethodMultipleCompoundOut(Compound& argumentOne /* @out */, Compound& argumentTwo /* @out */) const = 0;
    virtual uint32_t MethodMultipleCompoundInOut(Compound& argumentOne /* @inout */, Compound& argumentTwo /* @inout */)  = 0;
    virtual uint32_t MethodMultipleCompoundInAndOut(const Compound& argumentOne, Compound& argumentTwo /* @out */) = 0;
    virtual uint32_t MethodMultipleCompoundOutAndIn(Compound& argumentOne /* @out */, const Compound argumentTwo) const = 0;
    virtual uint32_t MethodMultipleCompoundInAndInOut(const Compound& argumentOne, Compound& argumentTwo /* @inout */) = 0;
    virtual uint32_t MethodMultipleCompoundOutAndInOut(Compound& argumentOne /* @out */, Compound& argumentTwo /* @inout */) = 0;
#endif

#if 0
    virtual uint32_t MethodMultipleNestedCompoundIn(const NestedCompound& argumentOne, const NestedCompound& argumentTwo) = 0;
    virtual uint32_t MethodMultipleNestedCompoundOut(NestedCompound& argumentOne /* @out */, NestedCompound& argumentTwo /* @out */) const = 0;
    virtual uint32_t MethodMultipleNestedCompoundInOut(NestedCompound& argumentOne /* @inout */, NestedCompound& argumentTwo /* @inout */)  = 0;
    virtual uint32_t MethodMultipleNestedCompoundInAndOut(const NestedCompound& argumentOne, NestedCompound& argumentTwo /* @out */) = 0;
    virtual uint32_t MethodMultipleNestedCompoundOutAndIn(NestedCompound& argumentOne /* @out */, const NestedCompound argumentTwo) const = 0;
    virtual uint32_t MethodMultipleNestedCompoundInAndInOut(const NestedCompound& argumentOne, NestedCompound& argumentTwo /* @inout */) = 0;
    virtual uint32_t MethodMultipleNestedCompoundOutAndInOut(NestedCompound& argumentOne /* @out */, NestedCompound& argumentTwo /* @inout */) = 0;
#endif

/////////// PROPERTIES

#if 1
    // @property
    virtual uint32_t PropertyFundmentalReadOnly(uint8_t& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalWriteOnly(const uint8_t argument) = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWrite(const uint8_t argument) = 0;
    virtual uint32_t PropertyFundmentalReadWrite(uint8_t& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrder(uint8_t& argument /* @out */) const = 0;
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrder(const uint8_t argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyStringReadOnly(string& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringRWriteOnly(const string& argument) = 0;
    // @property
    virtual uint32_t PropertyStringReadWrite(const string& argument) = 0;
    virtual uint32_t PropertyStringReadWrite(string& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringReadWriteDifferentOrder(string& argument /* @out */) const = 0;
    virtual uint32_t PropertyStringReadWriteDifferentOrder(const string& argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyEnumReadOnly(anenum& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyEnumWriteOnly(const anenum argument) = 0;
    // @property
    virtual uint32_t PropertyEnumReadWrite(const anenum argument) = 0;
    virtual uint32_t PropertyEnumReadWrite(anenum& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyEnumReadWriteDifferentOrder(anenum& argument /* @out */) const = 0;
    virtual uint32_t PropertyEnumReadWriteDifferentOrder(const anenum& argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyStringIteratorReadOnly(IStringIterator*& list /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringIteratorWriteOnly(IStringIterator* const list) = 0;
    // @property
    virtual uint32_t PropertyStringIteratorReadWrite(IStringIterator* const list) = 0;
    virtual uint32_t PropertyStringIteratorReadWrite(IStringIterator*& list /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringIteratorReadWriteDifferentOrder(IStringIterator*& list /* @out */) const = 0;
    virtual uint32_t PropertyStringIteratorReadWriteDifferentOrder(IStringIterator* const list) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyCompoundReadOnly(Compound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyCompoundWriteOnly(const Compound& argument) = 0;
    // @property
    virtual uint32_t PropertyCompoundReadWrite(const Compound& argument) = 0;
    virtual uint32_t PropertyCompoundReadWrite(Compound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyCompoundReadWriteDifferentOrder(Compound& argument /* @out */) const = 0;
    virtual uint32_t PropertyCompoundReadWriteDifferentOrder(const Compound& argument) = 0;
#endif

#if 0
    // @property
    virtual uint32_t PropertyNestedCompoundReadOnly(NestedCompound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundWriteOnly(const NestedCompound& argument) = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundReadWrite(const NestedCompound& argument) = 0;
    virtual uint32_t PropertyNestedCompoundReadWrite(NestedCompound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundReadWriteDifferentOrder(NestedCompound argument& /* @out */) const = 0;
    virtual uint32_t PropertyNestedCompoundReadWriteDifferentOrder(const NestedCompound& argument) = 0;
#endif

/////////// INDEXED PROPERTIES


#if 1
    // @property
    virtual uint32_t PropertyFundmentalReadOnlyIntegerIndex(const uint16_t index /* @index */, int& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalWriteOnlyIntegerIndex(const uint16_t index /* @index */, const int argument) = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWriteIntegerIndex(const uint16_t index /* @index */, const int argument) = 0;
    virtual uint32_t PropertyFundmentalReadWriteIntegerIndex(const uint16_t index /* @index */, int& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrderIntegerIndex(const uint16_t index /* @index */, int& argument /* @out */) const = 0;
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrderIntegerIndex(const uint16_t index /* @index */, const int argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyFundmentalReadOnlyStringIndex(const string& index /* @index */, int& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalWriteOnlyStringIndex(const string& index /* @index */, const int argument) = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWriteStringIndex(const string& index /* @index */, const int argument) = 0;
    virtual uint32_t PropertyFundmentalReadWriteStringIndex(const string& index /* @index */, int& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrderStringIndex(const string& index /* @index */, int& argument /* @out */) const = 0;
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrderStringIndex(const string& index /* @index */, const int argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyFundmentalReadOnlyEnumIndex(const anenum& index /* @index */, int& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalWriteOnlyEnumIndex(const anenum& index /* @index */, const int argument) = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWriteEnumIndex(const anenum& index /* @index */, const int argument) = 0;
    virtual uint32_t PropertyFundmentalReadWriteEnumIndex(const anenum& index /* @index */, int& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrderEnumIndex(const anenum& index /* @index */, int& argument /* @out */) const = 0;
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrderEnumIndex(const anenum& index /* @index */, const int argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyStringReadOnlyStringIndex(const string& index /* @index */, string& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringWriteOnlyStringIndex(const string& index /* @index */, const string& argument) = 0;
    // @property
    virtual uint32_t PropertyStringReadWriteStringIndex(const string& index /* @index */, const string& argument) = 0;
    virtual uint32_t PropertyStringReadWriteStringIndex(const string& index /* @index */, string& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringReadWriteDifferentOrderStringIndex(const string& index /* @index */, string& argument /* @out */) const = 0;
    virtual uint32_t PropertyStringReadWriteDifferentOrderStringIndex(const string& index /* @index */, const string& argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyEnumReadOnlyStringIndex(const string& index /* @index */, anenum& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyEnumWriteOnlyStringIndex(const string& index /* @index */, const anenum argument) = 0;
    // @property
    virtual uint32_t PropertyEnumReadWriteStringIndex(const string& index /* @index */, const anenum argument) = 0;
    virtual uint32_t PropertyEnumReadWriteStringIndex(const string& index /* @index */, anenum& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyEnumReadWriteDifferentOrderStringIndex(const string& index /* @index */, anenum& argument /* @out */) const = 0;
    virtual uint32_t PropertyEnumReadWriteDifferentOrderStringIndex(const string& index /* @index */, const anenum argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyStringIteratorReadOnlyStringIndex(const string& index /* @index */, IStringIterator*& list /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringIteratorWriteOnlyStringIndex(const string& index /* @index */, IStringIterator* const list) = 0;
    // @property
    virtual uint32_t PropertyStringIteratorReadWriteStringIndex(const string& index /* @index */, IStringIterator* const list) = 0;
    virtual uint32_t PropertyStringIteratorReadWriteStringIndex(const string& index /* @index */, IStringIterator*& list /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringIteratorReadWriteDifferentOrderStringIndex(const string& index /* @index */, IStringIterator*& list /* @out */) const = 0;
    virtual uint32_t PropertyStringIteratorReadWriteDifferentOrderStringIndex(const string& index /* @index */, IStringIterator* const list) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyCompoundteratorReadOnlyEnumIndex(const anenum index /* @index */, ICompoundIterator*& list /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyCompoundIteratorWriteOnlyEnumIndex(const anenum index /* @index */, ICompoundIterator* const list) = 0;
    // @property
    virtual uint32_t PropertyCompoundIteratorReadWriteEnumIndex(const anenum index /* @index */, ICompoundIterator* const list) = 0;
    virtual uint32_t PropertyCompoundIteratorReadWriteEnumIndex(const anenum index /* @index */, ICompoundIterator*& list /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyCompoundIteratorReadWriteDifferentOrderStringIndex(const anenum index /* @index */, ICompoundIterator*& list /* @out */) const = 0;
    virtual uint32_t PropertyCompoundIteratorReadWriteDifferentOrderStringIndex(const anenum index /* @index */, ICompoundIterator* const list) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyCompoundReadOnlyStringIndex(const string& index /* @index */, Compound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyCompoundWriteOnlyStringIndex(const string& index /* @index */, const Compound argument) = 0;
    // @property
    virtual uint32_t PropertyCompoundReadWriteStringIndex(const string& index /* @index */, const Compound argument) = 0;
    virtual uint32_t PropertyCompoundReadWriteStringIndex(const string& index /* @index */, Compound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyCompoundReadWriteDifferentOrderStringIndex(const string& index /* @index */, Compound& argument /* @out */) const = 0;
    virtual uint32_t PropertyCompoundReadWriteDifferentOrderStringIndex(const string& index /* @index */, const Compound argument) = 0;
#endif

#if 0
    // @property
    virtual uint32_t PropertyNestedCompoundReadOnlyStringIndex(const string& index /* @index */, NestedCompound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundWriteOnlyStringIndex(const string& index /* @index */, const NestedCompound argument) = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundReadWriteStringIndex(const string& index /* @index */, const NestedCompound argument) = 0;
    virtual uint32_t PropertyNestedCompoundReadWriteStringIndex(const string& index /* @index */, NestedCompound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundReadWriteDifferentOrderStringIndex(const string& index /* @index */, NestedCompound& argument /* @out */) const = 0;
    virtual uint32_t PropertyNestedCompoundReadWriteDifferentOrderStringIndex(const string& index /* @index */, const NestedCompound argument) = 0;
#endif


#if 0
    // @property
    virtual uint32_t PropertyNestedCompoundReadOnlyIntegerIndex(const uint8_t& index /* @index */, NestedCompound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundWriteOnlyIntegerIndex(const uint8_t& index /* @index */, const NestedCompound argument) = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundReadWriteIntegerIndex(const uint8_t& index /* @index */, const NestedCompound argument) = 0;
    virtual uint32_t PropertyNestedCompoundReadWriteIntegerIndex(const uint8_t& index /* @index */, NestedCompound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundReadWriteDifferentOrderIntegerIndex(const uint8_t& index /* @index */, NestedCompound& argument /* @out */) const = 0;
    virtual uint32_t PropertyNestedCompoundReadWriteDifferentOrderIntegerIndex(const uint8_t& index /* @index */, const NestedCompound argument) = 0;
#endif

}; // class IJsonGeneratorCorpus

// @json @extended
struct EXTERNAL IJsonGeneratorCorpusExtended : virtual public Core::IUnknown {

    enum { ID = 0xF0000000 };

    enum {
        ID_STRINGITERATOR = ID + 1,
        ID_VALUEITERATOR,
        ID_ENUMITERATOR,
        ID_COMPOUNDITERATOR
    };

    enum anenum : uint8_t {
        ENUM_ONE,
        ENUM_TWO,
        ENUM_THREE
    };

    struct Compound {
        uint32_t integerElem;
        string stringElem;
        bool boolElem;
        anenum enumElem;
    };

    struct NestedCompound {
        string stringElem;
        Compound complexElem;
    };

    using IStringIterator = RPC::IIteratorType<string, ID_STRINGITERATOR>;
    using IValueIterator = RPC::IIteratorType<uint32_t, ID_VALUEITERATOR>;
    using IEnumIterator = RPC::IIteratorType<anenum, ID_ENUMITERATOR>;
    using ICompoundIterator = RPC::IIteratorType<Compound, ID_COMPOUNDITERATOR>;


/////////// NOTIFICATIONS

    // @event @extended
    class INotification {
        enum { ID = IJsonGeneratorCorpus::ID + 10 };

#if 1
        virtual void NotificationSingleFundamental(const uint32_t argument) = 0;
        virtual void NotificationSingleString(const string& argument) = 0;
        virtual void NotificationSingleEnum(const anenum argument) = 0;
        virtual void NotificationSingleCompound(const Compound& argument) = 0;

        virtual void NotificationMultipleFundamental(const uint32_t argumentOne, const uint32_t argumentTwo) = 0;
        virtual void NotificationMultipleString(const string& argumentOne, const string& argumentTwo) = 0;
        virtual void NotificationMultipleEnum(const anenum argumentOne, const anenum argumentTwo) = 0;
        virtual void NotificationMultipleCompound(const Compound& argumentOne, const Compound& argumentTwo) = 0;
#endif

#if 1
        virtual void NotificationSingleFundamentalWithSendif(const string& id /* @index */, const uint32_t argument /* @index */) = 0;
        virtual void NotificationSingleStringWithSendif(const string& id /* @index */, const string& argument) = 0;
        virtual void NotificationSingleEnumWithSendif(const string& id /* @index */, const anenum argument) = 0;
        virtual void NotificationSingleCompoundWithSendif(const string& id /* @index */, const Compound& argument) = 0;

        virtual void NotificationMultipleFundamentalWithSendif(const string& id /* @index */, const uint32_t argumentOne, const uint32_t argumentTwo) = 0;
        virtual void NotificationMultipleStringWithSendif(const string& id /* @index */, const string& argumentOne, const string& argumentTwo) = 0;
        virtual void NotificationMultipleEnumWithSendif(const string& id /* @index */, const anenum argumentOne, const anenum argumentTwo) = 0;
        virtual void NotificationMultipleCompoundWithSendif(const string& id /* @index */, const Compound& argumentOne, const Compound& argumentTwo) = 0;
#endif

#if 0
        virtual void NotificationMultipleFundamentalWithSendifInteger(const uint8_t& id /* @index */, const uint32_t argumentOne, const uint32_t argumentTwo) = 0;
        virtual void NotificationMultipleStringWithSendifInteger(const uint8_t& id /* @index */, const string& argumentOne, const string& argumentTwo) = 0;
        virtual void NotificationMultipleEnumWithSendifInteger(const uint8_t& id /* @index */, const anenum argumentOne, const anenum argumentTwo) = 0;
        virtual void NotificationMultipleCompoundWithSendifInteger(const uint8_t& id /* @index */, const Compound& argumentOne, const Compound& argumentTwo) = 0;
#endif

#if 1
        virtual void NotificationMultipleFundamentalWithSendifEnum(const anenum& id /* @index */, const uint32_t argumentOne, const uint32_t argumentTwo) = 0;
        virtual void NotificationMultipleStringWithSendifEnum(const anenum& id /* @index */, const string& argumentOne, const string& argumentTwo) = 0;
        virtual void NotificationMultipleEnumWithSendifEnum(const anenum& id /* @index */, const anenum argumentOne, const anenum argumentTwo) = 0;
        virtual void NotificationMultipleCompoundWithSendifEnum(const anenum& id /* @index */, const Compound& argumentOne, const Compound& argumentTwo) = 0;
#endif
    };

#if 1
    uint32_t Register(INotification* event);
    uint32_t Unregister(INotification* event);
#endif


/////////// METHODS

#if 1
    virtual uint32_t MethodSingleFundmentalIn(const uint8_t argument) = 0;
    virtual uint32_t MethodSingleFundmentalOut(uint8_t& argument /* @out */) const = 0;
    virtual uint32_t MethodSingleFundmentalInOut(uint8_t& argument /* @inout */) = 0;

    virtual uint32_t MethodSingleStringIn(const string& argument) = 0;
    virtual uint32_t MethodSingleStringOut(string& argument /* @out */) const = 0;
    virtual uint32_t MethodSingleStringInOut(string& argument /* @inout */) = 0;

    virtual uint32_t MethodSingleEnumIn(const anenum argument) = 0;
    virtual uint32_t MethodSingleEnumOut(anenum& argument /* @out */) const = 0;
    virtual uint32_t MethodSingleEnumInOut(anenum& argument /* @inout */) = 0;
#endif

#if 1
    virtual uint32_t MethodSingleBufferInSizeAfter(const uint8_t argument[] /* @length:size */, const uint16_t size) = 0;
    virtual uint32_t MethodSingleBufferOutSizeAfter(uint8_t argument[] /* @out @length:size */, const uint16_t size) const = 0;
    virtual uint32_t MethodSingleBufferInOutSizeAfter(uint8_t argument[] /* @inout @length:size */, const uint16_t size) = 0;
    virtual uint32_t MethodSingleBufferInSizeBefore(const uint16_t size, const uint8_t argument[] /* @length:size */) = 0;
    virtual uint32_t MethodSingleBufferOutSizeBefore(const uint16_t size, uint8_t argument[] /* @out @length:size */) const = 0;
    virtual uint32_t MethodSingleBufferInOutSizeBefore(const uint16_t size, uint8_t argument[] /* @inout @length:size */) = 0;
    virtual uint32_t MethodSingleBufferInSizeBeforeInOut(uint16_t& size /* @inout */, const uint8_t argument[] /* @length:size */) = 0;
    virtual uint32_t MethodSingleBufferOutSizeBeforeInOut(uint16_t& size /* @inout */, uint8_t argument[] /* @out @length:size */) const = 0;
    virtual uint32_t MethodSingleBufferInOutSizeBeforeInOut(uint16_t& size /* @inout */, uint8_t argument[] /* @inout @length:size */) = 0;
#endif

#if 1
    virtual uint32_t MethodSingleStringIteratorIn(IStringIterator* const list) = 0;
    virtual uint32_t MethodSingleStringIteratorOut(IStringIterator*& list /* @out */) const = 0;

    virtual uint32_t MethodSingleValueIteratorIn(IValueIterator* const list) = 0;
    virtual uint32_t MethodSingleValueIteratorOut(IValueIterator*& list /* @out */) const = 0;

    virtual uint32_t MethodSingleEnumIteratorIn(IEnumIterator* const list) = 0;
    virtual uint32_t MethodSingleEnumIteratorOut(IEnumIterator*& list /* @out */) const = 0;

    virtual uint32_t MethodSingleCompoundIteratorIn(ICompoundIterator* const list) = 0;
    virtual uint32_t MethodSingleCompoundIteratorOut(ICompoundIterator*& list /* @out */) const = 0;
#endif

#if 1
    virtual uint32_t MethodSingleCompoundIn(const Compound argument) = 0;
    virtual uint32_t MethodSingleCompoundOut(Compound& argument /* @out */) const = 0;
    virtual uint32_t MethodSingleCompoundInOut(Compound& argument /* @inout */) = 0;
#endif

#if 0
    // Nested compound currently not supported by ProxyStubGenerator
    virtual uint32_t MethodSingleNestedCompoundIn(const NestedCompound& argumentParam) = 0;
    virtual uint32_t MethodSingleNestedCompoundOut(NestedCompound& argumentParam /* @out */) const = 0;
    virtual uint32_t MethodSingleNestedCompoundInOut(NestedCompound& argumentParam /* @inout */) = 0;
#endif

#if 1
    virtual uint32_t MethodMultipleFundmentalIn(const uint8_t argumentOne, const uint8_t argumentTwo) = 0;
    virtual uint32_t MethodMultipleFundmentalOut(uint8_t& argumentOne /* @out */, uint8_t& argumentTwo /* @out */) const = 0;
    virtual uint32_t MethodMultipleFundmentalInOut(uint8_t& argumentOne /* @inout */, uint8_t& argumentTwo /* @inout */)  = 0;
    virtual uint32_t MethodMultipleFundmentalInAndOut(const uint8_t& argumentOne, uint8_t& argumentTwo /* @out */) = 0;
    virtual uint32_t MethodMultipleFundmentalOutAndIn(uint8_t& argumentOne /* @out */, const uint8_t argumentTwo) const = 0;
    virtual uint32_t MethodMultipleFundmentalInAndInOut(const uint8_t& argumentOne, uint8_t& argumentTwo /* @inout */) = 0;
    virtual uint32_t MethodMultipleFundmentalOutAndInOut(uint8_t& argumentOne /* @out */, uint8_t& argumentTwo /* @inout */) = 0;
#endif

#if 1
    virtual uint32_t MethodMultipleCompoundIn(const Compound& argumentOne, const Compound& argumentTwo) = 0;
    virtual uint32_t MethodMultipleCompoundOut(Compound& argumentOne /* @out */, Compound& argumentTwo /* @out */) const = 0;
    virtual uint32_t MethodMultipleCompoundInOut(Compound& argumentOne /* @inout */, Compound& argumentTwo /* @inout */)  = 0;
    virtual uint32_t MethodMultipleCompoundInAndOut(const Compound& argumentOne, Compound& argumentTwo /* @out */) = 0;
    virtual uint32_t MethodMultipleCompoundOutAndIn(Compound& argumentOne /* @out */, const Compound argumentTwo) const = 0;
    virtual uint32_t MethodMultipleCompoundInAndInOut(const Compound& argumentOne, Compound& argumentTwo /* @inout */) = 0;
    virtual uint32_t MethodMultipleCompoundOutAndInOut(Compound& argumentOne /* @out */, Compound& argumentTwo /* @inout */) = 0;
#endif

#if 0
    virtual uint32_t MethodMultipleNestedCompoundIn(const NestedCompound& argumentOne, const NestedCompound& argumentTwo) = 0;
    virtual uint32_t MethodMultipleNestedCompoundOut(NestedCompound& argumentOne /* @out */, NestedCompound& argumentTwo /* @out */) const = 0;
    virtual uint32_t MethodMultipleNestedCompoundInOut(NestedCompound& argumentOne /* @inout */, NestedCompound& argumentTwo /* @inout */)  = 0;
    virtual uint32_t MethodMultipleNestedCompoundInAndOut(const NestedCompound& argumentOne, NestedCompound& argumentTwo /* @out */) = 0;
    virtual uint32_t MethodMultipleNestedCompoundOutAndIn(NestedCompound& argumentOne /* @out */, const NestedCompound argumentTwo) const = 0;
    virtual uint32_t MethodMultipleNestedCompoundInAndInOut(const NestedCompound& argumentOne, NestedCompound& argumentTwo /* @inout */) = 0;
    virtual uint32_t MethodMultipleNestedCompoundOutAndInOut(NestedCompound& argumentOne /* @out */, NestedCompound& argumentTwo /* @inout */) = 0;
#endif

/////////// PROPERTIES

#if 1
    // @property
    virtual uint32_t PropertyFundmentalReadOnly(uint8_t& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalWriteOnly(const uint8_t argument) = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWrite(const uint8_t argument) = 0;
    virtual uint32_t PropertyFundmentalReadWrite(uint8_t& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrder(uint8_t& argument /* @out */) const = 0;
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrder(const uint8_t argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyStringReadOnly(string& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringRWriteOnly(const string& argument) = 0;
    // @property
    virtual uint32_t PropertyStringReadWrite(const string& argument) = 0;
    virtual uint32_t PropertyStringReadWrite(string& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringReadWriteDifferentOrder(string& argument /* @out */) const = 0;
    virtual uint32_t PropertyStringReadWriteDifferentOrder(const string& argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyEnumReadOnly(anenum& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyEnumWriteOnly(const anenum argument) = 0;
    // @property
    virtual uint32_t PropertyEnumReadWrite(const anenum argument) = 0;
    virtual uint32_t PropertyEnumReadWrite(anenum& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyEnumReadWriteDifferentOrder(anenum& argument /* @out */) const = 0;
    virtual uint32_t PropertyEnumReadWriteDifferentOrder(const anenum& argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyStringIteratorReadOnly(IStringIterator*& list /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringIteratorWriteOnly(IStringIterator* const list) = 0;
    // @property
    virtual uint32_t PropertyStringIteratorReadWrite(IStringIterator* const list) = 0;
    virtual uint32_t PropertyStringIteratorReadWrite(IStringIterator*& list /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringIteratorReadWriteDifferentOrder(IStringIterator*& list /* @out */) const = 0;
    virtual uint32_t PropertyStringIteratorReadWriteDifferentOrder(IStringIterator* const list) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyCompoundReadOnly(Compound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyCompoundWriteOnly(const Compound& argument) = 0;
    // @property
    virtual uint32_t PropertyCompoundReadWrite(const Compound& argument) = 0;
    virtual uint32_t PropertyCompoundReadWrite(Compound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyCompoundReadWriteDifferentOrder(Compound& argument /* @out */) const = 0;
    virtual uint32_t PropertyCompoundReadWriteDifferentOrder(const Compound& argument) = 0;
#endif

#if 0
    // @property
    virtual uint32_t PropertyNestedCompoundReadOnly(NestedCompound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundWriteOnly(const NestedCompound& argument) = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundReadWrite(const NestedCompound& argument) = 0;
    virtual uint32_t PropertyNestedCompoundReadWrite(NestedCompound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundReadWriteDifferentOrder(NestedCompound argument& /* @out */) const = 0;
    virtual uint32_t PropertyNestedCompoundReadWriteDifferentOrder(const NestedCompound& argument) = 0;
#endif

/////////// INDEXED PROPERTIES


#if 1
    // @property
    virtual uint32_t PropertyFundmentalReadOnlyIntegerIndex(const uint16_t index /* @index */, int& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalWriteOnlyIntegerIndex(const uint16_t index /* @index */, const int argument) = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWriteIntegerIndex(const uint16_t index /* @index */, const int argument) = 0;
    virtual uint32_t PropertyFundmentalReadWriteIntegerIndex(const uint16_t index /* @index */, int& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrderIntegerIndex(const uint16_t index /* @index */, int& argument /* @out */) const = 0;
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrderIntegerIndex(const uint16_t index /* @index */, const int argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyFundmentalReadOnlyStringIndex(const string& index /* @index */, int& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalWriteOnlyStringIndex(const string& index /* @index */, const int argument) = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWriteStringIndex(const string& index /* @index */, const int argument) = 0;
    virtual uint32_t PropertyFundmentalReadWriteStringIndex(const string& index /* @index */, int& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrderStringIndex(const string& index /* @index */, int& argument /* @out */) const = 0;
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrderStringIndex(const string& index /* @index */, const int argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyFundmentalReadOnlyEnumIndex(const anenum& index /* @index */, int& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalWriteOnlyEnumIndex(const anenum& index /* @index */, const int argument) = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWriteEnumIndex(const anenum& index /* @index */, const int argument) = 0;
    virtual uint32_t PropertyFundmentalReadWriteEnumIndex(const anenum& index /* @index */, int& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrderEnumIndex(const anenum& index /* @index */, int& argument /* @out */) const = 0;
    virtual uint32_t PropertyFundmentalReadWriteDifferentOrderEnumIndex(const anenum& index /* @index */, const int argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyStringReadOnlyStringIndex(const string& index /* @index */, string& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringWriteOnlyStringIndex(const string& index /* @index */, const string& argument) = 0;
    // @property
    virtual uint32_t PropertyStringReadWriteStringIndex(const string& index /* @index */, const string& argument) = 0;
    virtual uint32_t PropertyStringReadWriteStringIndex(const string& index /* @index */, string& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringReadWriteDifferentOrderStringIndex(const string& index /* @index */, string& argument /* @out */) const = 0;
    virtual uint32_t PropertyStringReadWriteDifferentOrderStringIndex(const string& index /* @index */, const string& argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyEnumReadOnlyStringIndex(const string& index /* @index */, anenum& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyEnumWriteOnlyStringIndex(const string& index /* @index */, const anenum argument) = 0;
    // @property
    virtual uint32_t PropertyEnumReadWriteStringIndex(const string& index /* @index */, const anenum argument) = 0;
    virtual uint32_t PropertyEnumReadWriteStringIndex(const string& index /* @index */, anenum& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyEnumReadWriteDifferentOrderStringIndex(const string& index /* @index */, anenum& argument /* @out */) const = 0;
    virtual uint32_t PropertyEnumReadWriteDifferentOrderStringIndex(const string& index /* @index */, const anenum argument) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyStringIteratorReadOnlyStringIndex(const string& index /* @index */, IStringIterator*& list /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringIteratorWriteOnlyStringIndex(const string& index /* @index */, IStringIterator* const list) = 0;
    // @property
    virtual uint32_t PropertyStringIteratorReadWriteStringIndex(const string& index /* @index */, IStringIterator* const list) = 0;
    virtual uint32_t PropertyStringIteratorReadWriteStringIndex(const string& index /* @index */, IStringIterator*& list /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyStringIteratorReadWriteDifferentOrderStringIndex(const string& index /* @index */, IStringIterator*& list /* @out */) const = 0;
    virtual uint32_t PropertyStringIteratorReadWriteDifferentOrdeStringIndexr(const string& index /* @index */, IStringIterator* const list) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyCompoundteratorReadOnlyEnumIndex(const anenum index /* @index */, ICompoundIterator*& list /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyCompoundIteratorWriteOnlyEnumIndex(const anenum index /* @index */, ICompoundIterator* const list) = 0;
    // @property
    virtual uint32_t PropertyCompoundIteratorReadWriteEnumIndex(const anenum index /* @index */, ICompoundIterator* const list) = 0;
    virtual uint32_t PropertyCompoundIteratorReadWriteEnumIndex(const anenum index /* @index */, ICompoundIterator*& list /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyCompoundIteratorReadWriteDifferentOrderStringIndex(const anenum index /* @index */, ICompoundIterator*& list /* @out */) const = 0;
    virtual uint32_t PropertyCompoundIteratorReadWriteDifferentOrdeStringIndexr(const anenum index /* @index */, ICompoundIterator* const list) = 0;
#endif

#if 1
    // @property
    virtual uint32_t PropertyCompoundReadOnlyStringIndex(const string& index /* @index */, Compound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyCompoundWriteOnlyStringIndex(const string& index /* @index */, const Compound argument) = 0;
    // @property
    virtual uint32_t PropertyCompoundReadWriteStringIndex(const string& index /* @index */, const Compound argument) = 0;
    virtual uint32_t PropertyCompoundReadWriteStringIndex(const string& index /* @index */, Compound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyCompoundReadWriteDifferentOrderStringIndex(const string& index /* @index */, Compound& argument /* @out */) const = 0;
    virtual uint32_t PropertyCompoundReadWriteDifferentOrderStringIndex(const string& index /* @index */, const Compound argument) = 0;
#endif

#if 0
    // @property
    virtual uint32_t PropertyNestedCompoundReadOnlyStringIndex(const string& index /* @index */, NestedCompound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundWriteOnlyStringIndex(const string& index /* @index */, const NestedCompound argument) = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundReadWriteStringIndex(const string& index /* @index */, const NestedCompound argument) = 0;
    virtual uint32_t PropertyNestedCompoundReadWriteStringIndex(const string& index /* @index */, NestedCompound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundReadWriteDifferentOrderStringIndex(const string& index /* @index */, NestedCompound& argument /* @out */) const = 0;
    virtual uint32_t PropertyNestedCompoundReadWriteDifferentOrderStringIndex(const string& index /* @index */, const NestedCompound argument) = 0;
#endif


#if 0
    // @property
    virtual uint32_t PropertyNestedCompoundReadOnlyIntegerIndex(const uint8_t& index /* @index */, NestedCompound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundWriteOnlyIntegerIndex(const uint8_t& index /* @index */, const NestedCompound argument) = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundReadWriteIntegerIndex(const uint8_t& index /* @index */, const NestedCompound argument) = 0;
    virtual uint32_t PropertyNestedCompoundReadWriteIntegerIndex(const uint8_t& index /* @index */, NestedCompound& argument /* @out */) const = 0;
    // @property
    virtual uint32_t PropertyNestedCompoundReadWriteDifferentOrderIntegerIndex(const uint8_t& index /* @index */, NestedCompound& argument /* @out */) const = 0;
    virtual uint32_t PropertyNestedCompoundReadWriteDifferentOrderIntegerIndex(const uint8_t& index /* @index */, const NestedCompound argument) = 0;
#endif

}; // class IJsonGeneratorCorpusExtended

} // namespace Exchange

}
