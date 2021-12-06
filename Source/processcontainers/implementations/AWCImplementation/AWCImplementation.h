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

#include "processcontainers/ProcessContainer.h"
#include "processcontainers/common/BaseAdministrator.h"
#include "processcontainers/common/BaseRefCount.h"
#include "Module.h"
#include "Tracing.h"
#include <cctype>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <AWCClient.h>

namespace WPEFramework {
namespace ProcessContainers {

    class AWCStateChangeListener {
    public:
        virtual void notifyStateChange(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid) = 0;
    };

    class AWCStateChangeNotifier {
    public:
        AWCStateChangeNotifier();
        void addListener(AWCStateChangeListener * listener);
        void removeListener(AWCStateChangeListener * listener);
        void notify(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid);

    private:
        std::vector<AWCStateChangeListener *> _listeners;
        std::mutex _mutex;
    };

    class AWCContainer : public BaseRefCount<IContainer>, AWCStateChangeListener {

    private:
        friend class AWCContainerAdministrator;

        AWCContainer(const string& name, awc::AWCClient * client, AWCStateChangeNotifier * notifier, const string& containerLogDir, const string& configuration);

    public:
        AWCContainer(const AWCContainer&) = delete;
        ~AWCContainer() override;

        AWCContainer& operator=(const AWCContainer&) = delete;

        const string& Id() const override;
        uint32_t Pid() const override;
        IMemoryInfo* Memory() const override;
        IProcessorInfo* ProcessorInfo() const override;
        INetworkInterfaceIterator* NetworkInterfaces() const override;
        bool IsRunning() const override;

        bool Start(const string& command, ProcessContainers::IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/) override;

        void AddRef() const override;
        uint32_t Release() const override;
        void notifyStateChange(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid) override;

    private:
        const string _name;
        uint32_t _pid;
        int _runId;
        awc::awc_app_state_t _appState;
        mutable uint32_t _referenceCount;
        awc::AWCClient * _client;
        AWCStateChangeNotifier * _notifier;
        mutable std::mutex _mutex;
        std::condition_variable _cv;
        bool _waitForResponse;
        static std::string const _envVariables[];
    };

    class AWCContainerAdministrator : public BaseContainerAdministrator<AWCContainer>, public AWCStateChangeNotifier {
        friend class AWCContainer;
        friend class Core::SingletonType<AWCContainerAdministrator>;

    private:
        awc::AWCClient * _client;
    private:
        AWCContainerAdministrator();

    public:
        AWCContainerAdministrator(const AWCContainerAdministrator&) = delete;
        AWCContainerAdministrator& operator=(const AWCContainerAdministrator&) = delete;
        ~AWCContainerAdministrator() override;
        ProcessContainers::IContainer* Container(const string& name, IStringIterator& searchpaths, const string& containerLogDir, const string& configuration) override;
        void Logging(const string& globalLogDir, const string& loggingOptions) override;

        class AWCListener : public awc::AWCClient::Listener
        {
        public:
            AWCListener(AWCStateChangeNotifier * notifier);
            ~AWCListener() {};
            void notifyWindowChange(int window_id, awc::AWCClient::awc_window_state_t window_state, unsigned int pid);
            void notifyStateChange(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid);
        private:
            AWCStateChangeNotifier * _notifier;
        };
    };
}
}
