#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IPackager : virtual public Core::IUnknown {
        enum { ID = ID_PACKAGER };

        enum state {
            IDLE,
            DOWNLOADING,
            DOWNLOADED,
            DECRYPTING,
            DECRYPTED,
            VERIFYING,
            VERIFIED,
            INSTALLING,
            INSTALLED
        };

        struct IInstallationInfo : virtual public Core::IUnknown {
            enum { ID = ID_PACKAGER_INSTALLATIONINFO };
            virtual state State() const = 0;
            virtual uint8_t Progress() const = 0;
            virtual uint32_t ErrorCode() const = 0;
            virtual uint32_t Abort() = 0;
        };

        struct IPackageInfo : virtual public Core::IUnknown {
            enum { ID = ID_PACKAGER_PACKAGEINFO };
            virtual string Name() const = 0;
            virtual string Version() const = 0;
            virtual string Architecture() const = 0;
        };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = ID_PACKAGER_NOTIFICATION };
            virtual void StateChange(IPackageInfo* package, IInstallationInfo* install) = 0;
            virtual void RepositorySynchronize(uint32_t status) = 0;
        };

        virtual void Register(INotification* observer) = 0;
        virtual void Unregister(const INotification* observer) = 0;
        virtual uint32_t Configure(PluginHost::IShell* service) = 0;
        virtual uint32_t Install(const string& name, const string& version, const string& arch) = 0;
        virtual uint32_t SynchronizeRepository() = 0;
    };
}
}
