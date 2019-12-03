#pragma once

#include "Module.h"
#include "Tracing.h"
#include <lxc/lxccontainer.h>
#include <vector>
#include <utility>
#include <thread>
#include <cctype>
#include "../../ProcessContainer.h"

namespace WPEFramework {
namespace ProcessContainers {
    using LxcContainerType = struct lxc_container;

    class LXCContainer : public IContainer {
    private:

        class Config : public Core::JSON::Container {
        public:
            class ConfigItem : public Core::JSON::Container {
            public:
                ConfigItem& operator=(const ConfigItem&) = delete;

                ConfigItem(const ConfigItem& rhs) 
                    : Core::JSON::Container()
                    , Key(rhs.Key)
                    , Value(rhs.Value)
                {
                    Add(_T("key"), &Key);
                    Add(_T("value"), &Value); 
                }

                ConfigItem()
                    : Core::JSON::Container()
                    , Key()
                    , Value()
                {
                    Add(_T("key"), &Key);
                    Add(_T("value"), &Value); 
                }
                
                ~ConfigItem() = default;

                Core::JSON::String Key;
                Core::JSON::String Value;
            };
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , ConsoleLogging("0")
                , ConfigItems()
        #ifdef __DEBUG__
                , Attach(false)
        #endif
            {
                Add(_T("console"), &ConsoleLogging); // should be a power of 2 when converted to bytes. Valid size prefixes are 'KB', 'MB', 'GB', 0 is off, auto is auto determined

                Add(_T("items"), &ConfigItems);

#ifdef __DEBUG__
                Add(_T("attach"), &Attach);
#endif
            }
            
            ~Config() = default;

            Core::JSON::String ConsoleLogging;
            Core::JSON::ArrayType<ConfigItem> ConfigItems;
#ifdef __DEBUG__
            Core::JSON::Boolean Attach;
#endif
        };

    public:

        LXCContainer(const LXCContainer&) = delete;
        LXCContainer& operator=(const LXCContainer&) = delete;

        LXCContainer(const string& name, LxcContainerType* lxccontainer, const string& logpath, const string& configuration, const string& lxcpath);

        const string Id() const override;
        pid_t Pid() const override;
        MemoryInfo Memory() const override;
        CPUInfo Cpu() const override;
        string ConfigPath() const override;
        string LogPath() const override;
        ProcessContainers::IConstStringIterator NetworkInterfaces() const override;
        std::vector<Core::NodeId> IPs(const string& interface) const override;
        bool IsRunning() const override;

        bool Start(const string& command, ProcessContainers::IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/) override;

        void AddRef() const override;
        uint32_t Release() const override;

        private:
            inline std::vector<string> GetNetworkInterfaces();

        private:
            const string _name;
            pid_t _pid;
            string _lxcpath;
            string _logpath;
            mutable Core::CriticalSection _adminLock;
            mutable uint32_t _referenceCount;
            LxcContainerType* _lxccontainer;
            std::vector<string> _networkInterfaces;
            #ifdef __DEBUG__
                bool _attach;
            #endif
    };

    class LXCContainerAdministrator : public ProcessContainers::IContainerAdministrator {
    public:
        friend class LXCContainer;

    private:
        static constexpr char* logFileName = "lxclogging.log";
        static constexpr char* configFileName = "config";
        static constexpr uint32_t maxReadSize = 32 * (1 << 10); // 32KiB
    public:
        LXCContainerAdministrator(const LXCContainerAdministrator&) = delete;
        LXCContainerAdministrator& operator=(const LXCContainerAdministrator&) = delete;

        LXCContainerAdministrator();
        virtual ~LXCContainerAdministrator();

        // Lifetime management
        void AddRef() const override;
        uint32_t Release() const override;

        ProcessContainers::IContainer* Container(const string& name, 
                                                                        ProcessContainers::IStringIterator& searchpaths, 
                                                                        const string& logpath,
                                                                        const string& configuration) override;

        void Logging(const string& logpath, const string& logid, const string& logging) override;
        ContainerIterator Containers() override;


    private:
        mutable Core::CriticalSection _lock;
        std::list<IContainer*> _containers;
    };

}
}