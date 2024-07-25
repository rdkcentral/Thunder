/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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
#include "BaseCategory.h"

namespace Thunder {

namespace Trace {

    // for backwards compatibility 
    template <typename... Args>
    inline void Format(string& dst, Args&&... args) { Core::Format(dst, std::forward<Args>(args)...); }
    template <typename... Args>
    inline string Format(Args&&... args) { return (Core::Format(std::forward<Args>(args)...)); }

    DEFINE_MESSAGING_CATEGORY(Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING>, Text)
    DEFINE_MESSAGING_CATEGORY(Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING>, Initialisation)
    DEFINE_MESSAGING_CATEGORY(Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING>, Information)
    DEFINE_MESSAGING_CATEGORY(Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING>, Warning)
    DEFINE_MESSAGING_CATEGORY(Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING>, Error)
    DEFINE_MESSAGING_CATEGORY(Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING>, Fatal)

    class EXTERNAL Constructor {
    private:
        Constructor() = default;
        ~Constructor() = default;
        Constructor(const Constructor&) = delete;
        Constructor& operator=(const Constructor&) = delete;

    public:
        const char* Data() const {
            return (_text.c_str());
        }

        uint16_t Length() const {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        static const string _text;
    };

    class EXTERNAL CopyConstructor {
    private:
        CopyConstructor() = default;
        ~CopyConstructor() = default;
        CopyConstructor(const CopyConstructor&) = delete;
        CopyConstructor& operator=(const CopyConstructor&) = delete;

    public:
        const char* Data() const {
            return (_text.c_str());
        }

        uint16_t Length() const {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        static const string _text;
    };

    class EXTERNAL AssignmentOperator {
    private:
        AssignmentOperator() = default;
        ~AssignmentOperator() = default;
        AssignmentOperator(const AssignmentOperator&) = delete;
        AssignmentOperator& operator=(const AssignmentOperator&) = delete;

    public:
        const char* Data() const {
            return (_text.c_str());
        }

        uint16_t Length() const {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        static const string _text;
    };

    class EXTERNAL Destructor {
    private:
        Destructor() = default;
        ~Destructor() = default;
        Destructor(const Destructor&) = delete;
        Destructor& operator=(const Destructor&) = delete;

    public:
        const char* Data() const {
            return (_text.c_str());
        }

        uint16_t Length() const {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        static const string _text;
    };

    class EXTERNAL MethodEntry : public Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING> {
    private:
        MethodEntry() = delete;
        ~MethodEntry() = default;
        MethodEntry(const MethodEntry&) = delete;
        MethodEntry& operator=(const MethodEntry&) = delete;

    public:
        MethodEntry(const string& methodName)
            : BaseCategory("Entered method: " + methodName)
        {
        }
    };

    class EXTERNAL MethodExit : public Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING> {
    private:
        MethodExit() = delete;
        ~MethodExit() = default;
        MethodExit(const MethodExit&) = delete;
        MethodExit& operator=(const MethodExit&) = delete;

    public:
        MethodExit(const string& methodName)
            : BaseCategory("Exited method: " + methodName)
        {
        }
    };

    class EXTERNAL Duration : public Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING> {
    public:
        Duration() = delete;
        Duration(const Duration&) = delete;
        Duration& operator=(const Duration&) = delete;

        template<typename... Args>
        Duration(const Core::Time& startTime, const string& formatter, Args... args)
            : BaseCategory()
        {
            const uint64_t duration(Core::Time::Now().Ticks() - startTime.Ticks());
            std::string message;
            Core::Format(message, (string(_T("%" PRIu64 "us, ")) + formatter), duration, args...);
            Set(message);
        }

        explicit Duration(const Core::Time& startTime)
            : BaseCategory()
        {
            const uint64_t duration(Core::Time::Now().Ticks() - startTime.Ticks());
            std::string message;
            Core::Format(message, _T("%" PRIu64 "us"), duration);
            Set(message);
        }
    };

} // namespace Trace
}
