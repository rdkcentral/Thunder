#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct ITestController : virtual public Core::IUnknown {

        enum { ID = ID_TESTCONTROLLER };

        struct ITest : virtual public Core::IUnknown {

            enum { ID = ID_TESTCONTROLLER_TEST };

            struct IIterator : virtual public Core::IUnknown {

                enum { ID = ID_TESTCONTROLLER_TEST_ITERATOR };

                virtual void Reset() = 0;
                virtual bool IsValid() const = 0;
                virtual bool Next() = 0;

                virtual ITest* Test() const = 0;
            };

            virtual string Execute(const string& params) = 0;

            virtual string Description() const = 0;
            virtual string Name() const = 0;
        };

        struct ICategory : virtual public Core::IUnknown {

            enum { ID = ID_TESTCONTROLLER_CATEGORY };

            struct IIterator : virtual public Core::IUnknown {

                enum { ID = ID_TESTCONTROLLER_CATEGORY_ITERATOR };

                virtual void Reset() = 0;
                virtual bool IsValid() const = 0;
                virtual bool Next() = 0;

                virtual ICategory* Category() const = 0;
            };
            virtual string Name() const = 0;

            virtual void Setup() = 0;
            virtual void TearDown() = 0;

            virtual void Register(ITest* test) = 0;
            virtual void Unregister(ITest* test) = 0;
            virtual ITest::IIterator* Tests() const = 0;
            virtual ITest* Test(const string& name) const = 0;
        };

        virtual void Setup() = 0;
        virtual void TearDown() = 0;

        virtual ICategory::IIterator* Categories() const = 0;
        virtual ICategory* Category(const string& category) const = 0;
    };

} // namespace Exchange
} // namespace WPEFramework
