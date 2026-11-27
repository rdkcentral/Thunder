/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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
#include "NodeId.h"
#include "Portability.h"
#include "Errors.h"

#ifdef __POSIX__
#include <grp.h>
#include <pwd.h>
#endif

namespace Thunder {
namespace Core {

    struct AccessControl {
    private:
#ifdef __POSIX__
        static uint32_t ResolveOwnership(const string& userName, const string& groupName, uid_t& uid, gid_t& gid)
        {
            uint32_t result(Core::ERROR_NONE);

            uid = static_cast<uid_t>(-1);
            gid = static_cast<gid_t>(-1);

            if (userName.empty() == false) {
                const struct passwd* pwd = ::getpwnam(userName.c_str());

                if (pwd != nullptr) {
                    uid = pwd->pw_uid;
                } else {
                    result = Core::ERROR_NOT_EXIST;
                }
            }

            if (groupName.empty() == false) {
                const struct group* grp = ::getgrnam(groupName.c_str());

                if (grp != nullptr) {
                    gid = grp->gr_gid;
                } else {
                    result = Core::ERROR_NOT_EXIST;
                }
            }

            return result;
        }
#endif

    public:
        static uint32_t OwnerShip(const string& node, const string& userName, const string& groupName)
        {
            uint32_t result(Core::ERROR_NONE);

#ifdef __POSIX__
            uid_t uid;
            gid_t gid;

            result = ResolveOwnership(userName, groupName, uid, gid);
            if (::chown(node.c_str(), uid, gid) != 0) {
                result = Core::ERROR_GENERAL;
            }
#else
            result = Core::ERROR_NOT_SUPPORTED;
#endif
            return result;
        }

#ifdef __POSIX__
        static uint32_t OwnerShip(const int descriptor, const string& userName, const string& groupName)
        {
            uint32_t result(Core::ERROR_NONE);

            uid_t uid;
            gid_t gid;

            result = ResolveOwnership(userName, groupName, uid, gid);
            if ((result == Core::ERROR_NONE) && (::fchown(descriptor, uid, gid) != 0)) {
                result = Core::ERROR_GENERAL;
            }
            return result;
        }
#endif
    
        static uint32_t Permission(const string& node, const uint16_t modeFlags){
            uint32_t result(Core::ERROR_NONE);

#ifdef __POSIX__
            // Lucky us we mapped File::Mode 1:1 to POSIX mode, 
            // so we only have to filter some values :-). 
            mode_t mode = modeFlags & 0xFFFF;

            if (::chmod(node.c_str(), mode) != 0) {
                result = Core::ERROR_GENERAL;
            }
#else
            result = Core::ERROR_NOT_SUPPORTED;
#endif
            return result;
        }

        static uint32_t Apply(const Core::NodeId& node)
        {
            uint32_t status(Core::ERROR_NONE);

            if (node.Rights() <= 0777) {
                status = Permission(node.HostName(), node.Rights());
            }
            if (node.Group().empty() != true) {
                status = OwnerShip(node.HostName(), "", node.Group());
            }
            return status;
        }

    };
}
} // namespace Core
