/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IPackager : virtual public Core::IUnknown {
        enum { ID = ID_PACKAGER };

        enum state : uint8_t {
            IDLE,
            DOWNLOADING,
            DOWNLOADED,
            DECRYPTING,
            DECRYPTED,
            VERIFYING,
            VERIFIED,
            INSTALLING,
            INSTALLED,

            DOWNLOAD_FAILED,
            DECRYPTION_FAILED,
            EXTRACTION_FAILED,
            VERIFICATION_FAILED,
            INSTALL_FAILED,
            REMOVE_FAILED
        };

        struct EXTERNAL IInstallationInfo : virtual public Core::IUnknown {
            enum { ID = ID_PACKAGER_INSTALLATIONINFO };
            virtual state State() const = 0;
            virtual uint8_t Progress() const = 0;
            virtual uint32_t ErrorCode() const = 0;
            virtual uint32_t Abort() = 0;
        };

        struct EXTERNAL IPackageInfo : virtual public Core::IUnknown {
            enum { ID = ID_PACKAGER_PACKAGEINFO };
            virtual string Name() const = 0;
            virtual string Version() const = 0;
            virtual string Architecture() const = 0;
        };

        struct EXTERNAL INotification : virtual public Core::IUnknown {
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
