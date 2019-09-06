#pragma once

#include "PluginServer.h"

namespace WPEFramework {
namespace PluginHost {
    class EnvironmentConfig {
    private:
        static constexpr TCHAR Delimeter = _T('%');
        typedef string (*ValueHandler)(const Server::Config& config);

    public:
        typedef std::map<string, ValueHandler> VariableMap;
        EnvironmentConfig(const EnvironmentConfig&) = delete;
        EnvironmentConfig& operator=(const EnvironmentConfig&) = delete;

        EnvironmentConfig()
        {
        }
        virtual ~EnvironmentConfig()
        {
        }

        static void SetEnvironments(const Server::Config& config, const Core::JSON::ArrayType<Server::Config::Environment>& environments)
        {
            bool status = true;
            Core::JSON::ArrayType<Server::Config::Environment>::ConstIterator index(environments.Elements());
            while (index.Next() == true) {
                if ((index.Current().Key.IsSet() == true) && (index.Current().Value.IsSet() == true)) {
                    string value = index.Current().Value.Value();
                    status = SubstituteVariables(config, value);
                    if (status == true) {
                        status = Core::SystemInfo::SetEnvironment(index.Current().Key.Value(), value, index.Current().Override.Value());
                        if (status != true) {
                            TRACE_L1("Failure in setting Key:Value:[%s]:[%s]\n", index.Current().Key.Value().c_str(), index.Current().Value.Value().c_str());
                        }
                    }
                }
            }
        }

        static void Initialize(const Server::Config& config)
        {
            _variables.insert(std::make_pair("port", [](const Server::Config& config) { return std::to_string(config.Port.Value());}));
            _variables.insert(std::make_pair("binding", [](const Server::Config& config) { return config.Binding.Value();}));
            _variables.insert(std::make_pair("datapath", [](const Server::Config& config) { return config.DataPath.Value();}));
            _variables.insert(std::make_pair("persistentpath", [](const Server::Config& config) { return config.PersistentPath.Value();}));
            _variables.insert(std::make_pair("systempath", [](const Server::Config& config) { return config.SystemPath.Value();}));
            _variables.insert(std::make_pair("volatilepath", [](const Server::Config& config) { return config.VolatilePath.Value();}));
            _variables.insert(std::make_pair("proxystubpath", [](const Server::Config& config) { return config.ProxyStubPath.Value();}));
        }

    private:
        static bool SubstituteVariables(const Server::Config& config, string& value)
        {
            bool status = true;
            std::size_t begin = 0;
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
                                status = false;
                                TRACE_L1("%s variable is not found in the Variable lookup table\n", variable.c_str());
                                break;
                            }
                            begin = end;
                        } else {
                            status = false;
                            TRACE_L1("%s variable is not given between the delimeter:[%c]\n", Delimeter);
                            break;
                        }
                    } else {
                        status = false;
                        TRACE_L1("There is no closing delemeter-%s for the value\n", Delimeter, value.c_str());
                        break;
                    }
                }
            }
            return status;
        }

    public:
        static VariableMap _variables;
    };
}
}
