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

#include "Communicator.h"
#include "IUnknown.h"
#include "ITrace.h"

namespace WPEFramework {
namespace ProxyStub {

    // Creat a Handler for the Trace Controller:
    class TraceIterator : public Trace::ITraceIterator {
    private:
        TraceIterator(const TraceIterator&);
        TraceIterator& operator=(const TraceIterator&);

    public:
        TraceIterator()
            : _traceIterator(Trace::TraceUnit::Instance().GetCategories())
        {
            TRACE_L1("Created an object for interfaceId <Trace::ITraceIterator>, located @0x%p.\n", this);
        }
        virtual ~TraceIterator()
        {
            TRACE_L1("Destructed an object for interfaceId <Trace::ITraceIterator>, located @0x%p.\n", this);
        }

    public:
        virtual void Reset() override
        {
            _traceIterator.Reset(0);
        }
        virtual bool Info(bool& enabled, string& module, string& category) const override
        {
            bool result = _traceIterator.Next();

            if (result == true) {
                category = _traceIterator->Category();
                module = _traceIterator->Module();
                enabled = _traceIterator->Enabled();
            }

            return (result);
        }

        BEGIN_INTERFACE_MAP(TraceIterator)
        INTERFACE_ENTRY(Trace::ITraceIterator)
        END_INTERFACE_MAP

    private:
        mutable Trace::TraceUnit::Iterator _traceIterator;
    };

    SERVICE_REGISTRATION(TraceIterator, 1, 0);

    class TraceController : public Trace::ITraceController {
    private:
        TraceController(const TraceController&);
        TraceController& operator=(const TraceController&);

    public:
        TraceController()
        {
            TRACE_L1("Constructed an object for interfaceId <Trace::ITraceController>, located @0x%p.\n", this);
        }
        virtual ~TraceController()
        {
            TRACE_L1("Destructed an object for interfaceId <Trace::ITraceController>, located @0x%p.\n", this);
        }

    public:
        virtual void Enable(const bool enabled, const string& module, const string& category) override
        {
            Trace::TraceUnit::Instance().SetCategories(
                enabled,
                (module.empty() == false ? module.c_str() : nullptr),
                (category.empty() == false ? category.c_str() : nullptr));
        }

        BEGIN_INTERFACE_MAP(TraceController)
        INTERFACE_ENTRY(Trace::ITraceController)
        END_INTERFACE_MAP
    };

    SERVICE_REGISTRATION(TraceController, 1, 0);
}
}
