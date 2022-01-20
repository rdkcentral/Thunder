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

namespace WPEFramework {

namespace PluginHost {
    struct IShell;
}

namespace Connector {

    class EXTERNAL Connector {
    
    protected:
        Connector()
            : _controller(nullptr)
        {
        }
    public:
        Connector(const Connector&) = delete;
        Connector& operator=(const Connector&) = delete;
        ~Connector();
        static Connector& Instance();

        void Announce(PluginHost::IShell* controller);
        void Revoke(PluginHost::IShell* controller);
        PluginHost::IShell* Controller();
  
    private:
        PluginHost::IShell* _controller;
    };

}//namespace Connector
}//namespace WPEFramework

extern "C" {
    EXTERNAL void connector_announce(WPEFramework::PluginHost::IShell *);
    EXTERNAL void connector_revoke(WPEFramework::PluginHost::IShell *);

    EXTERNAL WPEFramework::PluginHost::IShell* connector_controller();
}
