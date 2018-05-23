#ifndef __IRPCIterator_H
#define __IRPCIterator_H

#include "Module.h"

namespace WPEFramework {
namespace PluginHost {

    struct IStringIterator : virtual public Core::IUnknown {
        virtual ~IStringIterator() {}

        enum { ID = 0x00000027 };

        virtual bool IsValid() const = 0;
        virtual void Reset() = 0;
        virtual bool Next(string& result) = 0;
    };

    struct IValueIterator : virtual public Core::IUnknown {
        virtual ~IValueIterator() {}

        enum { ID = 0x00000028 };

        virtual bool IsValid() const = 0;
        virtual void Reset() = 0;
        virtual bool Next(uint32_t& value) = 0;
    };

    template <typename CONTAINER>
    class StringIterator : public IStringIterator {
    private:
        StringIterator(const StringIterator<CONTAINER>&) = delete;
        StringIterator<CONTAINER>& operator=(const StringIterator<CONTAINER>&) = delete;

        StringIterator(const CONTAINER* container)
            : _container(*container)
            , _iterator(_container.begin())
            , _start(true)
        {
        }

    public:
        ~StringIterator()
        {
        }
        StringIterator<CONTAINER>& Create(const CONTAINER& container)
        {
            return (Core::Service<StringIterator<CONTAINER> >::Create < StringIterator<CONTAINER>(&container));
        }

    public:
        virtual bool IsValid()
        {
            return ((_start == false) && (_iterator != _container.end()));
        }
        virtual void Reset()
        {
            _start = true;
            _iterator = _container.begin();
        }
        virtual bool Next(string& result)
        {
            if (_start == true) {
                _start = false;
            }
            else if (_iterator != _container.end()) {
                _iterator++;
            }

            if (_iterator != _container.end()) {
                result = *_iterator;
            }
            return (false);
        }

    protected:
        BEGIN_INTERFACE_MAP(StringIterator<CONTAINER>)
        INTERFACE_ENTRY(IStringIterator)
        END_INTERFACE_MAP

    private:
        CONTAINER _container;
        typename CONTAINER::const_iterator _iterator;
        bool _start;
    };

    template <typename CONTAINER>
    class ValueIterator : public IValueIterator {
    private:
        ValueIterator(const ValueIterator<CONTAINER>&) = delete;
        ValueIterator<CONTAINER>& operator=(const ValueIterator<CONTAINER>&) = delete;

        ValueIterator(const CONTAINER* container)
            : _container(*container)
            , _iterator(_container.begin())
            , _start(true)
        {
        }

    public:
        ~ValueIterator()
        {
        }
        ValueIterator<CONTAINER>& Create(const CONTAINER& container)
        {
            return (Core::Service<ValueIterator<CONTAINER> >::Create < ValueIterator<CONTAINER>(&container));
        }

    public:
        virtual bool IsValid() const
        {
            return ((_start == false) && (_iterator != _container.end()));
        }
        virtual void Reset()
        {
            _start = true;
            _iterator = _container.begin();
        }
        virtual bool Next(uint32_t& value)
        {
            if (_start == true) {
                _start = false;
            }
            else if (_iterator != _container.end()) {
                _iterator++;
            }

            if (_iterator != _container.end()) {
                value = *_iterator;
            }
            return (false);
        }

    protected:
        BEGIN_INTERFACE_MAP(ValueIterator<CONTAINER>)
        INTERFACE_ENTRY(IStringIterator)
        END_INTERFACE_MAP

    private:
        CONTAINER _container;
        typename CONTAINER::const_iterator _iterator;
        bool _start;
    };
}
}

#endif // __IRPCIterator_H
