#pragma once

#include "Module.h"

namespace WPEFramework {
namespace ProcessContainers {

    using IStringIterator = Core::IteratorType<std::vector<string>, const string>;
    struct IContainerAdministrator {

        struct IContainer {
            enum State {
                STOPPED,
                STARTING,
                RUNNING,
                ABORTING,
                STOPPING
            };

            IContainer() = default;
            virtual ~IContainer() = default;

            virtual const string& Id() const = 0;
            virtual pid_t Pid() const = 0;
            virtual string Memory() const = 0; // In bytes
            virtual string Cpu() const = 0; // in nanoseconds of cpu time
            virtual string Configuration() const = 0;
            virtual string Log() const = 0;
            virtual void Networks(std::vector<std::string>& networks) const = 0;
            virtual void IPs(std::vector<Core::NodeId>& ips) const = 0;
            virtual State ContainerState() const = 0;
            virtual bool IsRunning() const = 0;

            virtual bool Start(const string& command, IStringIterator& parameters) = 0; // returns true when started
            virtual bool Stop(const uint32_t timeout /*ms*/) = 0; // returns true when stopped, note if timeout == 0 asynchronous

            virtual void AddRef() const = 0;
            virtual uint32_t Release() const = 0;
        };

        using ContainerIterator = Core::IteratorType<std::list<IContainer*>, IContainer*>;

        static IContainerAdministrator& Instance();

        IContainerAdministrator() = default;
        virtual ~IContainerAdministrator() = default;

        // Lifetime management
        virtual void AddRef() const = 0;
        virtual uint32_t Release() const = 0;

        // Methods
        virtual IContainer* Container(const string& id, 
                                      IStringIterator& searchpaths, 
                                      const string& logpath,
                                      const string& configuration) = 0; //searchpaths will be searched in order in which they are iterated

        virtual void Logging(const string& logpath, const string& logid, const string& loggingoptions) = 0;
        virtual ContainerIterator Containers() = 0;
        
        virtual IContainer* Find(const string& name);
    };
} // ProcessContainers
} // WPEFramework

