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

#ifndef __FACTORY_H
#define __FACTORY_H

#include <map>

#include "Portability.h"
#include "Proxy.h"

namespace Thunder {
namespace Core {
    template <typename BASEOBJECT, typename IDENTIFIER>
    class FactoryType {
    private:
        struct IFactory {
            virtual ~IFactory() = default;

            virtual Core::ProxyType<BASEOBJECT> GetElement() = 0;
            virtual uint32_t CreatedElements() const = 0;
            virtual uint32_t QueuedElements() const = 0;
        };

        template <typename FACTORYELEMENT>
        class InternalFactoryType : public IFactory {
        public:
            InternalFactoryType(const InternalFactoryType<FACTORYELEMENT>&) = delete;
            InternalFactoryType<FACTORYELEMENT>& operator=(const InternalFactoryType<FACTORYELEMENT>&) = delete;

            InternalFactoryType(const uint32_t initialQueueSize)
                : _warehouse(initialQueueSize)
            {
            }
            ~InternalFactoryType() override = default;

        public:
            Core::ProxyType<BASEOBJECT> GetElement() override
            {
                return (Core::ProxyType<BASEOBJECT>(_warehouse.Element()));
            }
            uint32_t CreatedElements() const override
            {
                return (_warehouse.CreatedElements());
            }
            uint32_t QueuedElements() const override
            {
                return (_warehouse.QueuedElements());
            }

        private:
            Core::ProxyPoolType<FACTORYELEMENT> _warehouse;
        };

    public:
        class Iterator {
        public:
            Iterator() = delete;
            Iterator& operator=(const Iterator&) = delete;

            Iterator(const std::map<IDENTIFIER, IFactory*>& warehouse)
                : _map(warehouse)
                , _index(warehouse.begin())
                , _start(true)
            {
            }
            Iterator(const Iterator& copy)
                : _map(copy._map)
                , _index(_map.begin())
                , _start(true)
            {
            }
            Iterator(Iterator&& move)
                : _map(move._map.begin(), move._map.end())
                , _index(move._index.begin(), move._index.end())
                , _start(move._start)
            {
                move._map.clear();
                move._index.clear();
                move._start = true;
            }
            ~Iterator() = default;

        public:
            bool IsValid() const
            {
                return ((_start == false) && (_index != _map.end()));
            }
            void Reset()
            {
                _start = true;
                _index = _map.begin();
            }
            bool Next()
            {
                if (_start == true) {
                    _start = false;
                } else if (_index != _map.end()) {
                    _index++;
                }

                return (_index != _map.end());
            }
            inline const IDENTIFIER& Label() const
            {
                ASSERT(IsValid());

                return (_index->first);
            }
            inline uint32_t CreatedElements() const
            {
                ASSERT(IsValid());

                return (_index->second->CreatedElements());
            }
            inline uint32_t QueuedElements() const
            {
                ASSERT(IsValid());

                return (_index->second->QueuedElements());
            }
            inline uint32_t CurrentQueueSize() const
            {
                ASSERT(IsValid());

                return (_index->second->CurrentQueueSize());
            }

        private:
            const std::map<IDENTIFIER, IFactory*>& _map;
            typename std::map<IDENTIFIER, IFactory*>::const_iterator _index;
            bool _start;
        };

    public:
        FactoryType(const FactoryType<BASEOBJECT, IDENTIFIER>&) = delete;
        FactoryType<BASEOBJECT, IDENTIFIER>& operator=(const FactoryType<BASEOBJECT, IDENTIFIER>&) = delete;

        FactoryType() = default;
        FactoryType(const uint8_t /* queueuSize */) {
        }
        ~FactoryType()
        {
            // Please dispose of the created factories that you have created...
            ASSERT(_receptors.size() == 0);
        }

    public:
        void DestroyFactories()
        {
            // Start deleting all factories...
            while (_receptors.size() > 0) {
                delete (_receptors.begin()->second);

                _receptors.erase(_receptors.begin());
            }
        }
        void DestroyFactory(const IDENTIFIER& identifier)
        {
            typename std::map<IDENTIFIER, IFactory*>::iterator index(_receptors.find(identifier));

            // Only if we find the factory, we delete it. Could be made an assert :-)
            if (index != _receptors.end()) {
                delete index->second;

                _receptors.erase(index);
            }
        }
        template <typename ACTUALELEMENT>
        inline void DestroyFactory()
        {
            DestroyFactory(ACTUALELEMENT::Id());
        }
        template <typename ACTUALELEMENT>
        void CreateFactory(const uint32_t initialQueueSize)
        {
            IDENTIFIER label(ACTUALELEMENT::Id());
            typename std::map<IDENTIFIER, IFactory*>::iterator index(_receptors.find(label));

            // Do not insert the same identifier twice !!!!
            ASSERT(index == _receptors.end());

            _receptors.insert(std::pair<IDENTIFIER, IFactory*>(label, new InternalFactoryType<ACTUALELEMENT>(initialQueueSize)));
        }
        template <typename ACTUALELEMENT>
        void CreateFactory(const IDENTIFIER& identifier, const uint32_t initialQueueSize)
        {
            typename std::map<IDENTIFIER, IFactory*>::iterator index(_receptors.find(identifier));

            // Do not insert the same identifier twice !!!!
            ASSERT(index == _receptors.end());

            _receptors.insert(std::pair<IDENTIFIER, IFactory*>(identifier, new InternalFactoryType<ACTUALELEMENT>(initialQueueSize)));
        }

        template <typename ACTUALELEMENT>
        inline Core::ProxyType<ACTUALELEMENT> Get(const IDENTIFIER& identifier)
        {
            return (Core::ProxyType<ACTUALELEMENT>(Element(identifier)));
        }

        Core::ProxyType<BASEOBJECT> Element(const IDENTIFIER& identifier)
        {
            Core::ProxyType<BASEOBJECT> result;
            typename std::map<IDENTIFIER, IFactory*>::iterator index(_receptors.find(identifier));

            if (index != _receptors.end()) {
                result = index->second->GetElement();
            }

            return result;
        }
        inline Iterator Factories() const
        {
            return (Iterator(_receptors));
        }

    private:
        std::map<IDENTIFIER, IFactory*> _receptors;
    };
}
} // namespace Core

#endif // __FACTORY_H
