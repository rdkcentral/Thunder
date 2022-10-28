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
#include "Proxy.h"
#include "Sync.h"
#include "Frame.h"

namespace WPEFramework {
namespace Core {

    namespace Messaging {

        struct EXTERNAL IEvent {
            virtual ~IEvent() = default;
            virtual uint16_t Serialize(uint8_t buffer[], const uint16_t length) const = 0;
            virtual uint16_t Deserialize(const uint8_t buffer[], const uint16_t length) = 0;
            virtual const string& Data() const = 0;
        };

        /**
        * @brief Data-Carrier class storing information about basic information about the Message.
        *
        */
        class EXTERNAL Metadata {
        public:
            enum type : uint8_t {
                INVALID   = 0,
                TRACING   = 1,
                LOGGING   = 2,
                REPORTING = 3
            };

        public:
            Metadata(const Metadata&) = default;
            Metadata& operator=(const Metadata&) = default;
            Metadata(Metadata&&) = default;
            Metadata& operator=(Metadata&&) = default;

            Metadata()
                : _type(INVALID)
                , _category()
                , _module()
            {
            }
            Metadata(const type what, const string& category, const string& module)
                : _type(what)
                , _category(category)
                , _module(module)
            {
            }
            bool operator==(const Metadata& other) const
            {
                return ((_type == other._type) && (_category == other._category) && (_module == other._module));
            }
            bool operator!=(const Metadata& other) const
            {
                return !operator==(other);
            }

        public:
            type Type() const {
                return _type;
            }
            const string& Module() const {
                return _module;
            }
            const string& Category() const {
                return _category;
            }
            bool Default() const {
                return (_type == type::TRACING ? false : true);
            }
            bool Specific() const {
                return ((_type == type::LOGGING) || ((Category().empty() == false) && (Module().empty() == false)));
            }
            bool Applicable(const Metadata& rhs) const {
                return ((rhs.Type() == Type()) &&
                    (rhs.Module().empty() || Module().empty() || (rhs.Module() == Module())) &&
                    (rhs.Category().empty() || Category().empty() || (rhs.Category() == Category())));
            }

        public:
            uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const;
            uint16_t Deserialize(const uint8_t buffer[], const uint16_t bufferSize);

        private:
            type _type;
            string _category;
            string _module;
        };
 
        struct EXTERNAL IControl {

            struct EXTERNAL IHandler {
                virtual ~IHandler() = default;
                virtual void Handle (IControl* element) = 0;
            };

            virtual ~IControl() = default;
            virtual void Enable(bool enable) = 0;
            virtual bool Enable() const = 0;
            virtual void Destroy() = 0;

            virtual const Core::Messaging::Metadata& Metadata() const = 0;

            static void Announce(IControl* control);
            static void Revoke(IControl* control);
            static void Iterate(IHandler& handler);
        };

        /**
        * @brief Class responsible for:
        *        - Pushing data (Information) to a location
        */
        struct EXTERNAL IStore {
           /**
            * @brief Data-Carrier, extended information about the message
            */
            class EXTERNAL Information : public Metadata {
            public:
                Information(const Information&) = default;
                Information& operator=(const Information&) = default;

                Information()
                    : Metadata()
                    , _fileName()
                    , _lineNumber(0)
                    , _className()
                    , _timeStamp(0)
                {
                }
                Information(const Metadata& metadata, const string& fileName, const uint16_t lineNumber, const string& className, const uint64_t timeStamp)
                    : Metadata(metadata)
                    , _fileName(fileName)
                    , _lineNumber(lineNumber)
                    , _className(className)
                    , _timeStamp(timeStamp)
                {
                }
                ~Information() = default;

            public:
                const string& FileName() const
                {
                    return (_fileName);
                }
                uint16_t LineNumber() const
                {
                    return (_lineNumber);
                }
                const string& ClassName() const
                {
                    return (_className);
                }
                uint64_t TimeStamp() const
                {
                    return (_timeStamp);
                }

            public:
                uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const;
                uint16_t Deserialize(const uint8_t buffer[], const uint16_t bufferSize);

            private:
                string _fileName;
                uint16_t _lineNumber;
                string _className;
                uint64_t _timeStamp;
            };

	public:
            virtual ~IStore() = default;
            static IStore* Instance();
            static void Set (IStore*);

            virtual void Push(const Information& info, const IEvent* message) = 0;
        };


    } // namespace Messaging
} // namespace Core
}
