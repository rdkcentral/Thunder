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

#include "Module.h"
#include "Metadata.h"
#include "IStateControl.h"
#include "ISubSystem.h"

namespace Thunder {

    ENUM_CONVERSION_BEGIN(PluginHost::ISubSystem::IInternet::network_type)

    { PluginHost::ISubSystem::IInternet::UNKNOWN, _TXT("Unknown") },
    { PluginHost::ISubSystem::IInternet::IPV4, _TXT("IPv4") },
    { PluginHost::ISubSystem::IInternet::IPV6, _TXT("IPv6") },

    ENUM_CONVERSION_END(PluginHost::ISubSystem::IInternet::network_type)

namespace PluginHost
{

    /* static */ const TCHAR* ISubSystem::IInternet::ToString(const network_type value)
    {
        return (Core::EnumerateType<network_type>(value).Data());
    }

    Metadata::Service::State& Metadata::Service::State::operator=(const PluginHost::IShell* RHS)
    {
        Core::JSON::EnumType<state>::operator=(static_cast<state>(RHS->State()));

        if (RHS->State() == PluginHost::IShell::ACTIVATED) {
            // See if there is a state interface, if so, we are suspended or resumed :-)
            PluginHost::IStateControl* mode = const_cast<PluginHost::IShell*>(RHS)->QueryInterface<PluginHost::IStateControl>();

            if (mode != nullptr) {
                Core::JSON::EnumType<state>::operator=(mode->State() == PluginHost::IStateControl::RESUMED ? state::RESUMED : state::SUSPENDED);
                mode->Release();
            }
        }

        return (*this);
    }

    Metadata::Service::State& Metadata::Service::State::operator=(const Metadata::Service::State& RHS) 
    {
        Core::JSON::EnumType<state>::operator= (RHS);

        return (*this);
    }

    Metadata::Service::State& Metadata::Service::State::operator=(Metadata::Service::State&& move)
    {
        if (this != &move) {
            Core::JSON::EnumType<state>::operator= (move);
        }
        return (*this);
    }

    string Metadata::Service::State::Data() const
    {
        return (Core::JSON::EnumType<state>::Data());
    }

    Metadata::Channel::State& Metadata::Channel::State::operator=(const Metadata::Channel::state RHS)
    {
        Core::JSON::EnumType<state>::operator=(RHS);

        return (*this);
    }

    Metadata::Channel::State& Metadata::Channel::State::operator=(Metadata::Channel::State&& move)
    {
        if (this != &move) {
            Core::JSON::EnumType<state>::operator=(move);
        }

        return (*this);
    }

    Metadata::Channel::State& Metadata::Channel::State::operator=(const Metadata::Channel::State& RHS)
    {
        Core::JSON::EnumType<state>::operator=(RHS);

        return (*this);
    }

    string Metadata::Channel::State::Data() const
    {
        return (Core::JSON::EnumType<state>::Data());
    }

    Metadata::Service::Service()
        : Plugin::Config()
        , JSONState()
#if THUNDER_RUNTIME_STATISTICS
        , ProcessedRequests(0)
        , ProcessedObjects(0)
#endif
#if THUNDER_RESTFULL_API
        , Observers(0)
#endif
        , ServiceVersion()
        , Module()
        , InterfaceVersion()
    {
        Add(_T("state"), &JSONState);
#if THUNDER_RUNTIME_STATISTICS
        Add(_T("processedrequests"), &ProcessedRequests);
        Add(_T("processedobjects"), &ProcessedObjects);
#endif
#if THUNDER_RESTFULL_API
        Add(_T("observers"), &Observers);
#endif
        Add(_T("module"), &Module);
        Add(_T("version"), &ServiceVersion);
        Add(_T("interface"), &InterfaceVersion);
    }
    Metadata::Service::Service(Metadata::Service&& move)
        : Plugin::Config(move)
        , JSONState(std::move(move.JSONState))
#if THUNDER_RUNTIME_STATISTICS
        , ProcessedRequests(std::move(move.ProcessedRequests))
        , ProcessedObjects(std::move(move.ProcessedObjects))
#endif
#if THUNDER_RESTFULL_API
        , Observers(std::move(move.Observers))
#endif
        , ServiceVersion(std::move(move.ServiceVersion))
        , Module(std::move(move.Module))
        , InterfaceVersion(std::move(move.InterfaceVersion))
    {
        Add(_T("state"), &JSONState);
#if THUNDER_RUNTIME_STATISTICS
        Add(_T("processedrequests"), &ProcessedRequests);
        Add(_T("processedobjects"), &ProcessedObjects);
#endif
#if THUNDER_RESTFULL_API
        Add(_T("observers"), &Observers);
#endif
        Add(_T("module"), &Module);
        Add(_T("version"), &ServiceVersion);

        Add(_T("interface"), &InterfaceVersion);
    }
    Metadata::Service::Service(const Metadata::Service& copy)
        : Plugin::Config(copy)
        , JSONState(copy.JSONState)
#if THUNDER_RUNTIME_STATISTICS
        , ProcessedRequests(copy.ProcessedRequests)
        , ProcessedObjects(copy.ProcessedObjects)
#endif
#if THUNDER_RESTFULL_API
        , Observers(copy.Observers)
#endif
        , ServiceVersion(copy.ServiceVersion)
        , Module(copy.Module)
        , InterfaceVersion(copy.InterfaceVersion)
    {
        Add(_T("state"), &JSONState);
#if THUNDER_RUNTIME_STATISTICS
        Add(_T("processedrequests"), &ProcessedRequests);
        Add(_T("processedobjects"), &ProcessedObjects);
#endif
#if THUNDER_RESTFULL_API
        Add(_T("observers"), &Observers);
#endif
        Add(_T("module"), &Module);
        Add(_T("version"), &ServiceVersion);

        Add(_T("interface"), &InterfaceVersion);
    }

    Metadata::Service& Metadata::Service::operator=(const Plugin::Config& RHS)
    {
        Plugin::Config::operator=(RHS);
        return (*this);
    }

    Metadata::Channel::Channel()
        : Core::JSON::Container()
    {
        Core::JSON::Container::Add(_T("remote"), &Remote);
        Core::JSON::Container::Add(_T("state"), &JSONState);
        Core::JSON::Container::Add(_T("activity"), &Activity);
        Core::JSON::Container::Add(_T("id"), &ID);
        Core::JSON::Container::Add(_T("name"), &Name);
    }
    Metadata::Channel::Channel(Metadata::Channel&& move)
        : Core::JSON::Container()
        , Remote(std::move(move.Remote))
        , JSONState(std::move(move.JSONState))
        , Activity(std::move(move.Activity))
        , ID(std::move(move.ID))
        , Name(std::move(move.Name))
    {
        Core::JSON::Container::Add(_T("remote"), &Remote);
        Core::JSON::Container::Add(_T("state"), &JSONState);
        Core::JSON::Container::Add(_T("activity"), &Activity);
        Core::JSON::Container::Add(_T("id"), &ID);
        Core::JSON::Container::Add(_T("name"), &Name);
    }
    Metadata::Channel::Channel(const Metadata::Channel& copy)
        : Core::JSON::Container()
        , Remote(copy.Remote)
        , JSONState(copy.JSONState)
        , Activity(copy.Activity)
        , ID(copy.ID)
        , Name(copy.Name)
    {
        Core::JSON::Container::Add(_T("remote"), &Remote);
        Core::JSON::Container::Add(_T("state"), &JSONState);
        Core::JSON::Container::Add(_T("activity"), &Activity);
        Core::JSON::Container::Add(_T("id"), &ID);
        Core::JSON::Container::Add(_T("name"), &Name);
    }

    Metadata::Channel& Metadata::Channel::operator=(Metadata::Channel&& move)
    {
        if (this != &move) {
            Remote = std::move(move.Remote);
            JSONState = std::move(move.JSONState);
            Activity = std::move(move.Activity);
            ID = std::move(move.ID);
            Name = std::move(move.Name);
        }

        return (*this);
    }
    Metadata::Channel& Metadata::Channel::operator=(const Metadata::Channel& RHS)
    {
        Remote = RHS.Remote;
        JSONState = RHS.JSONState;
        Activity = RHS.Activity;
        ID = RHS.ID;
        Name = RHS.Name;

        return (*this);
    }

    Metadata::Server::Minion::Minion() 
        : Core::JSON::Container()
        , Id(0)
        , Job()
        , Runs(0) {
        Add(_T("id"), &Id);
        Add(_T("job"), &Job);
        Add(_T("runs"), &Runs);
    }
    Metadata::Server::Minion::Minion(Minion&& move)
        : Core::JSON::Container()
        , Id(std::move(move.Id))
        , Job(std::move(move.Job))
        , Runs(std::move(move.Runs)) {
        Add(_T("id"), &Id);
        Add(_T("job"), &Job);
        Add(_T("runs"), &Runs);
    }
    Metadata::Server::Minion::Minion(const Minion& copy)
        : Core::JSON::Container()
        , Id(copy.Id)
        , Job(copy.Job)
        , Runs(copy.Runs) {
        Add(_T("id"), &Id);
        Add(_T("job"), &Job);
        Add(_T("runs"), &Runs);
    }
    Metadata::Server::Minion& Metadata::Server::Minion::operator=(const Core::ThreadPool::Metadata& info) {
        Id = (Core::instance_id)info.WorkerId;

        Runs = info.Runs;
        if (info.Job.IsSet() == false) {
            Job.Clear();
        }
        else {
            Job = info.Job.Value();
        }
        return (*this);
    }

    Metadata::Server::Server()
        : Core::JSON::Container()
        , ThreadPoolRuns()
        , PendingRequests()
    {
        Core::JSON::Container::Add(_T("threads"), &ThreadPoolRuns);
        Core::JSON::Container::Add(_T("pending"), &PendingRequests);
    }

    Metadata::Metadata()
        : Core::JSON::Container()
        , SubSystems()
        , Plugins()
        , Channels()
        , Bridges()
        , Process()
        , Value()
        , AppVersion()
    {
        Core::JSON::Container::Add(_T("subsystems"), &SubSystems);
        Core::JSON::Container::Add(_T("plugins"), &Plugins);
        Core::JSON::Container::Add(_T("channel"), &Channels);
        Core::JSON::Container::Add(_T("bridges"), &Bridges);
        Core::JSON::Container::Add(_T("server"), &Process);
        Core::JSON::Container::Add(_T("value"), &Value);
        Core::JSON::Container::Add(_T("version"), &AppVersion);
    }
    Metadata::~Metadata()
    {
    }

    Metadata::SubSystem::SubSystem()
        : Core::JSON::Container()
        , _subSystems()
    {
    }
    Metadata::SubSystem::SubSystem(SubSystem&& move)
        : Core::JSON::Container()
        , _subSystems(move._subSystems)
    {
        SubSystems::iterator index(_subSystems.begin());

        while (index != _subSystems.end()) {
            Core::JSON::Container::Add(index->first.c_str(), &(index->second));
            index++;
        }
    }
    Metadata::SubSystem::SubSystem(const SubSystem& copy)
        : Core::JSON::Container()
        , _subSystems(copy._subSystems)
    {
        SubSystems::iterator index(_subSystems.begin());

        while (index != _subSystems.end()) {
            Core::JSON::Container::Add(index->first.c_str(), &(index->second));
            index++;
        }
    }

    void Metadata::SubSystem::Add(const PluginHost::ISubSystem::subsystem type, const bool available)
    {
        const string name(Core::EnumerateType<PluginHost::ISubSystem::subsystem>(type).Data());

        // Create an element for this service with its callsign
        std::pair<SubSystems::iterator, bool> index(_subSystems.insert(std::pair<string, Core::JSON::Boolean>(name, Core::JSON::Boolean())));

        // Store the override config in the JSON String created in the map
        Core::JSON::Container::Add(index.first->first.c_str(), &(index.first->second));

        index.first->second = available;
    }
}
}
