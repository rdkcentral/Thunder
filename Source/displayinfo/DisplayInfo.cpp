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

#include <stdlib.h>

#include <com/com.h>
#include <core/core.h>

#include <displayinfo.h>
#include <interfaces/IDisplayInfo.h>

static string GetEndPoint()
{
    TCHAR* value = ::getenv(_T("COMMUNICATOR_PATH"));

    return (value == nullptr ?
#ifdef __WINDOWS__
                             _T("127.0.0.1:62000")
#else
                             _T("/tmp/communicator")
#endif
                             : value);
}

#define Trace(fmt, ...)                                                                                                                     \
    do {                                                                                                                                    \
        fprintf(stdout, "\033[1;32m[%s:%d](%s){%p}<%d>:" fmt "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, this, getpid(), ##__VA_ARGS__); \
        fflush(stdout);                                                                                                                     \
    } while (0)

namespace WPEFramework {

class DisplayInfoAdministrator {
private:
    DisplayInfoAdministrator()
        : _comChannel(Core::ProxyType<RPC::CommunicatorClient>::Create(
            Core::NodeId(GetEndPoint().c_str())))
    {
        Trace("COM channel node %s, Build %s", GetEndPoint().c_str(), __TIMESTAMP__);

        ASSERT(_comChannel.IsValid());
    }

    class DisplayInfo {
    private:
        typedef std::map<displayinfo_updated_cb, void*> Callbacks;

        class Notification : public Exchange::IConnectionProperties::INotification {
        public:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

            Notification(DisplayInfo* parent)
                : _parent(*parent)
            {
            }

            void Updated() override
            {
                Trace("Display changed");
                _parent.Updated();
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(Exchange::IConnectionProperties::INotification)
            END_INTERFACE_MAP

        private:
            DisplayInfo& _parent;
        };

    public:
        DisplayInfo() = delete;
        DisplayInfo(const DisplayInfo&) = delete;
        DisplayInfo& operator=(const DisplayInfo&) = delete;

        DisplayInfo(DisplayInfoAdministrator& parent, const std::string& displayName)
            : _parent(parent)
            , _refCount(1)
            , _name(displayName)
            , _displayConnection(nullptr)
            , _notification(this)
            , _callbacks()
        {
            _parent.Register(this);
            Trace("Created %s", _name.c_str());
        }

        ~DisplayInfo()
        {
            _parent.Unregister(this);
            Trace("Destroyed %s", _name.c_str());
        }

        void Initialize(Core::ProxyType<RPC::CommunicatorClient>& comChannel)
        {
            if (comChannel->IsOpen() == true) {
                _displayConnection = comChannel->Aquire<Exchange::IConnectionProperties>(RPC::CommunicationTimeOut, _name.c_str(), ~0);
            }

            if (_displayConnection) {
                _displayConnection->Register(&_notification);
            }
        }

        void Deinitialize()
        {
            if (_displayConnection != nullptr) {
                _displayConnection->Unregister(&_notification);
                _displayConnection->Release();
            }
        }

        void AddRef() const
        {
            Core::InterlockedIncrement(_refCount);
        }
        uint32_t Release() const
        {
            if (Core::InterlockedDecrement(_refCount) == 0) {
                delete const_cast<DisplayInfo*>(this);
            }

            return (0);
        }

        void Updated()
        {
            Trace("Display changed");

            Callbacks::iterator index(_callbacks.begin());

            while (index != _callbacks.end()) {
                index->first(reinterpret_cast<displayinfo_type*>(this), index->second);
                index++;
            }
        }
        void Register(displayinfo_updated_cb callback, void* userdata)
        {
            Callbacks::iterator index(_callbacks.find(callback));

            if (index == _callbacks.end()) {

                Trace("Registering displayinfo_updated_cb=%p", callback);

                _callbacks.emplace(std::piecewise_construct,
                    std::forward_as_tuple(callback),
                    std::forward_as_tuple(userdata));
            }
        }
        void Unregister(displayinfo_updated_cb callback)
        {
            Callbacks::iterator index(_callbacks.find(callback));

            if (index != _callbacks.end()) {
                Trace("Unregistering displayinfo_updated_cb=%p", callback);

                _callbacks.erase(index);
            }
        }

        const string& Name() const { return _name; }
        const bool IsValid() const { return _displayConnection != nullptr; }

        bool IsAudioPassthrough() const
        {
            ASSERT(_displayConnection != nullptr);
            return _displayConnection->IsAudioPassthrough();
        }

        bool Connected() const
        {
            ASSERT(_displayConnection != nullptr);
            return _displayConnection->Connected();
        }

        uint32_t Width() const
        {
            ASSERT(_displayConnection != nullptr);
            return _displayConnection->Width();
        }

        uint32_t Height() const
        {
            ASSERT(_displayConnection != nullptr);
            return _displayConnection->Height();
        }

        Exchange::IConnectionProperties::HDRType HDR() const
        {
            ASSERT(_displayConnection != nullptr);
            return _displayConnection->Type();
            ;
        }

        Exchange::IConnectionProperties::HDCPProtectionType HDCPProtection() const
        {
            ASSERT(_displayConnection != nullptr);
            return _displayConnection->HDCPProtection();
        }

    private:
        DisplayInfoAdministrator& _parent;
        mutable uint32_t _refCount;
        const string _name;
        Exchange::IConnectionProperties* _displayConnection;
        Core::Sink<Notification> _notification;
        Callbacks _callbacks;
    };

public:
    ~DisplayInfoAdministrator()
    {
        ASSERT(_clients.size() == 0);

        for (auto client : _clients) {
            Trace("Closing %s", client->Name().c_str());
            delete client;
        }

        if (_comChannel->IsOpen()) {
            _comChannel->Close(RPC::CommunicationTimeOut);
            Trace("Closed COM channel %d", _comChannel->ConnectionId());
        }
    }

    DisplayInfoAdministrator(const DisplayInfoAdministrator&) = delete;
    DisplayInfoAdministrator& operator=(const DisplayInfoAdministrator&) = delete;

    void Register(DisplayInfo* client)
    {
        std::list<DisplayInfo*>::iterator index(std::find(_clients.begin(), _clients.end(), client));

        if (index == _clients.end()) {
            if (_clients.size() == 0) {
                _comChannel->Open(RPC::CommunicationTimeOut);
                Trace("Opened COM channel %d", _comChannel->ConnectionId());
            }

            client->Initialize(_comChannel);

            if (client->IsValid() == true) {
                Trace("Added client %p to registery[%d]", client, _clients.size());
                _clients.push_back(client);
            }

            if (_clients.size() == 0) {
                _comChannel->Close(RPC::CommunicationTimeOut);
                Trace("Closed COM channel %d", _comChannel->ConnectionId());
            }
        }
    }
    void Unregister(DisplayInfo* client)
    {
        std::list<DisplayInfo*>::iterator index(std::find(_clients.begin(), _clients.end(), client));

        if (index != _clients.end()) {

            _clients.erase(index);

            if (client->IsValid() == true) {
                (*index)->Deinitialize();
            }

            Trace("Removed client %p from registery[%d]", (*index), _clients.size());

            if (_clients.size() == 0) {
                _comChannel->Close(RPC::CommunicationTimeOut);
                Trace("Closed COM channel %d", _comChannel->ConnectionId());
            }
        }
    }

    DisplayInfo* Get(const string& displayName)
    {
        DisplayInfo* client(nullptr);

        std::list<DisplayInfo*>::iterator index(_clients.begin());

        while ((index != _clients.end()) && ((*index)->Name().c_str() != displayName)) {
            index++;
        }

        if (index != _clients.end()) {
            client = (*index);
            Trace("Found existing connection %p to %s", client, displayName.c_str());
        } else {
            Trace("Creating new connection to %s", displayName.c_str());

            _comChannel->Open(RPC::CommunicationTimeOut);

            client = new DisplayInfo(*this, displayName);
        }

        return client;
    };

    static DisplayInfoAdministrator& Instance()
    {
        static DisplayInfoAdministrator instance;
        return instance;
    }

    bool Enumerate(const uint8_t element, string& instance) const
    {
        // TODO: iterate over all active plugins to find all available Exchange::IConnectionProperties interfaces.

        std::vector<string> instances = {"DisplayInfo"} ;

        if (element < instances.size()) {
            instance = instances[element];
        }

        return element < instances.size();
    }

private:
    DisplayInfo* Get(displayinfo_type* instance)
    {
        ASSERT(instance != nullptr);

        DisplayInfo* result(nullptr);

        std::list<DisplayInfo*>::iterator index(std::find(_clients.begin(), _clients.end(), reinterpret_cast<DisplayInfo*>(instance)));

        if (index != _clients.end()) {
            result = (*index);
        }

        ASSERT(result != nullptr)

        return result;
    }

public:
    void Release(displayinfo_type* instance)
    {
        DisplayInfo* displayinfo = Get(instance);
        if (displayinfo != nullptr) {
            displayinfo->Release();
        }
    }

    void Register(displayinfo_type* instance, displayinfo_updated_cb callback, void* userdata)
    {
        DisplayInfo* displayinfo = Get(instance);
        if (displayinfo != nullptr) {
            displayinfo->Register(callback, userdata);
        }
    }

    void Unregister(displayinfo_type* instance, displayinfo_updated_cb callback)
    {
        DisplayInfo* displayinfo = Get(instance);
        if (displayinfo != nullptr) {
            displayinfo->Unregister(callback);
        }
    }

    bool IsAudioPassthrough(displayinfo_type* instance)
    {
        DisplayInfo* displayinfo = Get(instance);
        return (displayinfo != nullptr) ? displayinfo->IsAudioPassthrough() : false;
    }

    bool Connected(displayinfo_type* instance)
    {
        DisplayInfo* displayinfo = Get(instance);
        return (displayinfo != nullptr) ? displayinfo->Connected() : false;
    }

    uint32_t Width(displayinfo_type* instance)
    {
        DisplayInfo* displayinfo = Get(instance);
        return (displayinfo != nullptr) ? displayinfo->Width() : 0;
    }

    uint32_t Height(displayinfo_type* instance)
    {
        DisplayInfo* displayinfo = Get(instance);
        return (displayinfo != nullptr) ? displayinfo->Height() : 0;
    }

    Exchange::IConnectionProperties::HDRType HDR(displayinfo_type* instance)
    {
        DisplayInfo* displayinfo = Get(instance);
        return (displayinfo != nullptr) ? displayinfo->HDR() : Exchange::IConnectionProperties::HDR_OFF;
    }

    Exchange::IConnectionProperties::HDCPProtectionType HDCPProtection(displayinfo_type* instance)
    {
        DisplayInfo* displayinfo = Get(instance);
        return (displayinfo != nullptr) ? displayinfo->HDCPProtection() : Exchange::IConnectionProperties::HDCP_Unencrypted;
    }

private:
    Core::ProxyType<RPC::CommunicatorClient> _comChannel;
    std::list<DisplayInfo*> _clients;
};

} // namespace WPEFramework

using namespace WPEFramework;

extern "C" {
bool displayinfo_enumerate(const uint8_t index, const uint8_t length, char* buffer)
{
    string temp;
    bool result(false);

    if (DisplayInfoAdministrator::Instance().Enumerate(index, temp) == true) {
        if (length > 0) {
            ASSERT(buffer != nullptr);
            ::strncpy(buffer, temp.c_str(), length);
        }
        result = true;
    }

    return result;
}

struct displayinfo_type* displayinfo_instance(const char displayName[])
{
    return reinterpret_cast<displayinfo_type*>(DisplayInfoAdministrator::Instance().Get(string(displayName)));
}

void displayinfo_release(struct displayinfo_type* displayinfo)
{
    DisplayInfoAdministrator::Instance().Release(displayinfo);
}

void displayinfo_register(struct displayinfo_type* displayinfo, displayinfo_updated_cb callback, void* userdata)
{
    DisplayInfoAdministrator::Instance().Register(displayinfo, callback, userdata);
}

void displayinfo_unregister(struct displayinfo_type* displayinfo, displayinfo_updated_cb callback)
{
    DisplayInfoAdministrator::Instance().Unregister(displayinfo, callback);
}

bool displayinfo_is_audio_passthrough(struct displayinfo_type* displayinfo)
{
    return DisplayInfoAdministrator::Instance().IsAudioPassthrough(displayinfo);
}

bool displayinfo_connected(struct displayinfo_type* displayinfo)
{
    return DisplayInfoAdministrator::Instance().Connected(displayinfo);
}

uint32_t displayinfo_width(struct displayinfo_type* displayinfo)
{
    return DisplayInfoAdministrator::Instance().Width(displayinfo);
}

uint32_t displayinfo_height(struct displayinfo_type* displayinfo)
{
    return DisplayInfoAdministrator::Instance().Height(displayinfo);
}

displayinfo_hdr_t displayinfo_hdr(struct displayinfo_type* displayinfo)
{
    displayinfo_hdr_t result = DISPLAYINFO_HDR_UNKNOWN;

    switch (DisplayInfoAdministrator::Instance().HDR(displayinfo)) {
    case Exchange::IConnectionProperties::HDR_OFF:
        result = DISPLAYINFO_HDR_OFF;
        break;
    case Exchange::IConnectionProperties::HDR_10:
        result = DISPLAYINFO_HDR_10;
        break;
    case Exchange::IConnectionProperties::HDR_10PLUS:
        result = DISPLAYINFO_HDR_10PLUS;
        break;
    case Exchange::IConnectionProperties::HDR_DOLBYVISION:
        result = DISPLAYINFO_HDR_DOLBYVISION;
        break;
    case Exchange::IConnectionProperties::HDR_TECHNICOLOR:
        result = DISPLAYINFO_HDR_TECHNICOLOR;
        break;
    default:
        result = DISPLAYINFO_HDR_UNKNOWN;
        break;
    }

    return result;
}

displayinfo_hdcp_protection_t displayinfo_hdcp_protection(struct displayinfo_type* displayinfo)
{

    displayinfo_hdcp_protection_t type = DISPLAYINFO_HDCP_UNKNOWN;

    switch (DisplayInfoAdministrator::Instance().HDCPProtection(displayinfo)) {
    case Exchange::IConnectionProperties::HDCP_Unencrypted:
        type = DISPLAYINFO_HDCP_UNENCRYPTED;
        break;
    case Exchange::IConnectionProperties::HDCP_1X:
        type = DISPLAYINFO_HDCP_1X;
        break;
    case Exchange::IConnectionProperties::HDCP_2X:
        type = DISPLAYINFO_HDCP_2X;
        break;
    default:
        type = DISPLAYINFO_HDCP_UNKNOWN;
        break;
    }

    return type;
}

} // extern "C"