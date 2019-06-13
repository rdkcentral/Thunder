#pragma once

#include "Module.h"

namespace WPEFramework {
namespace ProcessContainers {

    using IStringIterator = Core::IteratorType<std::vector<string>, const string>;

    struct IContainerAdministrator {

        struct IContainer {
            IContainer() = default;
            virtual ~IContainer() = default;

            virtual const string& Id() const = 0;
            virtual void Start(const string& command, IStringIterator& parameters) = 0;
            virtual bool Stop(const uint32_t timeout /*ms*/, const bool kill) = 0; //returns true on success

            virtual void AddRef() const = 0;
            virtual uint32_t Release() const = 0;
        };

        static IContainerAdministrator& Instance();

        virtual void ContainerDefinitionSearchPaths(IStringIterator& searchpaths) = 0;  //note will be searched in order in which they are iterated

        IContainerAdministrator() = default;
        virtual ~IContainerAdministrator() = default;

        // Lifetime management
        virtual void AddRef() const = 0;
        virtual uint32_t Release() const = 0;

        // Methods
        virtual IContainer* Container(const string& id) = 0;
    };
} // ProcessContainers
} // WPEFramework

