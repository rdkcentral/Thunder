#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct ITestUtility : virtual public Core::IUnknown {
        enum { ID = ID_TESTUTILITY };

        virtual ~ITestUtility(){};

        struct ICommand : virtual public Core::IUnknown {

            enum { ID = ID_TESTUTILITY_COMMAND };

            virtual ~ICommand(){};

            struct IIterator : virtual public Core::IUnknown {

                enum { ID = ID_TESTUTILITY_ITERATOR };

                virtual ~IIterator(){};

                virtual void Reset() = 0;
                virtual bool IsValid() const = 0;
                virtual bool Next() = 0;

                virtual ICommand* Command() const = 0;
            };

            virtual string Execute(const string& params) = 0;

            virtual string Description() const = 0;
            virtual string Signature() const = 0;
            virtual string Name() const = 0;
        };

        virtual ICommand::IIterator* Commands() const = 0;
        virtual ICommand* Command(const string& name) const = 0;
        virtual uint32_t ShutdownTimeout(const uint32_t timeout) = 0;
    };

} // namespace Exchange
} // namespace WPEFramework
