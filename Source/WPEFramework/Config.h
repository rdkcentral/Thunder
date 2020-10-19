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

namespace PluginHost {

    class EXTERNAL Config {
    private:
        class Substituter {
        private:
            static constexpr TCHAR Delimeter = _T('%');
            typedef string (*ValueHandler)(const Config& config, const Plugin::Config* info);
            typedef std::map<string, ValueHandler> VariableMap;

        public:
            Substituter() = delete;
            Substituter(const Substituter&) = delete;
            Substituter& operator=(const Substituter&) = delete;

            Substituter(const Config& parent)
                : _parent(parent)
            {
                _variables.insert(std::make_pair("datapath", [](const Config& config, const Plugin::Config* info) { 
                    return (info == nullptr ? config.DataPath() : info->DataPath(config.DataPath()));
                }));
                _variables.insert(std::make_pair("persistentpath", [](const Config& config, const Plugin::Config* info) { 
                    return (info == nullptr ? config.PersistentPath() : info->PersistentPath(config.PersistentPath()));
                }));
                _variables.insert(std::make_pair("systempath", [](const Config& config, const Plugin::Config*) {
                    return (config.SystemPath());
                }));
                _variables.insert(std::make_pair("volatilepath", [](const Config& config, const Plugin::Config* info) {
                    return (info == nullptr ? config.VolatilePath() : info->VolatilePath(config.VolatilePath()));
                }));
                _variables.insert(std::make_pair("proxystubpath", [](const Config& config, const Plugin::Config*) {
                    return (config.ProxyStubPath());
                }));
                _variables.insert(std::make_pair("postmortempath", [](const Config& config, const Plugin::Config*) {
                    return (config.PostMortemPath());
                }));
            }
            ~Substituter()
            {
            }

        public:
            string Substitute(const string& input, const Plugin::Config* plugin) const
            {
                std::size_t begin = 0;
                string value = input;
                while (begin != std::string::npos) {
                    begin = value.find_first_of(Delimeter, begin);
                    if (begin != std::string::npos) {
                        std::size_t end = value.find_first_of(Delimeter, begin + 1);
                        if (end != std::string::npos) {
                            string variable(value, begin + 1, end - (begin + 1));
                            if (variable.empty() != true) {
                                VariableMap::const_iterator index = _variables.find(variable);
                                if (index != _variables.end()) {
                                    string replacement (index->second(_parent, plugin));
                                    value.replace(begin, (end + 1) - begin, replacement);
                                    end = begin + replacement.length();
                                } else {
                                    value.clear();
                                    TRACE_L1("%s variable is not found in the Variable lookup table\n", variable.c_str());
                                    break;
                                }
                                begin = end;
                            } else {
                                value.clear();
                                TRACE_L1("variable is not given between the delimeter:[%c]\n", Delimeter);
                                break;
                            }
                        } else {
                            value.clear();
                            TRACE_L1("There is no closing delimeter-%c for the value:%s\n", Delimeter, value.c_str());
                            break;
                        }
                    }
                }
                return value;
            }

        private:
            const Config& _parent;
            VariableMap _variables;
        };

        // Configuration to get a server (PluginHost server) up and running.
        class JSONConfig : public Core::JSON::Container {
        public:
            class Environment : public Core::JSON::Container {
            public:
                Environment()
                    : Core::JSON::Container()
                    , Key()
                    , Value()
                    , Override(false)
                {
                    Add(_T("key"), &Key);
                    Add(_T("value"), &Value);
                    Add(_T("override"), &Override);
                }
                Environment(const Environment& copy)
                    : Core::JSON::Container()
                    , Key(copy.Key)
                    , Value(copy.Value)
                    , Override(copy.Override)
                {
                    Add(_T("key"), &Key);
                    Add(_T("value"), &Value);
                    Add(_T("override"), &Override);
                }
                virtual ~Environment()
                {
                }
                Environment& operator=(const Environment& RHS)
                {
                    Key = RHS.Key;
                    Value = RHS.Value;
                    Override = RHS.Override;

                    return (*this);
                }

            public:
                Core::JSON::String Key;
                Core::JSON::String Value;
                Core::JSON::Boolean Override;
            };

            class ProcessSet : public Core::JSON::Container {
            public:
                ProcessSet()
                    : Core::JSON::Container()
                    , User()
                    , Group()
                    , Priority(0)
                    , OOMAdjust(0)
                    , Policy()
                    , StackSize(0)
                    , Umask(1)
                {
                    Add(_T("user"), &User);
                    Add(_T("group"), &Group);
                    Add(_T("priority"), &Priority);
                    Add(_T("policy"), &Policy);
                    Add(_T("oomadjust"), &OOMAdjust);
                    Add(_T("stacksize"), &StackSize);
                    Add(_T("umask"), &Umask);
                }
                ProcessSet(const ProcessSet& copy)
                    : Core::JSON::Container()
                    , User(copy.User)
                    , Group(copy.Group)
                    , Priority(copy.Priority)
                    , OOMAdjust(copy.OOMAdjust)
                    , Policy(copy.Policy)
                    , StackSize(copy.StackSize)
                    , Umask(copy.Umask)
                {
                    Add(_T("user"), &User);
                    Add(_T("group"), &Group);
                    Add(_T("priority"), &Priority);
                    Add(_T("policy"), &Policy);
                    Add(_T("oomadjust"), &OOMAdjust);
                    Add(_T("stacksize"), &StackSize);
                    Add(_T("umask"), &Umask);
                }
                ~ProcessSet() override = default;

                ProcessSet& operator=(const ProcessSet& RHS)
                {
                    User = RHS.User;
                    Group = RHS.Group;
                    Priority = RHS.Priority;
                    Policy = RHS.Policy;
                    OOMAdjust = RHS.OOMAdjust;
                    StackSize = RHS.StackSize;
                    Umask = RHS.Umask;

                    return (*this);
                }

                Core::JSON::String User;
                Core::JSON::String Group;
                Core::JSON::DecSInt8 Priority;
                Core::JSON::DecSInt8 OOMAdjust;
                Core::JSON::EnumType<Core::ProcessInfo::scheduler> Policy;
                Core::JSON::DecUInt32 StackSize;
                Core::JSON::DecUInt16 Umask;
            };

            class InputConfig : public Core::JSON::Container {
            public:
                InputConfig()
#ifdef __WINDOWS__
                    : Locator("127.0.0.1:9631")
                    , Type(InputHandler::VIRTUAL)
#else
                    : Locator("/tmp/keyhandler|0760")
                    , Type(InputHandler::VIRTUAL)
#endif
                    , OutputEnabled(true)
                {

                    Add(_T("locator"), &Locator);
                    Add(_T("type"), &Type);
                    Add(_T("output"), &OutputEnabled);
                }
                InputConfig(const InputConfig& copy)
                    : Locator(copy.Locator)
                    , Type(copy.Type)
                    , OutputEnabled(copy.OutputEnabled)
                {
                    Add(_T("locator"), &Locator);
                    Add(_T("type"), &Type);
                    Add(_T("output"), &OutputEnabled);
                }
                ~InputConfig() override = default;

                InputConfig& operator=(const InputConfig& RHS)
                {
                    Locator = RHS.Locator;
                    Type = RHS.Type;
                    OutputEnabled = RHS.OutputEnabled;
                    return (*this);
                }

                Core::JSON::String Locator;
                Core::JSON::EnumType<InputHandler::type> Type;
                Core::JSON::Boolean OutputEnabled;
            };

#ifdef PROCESSCONTAINERS_ENABLED

            class ProcessContainerConfig : public Core::JSON::Container {
            public:
                ProcessContainerConfig()
                    : Logging(_T("NONE"))
                {

                    Add(_T("logging"), &Logging);
                }
                ProcessContainerConfig(const ProcessContainerConfig& copy)
                    : Logging(copy.Logging)
                {
                    Add(_T("logging"), &Logging);
                }
                ~ProcessContainerConfig() override = default;

                ProcessContainerConfig& operator=(const ProcessContainerConfig& RHS)
                {
                    Logging = RHS.Logging;
                    return (*this);
                }

                Core::JSON::String Logging;
            };

#endif

        public:
            JSONConfig(const Config&) = delete;
            JSONConfig& operator=(const Config&) = delete;

            JSONConfig()
                : Version()
                , Model()
                , Port(80)
                , Binding(_T("0.0.0.0"))
                , Interface()
                , Prefix(_T("Service"))
                , JSONRPC(_T("jsonrpc"))
                , PersistentPath()
                , DataPath()
                , SystemPath()
#ifdef __WINDOWS__
                , VolatilePath(_T("c:/temp"))
#else
                , VolatilePath(_T("/tmp"))
#endif
                , ProxyStubPath()
                , PostMortemPath(_T("/opt/minidumps"))
#ifdef __WINDOWS__
                , Communicator(_T("127.0.0.1:7889"))
#else
                , Communicator(_T("/tmp/communicator|0777"))
#endif
                , Redirect(_T("http://127.0.0.1/Service/Controller/UI"))
                , Signature(_T("TestSecretKey"))
                , IdleTime(0)
                , IPV6(false)
                , DefaultTraceCategories(false)
                , Process()
                , Input()
                , Configs()
                , Environments()
                , ExitReasons()
#ifdef PROCESSCONTAINERS_ENABLED
                , ProcessContainers()
#endif
                , LinkerPluginPaths()
            {
                // No IdleTime
                Add(_T("version"), &Version);
                Add(_T("model"), &Model);
                Add(_T("port"), &Port);
                Add(_T("binding"), &Binding);
                Add(_T("interface"), &Interface);
                Add(_T("prefix"), &Prefix);
                Add(_T("persistentpath"), &PersistentPath);
                Add(_T("datapath"), &DataPath);
                Add(_T("systempath"), &SystemPath);
                Add(_T("volatilepath"), &VolatilePath);
                Add(_T("proxystubpath"), &ProxyStubPath);
                Add(_T("postmortempath"), &PostMortemPath);
                Add(_T("communicator"), &Communicator);
                Add(_T("signature"), &Signature);
                Add(_T("idletime"), &IdleTime);
                Add(_T("ipv6"), &IPV6);
                Add(_T("tracing"), &DefaultTraceCategories);
                Add(_T("redirect"), &Redirect);
                Add(_T("process"), &Process);
                Add(_T("input"), &Input);
                Add(_T("plugins"), &Plugins);
                Add(_T("configs"), &Configs);
                Add(_T("environments"), &Environments);
                Add(_T("exitreasons"), &ExitReasons);
#ifdef PROCESSCONTAINERS_ENABLED
                Add(_T("processcontainers"), &ProcessContainers);
#endif
                Add(_T("linkerpluginpaths"), &LinkerPluginPaths);
            }
            ~JSONConfig() override = default;

        public:
            Core::JSON::String Version;
            Core::JSON::String Model;
            Core::JSON::DecUInt16 Port;
            Core::JSON::String Binding;
            Core::JSON::String Interface;
            Core::JSON::String Prefix;
            Core::JSON::String JSONRPC;
            Core::JSON::String PersistentPath;
            Core::JSON::String DataPath;
            Core::JSON::String SystemPath;
            Core::JSON::String VolatilePath;
            Core::JSON::String ProxyStubPath;
            Core::JSON::String PostMortemPath;
            Core::JSON::String Communicator;
            Core::JSON::String Redirect;
            Core::JSON::String Signature;
            Core::JSON::DecUInt16 IdleTime;
            Core::JSON::Boolean IPV6;
            Core::JSON::String DefaultTraceCategories;
            ProcessSet Process;
            InputConfig Input;
            Core::JSON::String Configs;
            Core::JSON::ArrayType<Plugin::Config> Plugins;
            Core::JSON::ArrayType<Environment> Environments;
            Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::IShell::reason>> ExitReasons;
#ifdef PROCESSCONTAINERS_ENABLED
            ProcessContainerConfig ProcessContainers;
#endif
            Core::JSON::ArrayType<Core::JSON::String> LinkerPluginPaths;
        };

    public:
        class InputInfo {
        private:
            friend Config;

            InputInfo()
                : _locator()
                , _type(InputHandler::VIRTUAL)
                , _enabled(false) {
            } 
            void Set(const JSONConfig::InputConfig& input) {
                _locator = input.Locator.Value();
                _type = input.Type.Value();
                _enabled = input.OutputEnabled.Value();
            } 

        public:
            InputInfo(const InputInfo&) = delete;
            InputInfo& operator= (const InputInfo&) = delete;

            ~InputInfo() = default;

        public:
            inline const string& Locator() const {
                return(_locator);
            }
            inline InputHandler::type Type() const {
                return(_type);
            }
            inline bool Enabled() const {
                return(_enabled);
            }
 
        private:
            string _locator;
            InputHandler::type _type;
            bool _enabled;
        };
        class ProcessInfo {
        private:
            friend Config;

            ProcessInfo()
                : _isSet(false)
                , _user()
                , _group()
                , _priority(0)
                , _OOMAdjust(0)
                , _policy()
                , _umask() {
            } 
            void Set(const JSONConfig::ProcessSet& input) {
                _isSet = true;
                _user = input.User.Value();
                _group = input.Group.Value();
                _priority = input.Priority.Value();
                _OOMAdjust = input.OOMAdjust.Value();
                _policy = input.Policy.Value();
                _umask = input.Umask.Value();
            } 

        public:
            ProcessInfo(const ProcessInfo&) = delete;
            ProcessInfo& operator= (const ProcessInfo&) = delete;

            ~ProcessInfo() = default;

        public:
            inline bool IsSet() const {
                return (_isSet);
            }
            inline const string& User() const {
                return(_user);
            }
            inline const string& Group() const {
                return(_group);
            }
            inline int8_t Priority() const {
                return(_priority);
            }
            inline int8_t OOMAdjust() const {
                return(_OOMAdjust);
            }
            inline Core::ProcessInfo::scheduler Policy() const {
                return(_policy);
            }
            inline uint16_t UMask() const {
                return(_umask);
            }
 
        private:
            bool _isSet;
            string _user;
            string _group;
            int8_t _priority;
            int8_t _OOMAdjust;
            Core::ProcessInfo::scheduler _policy;
            uint16_t _umask;
        };

    public:
        Config() = delete;
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

        Config(Core::File& file, const bool background, Core::OptionalType<Core::JSON::Error>& error)
            : _background(background)
            , _security(nullptr)
            , _inputInfo()
            , _processInfo()
            , _plugins()
            , _reasons()
            , _substituter(*this)
        {
            JSONConfig config;

            config.IElement::FromFile(file, error);

            if (error.IsSet() == false) {
                _webPrefix = '/' + config.Prefix.Value();
                _JSONRPCPrefix = '/' + config.JSONRPC.Value();
#ifdef PROCESSCONTAINERS_ENABLED
                _ProcessContainersLogging = config.ProcessContainers.Logging.Value();
#endif
                _volatilePath = Core::Directory::Normalize(config.VolatilePath.Value());
                _persistentPath = Core::Directory::Normalize(config.PersistentPath.Value());
                _dataPath = Core::Directory::Normalize(config.DataPath.Value());
                _systemPath = Core::Directory::Normalize(config.SystemPath.Value());
                _configsPath = Core::Directory::Normalize(config.Configs.Value());
                _proxyStubPath = Core::Directory::Normalize(config.ProxyStubPath.Value());
                _postMortemPath = Core::Directory::Normalize(config.PostMortemPath.Value());
                _appPath = Core::File::PathName(Core::ProcessInfo().Executable());
                _hashKey = config.Signature.Value();
                _communicator = Core::NodeId(config.Communicator.Value().c_str());
                _redirect = config.Redirect.Value();
                _version = config.Version.Value();
                _IPV6 = config.IPV6.Value();
                _binding = config.Binding.Value();
                _interface = config.Interface.Value();
                _portNumber = config.Port.Value();
                _stackSize = config.Process.IsSet() ? config.Process.StackSize.Value() : 0;
                _inputInfo.Set(config.Input);
                _processInfo.Set(config.Process);

                _traceCategoriesFile = config.DefaultTraceCategories.IsQuoted();
                if (_traceCategoriesFile == true) {
                    config.DefaultTraceCategories.SetQuoted(true);
                }
                _traceCategories = config.DefaultTraceCategories.Value();

                if (config.Model.IsSet()) {
                    _model = config.Model.Value();
                } else if (Core::SystemInfo::GetEnvironment(_T("MODEL_NAME"), _model) == false) {
                    _model = "UNKNOWN";
                }

                if ((config.ExitReasons.IsSet() == true) && (config.ExitReasons.Length() > 0)) {
                    Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::IShell::reason>>::Iterator index(config.ExitReasons.Elements());

                    while (index.Next() == true) {
                        _reasons.push_back(index.Current().Value());
                    }
                }

                bool status = true;
                Core::JSON::ArrayType<JSONConfig::Environment>::ConstIterator index(static_cast<const JSONConfig&>(config).Environments.Elements());
                while (index.Next() == true) {
                    if ((index.Current().Key.IsSet() == true) && (index.Current().Value.IsSet() == true)) {
                        string value = _substituter.Substitute(index.Current().Value.Value(), nullptr);
                        if (value.empty() != true) {
                            status = Core::SystemInfo::SetEnvironment(index.Current().Key.Value(), value, index.Current().Override.Value());
                            if (status != true) {
                                SYSLOG(Logging::Startup, (_T("Failure in setting Key:Value:[%s]:[%s]\n"), index.Current().Key.Value().c_str(), index.Current().Value.Value().c_str()));
                            }
                        } else {
                            SYSLOG(Logging::Startup, (_T("Failure in Substituting Value of Key:Value:[%s]:[%s]\n"), index.Current().Key.Value().c_str(), index.Current().Value.Value().c_str()));
                        }
                    }
                }

                UpdateBinder();

                // Get all in the config configure Plugins..
                _plugins = config.Plugins;

                Core::JSON::ArrayType<Core::JSON::String>::Iterator itr(config.LinkerPluginPaths.Elements());
                while (itr.Next())
                    _linkerPluginPaths.push_back(itr.Current().Value());
            }
        }
        ~Config()
        {
            ASSERT(_security != nullptr);
            _security->Release();
        }

    public:
        inline const string& Version() const
        {
            return (_version);
        }
        inline const string& Model() const
        {
            return (_model);
        }
        inline bool TraceCategoriesFile() const
        {
            return (_traceCategoriesFile);
        }
        inline const string& TraceCategories() const
        {
            return (_traceCategories);
        }
        inline const string& Redirect() const
        {
            return (_redirect);
        }
        inline const string& WebPrefix() const
        {
            return (_webPrefix);
        }
        inline const string& JSONRPCPrefix() const
        {
            return (_JSONRPCPrefix);
        }
#ifdef PROCESSCONTAINERS_ENABLED
        inline const string& ProcessContainersLogging() const {
            return (_ProcessContainersLogging);
        }
#endif
        inline const string& VolatilePath() const
        {
            return (_volatilePath);
        }
        inline const string& PersistentPath() const
        {
            return (_persistentPath);
        }
        inline const string& DataPath() const
        {
            return (_dataPath);
        }
        inline const string& AppPath() const
        {
            return (_appPath);
        }
        inline const Core::NodeId& Accessor() const
        {
            return (_accessor);
        }
        inline const Core::NodeId& Communicator() const
        {
            return (_communicator);
        }
        inline const Core::NodeId& Binder() const
        {
            return (_binder);
        }
        inline const string& SystemPath() const
        {
            return (_systemPath);
        }
        inline const string& ConfigsPath() const
        {
            return (_configsPath);
        }
        inline const string& ProxyStubPath() const
        {
            return (_proxyStubPath);
        }
        inline const string& PostMortemPath() const
        {
            return (_postMortemPath);
        }
        inline const bool PostMortemAllowed(PluginHost::IShell::reason why) const
        {
            std::list<PluginHost::IShell::reason>::const_iterator index(std::find(_reasons.begin(), _reasons.end(), why));
            return ((index != _reasons.end()) ? true: false);
        }
        inline const string& HashKey() const
        {
            return (_hashKey);
        }
        inline ISecurity* Security() const
        {
            _security->AddRef();
            return (_security);
        }
        inline bool Background() const
        {
            return (_background);
        }
        inline string Substitute(const string& input, const Plugin::Config& plugin) const {
            return (_substituter.Substitute(input, &plugin));
        }
        inline uint16_t IdleTime() const {
            return (_idleTime);
        }
        inline const string& URL() const {
            return (_URL);
        }
        inline uint32_t StackSize() const {
            return (_stackSize);
        }
        inline const InputInfo& Input() const {
            return(_inputInfo);
        }
        inline const ProcessInfo& Process() const {
            return(_processInfo);
        }
        inline bool IPv6() const {
            return (_IPV6);
        }
        const Plugin::Config* Plugin(const string& name) const {
            Core::JSON::ArrayType<Plugin::Config>::ConstIterator index(_plugins.Elements());

            // Check if there is already a plugin config with this callsign
            while ((index.Next() == true) && (index.Current().Callsign.Value() != name)) /* INTENTIONALLY */ ;

            return (index.IsValid() ? &(index.Current()) : nullptr);
        }
        Core::JSON::ArrayType<Plugin::Config>::Iterator Plugins() {
            return (_plugins.Elements());
        }
        bool Add(const Plugin::Config& plugin) {

            bool added = false;
            const string& name (plugin.Callsign.Value());

            Core::JSON::ArrayType<Plugin::Config>::Iterator index(_plugins.Elements());

            // Check if there is already a plugin config with this callsign
            while ((index.Next() == true) && (index.Current().Callsign.Value() != name)) /* INTENTIONALLY */ ;

            if (index.IsValid()  == false) {
                added = true;
                _plugins.Add(plugin);
            }
            return (added);
        }
        void UpdateAccessor() {
            bool validAccessor = true;
            Core::NodeId result(_binding.c_str());

            if (_interface.empty() == false) {
                Core::NodeId selectedNode = Plugin::Config::IPV4UnicastNode(_interface);

                if (selectedNode.IsValid() == true) {
                    _accessor = selectedNode;
                    result = _accessor;
                }
            } else if (result.IsAnyInterface() == true) {
                // TODO: We should iterate here over all interfaces to find a suitable IPv4 address or IPv6.
                Core::NodeId selectedNode = Plugin::Config::IPV4UnicastNode(_interface);

                if (selectedNode.IsValid() == true) {
                    _accessor = selectedNode;
                }
            } else {
                _accessor = result;
            }

            if (_accessor.IsValid() == false) {

                // Let's go for the default and make the best of it :-)
                struct sockaddr_in value;

                value.sin_addr.s_addr = 0;
                value.sin_family = AF_INET;
                value.sin_port = htons(_portNumber);

                _accessor = value;
                _URL.clear();
                validAccessor = false;
            } else {
                if (_portNumber == 80) {
                    _URL = string(_T("http://")) + _accessor.HostAddress() + _webPrefix;
                } else {
                    _URL = string(_T("http://")) + _accessor.HostAddress() + ':' + Core::NumberType<uint16_t>(_portNumber).Text() + _webPrefix;
                }

                _accessor.PortNumber(_portNumber);
            }

            if (validAccessor == false) {
                SYSLOG(Logging::Startup, ("Invalid config information could not resolve to a proper IP set to: (%s:%d)", _accessor.HostAddress().c_str(), _accessor.PortNumber()));
            }
            else {
                SYSLOG(Logging::Startup, (_T("Accessor: %s"), _URL.c_str()));
                SYSLOG(Logging::Startup, (_T("Interface IP: %s"), _accessor.HostAddress().c_str()));
            }

            return;
        }

        inline const std::vector<std::string>& LinkerPluginPaths() const
        {
            return _linkerPluginPaths;
        }

    private:
        friend class Server;

        inline void UpdateBinder() {
            // Update binding address
            if (_interface.empty() == false) {
                _binder = Plugin::Config::IPV4UnicastNode(_interface);

            }
            if (_binder.IsValid() == false) {
                Core::NodeId binder(_binding.c_str(), _portNumber);
                _binder = binder;
            }
            else {
                _binder.PortNumber(_portNumber);
            }
            SYSLOG(Logging::Startup, (_T("Binder: [%s:%d]"), _binder.HostAddress().c_str(), _binder.PortNumber()));
        }

        inline void Security(ISecurity* security)
        {
            ASSERT((_security == nullptr) && (security != nullptr));

            _security = security;

            if (_security != nullptr) {
                _security->AddRef();
            }
        }


    private:
        const bool _background;
        string _webPrefix;
        string _JSONRPCPrefix;
        string _volatilePath;
        string _persistentPath;
        string _dataPath;
        string _hashKey;
        string _appPath;
        string _systemPath;
        string _configsPath;
        string _proxyStubPath;
        string _postMortemPath;
        Core::NodeId _accessor;
        Core::NodeId _communicator;
        Core::NodeId _binder;
        string _redirect;
        ISecurity* _security;
        string _version;
        string _model;
        string _traceCategories;
        bool _traceCategoriesFile;
        string _binding;
        string _interface;
        string _URL;
        uint16_t _portNumber;
        bool _IPV6;
        uint16_t _idleTime;
        uint32_t _stackSize;
        InputInfo _inputInfo;
        ProcessInfo _processInfo;
        Core::JSON::ArrayType<Plugin::Config> _plugins;
        std::list<PluginHost::IShell::reason> _reasons;
        Substituter _substituter;
#ifdef PROCESSCONTAINERS_ENABLED
        string _ProcessContainersLogging;
#endif
        std::vector<std::string> _linkerPluginPaths;
    };
}
}
