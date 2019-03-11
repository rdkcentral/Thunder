#pragma once

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"

namespace WPEFramework {
namespace RPC {
    struct IStringIterator : virtual public Core::IUnknown {
        enum { ID = ID_STRINGITERATOR };

        virtual ~IStringIterator(){};

        virtual bool Next(string& result) = 0;
        virtual bool Previous(string& result) = 0;
        virtual void Reset(const uint32_t position) = 0;
        virtual bool IsValid() const = 0;
        virtual uint32_t Count() const = 0;
        virtual string Current() const = 0;
    };

    class StringIterator : virtual public IStringIterator {
    private:
        StringIterator() = delete;
        StringIterator(const StringIterator&) = delete;
        StringIterator& operator=(const StringIterator&) = delete;

    public:
        template <typename CONTAINER, typename PREDICATE>
        StringIterator(const CONTAINER& container, PREDICATE predicate)
            : _container()
            , _index(0)
        {
            std::copy_if(container.begin(), container.end(), std::back_inserter(_container), predicate);
            _iterator = _container.begin();
        }
        template <typename CONTAINER>
        StringIterator(const CONTAINER& container)
            : _container()
            , _index(0)
        {
            std::copy_if(container.begin(), container.end(), std::back_inserter(_container), [](const string& data) { return (true); });
            _iterator = _container.begin();
        }
        template <typename KEY, typename VALUE>
        StringIterator(const std::map<KEY, VALUE>& container)
            : _container()
            , _index(0)
        {
            typename std::map<KEY, VALUE>::const_iterator index(container.begin());
            while (index != container.end()) {
                _container.push_back(index->first);
                index++;
            }
            _iterator = _container.begin();
        }
        StringIterator(IStringIterator* index)
            : _container()
            , _index(0)
        {
            string result;
            while (index->Next(result) == true) {
                _container.push_back(result);
            }
            _iterator = _container.begin();
        }

        ~StringIterator()
        {
        }

    public:
        virtual bool IsValid() const override
        {
            return ((_index > 0) && (_index <= Count()));
        }
        virtual void Reset(const uint32_t position) override
        {
            if (position == 0) {
                _iterator = _container.begin();
                _index = 0;
            } else if (position > Count()) {
                _iterator = _container.end();
                _index = Count() + 1;
            } else if ((position < _index) && ((_index - position) < position)) {
                // Better that we walk back from where we are ;-)
                while (_index != position) {
                    _index--;
                    _iterator--;
                }
            } else {
                _iterator = _container.begin();
                _index = position;

                // Move forward the requested number of steps..
                for (uint32_t teller = 1; teller < position; teller++) {
                    _iterator++;
                }

                ASSERT(_iterator != _container.end());
            }
        }

        virtual bool Previous(string& result) override
        {
            if (_index != 0) {
                if (_index > 1) {
                    _iterator--;
                }
                _index--;

                ASSERT((_index != 0) || (_iterator == _container.begin()));

                if (_index > 0) {
                    result = *_iterator;
                }
            }
            return (IsValid());
        }
        virtual bool Next(string& result) override
        {
            uint32_t length = static_cast<uint32_t>(_container.size());

            if (_index <= length) {
                _index++;

                if (_index != 1) {
                    _iterator++;

                    ASSERT((_index <= length) || (_iterator == _container.end()));
                }

                if (_index <= length) {
                    result = *_iterator;
                }
            }
            return (IsValid());
        }
        virtual uint32_t Count() const override
        {
            return (static_cast<uint32_t>(_container.size()));
        }
        virtual string Current() const override
        {
            ASSERT(IsValid());

            return (*_iterator);
        }

        BEGIN_INTERFACE_MAP(StringIterator)
        INTERFACE_ENTRY(IStringIterator)
        END_INTERFACE_MAP

    private:
        std::list<string> _container;
        mutable std::list<string>::iterator _iterator;
        mutable uint32_t _index;
    };
}
}
