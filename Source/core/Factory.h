#ifndef __FACTORY_H
#define __FACTORY_H

#include <map>

#include "Portability.h"
#include "Proxy.h"

namespace WPEFramework {
namespace Core {
    template <typename BASEOBJECT, typename IDENTIFIER>
    class FactoryType {
    private:
        FactoryType(const FactoryType<BASEOBJECT, IDENTIFIER>&);
        FactoryType<BASEOBJECT, IDENTIFIER>& operator=(const FactoryType<BASEOBJECT, IDENTIFIER>&);

        struct IFactory {
            virtual ~IFactory(){};

            virtual Core::ProxyType<BASEOBJECT> GetElement() = 0;
            virtual uint32_t CreatedElements() const = 0;
            virtual uint32_t QueuedElements() const = 0;
            virtual uint32_t CurrentQueueSize() const = 0;
        };

        template <typename FACTORYELEMENT>
        class InternalFactoryType : public IFactory {
        private:
            InternalFactoryType(const InternalFactoryType<FACTORYELEMENT>&);
            InternalFactoryType<FACTORYELEMENT>& operator=(const InternalFactoryType<FACTORYELEMENT>&);

        public:
            InternalFactoryType(const uint32_t initialQueueSize)
                : _warehouse(initialQueueSize)
            {
            }
            virtual ~InternalFactoryType()
            {
            }

        public:
            virtual Core::ProxyType<BASEOBJECT> GetElement()
            {
                return (Core::proxy_cast<BASEOBJECT>(_warehouse.Element()));
            }
            virtual uint32_t CreatedElements() const
            {
                return (_warehouse.CreatedElements());
            }
            virtual uint32_t QueuedElements() const
            {
                return (_warehouse.QueuedElements());
            }
            virtual uint32_t CurrentQueueSize() const
            {
                return (_warehouse.CurrentQueueSize());
            }

        private:
            Core::ProxyPoolType<FACTORYELEMENT> _warehouse;
        };

    public:
        class Iterator {
        private:
            Iterator();
            Iterator& operator=(const Iterator&);

        public:
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
            ~Iterator()
            {
            }

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
                }
                else if (_index != _map.end()) {
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
        FactoryType()
        {
        }
        FactoryType(const uint8_t /* queueuSize */)
        {
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
            return (Core::proxy_cast<ACTUALELEMENT>(Element(identifier)));
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
