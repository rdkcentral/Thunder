#pragma once

#include "Configuration.h"

namespace WPEFramework {
namespace PluginHost {
    class Environment {
    private:
        static constexpr TCHAR Delimeter = _T('%');
        typedef string (*ValueHandler)(const PluginHost::Config& config);
        typedef std::map<string, ValueHandler> VariableMap;

    public:
        class Config : public Core::JSON::Container {
        public:
            Config()
                : Core::JSON::Container()
                , Key()
                , Value()
                , Override(false)
            {
                Add(_T("key"), &Key);
                Add(_T("value"), &Value);
                Add(_T("override"), &Override);
            }
            Config(const Config& copy)
                : Core::JSON::Container()
                , Key(copy.Key)
                , Value(copy.Value)
                , Override(copy.Override)
            {
                Add(_T("key"), &Key);
                Add(_T("value"), &Value);
                Add(_T("override"), &Override);
            }
            virtual ~Config()
            {
            }
            Config& operator=(const Config& RHS)
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

        Environment(const Environment&) = delete;
        Environment& operator=(const Environment&) = delete;

        Environment()
        {
            _variables.insert(std::make_pair("datapath", [](const PluginHost::Config& config) { return config.DataPath();}));
            _variables.insert(std::make_pair("persistentpath", [](const PluginHost::Config& config) { return config.PersistentPath();}));
            _variables.insert(std::make_pair("systempath", [](const PluginHost::Config& config) { return config.SystemPath();}));
            _variables.insert(std::make_pair("volatilepath", [](const PluginHost::Config& config) { return config.VolatilePath();}));
            _variables.insert(std::make_pair("proxystubpath", [](const PluginHost::Config& config) { return config.ProxyStubPath();}));
        }
        virtual ~Environment()
        {
        }

        void Set(const PluginHost::Config& config, const Core::JSON::ArrayType<Config>& environments)
        {
            bool status = true;
            Core::JSON::ArrayType<Config>::ConstIterator index(environments.Elements());
            while (index.Next() == true) {
                if ((index.Current().Key.IsSet() == true) && (index.Current().Value.IsSet() == true)) {
                    string value = SubstituteVariables(config, index.Current().Value.Value());
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
        }

        string SubstituteVariables(const PluginHost::Config& config, const string& input) const
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
                                value.replace(begin, (end + 1) - begin, index->second(config));
                                end = begin + index->second(config).length();
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
        VariableMap _variables;
    };
}
}
