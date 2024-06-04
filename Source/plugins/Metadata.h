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

#pragma once

#include "Module.h"
#include "Configuration.h"

#include "IController.h"
#include <plugins/json/JsonData_Metadata.h>
#include <plugins/json/JsonData_Discovery.h>

namespace Thunder {
namespace PluginHost {

    // Status information
    // this class holds interesting information that can be requested from the Server
    class EXTERNAL Metadata : public Core::JSON::Container {
    public:
        using Version = JsonData::Metadata::VersionInfo;
        using Bridge = JsonData::Discovery::DiscoveryResultData;

        class EXTERNAL Service : public Plugin::Config {
        public:
            using state = Exchange::Controller::IMetadata::Data::Service::state;

            class EXTERNAL State : public Core::JSON::EnumType<state> {
            public:
                State()
                    : Core::JSON::EnumType<state>()
                {
                }
                State(State&& move)
                    : Core::JSON::EnumType<state>(move)
                {
                }
                State(const State& copy)
                    : Core::JSON::EnumType<state>(copy)
                {
                }
                ~State() override = default;

            public:
                State& operator=(State&& RHS);
                State& operator=(const State& RHS);
                State& operator=(const PluginHost::IShell* RHS);

                string Data() const;
            };

        public:
            Service& operator=(Service&&) = delete;
            Service& operator=(const Service&) = delete;

            Service();
            Service(Service&& move);
            Service(const Service& copy);
            ~Service() override = default;

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
            Version ServiceVersion;
            Core::JSON::String Module;
            Core::JSON::ArrayType<Core::JSON::String> InterfaceVersion;
        };
        class EXTERNAL Channel : public Core::JSON::Container {
        public:
            using state = Exchange::Controller::IMetadata::Data::Link::state;

            class EXTERNAL State : public Core::JSON::EnumType<state> {
            public:
                inline State() = default;
                inline State(State&& move)
                    : Core::JSON::EnumType<state>(move)
                {
                }
                inline State(const State& copy)
                    : Core::JSON::EnumType<state>(copy)
                {
                }
                inline ~State() override = default;

            public:
                State& operator=(const state RHS);
                State& operator=(State&& move);
                State& operator=(const State& RHS);

                string Data() const;
            };

        public:
            Channel();
            Channel(Channel&& move);
            Channel(const Channel& copy);
            ~Channel() override = default;

            Channel& operator=(Channel&& move);
            Channel& operator=(const Channel& RHS);

        public:
            Core::JSON::String Remote;
            State JSONState;
            Core::JSON::Boolean Activity;
            Core::JSON::DecUInt32 ID;
            Core::JSON::String Name;
        };
        class EXTERNAL Server : public Core::JSON::Container {
        public:
            class EXTERNAL Minion : public Core::JSON::Container {
            public:
                Minion& operator=(Minion&&) = delete;
                Minion& operator=(const Minion&) = delete;

                Minion();
                Minion(Minion&& move);
                Minion(const Minion& copy);
                ~Minion() override = default;

                Minion& operator=(const Core::ThreadPool::Metadata&);

            public:
                Core::JSON::InstanceId Id;
                Core::JSON::String Job;
                Core::JSON::DecUInt32 Runs;
            };

        public:
            Server(Server&&) = delete;
            Server(const Server&) = delete;
            Server& operator=(Server&&) = delete;
            Server& operator=(const Server&) = delete;

            Server();
            ~Server() override = default;

            inline void Clear()
            {
                ThreadPoolRuns.Clear();
                PendingRequests.Clear();
            }

        public:
            Core::JSON::ArrayType<Minion> ThreadPoolRuns;
            Core::JSON::ArrayType<Core::JSON::String> PendingRequests;
        };
        class EXTERNAL SubSystem : public Core::JSON::Container {
        private:
            using SubSystems = std::unordered_map<string, Core::JSON::Boolean> ;

        public:
            SubSystem& operator=(SubSystem&&) = delete;
            SubSystem& operator=(const SubSystem&) = delete;

            SubSystem();
            SubSystem(SubSystem&& move);
            SubSystem(const SubSystem& copy);
            ~SubSystem() override = default;

        public:
            void Add(const PluginHost::ISubSystem::subsystem name, const bool available);

            void Clear()
            {
                SubSystems::iterator index(_subSystems.begin());
                while (index != _subSystems.end()) {

                    Core::JSON::Container::Remove(index->first.c_str());

                    index++;
                }
                _subSystems.clear();
            }

        private:
            SubSystems _subSystems;
        };
        class EXTERNAL COMRPC : public Core::JSON::Container {
        public:
            using Proxy = JsonData::Metadata::ProxyData;

        public:
            COMRPC& operator=(COMRPC&&) = delete;
            COMRPC& operator=(const COMRPC&) = delete;

            COMRPC()
                : Core::JSON::Container()
                , Remote()
                , Proxies() {
                Add(_T("link"), &Remote);
                Add(_T("proxies"), &Proxies);
            }
            COMRPC(COMRPC&& move)
                : Core::JSON::Container()
                , Remote(std::move(move.Remote))
                , Proxies(std::move(move.Proxies)) {
                Add(_T("link"), &Remote);
                Add(_T("proxies"), &Proxies);
            }
            COMRPC(const COMRPC& copy)
                : Core::JSON::Container()
                , Remote(copy.Remote)
                , Proxies(copy.Proxies) {
                Add(_T("link"), &Remote);
                Add(_T("proxies"), &Proxies);
            }
            ~COMRPC() override = default;

            void Clear() {
                Remote.Clear();
                Proxies.Clear();
            }

        public:
            Core::JSON::String Remote;
            Core::JSON::ArrayType<Proxy> Proxies;
        };

    public:
        Metadata(Metadata&&) = delete;
        Metadata(const Metadata&) = delete;
        Metadata& operator=(Metadata&&) = delete;
        Metadata& operator=(const Metadata&) = delete;

        Metadata();
        ~Metadata();

        inline void Clear()
        {
            SubSystems.Clear();
            Plugins.Clear();
            Channels.Clear();
            Bridges.Clear();
            Process.Clear();
            AppVersion.Clear();
        }

    public:
        SubSystem SubSystems;
        Core::JSON::ArrayType<Service> Plugins;
        Core::JSON::ArrayType<Channel> Channels;
        Core::JSON::ArrayType<Bridge> Bridges;
        Server Process;
        Core::JSON::String Value;
        Version AppVersion;
    };
}

namespace Plugin {

    using subsystem = PluginHost::ISubSystem::subsystem;

    struct EXTERNAL IMetadata : public Core::IService::IMetadata {
        ~IMetadata() override = default;

        virtual const TCHAR* InstanceId() const = 0;
        virtual const std::vector<subsystem>& Precondition() const = 0;
        virtual const std::vector<subsystem>& Termination() const = 0;
        virtual const std::vector<subsystem>& Control() const = 0;
    };

    // Baseclass to turn objects into services
    template <typename ACTUALSERVICE>
    class Metadata : public IMetadata {
    public:
        Metadata() = delete;
        Metadata(Metadata&&) = delete;
        Metadata(const Metadata&) = delete;
        Metadata& operator=(Metadata&&) = delete;
        Metadata& operator=(const Metadata&) = delete;

        Metadata(
            const uint8_t major,
            const uint8_t minor,
            const uint8_t patch,
            const std::initializer_list<subsystem>& precondition,
            const std::initializer_list<subsystem>& termination,
            const std::initializer_list<subsystem>& control)
            : _plugin()
            , _precondition(precondition)
            , _termination(termination)
            , _control(control)
            , _service(Core::System::MODULE_NAME, major, minor, patch) {
            ASSERT(Core::System::ROOT_META_DATA == nullptr);
            Core::System::ROOT_META_DATA = this;
        }
        ~Metadata() {
            Core::System::ROOT_META_DATA = nullptr;
        }

    public:
        uint8_t Major() const override {
            return (_service.Metadata()->Major());
        }
        uint8_t Minor() const override {
            return (_service.Metadata()->Minor());
        }
        uint8_t Patch() const override {
            return (_service.Metadata()->Patch());
        }
        const TCHAR* InstanceId() const override {
            if (_plugin.empty()) {
                _plugin = string(_service.Metadata()->Module()) + "::" + string(_service.Metadata()->ServiceName());
            }
            return (_plugin.c_str());
        }
        const TCHAR* ServiceName() const override {
            return (_service.Metadata()->ServiceName());
        }
        const TCHAR* Module() const override {
            return (_service.Metadata()->Module());
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
        mutable string _plugin;
        std::vector<subsystem> _precondition;
        std::vector<subsystem> _termination;
        std::vector<subsystem> _control;
        Core::PublishedServiceType<ACTUALSERVICE> _service;
    };
}

}
