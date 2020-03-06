#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // @json
    template<typename ELEMENT, const uint32_t INTERFACE_ID>
    struct IIteratorType : virtual public Core::IUnknown {

        typedef ELEMENT Element;

        enum { ID = INTERFACE_ID };

        virtual ~IIteratorType(){};

        // @brief GIVES NEXT
        // @Param info DUPA
        virtual bool Next(ELEMENT& /* @out */ info) = 0;
        virtual bool Previous(ELEMENT& /* @out */ info) = 0;
        virtual void Reset(const uint32_t position) = 0;
        virtual bool IsValid() const = 0;
        virtual uint32_t Count() const = 0;
        virtual ELEMENT Current() const = 0;
    };

    // @json
    typedef IIteratorType<string, 10> IStringIterator;

}
}