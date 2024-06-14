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


namespace Thunder {

namespace RPC {

    template<typename INTERFACE>
    class IteratorType : public INTERFACE {
    public:
        using Container = typename std::list<typename INTERFACE::Element>;

        IteratorType() = delete;
        IteratorType(const IteratorType&) = delete;
        IteratorType& operator=(const IteratorType&) = delete;

        explicit IteratorType(Container&& container)
            : _container(std::forward<Container>(container))
            , _index(0)
        {
            _iterator = _container.begin();
        }
        template <typename CONTAINER, typename PREDICATE>
        IteratorType(const CONTAINER& container, PREDICATE predicate)
            : _container()
            , _index(0)
        {
            std::copy_if(container.begin(), container.end(), std::back_inserter(_container), predicate);
            _iterator = _container.begin();
        }
        template <typename CONTAINER>
        IteratorType(const CONTAINER& container)
            : _container()
            , _index(0)
        {
            std::copy_if(container.begin(), container.end(), std::back_inserter(_container), [](const typename INTERFACE::Element& /* data */) { return (true); });
            _iterator = _container.begin();
        }
        template <typename KEY, typename VALUE>
        IteratorType(const std::map<KEY, VALUE>& container)
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
        IteratorType(INTERFACE* index)
            : _container()
            , _index(0)
        {
            if (index != nullptr) {
                typename INTERFACE::Element result;
                 while (index->Next(result) == true) {
                    _container.push_back(result);
                }
            }
            _iterator = _container.begin();
        }

        ~IteratorType()
        {
        }

    public:
        virtual uint32_t AddRef() const = 0;
        virtual uint32_t Release() const = 0;

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

        virtual bool Previous(typename INTERFACE::Element& result) override
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
        virtual bool Next(typename INTERFACE::Element& result) override
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
        virtual typename INTERFACE::Element Current() const override
        {
            ASSERT(IsValid());

            return (*_iterator);
        }
        void Add(const typename INTERFACE::Element& element) {
            _container.push_back(element);
        }

        BEGIN_INTERFACE_MAP(IteratorType<INTERFACE>)
            INTERFACE_ENTRY(INTERFACE)
        END_INTERFACE_MAP

    private:
        Container _container;
        mutable typename Container::iterator _iterator;
        mutable uint32_t _index;
    };
}
}
