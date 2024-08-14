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

#include "PluginServer.h"

namespace Thunder {

namespace PluginHost {

/* static */ void Server::PostMortem(Service& service, const IShell::reason why, RPC::IRemoteConnection* connection) {

    if (service.PostMortemAllowed(why) == true) {
        // See if this is an external process on which we could trigger a PostMortem log generation
        if (connection == nullptr) {
            // It is a plugin in the Thunder Process space, the current PostMortem Dump on the process
            // has the side-effect of killing the full process. Lets not do thet for internal Plugins :-)
            // Just log all the information we have from /proc
            Logging::DumpSystemFiles(0);
        }
        else {
            // Time to flush all the "System information", before we send signals. This too keep 
            // those figures as close to the moment of hange detect.
            Logging::DumpSystemFiles(static_cast<Core::process_t>(connection->RemoteId()));
        
            connection->PostMortem();
        }
    }
}

} } // Thunder::PluginHost
