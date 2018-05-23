#ifndef __ICOMMAND_H
#define __ICOMMAND_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This interface gives the possibility to create/defines commmands to be executed by
    // the CommanderPlugin
    struct ICommand {

        enum { ID = 0x00000044 };

        virtual ~ICommand() {}

        struct IFactory {
            virtual ~IFactory() {}

            virtual Core::ProxyType<ICommand> Create(const string& label, const string& configuration) = 0;
        };

       struct IRegistration {
            enum { ID = 0x00000045 };

            virtual ~IRegistration() {}

            virtual void Register(const string& className, IFactory* factory) = 0;
            virtual IFactory* Unregister(const string& className) = 0;
        };

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
            FactoryType(const FactoryType<COMMAND>&) = delete;
            FactoryType<COMMAND> operator=(const FactoryType<COMMAND>&) = delete;
 
        private:
            template <typename IMPLEMENTATION>
            class CommandType : public Exchange::ICommand {
            private:
                CommandType() = delete;
                CommandType(const CommandType<IMPLEMENTATION>& copy) = delete;
                CommandType<IMPLEMENTATION>& operator=(const CommandType<IMPLEMENTATION>&) = delete;
 
            public:
                CommandType(const string& label, const string& configuration)
                    : _label(label)
                    , _className(Core::ClassNameOnly(typeid(IMPLEMENTATION).name()).Text())
                    , _implementation(configuration)
                {
                }
                virtual ~CommandType()
                {
                }
 
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
            FactoryType()
            {
            }
            virtual ~FactoryType()
            {
            }
 
        public:
            virtual Core::ProxyType<Exchange::ICommand> Create(const string& label, const string& configuration)
            {
                return Core::proxy_cast<Exchange::ICommand>(Core::ProxyType<CommandType<COMMAND> >::Create(label, configuration));
            }
        };
    }

}
}

#endif // __ICOMMAND_H
