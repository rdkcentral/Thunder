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

// @stubgen:skip

namespace WPEFramework {
namespace Exchange {

    // This interface gives the possibility to create/defines commmands to be executed by
    // the CommanderPlugin
    struct EXTERNAL ICommand {

        struct EXTERNAL IFactory {

            virtual ~IFactory() = default;
            virtual Core::ProxyType<ICommand> Create(const string& label, const string& configuration) = 0;
        };

        struct EXTERNAL IRegistration {

            virtual ~IRegistration() = default;
            virtual void Register(const string& className, IFactory* factory) = 0;
            virtual IFactory* Unregister(const string& className) = 0;
        };

        virtual ~ICommand() = default;

        // Identification of the command. ClassName is the name of the class that implements
        // the logic associated with this command.
        virtual const string& ClassName() const = 0;

        // Label is the name of this instance. Execute returns, for example, the name of the
        // next command (identified by it's label) to be executed.
        virtual const string& Label() const = 0;

        // Excute the logic associated with this command. It is allowed to make this a blocking
        // call. The result of this command is the label of the next commmand to be executed.
        // Note: Empty label, means next step in sequence.
        virtual string Execute(PluginHost::IShell* service) = 0;

        // If the Command is blocking, make sure the Abort can terminate the flow of the
        // command within a determinable amount of time.
        virtual void Abort() = 0;
    };

    namespace Command {

        template <typename COMMAND>
        class FactoryType : public Exchange::ICommand::IFactory {
        private:
            template <typename IMPLEMENTATION>
            class CommandType : public Exchange::ICommand {
            public:
                CommandType() = delete;
                CommandType(const CommandType<IMPLEMENTATION>& copy) = delete;
                CommandType<IMPLEMENTATION>& operator=(const CommandType<IMPLEMENTATION>&) = delete;

                CommandType(const string& label, const string& configuration)
                    : _label(label)
                    , _className(Core::ClassNameOnly(typeid(IMPLEMENTATION).name()).Text())
                    , _implementation(configuration)
                {
                }
                ~CommandType() override = default;

            public:
                // Identification of the command. ClassName is the name of the class that implements
                // the logic associated with this command.
                virtual const string& ClassName() const
                {
                    return (_className);
                }

                // Label is the name of this instance. Execute returns, for example, the name of the
                // next command (identified by it's label) to be executed.
                virtual const string& Label() const
                {
                    return (_label);
                }

                // Excute the logic associated with this command. It is allowed to make this a blocking
                // call. The result of this command is the label of the next commmand to be executed.
                // Note: Empty label, means next step in sequence.
                virtual string Execute(PluginHost::IShell* service)
                {
                    return (_implementation.Execute(service));
                }

                // If the Command is blocking, make sure the Abort can terminate the flow of the
                // command within a determinable amount of time.
                virtual void Abort()
                {
                    __Abort<IMPLEMENTATION>();
                }

                // -----------------------------------------------------
                // Check for Abort method on Object
                // -----------------------------------------------------
                HAS_MEMBER(Abort, hasAbort);

                typedef hasAbort<IMPLEMENTATION, void (IMPLEMENTATION::*)()> TraitAbort;

                template <typename SUBJECT>
                inline typename Core::TypeTraits::enable_if<CommandType<SUBJECT>::TraitAbort::value, void>::type
                __Abort()
                {
                    _implementation.Abort();
                    ;
                }

                template <typename SUBJECT>
                inline typename Core::TypeTraits::enable_if<!CommandType<SUBJECT>::TraitAbort::value, void>::type
                __Abort()
                {
                }

            private:
                const string _label;
                const string _className;
                IMPLEMENTATION _implementation;
            };

        public:
            FactoryType(const FactoryType<COMMAND>&) = delete;
            FactoryType<COMMAND> operator=(const FactoryType<COMMAND>&) = delete;

            FactoryType() = default;
            ~FactoryType() override = default;

        public:
            virtual Core::ProxyType<Exchange::ICommand> Create(const string& label, const string& configuration)
            {
                return Core::proxy_cast<Exchange::ICommand>(Core::ProxyType<CommandType<COMMAND>>::Create(label, configuration));
            }
        };
    }
}
}

