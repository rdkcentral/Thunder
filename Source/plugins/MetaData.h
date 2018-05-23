#ifndef __WEBBRIDGESUPPORT_METADATA_H
#define __WEBBRIDGESUPPORT_METADATA_H

#include "Module.h"
#include "Configuration.h"

namespace WPEFramework {
namespace PluginHost {

    // Status information
    // this class holds interesting information that can be requested from the Server
    class EXTERNAL MetaData : public Core::JSON::Container {
    private:
        MetaData(const MetaData&) = delete;
        MetaData& operator=(const MetaData&) = delete;

    public:
        class EXTERNAL Service : public Plugin::Config {
        private:
            Service& operator=(const Service& RHS) = delete;

        public:
            enum state {
                DEACTIVATED = PluginHost::IShell::DEACTIVATED,
                DEACTIVATION = PluginHost::IShell::DEACTIVATION,
                ACTIVATED = PluginHost::IShell::ACTIVATED,
                ACTIVATION = PluginHost::IShell::ACTIVATION,
                DESTROYED = PluginHost::IShell::DESTROYED,
                PRECONDITION = PluginHost::IShell::PRECONDITION,
                SUSPENDED,
                RESUMED
            };

            class EXTERNAL State : public Core::JSON::EnumType<state> {
            public:
                inline State()
                    : Core::JSON::EnumType<state>()
                {
                }
                inline State(const State& copy)
                    : Core::JSON::EnumType<state>(copy)
                {
                }
                inline ~State()
                {
                }

            public:
                State& operator=(const PluginHost::IShell* RHS);

                string Data() const;
            };

        public:
            Service();
            Service(const Service& copy);
            ~Service();

        public:
            Service& operator=(const Plugin::Config& RHS);

            State JSONState;
#ifdef RUNTIME_STATISTICS
            Core::JSON::DecUInt32 ProcessedRequests;
            Core::JSON::DecUInt32 ProcessedObjects;
#endif
            Core::JSON::DecUInt32 Observers;
            Core::JSON::String Module;
            Core::JSON::String Hash;
        };

        class EXTERNAL Channel : public Core::JSON::Container {
        public:
            enum state {
                CLOSED,
                WEBSERVER,
                WEBSOCKET,
                RAWSOCKET,
                SUSPENDED
            };
            class EXTERNAL State : public Core::JSON::EnumType<state> {
            public:
                inline State()
                    : Core::JSON::EnumType<state>()
                {
                }
                inline State(const State& copy)
                    : Core::JSON::EnumType<state>(copy)
                {
                }
                inline ~State()
                {
                }

            public:
                State& operator=(const state RHS);

                string Data() const;
            };

        public:
            Channel();
            Channel(const Channel& copy);
            ~Channel();

            Channel& operator=(const Channel& RHS);

        public:
            Core::JSON::String Remote;
            State JSONState;
            Core::JSON::Boolean Activity;
            Core::JSON::DecUInt32 ID;
            Core::JSON::String Name;
        };

        class EXTERNAL Bridge : public Core::JSON::Container {
        private:
            Bridge& operator=(const Bridge&) = delete;

        public:
            Bridge();
            Bridge(const string& text, const uint32_t latency, const string& model, const bool secure);
            Bridge(const Bridge& copy);
            ~Bridge();

        public:
            Core::JSON::String Locator;
            Core::JSON::DecUInt32 Latency;
            Core::JSON::String Model;
            Core::JSON::Boolean Secure;
        };

        class EXTERNAL Server : public Core::JSON::Container {
        private:
            Server(const Server& copy) = delete;
            Server& operator=(const Server&) = delete;

        public:
            Server();
            ~Server();

            inline void Clear() {
                ThreadPoolRuns.Clear();
            }

        public:
            Core::JSON::ArrayType<Core::JSON::DecUInt32> ThreadPoolRuns;
            Core::JSON::DecUInt32 PendingRequests;
            Core::JSON::DecUInt32 PoolOccupation;
        };

        class EXTERNAL SubSystem : public Core::JSON::Container {
        private:
            SubSystem& operator=(const SubSystem&) = delete;

            typedef std::map<const string, Core::JSON::Boolean> SubSystemContainer;
            typedef SubSystemContainer::iterator Iterator;

        public:
            SubSystem();
            SubSystem(const SubSystem& copy);
            ~SubSystem();

        public:
            void Add(const PluginHost::ISubSystem::subsystem name, const bool available);

            void Clear() {
		SubSystemContainer::iterator index (_subSystems.begin());
                while (index != _subSystems.end()) {

                    Core::JSON::Container::Remove(index->first.c_str());
                    
                    index++;
                }
                _subSystems.clear();
            }

        private:
            SubSystemContainer _subSystems;
        };

    public:
        MetaData();
        ~MetaData();

	inline void Clear() {
            SubSystems.Clear();
            Plugins.Clear();
            Channels.Clear();
            Bridges.Clear();
            Process.Clear();
        }

    public:
        SubSystem SubSystems;
        Core::JSON::ArrayType<Service> Plugins;
        Core::JSON::ArrayType<Channel> Channels;
        Core::JSON::ArrayType<Bridge> Bridges;
        Server Process;
        Core::JSON::String Value;
    };
}
}

#endif // __WEBBRIDGESUPPORT_METADATA_H
