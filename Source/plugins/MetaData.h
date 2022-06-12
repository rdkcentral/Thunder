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
                UNAVAILABLE = PluginHost::IShell::UNAVAILABLE,
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
#if THUNDER_RUNTIME_STATISTICS
            Core::JSON::DecUInt32 ProcessedRequests;
            Core::JSON::DecUInt32 ProcessedObjects;
#endif
#if THUNDER_RESTFULL_API
            Core::JSON::DecUInt32 Observers;
#endif
            Core::JSON::String Module;
            Core::JSON::String Hash;
            Core::JSON::DecUInt8 Major;
            Core::JSON::DecUInt8 Minor;
            Core::JSON::DecUInt8 Patch;
            Core::JSON::ArrayType<Core::JSON::String> InterfaceVersion;
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

            inline void Clear()
            {
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

            void Clear()
            {
                SubSystemContainer::iterator index(_subSystems.begin());
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

        inline void Clear()
        {
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

namespace Plugin {

    using subsystem = PluginHost::ISubSystem::subsystem;

    struct EXTERNAL IMetadata : public Core::IServiceMetadata {
        virtual ~IMetadata() = default;

        virtual const std::vector<subsystem>& Precondition() const = 0;
        virtual const std::vector<subsystem>& Termination() const = 0;
        virtual const std::vector<subsystem>& Control() const = 0;
    };

    // Baseclass to turn objects into services
    template <typename ACTUALSERVICE>
    class Metadata : public IMetadata {
    private:
        template <typename SERVICE>
        class PluginImplementation : public Core::Service<SERVICE>, public IMetadata {
        public:
            PluginImplementation() = delete;
            PluginImplementation(const PluginImplementation<SERVICE>&) = delete;
            PluginImplementation<SERVICE>& operator=(const PluginImplementation<SERVICE>&) = delete;

            explicit PluginImplementation(const Core::Library& library, const IServiceMetadata* metadata)
                : Core::Service<SERVICE>()
                , _referenceLib(library)
                , _info(static_cast<const IMetadata*>(metadata)) {

                ASSERT(dynamic_cast<const IMetadata*>(metadata) != nullptr);
            }
            ~PluginImplementation() override {
                Core::ServiceAdministrator::Instance().ReleaseLibrary(_referenceLib);
            }

        public:
            uint8_t Major() const override {
                return (_info->Major());
            }
            uint8_t Minor() const override {
                return (_info->Minor());
            }
            uint8_t Patch() const override {
                return (_info->Patch());
            }
            const std::string& ServiceName() const override {
                return (_info->ServiceName());
            }
            const TCHAR* Module() const override {
                return (_info->Module());
            }
            const std::vector<subsystem>& Precondition() const override {
                return (_info->Precondition());
            }
            const std::vector<subsystem>& Termination() const override {
                return (_info->Termination());
            }
            const std::vector<subsystem>& Control() const override {
                return (_info->Control());
            }

        private:
            Core::Library _referenceLib;
            const IMetadata* _info;
        };

    public:
        Metadata() = delete;
        Metadata(const Metadata&) = delete;
        Metadata& operator=(const Metadata&) = delete;

        Metadata(
            const uint8_t major,
            const uint8_t minor,
            const uint8_t patch,
            const std::initializer_list<subsystem>& precondition,
            const std::initializer_list<subsystem>& termination,
            const std::initializer_list<subsystem>& control)
            : _version((major << 16) | (minor << 8) | patch)
            , _Id(Core::ClassNameOnly(typeid(ACTUALSERVICE).name()).Text())
            , _precondition(precondition)
            , _termination(termination)
            , _control(control) {

            ASSERT(Core::System::ModuleServiceMetadata() == nullptr);
            Core::System::SetModuleServiceMetadata(this);
            Core::ServiceAdministrator::Instance().Register(this, &_factory);
        }
        ~Metadata() {
            Core::ServiceAdministrator::Instance().Unregister(this, &_factory);
            Core::System::SetModuleServiceMetadata(nullptr);
        }

    public:
        uint8_t Major() const override {
            return (static_cast<uint8_t> ((_version >> 16) & 0xFF));
        }
        uint8_t Minor() const override {
            return (static_cast<uint8_t>((_version >> 8) & 0xFF));
        }
        uint8_t Patch() const override {
            return (static_cast<uint8_t>(_version & 0xFF));
        }
        const std::string& ServiceName() const override {
            return (_Id);
        }
        const TCHAR* Module() const override {
            return (Core::System::ModuleName());
        }
        const std::vector<subsystem>& Precondition() const override {
            return (_precondition);
        }
        const std::vector<subsystem>& Termination() const override {
            return (_termination);
        }
        const std::vector<subsystem>& Control() const override {
            return (_control);
        }

    private:
        uint32_t _version;
        string _Id;
        std::vector<subsystem> _precondition;
        std::vector<subsystem> _termination;
        std::vector<subsystem> _control;
        Core::ServiceAdministrator::ServiceFactoryType< PluginImplementation <ACTUALSERVICE> > _factory;
    };
}

}

#endif // __WEBBRIDGESUPPORT_METADATA_H
