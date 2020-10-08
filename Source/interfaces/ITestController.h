/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include "Module.h"

namespace WPEFramework {
    namespace Exchange {

        struct EXTERNAL ITestController : virtual public Core::IUnknown {
            enum { ID = ID_TESTCONTROLLER };

            struct EXTERNAL ITest : virtual public Core::IUnknown {
                enum { ID = ID_TESTCONTROLLER_TEST };

                struct EXTERNAL IIterator : virtual public Core::IUnknown {
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

            struct EXTERNAL ICategory : virtual public Core::IUnknown {
                enum { ID = ID_TESTCONTROLLER_CATEGORY };

                struct EXTERNAL IIterator : virtual public Core::IUnknown {
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