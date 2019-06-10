#pragma once

#include "Module.h"

namespace WPEFramework {
namespace ProcessContainers {

    struct IContainerAdministrator {

        struct IContainer {
            IContainer() = default;
            virtual ~IContainer() = default;

            virtual const string& Id() const = 0;
            virtual void Start(const string& command, const string parameters) = 0;

            virtual void AddRef() const = 0;
            virtual uint32_t Release() const = 0;
        };

        static IContainerAdministrator& Instance();

        // will search in the order, [0], [1], ...
        virtual void ContainerDefinitionSearchPaths(const std::vector<string>&& searchpaths) = 0;  

        IContainerAdministrator() = default;
        virtual ~IContainerAdministrator() = default;

        // Lifetime management
//        virtual void AddRef() const = 0;
//        virtual uint32_t Release() const = 0;

        // Methods
        virtual IContainer* Container(const string& id) = 0;

        virtual string  GetNames() const = 0; 
    };
} // ProcessContainers
} // WPEFramework

