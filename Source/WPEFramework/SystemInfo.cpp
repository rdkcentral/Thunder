#include "SystemInfo.h"
namespace WPEFramework {
namespace PluginHost {


#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
    SystemInfo::SystemInfo(Core::IDispatch* callback)
        : _adminLock()
        , _notificationClients()
        , _callback(callback)
        , _identifier(nullptr)
        , _location(nullptr)
        , _internet(nullptr)
        , _time(nullptr)
        , _flags(0)
    {
        ASSERT(callback != nullptr);
   }
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif

    /* virtual */ SystemInfo::~SystemInfo()
    {
    }

    void SystemInfo::Register(PluginHost::ISubSystem::INotification* notification)
    {
        _adminLock.Lock();

        ASSERT(std::find(_notificationClients.begin(), _notificationClients.end(), notification) == _notificationClients.end());

        _notificationClients.push_back(notification);

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
        } else {
            TRACE_L1("Notification(%p) not found.", notification);
        }

        _adminLock.Unlock();
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
        }
        else {
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

    bool SystemInfo::Location::Set(const PluginHost::ISubSystem::ILocation* info)
    {
        return Set(info->TimeZone(), info->Country(), info->Region(), info->City());
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

    bool SystemInfo::Id::Set(const PluginHost::ISubSystem::IIdentifier* info)
    {

        uint8_t buffer[119];

        uint8_t length = info->Identifier(sizeof(buffer), buffer);

        return Set(length, buffer);
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
    /* virtual */ string SystemInfo::BuildTreeHash() const
    {
        return (_T(EXPAND_AND_QUOTE(TREE_REFERENCE)));
    }
} //namspace Plugin
} // namespace WPEFramework
