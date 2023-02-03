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

#include "SystemInfo.h"
namespace WPEFramework {
namespace PluginHost {

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
    SystemInfo::SystemInfo(const Config& config, Core::IDispatch* callback)
        : _adminLock()
        , _config(config)
        , _notificationClients()
        , _callback(callback)
        , _identifier(nullptr)
        , _location(nullptr)
        , _internet(nullptr)
        , _security(nullptr)
        , _time(nullptr)
        , _provisioning(nullptr)
        , _flags(0)
    {
        ASSERT(callback != nullptr);
    }
POP_WARNING()

    /* virtual */ SystemInfo::~SystemInfo()
    {
      if (_identifier)
        _identifier->Release();
      if (_location)
        _location->Release();
      if (_internet)
        _internet->Release();
      if (_security)
        _security->Release();
      if (_time)
        _time->Release();
      if (_provisioning)
        _provisioning->Release();
    }

    void SystemInfo::Register(PluginHost::ISubSystem::INotification* notification)
    {
        _adminLock.Lock();

        ASSERT(std::find(_notificationClients.begin(), _notificationClients.end(), notification) == _notificationClients.end());

        _notificationClients.push_back(notification);
        notification->AddRef();

        // Give the registering sink a chance to evaluate the current info before one actually changes.
        notification->Updated();

        _adminLock.Unlock();
    }

    void SystemInfo::Unregister(PluginHost::ISubSystem::INotification* notification)
    {
        _adminLock.Lock();

        std::list<PluginHost::ISubSystem::INotification*>::iterator index(std::find(_notificationClients.begin(), _notificationClients.end(), notification));

        // ASSERT(index != _notificationClients.end());

        if (index != _notificationClients.end()) {
            _notificationClients.erase(index);
            notification->Release();
        } else {
            TRACE_L1("Notification(%p) not found.", notification);
        }

        _adminLock.Unlock();
    }

    // Use MAC address and let the framework handle the OTP ID.
    const uint8_t* SystemInfo::RawDeviceId(const string& interfaceName) const
    {
        static uint8_t* MACAddress = nullptr;
        static uint8_t MACAddressBuffer[Core::AdapterIterator::MacSize + 1];

        if (MACAddress == nullptr) {
            memset(MACAddressBuffer, 0, Core::AdapterIterator::MacSize + 1);

            if (interfaceName.empty() != true) {

                Core::AdapterIterator adapter(interfaceName);
                if ((adapter.IsValid() == true) && adapter.HasMAC() == true) {
                    adapter.MACAddress(&MACAddressBuffer[1], Core::AdapterIterator::MacSize);
                }
            } else {

                Core::AdapterIterator adapters;
                while ((adapters.Next() == true)) {
                    if (adapters.HasMAC() == true) {
                        adapters.MACAddress(&MACAddressBuffer[1], Core::AdapterIterator::MacSize);
                        break;
                    }
                }
            }
            MACAddressBuffer[0] = Core::AdapterIterator::MacSize;
            MACAddress = &MACAddressBuffer[0];
        }

        return MACAddress;
    }

    void SystemInfo::Update()
    {
        _adminLock.Lock();

        _callback->Dispatch();

        ClientIterator index(_notificationClients);

        RecursiveList(index);
    }

    void SystemInfo::RecursiveList(ClientIterator& index)
    {
        if (index.Next() == true) {
            PluginHost::ISubSystem::INotification* callee(*index);

            ASSERT(callee != nullptr);

            callee->AddRef();

            RecursiveList(index);

            callee->Updated();
            callee->Release();
        } else {
            _adminLock.Unlock();
        }
    }

    // Network information
    /* virtual */ string SystemInfo::Internet::PublicIPAddress() const
    {
        return (_ipAddress);
    }

    /* virtual */ PluginHost::ISubSystem::IInternet::network_type SystemInfo::Internet::NetworkType() const
    {
        return (_ipAddress.empty() == true ? PluginHost::ISubSystem::IInternet::UNKNOWN : (Core::NodeId::IsIPV6Enabled() ? PluginHost::ISubSystem::IInternet::IPV6 : PluginHost::ISubSystem::IInternet::IPV4));
    }

    bool SystemInfo::Internet::Set(const PluginHost::ISubSystem::IInternet* info)
    {
        return Set(info->PublicIPAddress());
    }

    // Security information
    /* virtual */ string SystemInfo::Security::Callsign() const
    {
        return (_callsign);
    }

    bool SystemInfo::Security::Set(const PluginHost::ISubSystem::ISecurity* info)
    {
        return Set(info->Callsign());
    }

    // Location Information
    /* virtual */ string SystemInfo::Location::TimeZone() const
    {
        return (_timeZone);
    }

    /* virtual */ string SystemInfo::Location::Country() const
    {
        return (_country);
    }

    /* virtual */ string SystemInfo::Location::Region() const
    {
        return (_region);
    }

    /* virtual */ string SystemInfo::Location::City() const
    {
        return (_city);
    }

    /* virtual */ int32_t SystemInfo::Location::Latitude() const
    {
        return (_latitude);
    }

    /* virtual */ int32_t SystemInfo::Location::Longitude() const
    {
        return (_longitude);
    }

    bool SystemInfo::Location::Set(const PluginHost::ISubSystem::ILocation* info)
    {
        return Set(info->TimeZone(), info->Country(), info->Region(), info->City(), info->Latitude(), info->Longitude());
    }

    // Device Identifier
    /* virtual */ uint8_t SystemInfo::Id::Identifier(const uint8_t length, uint8_t* buffer) const
    {
        uint8_t result = 0;

        if (_identifier != nullptr) {
            result = _identifier[0];
            ::memcpy(buffer, &(_identifier[1]), (result > length ? length : result));
        }

        return (result);
    }
    /* virtual */ string SystemInfo::Id::Architecture() const
    {
        return _architecture;
    }
        /* virtual */ string SystemInfo::Id::Chipset() const
    {
        return _chipset;
    }

    /* virtual */ string SystemInfo::Id::FirmwareVersion() const
    {
        return _firmwareVersion;
    }

    bool SystemInfo::Id::Set(const PluginHost::ISubSystem::IIdentifier* info)
    {
        uint8_t buffer[119];

        uint8_t length = info->Identifier(sizeof(buffer), buffer);

        return Set(length, buffer, info->Architecture(), info->Chipset(), info->FirmwareVersion());
    }

    // Time synchronisation
    /* virtual */ uint64_t SystemInfo::Time::TimeSync() const
    {
        return (_timeSync);
    }

    bool SystemInfo::Time::Set(const PluginHost::ISubSystem::ITime* info)
    {
        return Set(info->TimeSync());
    }

    // Software information
    string SystemInfo::BuildTreeHash() const /* override */ 
    {
        return (_T(EXPAND_AND_QUOTE(TREE_REFERENCE)));
    }

    string SystemInfo::Version() const /* override */
    {
        return (Core::Format(_T("%d.%d.%d"), PluginHost::Major, PluginHost::Minor, PluginHost::Patch));
    }

} //namspace Plugin
} // namespace WPEFramework
