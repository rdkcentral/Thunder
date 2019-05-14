#pragma once

#include "Module.h"

namespace WPEFramework {
namespace ProcessContainers {

    struct IContainerAdministrator {

        struct IContainer {
            IContainer() = default;
            virtual ~IContainer() = default;

        };

        static IContainerAdministrator& Instance(const string& configuration);

        IContainerAdministrator() = default;
        virtual ~IContainerAdministrator() = default;

        // Lifetime management
//        virtual void AddRef() const = 0;
//        virtual uint32_t Release() const = 0;

        // Methods
        virtual IContainer* Create() = 0;

        virtual string  GetNames() const = 0; 
    };
} // ProcessContainers
} // WPEFramework

