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

#ifndef __CONTROLLER_SYSTEMINFO_H__
#define __CONTROLLER_SYSTEMINFO_H__

#include "Module.h"
#include "Config.h"

namespace WPEFramework {
namespace PluginHost {

    class SystemInfo : public PluginHost::ISubSystem {
    public:
        SystemInfo() = delete;
        SystemInfo(const SystemInfo&) = delete;
        SystemInfo& operator=(const SystemInfo&) = delete;

    private:
        class Id : public PluginHost::ISubSystem::IIdentifier {
        public:
            Id(const Id&) = delete;
            Id& operator=(const Id&) = delete;

            Id()
                : _identifier(nullptr)
            {
            }

            ~Id() override
            {
              delete [] _identifier;
            }

        public:
            BEGIN_INTERFACE_MAP(Id)
            INTERFACE_ENTRY(PluginHost::ISubSystem::IIdentifier)
            END_INTERFACE_MAP

        public:
            uint8_t Identifier(const uint8_t length, uint8_t buffer[]) const override;

            bool Set(const PluginHost::ISubSystem::IIdentifier* info);
            
            inline bool Set(const uint8_t length, const uint8_t buffer[],
                            const string& architecture,
                            const string& chipset,
                            const string& firmwareversion)
            {
                bool result(true);

                if (_identifier != nullptr) {
                    delete [] _identifier;
                }

                _identifier = new uint8_t[length + 2];

                ASSERT(_identifier != nullptr);

                ::memcpy(&(_identifier[1]), buffer, length);

                _identifier[0] = length;
                _identifier[length + 1] = '\0';

                if ((_architecture != architecture) || 
                    (_chipset != chipset) || 
                    (_firmwareVersion != firmwareversion))
                {
                    _architecture = architecture;
                    _chipset = chipset;
                    _firmwareVersion = firmwareversion;
                }

                return result;
            }

            inline string Identifier() const
            {
                return (_identifier != nullptr) ? Core::SystemInfo::Instance().Id(_identifier, ~0) : string();
            }

            string Architecture() const override;
            string Chipset() const override;
            string FirmwareVersion() const override;

        private:
            uint8_t* _identifier;
            string _architecture;
            string _chipset;
            string _firmwareVersion;
        };

        class Provisioning : public RPC::IteratorType<PluginHost::ISubSystem::IProvisioning> {
        public:
            Provisioning(const Provisioning&) = delete;
            Provisioning& operator=(const Provisioning&) = delete;

            Provisioning() = delete;

            Provisioning(PluginHost::ISubSystem::IProvisioning* info)
                : RPC::IteratorType<PluginHost::ISubSystem::IProvisioning>(info)
                , _storage(info->Storage())
            {
            }

            Provisioning(std::list<std::string>&& labels, const std::string& storage)
                : RPC::IteratorType<PluginHost::ISubSystem::IProvisioning>(labels)
                , _storage(storage)
            {
            }

            ~Provisioning() override = default;

            string Storage() const override
            {
                return _storage;
            }

        private:
            string _storage;
        };

        class Internet : public PluginHost::ISubSystem::IInternet {
        public:
            Internet(const Internet&) = delete;
            Internet& operator=(const Internet&) = delete;

            Internet()
                : _ipAddress()
            {
            }
            ~Internet() override = default;

        public:
            BEGIN_INTERFACE_MAP(Internet)
            INTERFACE_ENTRY(PluginHost::ISubSystem::IInternet)
            END_INTERFACE_MAP

        public:
            string PublicIPAddress() const override;
            PluginHost::ISubSystem::IInternet::network_type NetworkType() const override;

            bool Set(const PluginHost::ISubSystem::IInternet* info);
            inline bool Set(const string& ip)
            {
                bool result(false);
                if (_ipAddress != ip) {
                    _ipAddress = ip;

                    result = true;
                }
                return result;
            }

        private:
            string _ipAddress;
        };

        class Security : public PluginHost::ISubSystem::ISecurity {
        public:
            Security(const Security&) = delete;
            Security& operator=(const Security&) = delete;

            Security()
                : _callsign()
            {
            }
            ~Security() override = default;

        public:
            BEGIN_INTERFACE_MAP(Security)
            INTERFACE_ENTRY(PluginHost::ISubSystem::ISecurity)
            END_INTERFACE_MAP

        public:
            string Callsign() const override;

            bool Set(const PluginHost::ISubSystem::ISecurity* info);
            inline bool Set(const string& callsign)
            {
                bool result(false);
                if (_callsign != callsign) {
                    _callsign = callsign;

                    result = true;
                }
                return result;
            }

        private:
            string _callsign;
        };

        class Location : public PluginHost::ISubSystem::ILocation {
        public:
            Location(const Location&) = delete;
            Location& operator=(const Location&) = delete;

            Location()
                : _timeZone()
                , _country()
                , _region()
                , _city()
                , _latitude(51977956)
                , _longitude(5726384)
            {
            }
            ~Location() override = default;

        public:
            BEGIN_INTERFACE_MAP(Location)
            INTERFACE_ENTRY(PluginHost::ISubSystem::ILocation)
            END_INTERFACE_MAP

        public:
            string TimeZone() const override;
            string Country() const override;
            string Region() const override;
            string City() const override;
            int32_t Latitude() const override;
            int32_t Longitude() const override;

            bool Set(const PluginHost::ISubSystem::ILocation* info);
            inline bool Set(const string& timeZone,
                const string& country,
                const string& region,
                const string& city,
                const int32_t latitude,
                const int32_t longitude)
            {
                bool result(false);

                if (_timeZone  != timeZone || 
                    _country   != country  || 
                    _region    != region   || 
                    _city      != city     || 
                    _latitude  != latitude || 
                    _longitude != longitude) {

                    _timeZone = timeZone;
                    _country = country;
                    _region = region;
                    _city = city;
                    _latitude = latitude;
                    _longitude = longitude;

                    result = true;
                }

                return result;
            }

        private:
            string _timeZone;
            string _country;
            string _region;
            string _city;
            int32_t _latitude; // 1.000.000 divider
            int32_t _longitude; // 1.000.000 divider
        };

        class Time : public PluginHost::ISubSystem::ITime {
        public:
            Time(const Time&) = delete;
            Time& operator=(const Time&) = delete;

            Time()
                : _timeSync()
            {
            }
            ~Time() override = default;

        public:
            BEGIN_INTERFACE_MAP(Time)
            INTERFACE_ENTRY(PluginHost::ISubSystem::ITime)
            END_INTERFACE_MAP

        public:
            uint64_t TimeSync() const;

            bool Set(const PluginHost::ISubSystem::ITime* info);
            inline bool Set(const uint64_t ticks)
            {
                bool result(false);

                if (_timeSync != ticks) {
                    _timeSync = ticks;

                    result = true;
                }

                return result;
            }

        private:
            uint64_t _timeSync;
        };

    public:
        SystemInfo(const Config& config, Core::IDispatch* callback);
        ~SystemInfo() override;

    public:
        void Register(PluginHost::ISubSystem::INotification* notification) override;
        void Unregister(PluginHost::ISubSystem::INotification* notification) override;

        string SecurityCallsign() const
        {
            string result;

            _adminLock.Lock();

            if (_security != nullptr) {
                result = _security->Callsign();
            }

            _adminLock.Unlock();

            return (result);
        }

        // Event methods
        void Set(const subsystem type, Core::IUnknown* information) override
        {
            bool sendUpdate(type < NEGATIVE_START ? IsActive(type) == false : IsActive(static_cast<subsystem>(type - NEGATIVE_START)) == true);

            switch (type) {
            case PLATFORM: {
                SYSLOG(Logging::Startup, (_T("EVENT: Platform")));
                break;
            }
            case NOT_PLATFORM: {
                /* Clearing the flag does not require information */
                SYSLOG(Logging::Shutdown, (_T("EVENT: Platform")));
                break;
            }
            case NETWORK: {
                SYSLOG(Logging::Startup, (_T("EVENT: Network")));
                break;
            }
            case NOT_NETWORK: {
                /* Clearing the flag does not require information */
                SYSLOG(Logging::Shutdown, (_T("EVENT: Network")));
                break;
            }
            case IDENTIFIER: {
                PluginHost::ISubSystem::IIdentifier* info = (information != nullptr ? information->QueryInterface<PluginHost::ISubSystem::IIdentifier>() : nullptr);

                if (info == nullptr) {

                    _adminLock.Lock();

                    if (_identifier != nullptr) {
                        _identifier->Release();
                    }

                    _identifier = Core::Service<Id>::Create<Id>();
                    const uint8_t* id(RawDeviceId(_config.EthernetCard()));
                    _identifier->Set(id[0], &id[1], 
                            Core::SystemInfo::Instance().Architecture(), 
                            Core::SystemInfo::Instance().Chipset(), 
                            Core::SystemInfo::Instance().FirmwareVersion()
                    );

                    _adminLock.Unlock();
                } else {
                    Id* id = Core::Service<Id>::Create<Id>();
                    sendUpdate = id->Set(info) || sendUpdate;

                    info->Release();

                    _adminLock.Lock();

                    if (_identifier != nullptr) {
                        _identifier->Release();
                    }

                    _identifier = id;
                    _adminLock.Unlock();
                }

                SYSLOG(Logging::Startup, (_T("EVENT: Identifier: %s"), _identifier->Identifier().c_str()));
                SYSLOG(Logging::Startup, (_T("EVENT: Architecture: %s"), _identifier->Architecture().c_str()));
                SYSLOG(Logging::Startup, (_T("EVENT: Chipset: %s"), _identifier->Chipset().c_str()));
                SYSLOG(Logging::Startup, (_T("EVENT: FirmwareVersion: %s"), _identifier->FirmwareVersion().c_str()));
                break;
            }
            case NOT_IDENTIFIER: {
                /* Clearing the flag does not require information */
                SYSLOG(Logging::Shutdown, (_T("EVENT: Identifier")));
                break;
            }
            case INTERNET: {
                PluginHost::ISubSystem::IInternet* info = (information != nullptr ? information->QueryInterface<PluginHost::ISubSystem::IInternet>() : nullptr);

                if (info == nullptr) {

                    _adminLock.Lock();

                    if (_internet != nullptr) {
                        _internet->Release();
                    }

                    _internet = Core::Service<Internet>::Create<Internet>();
                    _internet->Set(_T("127.0.0.1"));
                    _adminLock.Unlock();

                } else {
                    Internet* internet = Core::Service<Internet>::Create<Internet>();
                    sendUpdate = internet->Set(info) || sendUpdate;

                    info->Release();

                    _adminLock.Lock();

                    if (_internet != nullptr) {
                        _internet->Release();
                    }

                    _internet = internet;
                    _adminLock.Unlock();
                }

                SYSLOG(Logging::Startup, (_T("EVENT: Internet [%s]"), _internet->PublicIPAddress().c_str()));
                break;
            }
            case NOT_INTERNET: {
                /* Clearing the flag does not require information */
                SYSLOG(Logging::Shutdown, (_T("EVENT: Internet")));
                break;
            }
            case LOCATION: {
                PluginHost::ISubSystem::ILocation* info = (information != nullptr ? information->QueryInterface<PluginHost::ISubSystem::ILocation>() : nullptr);

                Location* location = Core::Service<Location>::Create<Location>();

                if (info != nullptr) {
                    sendUpdate = location->Set(info) || sendUpdate;
                    info->Release();
                }

                _adminLock.Lock();

                if (_location != nullptr) {
                    _location->Release();
                }

                _location = location;
                _adminLock.Unlock();

                SYSLOG(Logging::Startup, (_T("EVENT: TimeZone: %s, Country: %s, Region: %s, City: %s"), _location->TimeZone().c_str(), _location->Country().c_str(), _location->Region().c_str(), _location->City().c_str()));

                break;
            }
            case NOT_LOCATION: {
                /* Clearing the flag does not require information */
                SYSLOG(Logging::Shutdown, (_T("EVENT: Location")));
                break;
            }
            case TIME: {
                PluginHost::ISubSystem::ITime* info = (information != nullptr ? information->QueryInterface<PluginHost::ISubSystem::ITime>() : nullptr);

                if (info == nullptr) {

                    _adminLock.Lock();

                    if (_time != nullptr) {
                        _time->Release();
                    }

                    _time = Core::Service<Time>::Create<Time>();
                    _time->Set(Core::Time::Now().Ticks());

                    _adminLock.Unlock();
                } else {
                    Time* time = Core::Service<Time>::Create<Time>();
                    sendUpdate = time->Set(info) || sendUpdate;

                    info->Release();

                    _adminLock.Lock();

                    if (_time != nullptr) {
                        _time->Release();
                    }

                    _time = time;
                    _adminLock.Unlock();
                }

                SYSLOG(Logging::Startup, (_T("EVENT: Time: %s"), Core::Time(_time->TimeSync()).ToRFC1123(false).c_str()));
                break;
            }
            case NOT_TIME: {
                /* Clearing the flag does not require information */
                SYSLOG(Logging::Shutdown, (_T("EVENT: Time")));
                break;
            }
            case PROVISIONING: {
                PluginHost::ISubSystem::IProvisioning* info = (information != nullptr) ? information->QueryInterface<PluginHost::ISubSystem::IProvisioning>() : nullptr;

                if (info == nullptr) {
                    _adminLock.Lock();
                    
                    if (_provisioning != nullptr) {
                        _provisioning->Release();
                        _provisioning = nullptr;
                    }

                    _provisioning = Core::Service<Provisioning>::Create<PluginHost::ISubSystem::IProvisioning>(std::move(std::list<std::string>()), "");

                    _adminLock.Unlock();
                } else {
                    _adminLock.Lock();
                    
                    if (_provisioning != nullptr) {
                        _provisioning->Release();
                    }

                    _provisioning = Core::Service<Provisioning>::Create<PluginHost::ISubSystem::IProvisioning>(info);

                    _adminLock.Unlock();
                    
                    info->Release();
                }
                
                /* No information to set yet */
                SYSLOG(Logging::Startup, (_T("EVENT: Provisioning")));
                break;
            }
            case NOT_PROVISIONING: {
                /* No information to set yet */
                SYSLOG(Logging::Shutdown, (_T("EVENT: Provisioning")));
                break;
            }
            case DECRYPTION: {
                /* No information to set yet */
                SYSLOG(Logging::Startup, (_T("EVENT: Decryption")));
                break;
            }
            case NOT_DECRYPTION: {
                /* No information to set yet */
                SYSLOG(Logging::Shutdown, (_T("EVENT: Decryption")));
                break;
            }
            case GRAPHICS: {
                /* No information to set yet */
                SYSLOG(Logging::Startup, (_T("EVENT: Graphics")));
                break;
            }
            case NOT_GRAPHICS: {
                /* No information to set yet */
                SYSLOG(Logging::Shutdown, (_T("EVENT: Graphics")));
                break;
            }
            case WEBSOURCE: {
                /* No information to set yet */
                SYSLOG(Logging::Startup, (_T("EVENT: WebSource")));
                break;
            }
            case NOT_WEBSOURCE: {
                /* No information to set yet */
                SYSLOG(Logging::Shutdown, (_T("EVENT: WebSource")));
                break;
            }
            case STREAMING: {
                /* No information to set yet */
                SYSLOG(Logging::Startup, (_T("EVENT: Streaming")));
                break;
            }
            case NOT_STREAMING: {
                /* No information to set yet */
                SYSLOG(Logging::Shutdown, (_T("EVENT: Streaming")));
                break;
            }
            case BLUETOOTH: {
                /* No information to set yet */
                SYSLOG(Logging::Startup, (_T("EVENT: Bluetooth")));
                break;
            }
            case NOT_BLUETOOTH: {
                /* No information to set yet */
                SYSLOG(Logging::Shutdown, (_T("EVENT: Bluetooth")));
                break;
            }
            case SECURITY: {
                PluginHost::ISubSystem::ISecurity* info = (information != nullptr ? information->QueryInterface<PluginHost::ISubSystem::ISecurity>() : nullptr);

                if (info == nullptr) {

                    _adminLock.Lock();

                    if (_security != nullptr) {
                        _security->Release();
                    }

                    _security = Core::Service<Security>::Create<Security>();
                    _security->Set(_T(""));

                    _adminLock.Unlock();
                } else {
                    Security* security = Core::Service<Security>::Create<Security>();
                    sendUpdate = security->Set(info) || sendUpdate;

                    info->Release();

                    _adminLock.Lock();

                    if (_security != nullptr) {
                        _security->Release();
                    }

                    _security = security;
                    _adminLock.Unlock();
                }

                SYSLOG(Logging::Startup, (_T("EVENT: Security")));
                break;
            }
            case NOT_SECURITY: {
                /* No information to set yet */
                SYSLOG(Logging::Shutdown, (_T("EVENT: Security")));
                break;
            }

            default: {
                ASSERT(false && "Unknown Event");
            }
            }

            if (sendUpdate == true) {

                _adminLock.Lock();

                if (type >= NEGATIVE_START) {
                    _flags &= ~(1 << (type - NEGATIVE_START));
                } else {
                    _flags |= (1 << type);
                }

                _adminLock.Unlock();

                Update();
            }
        }
        const Core::IUnknown* Get(const subsystem type) const override
        {
            const Core::IUnknown* result(nullptr);

            _adminLock.Lock();

            if ((type < END_LIST) && (IsActive(type) == true)) {

                switch (type) {
                case NETWORK: {
                    /* No information to get yet */
                    break;
                }
                case IDENTIFIER: {
                    result = _identifier;
                    break;
                }
                case INTERNET: {
                    result = _internet;
                    break;
                }
                case LOCATION: {
                    result = _location;
                    break;
                }
                case TIME: {
                    result = _time;
                    break;
                }
                case PROVISIONING: {
                    result = _provisioning;
                    break;
                }
                case DECRYPTION: {
                    /* No information to get yet */
                    break;
                }
                case GRAPHICS: {
                    /* No information to get yet */
                    break;
                }
                case WEBSOURCE: {
                    /* No information to get yet */
                    break;
                }
                default: {
                    ASSERT(false && "Unknown Event");
                }
                }

                if (result != nullptr) {
                    result->AddRef();
                }
            }

            _adminLock.Unlock();

            return result;
        }
        bool IsActive(const subsystem type) const override
        {
            return ((type < END_LIST) && ((_flags & (1 << type)) != 0));
        };
        inline uint32_t Value() const
        {
            return (_flags);
        }

        string BuildTreeHash() const;
        string Version() const;

        BEGIN_INTERFACE_MAP(SystemInfo)
        INTERFACE_ENTRY(PluginHost::ISubSystem)
        END_INTERFACE_MAP

    private:
        // First byte of the RawDeviceId is the length of the DeviceId to follow.
        const uint8_t* RawDeviceId(const string& interfaceName) const;

        typedef Core::IteratorType<std::list<PluginHost::ISubSystem::INotification*>, PluginHost::ISubSystem::INotification*> ClientIterator;

        void RecursiveList(ClientIterator& index);
        void Update();

    private:
        mutable Core::CriticalSection _adminLock;
        const Config& _config;
        std::list<PluginHost::ISubSystem::INotification*> _notificationClients;
        Core::IDispatch* _callback;
        Id* _identifier;
        Location* _location;
        Internet* _internet;
        Security* _security;
        Time* _time;
        IProvisioning* _provisioning;
        uint32_t _flags;
    };
}
} // namespace PluginHost

#endif
