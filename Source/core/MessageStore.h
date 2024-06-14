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

namespace Thunder {

namespace Core {

    namespace Messaging {

        extern EXTERNAL const char* MODULE_LOGGING;
        extern EXTERNAL const char* MODULE_REPORTING;
        extern EXTERNAL const char* MODULE_OPERATIONAL_STREAM;

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
                INVALID             = 0,
                TRACING             = 1,
                LOGGING             = 2,
                REPORTING           = 3,
                OPERATIONAL_STREAM  = 4
            };

// @stop

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
            virtual ~Metadata() = default;
            
            bool operator==(const Metadata& other) const {
                return ((_type == other._type) && (_category == other._category) && (_module == other._module));
            }
            bool operator!=(const Metadata& other) const {
                return (!operator==(other));
            }

        public:
            type Type() const {
                return (_type);
            }

            const string& Module() const {
                return (_module);
            }

            const string& Category() const {
                return (_category);
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
            virtual uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const;
            virtual uint16_t Deserialize(const uint8_t buffer[], const uint16_t bufferSize);

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
        * @brief Data-Carrier class storing the Metadata and timeStamp of the Message.
        */
        class EXTERNAL MessageInfo : public Metadata {
        public:
            enum abbreviate : uint8_t {
                FULL        = 0,
                ABBREVIATED = 1
            };

        public:
            MessageInfo(const MessageInfo&) = default;
            MessageInfo& operator=(const MessageInfo&) = default;
            MessageInfo(MessageInfo&&) = default;
            MessageInfo& operator=(MessageInfo&&) = default;

            MessageInfo()
                : Metadata()
                , _timeStamp()
            {
            }
            MessageInfo(const Metadata& metadata, const uint64_t timeStamp)
                : Metadata(metadata)
                , _timeStamp(timeStamp)
            {
            }
            ~MessageInfo() = default;

        public:
            uint64_t TimeStamp() const {
                return (_timeStamp);
            }

        public:
            uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const override;
            uint16_t Deserialize(const uint8_t buffer[], const uint16_t bufferSize) override;
            virtual string ToString(const abbreviate abbreviate) const;

        private:
            uint64_t _timeStamp;
        };

        /**
        * @brief Class responsible for:
        *        - Pushing data (Information) to a location
        */
        struct EXTERNAL IStore {
           /**
            * @brief Data-Carrier, extended information about the logging-type message.
            *        No additional info for now, used for function overloading.
            */
            class EXTERNAL Logging : public MessageInfo {
            public:
                Logging(const Logging&) = default;
                Logging& operator=(const Logging&) = default;
                Logging(Logging&&) = default;
                Logging& operator=(Logging&&) = default;

                Logging()
                    : MessageInfo()
                {
                }
                Logging(const MessageInfo& messageInfo)
                    : MessageInfo(messageInfo)
                {
                }
                ~Logging() = default;
            };

            /**
            * @brief Data-Carrier, extended information about the tracing-type message
            */
            class EXTERNAL Tracing : public MessageInfo {
            public:
                Tracing(const Tracing&) = default;
                Tracing& operator=(const Tracing&) = default;
                Tracing(Tracing&&) = default;
                Tracing& operator=(Tracing&&) = default;

                Tracing()
                    : MessageInfo()
                    , _fileName()
                    , _lineNumber(0)
                    , _className()
                {
                }
                Tracing(const MessageInfo& messageInfo, const string& fileName, const uint16_t lineNumber, const string& className)
                    : MessageInfo(messageInfo)
                    , _fileName(fileName)
                    , _lineNumber(lineNumber)
                    , _className(className)
                {
                }
                ~Tracing() = default;

            public:
                const string& FileName() const {
                    return (_fileName);
                }

                uint16_t LineNumber() const {
                    return (_lineNumber);
                }

                const string& ClassName() const {
                    return (_className);
                }

            public:
                uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const override;
                uint16_t Deserialize(const uint8_t buffer[], const uint16_t bufferSize) override;
                string ToString(const abbreviate abbreviate) const override;

            private:
                string _fileName;
                uint16_t _lineNumber;
                string _className;
        };

           /**
            * @brief Data-Carrier, extended information about the warning-reporting-type message.
            */
            class EXTERNAL WarningReporting : public MessageInfo {
            public:
                WarningReporting(const WarningReporting&) = default;
                WarningReporting& operator=(const WarningReporting&) = default;
                WarningReporting(WarningReporting&&) = default;
                WarningReporting& operator=(WarningReporting&&) = default;

                WarningReporting()
                    : MessageInfo()
                {
                }
                WarningReporting(const MessageInfo& messageInfo, const string& callsign)
                    : MessageInfo(messageInfo),
                    _callsign(callsign)
                {
                }
                ~WarningReporting() = default;

            public:
                const string& Callsign() const {
                    return (_callsign);
                }

            public:
                uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const override;
                uint16_t Deserialize(const uint8_t buffer[], const uint16_t bufferSize) override;
                string ToString(const abbreviate abbreviate) const override;

            private:
                string _callsign;
            };

           /**
            * @brief Data-Carrier, extended information about the operational-stream-type message.
            *        No additional info for now, used for function overloading.
            */
            class EXTERNAL OperationalStream : public MessageInfo {
            public:
                OperationalStream(const OperationalStream&) = default;
                OperationalStream& operator=(const OperationalStream&) = default;
                OperationalStream(OperationalStream&&) = default;
                OperationalStream& operator=(OperationalStream&&) = default;

                OperationalStream()
                    : MessageInfo()
                {
                }
                OperationalStream(const MessageInfo& messageInfo)
                    : MessageInfo(messageInfo)
                {
                }
                ~OperationalStream() = default;
            };

            public:
            virtual ~IStore() = default;
            static IStore* Instance();
            static void Set(IStore*);

            virtual bool Default(const Metadata& metadata) const = 0;
            virtual void Push(const MessageInfo& messageInfo, const IEvent* message) = 0;
        };

    } // namespace Messaging
} // namespace Core
}
