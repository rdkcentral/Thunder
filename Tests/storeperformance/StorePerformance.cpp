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
 
#include <iostream>

#define MODULE_NAME StorePerformance

#include <core/core.h>
#include <com/com.h>
#include <plugins/Types.h>
#include <interfaces/IDictionary.h>
#include <interfaces/IStore.h>
#include <iostream>

MODULE_NAME_DECLARATION(BUILD_REFERENCE);

namespace Thunder = WPEFramework;
static constexpr uint16_t MaxLoad = 50;
typedef std::function<bool(const uint16_t)> PerformanceFunction;

static void Measure(const string& interface, const string& callSign, PerformanceFunction& performanceFunction)
{
    printf("Measurements [%s]:[%s]\n", interface.c_str(), callSign.c_str());
    uint64_t time;
    Thunder::Core::SystemInfo& systemInfo(Thunder::Core::SystemInfo::Instance());
    uint64_t loadBefore = systemInfo.GetCpuLoad();
    uint64_t freeRamBefore = systemInfo.GetFreeRam();
    Thunder::Core::StopWatch measurement;

    printf("CPU Load: Before Test:  %llu\n", loadBefore);
    printf("Free Memory: Before Test: %llu\n", freeRamBefore);
    uint64_t loadInBetween, loadPeak = loadBefore;
    uint64_t freeRamBetween, freeRamPeak = freeRamBefore;
    for (uint32_t run = 0; run < MaxLoad; run++) {
        performanceFunction(run);

        loadInBetween = systemInfo.GetCpuLoad();
        if (loadInBetween > loadPeak) {
            loadPeak = loadInBetween;
        }
        freeRamBetween = systemInfo.GetFreeRam();
        if (freeRamBetween < freeRamPeak) {
            freeRamPeak = freeRamBetween;
        }
    }
    time = measurement.Elapsed();
    uint64_t memDiff = 0;
    if (freeRamBefore > freeRamPeak) {
        memDiff = static_cast<uint64_t>(freeRamBefore - freeRamPeak);
    }
    uint64_t loadDiff = 0;
    if (loadPeak > loadBefore) {
        loadDiff = static_cast<uint64_t>(loadPeak - loadBefore);
    }
    printf("Performance: Total: %llu. Average: %llu\n", time, time / MaxLoad);
    printf("Free Memory: After = %llu, Diff = %llu\n", freeRamPeak, memDiff);
    printf("CPU Load: After = %llu, Diff = %llu\n", loadPeak, loadDiff);
}

class Dictionary : public Thunder::RPC::SmartInterfaceType<Thunder::Exchange::IDictionary > {
private:
    using BaseClass = Thunder::RPC::SmartInterfaceType<Thunder::Exchange::IDictionary >;
public:
    Dictionary(const uint32_t waitTime, const Thunder::Core::NodeId& node, const string& callsign)
        : BaseClass()
    {
        BaseClass::Open(waitTime, node, callsign);
        BaseClass::Aquire<Thunder::Exchange::IDictionary>(waitTime, node, callsign);
    }
    ~Dictionary() override
    {
        BaseClass::Close(Thunder::Core::infinite);
    }

public:
    bool Get(const string& nameSpace, const string& key, string& value) const
    {
        bool result = false;
        const Thunder::Exchange::IDictionary* impl = BaseClass::Interface();

        if (impl != nullptr) {
            string ns = "/" + nameSpace;
            result = impl->Get(ns, key, value);
            impl->Release();
        }

        return (result);
    }
    bool Set(const string& nameSpace, const string& key, const string& value)
    {
        bool result = false;
        Thunder::Exchange::IDictionary* impl = BaseClass::Interface();

        if (impl != nullptr) {
            string ns = "/" + nameSpace;
            result = impl->Set(ns, key, value);
            impl->Release();
        }

        return (result);
    }

private:
    void Operational(const bool upAndRunning) override
    {
        printf("Operational state of Dictionary: %s\n", upAndRunning ? _T("true") : _T("false"));
    }
};

class PersistentStore : public Thunder::RPC::SmartInterfaceType<Thunder::Exchange::IStore > {
private:
    using BaseClass = Thunder::RPC::SmartInterfaceType<Thunder::Exchange::IStore >;
public:
    PersistentStore(const uint32_t waitTime, const Thunder::Core::NodeId& node, const string& callsign)
        : BaseClass()
    {
        BaseClass::Open(waitTime, node, callsign);
    }
    ~PersistentStore()
    {
        BaseClass::Close(Thunder::Core::infinite);
    }

public:
    bool Get(const string& nameSpace, const string& key, string& value ) const
    {
        uint32_t result = Thunder::Core::ERROR_GENERAL;
        const Thunder::Exchange::IStore* impl = BaseClass::Interface();

        if (impl != nullptr) {
            result = const_cast<Thunder::Exchange::IStore*>(impl)->GetValue(nameSpace, key, value);
            impl->Release();
        }

        return (result == Thunder::Core::ERROR_NONE);
    }
    bool Set(const string& nameSpace, const string& key, const string& value)
    {
        uint32_t result = Thunder::Core::ERROR_GENERAL;
        Thunder::Exchange::IStore* impl = BaseClass::Interface();

        if (impl != nullptr) {
            result = impl->SetValue(nameSpace, key, value);
            impl->Release();
        }

        return (result == Thunder::Core::ERROR_NONE);
    }

private:
    void Operational(const bool upAndRunning) override
    {
        printf("Operational state of PersistentStore: %s\n", upAndRunning ? _T("true") : _T("false"));
    }
};

namespace JSONRPC {
class PersistentStore : public Thunder::JSONRPC::SmartLinkType<Thunder::Core::JSON::IElement> {
private:
    class Parameters : public Thunder::Core::JSON::Container {
    public:
        Parameters()
            : Thunder::Core::JSON::Container()
            , Key()
            , Value()
            , NameSpace()
        {
            Add(_T("key"), &Key);
            Add(_T("value"), &Value);
            Add(_T("namespace"), &NameSpace);
        }
        Parameters(const Parameters& copy)
            : Thunder::Core::JSON::Container()
            , Key(copy.Key)
            , Value(copy.Value)
            , NameSpace(copy.NameSpace)
        {
            Add(_T("key"), &Key);
            Add(_T("value"), &Value);
            Add(_T("namespace"), &NameSpace);
        }
        Parameters(const string& key, const string& value, const string& nameSpace)
            : Thunder::Core::JSON::Container()
            , Key()
            , Value()
            , NameSpace()
        {
            Add(_T("key"), &Key);
            Add(_T("value"), &Value);
            Add(_T("namespace"), &NameSpace);
            if (key.empty() == false) {
               Key = key;
            }

            if (value.empty() == false) {
                Value = value;
            }

            if (nameSpace.empty() == false) {
                NameSpace = nameSpace;
            }
        }
        ~Parameters() override = default;

        Parameters& operator=(const Parameters& RHS)
        {
            Key = RHS.Key;
            Value = RHS.Value;
            NameSpace = RHS.NameSpace;

            return (*this);
        }

    public:
        Thunder::Core::JSON::String Key;
        Thunder::Core::JSON::String Value;
        Thunder::Core::JSON::String NameSpace;
    };
    class Response : public Thunder::Core::JSON::Container {
    public:
        Response()
            : Thunder::Core::JSON::Container()
            , Value()
            , Success()
        {
            Add(_T("value"), &Value);
            Add(_T("success"), &Success);
        }
        Response(const Response& copy)
            : Thunder::Core::JSON::Container()
            , Value(copy.Value)
            , Success(copy.Success)
        {
            Add(_T("value"), &Value);
            Add(_T("success"), &Success);
        }
        Response(const string& value, const bool success)
            : Thunder::Core::JSON::Container()
            , Value()
            , Success()
        {
            Add(_T("value"), &Value);
            Add(_T("success"), &Success);
            if (value.empty() == false) {
                Value = value;
            }
            Success = success;
        }
        ~Response() override = default;

        Response& operator=(const Response& RHS)
        {
            Value = RHS.Value;
            Success = RHS.Success;

            return (*this);
        }

    public:
        Thunder::Core::JSON::String Value;
        Thunder::Core::JSON::Boolean Success;
    };


public:
    PersistentStore(const uint32_t waitTime VARIABLE_IS_NOT_USED, const Thunder::Core::NodeId& node VARIABLE_IS_NOT_USED, const string& callsign VARIABLE_IS_NOT_USED)
        : Thunder::JSONRPC::SmartLinkType<Thunder::Core::JSON::IElement>("PersistentStore.1", "client.monitor.2")
        , _isOperational(false)
    {
    }
    ~PersistentStore()
    {
    }

private:
    void StateChange() override{
        if (this->IsActivated() == true) {
            _isOperational = true;
            printf("Plugin is activated\n");
        } else {
            _isOperational = false;
            printf("Plugin is deactivated\n");
        }
    }
public:
    bool IsOperational()
    {
        return _isOperational;
    }
    bool Get(const string& nameSpace, const string& key, string& value) const
    {
        Parameters parameters;
        parameters.NameSpace = nameSpace;
        parameters.Key = key;

        Response response;
        const_cast<PersistentStore*>(this)->template Invoke<Parameters, Response>(10000, _T("getValue"), parameters, response);
        value = response.Value.Value();
        return response.Success.Value();
    }
    bool Set(const string& nameSpace, const string& key, const string& value)
    {
        Parameters parameters;
        parameters.NameSpace = nameSpace;
        parameters.Key = key;
        parameters.Value = value;

        Response response;
        this->template Invoke<Parameters, Response>(10000, _T("setValue"), parameters, response);
        return response.Success.Value();
    }

private:
    bool _isOperational;
};
}
namespace WPEFramework {
namespace ReST {
static Core::ProxyPoolType<Web::TextBody> textBodyDataFactory(4);

class Dictionary : public Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Web::SingleElementFactoryType<Web::Response>> {
private:
    static constexpr const TCHAR* PluginPath = "/Service/Dictionary/";
    static constexpr const TCHAR* HostIP = "127.0.0.1";
    static int8_t constexpr HostPort = 80;
    static int32_t constexpr WaitTime = 1000; // In milli seconds

    typedef Thunder::Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Web::SingleElementFactoryType<Web::Response>> BaseClass;

public:
    Dictionary(const uint32_t waitTime VARIABLE_IS_NOT_USED, const Thunder::Core::NodeId& node VARIABLE_IS_NOT_USED, const string& callsign VARIABLE_IS_NOT_USED)
        : BaseClass(1, false, Core::NodeId("0.0.0.0"), Core::NodeId(HostIP, HostPort), 1024, 1024)
        , _signal(false, true)
        , _request(Core::ProxyType<Web::Request>::Create())
        , _response()
    {
        BaseClass::Open(0);
    }
    ~Dictionary() override
    {
        _signal.ResetEvent();
        BaseClass::Close(0);
        if (_request.IsValid() == true) {
            _request.Release();
        }
        if (_response.IsValid() == true) {
            _response.Release();
        }
    }

public:
    bool IsOperational()
    {
        return true;
    }
    bool Get(const string& nameSpace, const string& key, string& value) const
    {
        bool status = false;
        string path = string(PluginPath) + nameSpace;
        path += "/" + key;
        _request->Path = path;
        _request->Verb = Web::Request::HTTP_GET;
        _request->Host = HostIP;

        uint32_t result = const_cast<Dictionary*>(this)->SendRequest();
        if ((result == Core::ERROR_INPROGRESS) ||
            (result == Core::ERROR_NONE)) {
            if (const_cast<Dictionary*>(this)->WaitForCompletion(WaitTime) == Core::ERROR_NONE) {
                if ((_response.IsValid() == true) && (_response->ErrorCode == Web::STATUS_OK) && (_response->HasBody() == true)) {
                    Core::ProxyType<const Web::TextBody> valueBody(_response->Body<Web::TextBody>());
                    value = (valueBody.IsValid() == true ? string(*valueBody) : string());
                    status = true;
                }
                _response.Release();
            }
        }
        return status;
    }
    bool Set(const string& nameSpace, const string& key, const string& value)
    {
        bool status = false;
        string path = string(PluginPath) + nameSpace;
        path += "/" + key;
        _request->Path = path;
        _request->Verb = Web::Request::HTTP_POST;
        _request->Host = HostIP;
        Core::ProxyType<Web::TextBody> valueBody(textBodyDataFactory.Element());

        *valueBody = value;

        _request->Body(Core::ProxyType<Web::IBody>(valueBody));

        uint32_t result = SendRequest();
        if ((result == Core::ERROR_INPROGRESS) ||
            (result == Core::ERROR_NONE)) {

            if (WaitForCompletion(WaitTime) == Core::ERROR_NONE) {
                if ((_response.IsValid() == true) && (_response->ErrorCode == Web::STATUS_OK)) {

                    status = true;
                }
                _response.Release();
            }
        }

        return status;
    }

private:
    uint32_t SendRequest()
    {
        ASSERT(_request.IsValid() == true);
        ASSERT(_response.IsValid() == false);

        uint32_t result = Core::ERROR_NONE;

        if (BaseClass::IsOpen() == true) {
            _signal.ResetEvent();
            BaseClass::Submit(_request);
        } else {
            result = BaseClass::Open(0);
        }
        return result;
    }
    void LinkBody(Core::ProxyType<Web::Response>& element) override
    {
        // Time to attach a String Body
        element->Body<Web::TextBody>(textBodyDataFactory.Element());
    }
    void Received(Core::ProxyType<Web::Response>& response) override
    {
        _response = response;
        _signal.SetEvent();
    }
    void Send(const Core::ProxyType<Web::Request>& request VARIABLE_IS_NOT_USED) override
    {
    }
    inline uint32_t WaitForCompletion(int32_t waitTime)
    {
        uint32_t status = _signal.Lock(waitTime);
        _signal.ResetEvent();
        return status;
    }
    void StateChange() override
    {
    }

private:
    Core::Event _signal;
    Core::ProxyType<Web::Request> _request;
    Core::ProxyType<Web::Response> _response;
};
}
}
void ShowMenu()
{
    printf("Enter\n"
           "\tD : Check Performance of Dictionary Plugin using COMRPC\n"
           "\tR : Check Performance of Dictionary Plugin using ReSTAPI\n"
           "\tP : Check Performance of PersistentStore Plugin using COMRPC\n"
           "\tJ : Check Performance of PersistentStore Plugin using JSONRPC\n"
           "\tQ : Quit\n");
}

void ShowPerformanceMenu()
{
    printf("Enter\n"
           "\tO : Check IsOperational \n"
           "\tS : Set key:value\n"
           "\tG : Get key:value\n"
           "\tL : Load test \n"
           "\tQ : Quit\n");
}


template <typename STORE>
class StorePerformance {

public:
    StorePerformance(uint32_t waitTime, const Thunder::Core::NodeId& nodeId, const string& callSign)
        : _store(waitTime, nodeId, callSign)
        , _callSign(callSign)
    {
    }
    ~StorePerformance() = default;

public:
    bool IsOperational()
    {
        return _store.IsOperational();
    }
    bool Set(uint16_t index)
    {
        string key = "key" + Thunder::Core::NumberType<int32_t>(index).Text();
        string value = "value" + Thunder::Core::NumberType<int32_t>(index).Text();

#if DEBUG_FUNC
        bool status = false;
        if ((status = _store.Set(_T("name"), key, value)) == true) {
            printf("Set value: %s\n", value.c_str());
        } else {
            printf("Set failed with key:value: %s :%s\n", key.c_str(), value.c_str());
        }
        return status;
#else
        return _store.Set(_T("name"), key, value);
#endif

    }
    bool Get(uint16_t index) const
    {
        string value;
        string key = "key" + Thunder::Core::NumberType<int32_t>(index).Text();

#if DEBUG_FUNC
        bool status = false;
        if ((status = _store.Get(_T("name"), key, value)) == true) {
            printf("Get value: %s\n", value.c_str());
        } else {
            printf("Get failed with key %s\n", key.c_str());
        }
        return status;
#else
        return _store.Get(_T("name"), key, value);
#endif
    }
    bool Load(uint16_t index)
    {
        bool status = true;
        for (uint16_t i = 0; i < MaxLoad; ++i) {
            string prefix = "-" + Thunder::Core::NumberType<int32_t>(index).Text();
            prefix += ":" + Thunder::Core::NumberType<int32_t>(i).Text();
            string write = "value" + prefix;
            string key = "key" + prefix;
            string read;
            if (_store.Set(_T("name"), key, write) == true) {
                if (_store.Get(_T("name"), key, read) == true) {
                    if (write != read) {
                        printf("Mistmach in set/get set:%s get:%s\n", write.c_str(), read.c_str());
                        status = false;
                        break;
                    }
                } else {
                    status = false;
                    printf("Failed to get value\n");
                    break;
                }
            } else {
                status = false;
                printf("Failed to set value\n");
                printf("Mistmach in set key:%s value:%s\n", key.c_str(), write.c_str());
                break;
            }
        }
        return status;
    }
private:
    STORE _store;
    string _callSign;
};

template <typename STORE>
void MeasureStore(const string& interface, const Thunder::Core::NodeId& nodeId, const string& callSign)
{
    int option = 0;
    StorePerformance<STORE> store(3000, nodeId, callSign);
    do {
        ShowPerformanceMenu();
        getchar(); // Skip white space
        option = toupper(getchar());
        switch (option) {
        case 'O': {
            printf("Operations state issue: %s\n", store.IsOperational() ? _T("true") : _T("false"));
            break;
        }
        case 'S': {
            PerformanceFunction implementation = [&store](const uint16_t index) -> bool {
               return store.Set(index);
            };

            Measure(interface, callSign, implementation);
            break;
        }
        case 'G': {
            PerformanceFunction implementation = [&store](const uint16_t index) -> bool {
                return store.Get(index);
            };

            Measure(interface, callSign, implementation);
            break;
        }
        case 'L': {
            PerformanceFunction implementation = [&store](const uint16_t index) -> bool {
                return store.Load(index);
            };

            Measure(interface, callSign, implementation);
            break;
        }
        }
    } while (option != 'Q');
}

int main(int argc VARIABLE_IS_NOT_USED, char* argv[] VARIABLE_IS_NOT_USED)
{
    // The core::NodeId can hold an IPv4, IPv6, domain, HCI, L2CAP or netlink address
    // Here we create a domain socket address
#ifdef __WINDOWS__
    Thunder::Core::NodeId nodeId("127.0.0.1:62000");
    Thunder::Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T("127.0.0.1:25555")));
#else
    Thunder::Core::NodeId nodeId("/tmp/communicator");
    Thunder::Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T("127.0.0.1:80")));
#endif
    {
        char keyPress;

        do {
            ShowMenu();
            keyPress = toupper(getchar());
            
            switch (keyPress) {
            case 'D': {
                MeasureStore<Dictionary>("COMRPC", nodeId, "Dictionary");
                break;
            }
            case 'R': {
                MeasureStore<Thunder::ReST::Dictionary>("RESTAPI", nodeId, "Dictionary");
                break;
            }
            case 'P': {
                MeasureStore<PersistentStore>("COMRPC", nodeId, "PersistentStore");
                break;
            }
            case 'J': {
                MeasureStore<JSONRPC::PersistentStore>("JSONRPC", nodeId, "PersistentStore");
                break;
            }
            case 'Q': break;
            default: break;
            };
        } while (keyPress != 'Q');
    }

    Thunder::Core::Singleton::Dispose();

    return 0;
}
