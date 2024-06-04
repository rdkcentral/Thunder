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

#ifndef __PLUGIN_FRAMEWORK_CHANNEL__
#define __PLUGIN_FRAMEWORK_CHANNEL__

#include "Module.h"
#include "Request.h"

namespace Thunder {
namespace PluginHost {

    class EXTERNAL Channel : public Web::WebSocketLinkType<Core::SocketStream, Request, Web::Response, RequestPool&> {
    private:
        typedef Web::WebSocketLinkType<Core::SocketStream, Request, Web::Response, RequestPool&> BaseClass;

        class EXTERNAL Package {
        public:
            Package() = delete;
            Package(const Package&) = delete;
            Package& operator=(const Package&) = delete;

            explicit Package(const Core::ProxyType<Core::JSON::IElement>& json)
                : _json(true)
                , _info(json)
            {
            }
            explicit Package(const string& text) 
                : _json(false)
                , _info(text)
            {
            }
            ~Package()
            {
                if (_json) {
                    _info.json.~ProxyType<Core::JSON::IElement>();                
                } else {
                    _info.text.~string();
                }
            }

        public:
            const string& Text() const
            {
                return (_info.text);
            }
            const Core::ProxyType<Core::JSON::IElement>& JSON() const
            {
                return (_info.json);
            }

        private:
            bool _json;
            union Info {
                Info(const Core::ProxyType<Core::JSON::IElement>& value)
                    : json(value)
                {
                }
                Info(const string& value)
                    : text(value)
                {
                }
                ~Info() {}

                Core::ProxyType<Core::JSON::IElement> json;
                string text;
            } _info;
        };
        class EXTERNAL SerializerImpl {
        public:
            SerializerImpl() = delete;
            SerializerImpl(const SerializerImpl&) = delete;
            SerializerImpl& operator=(const SerializerImpl&) = delete;

            SerializerImpl(Channel& parent)
                : _parent(parent)
                , _current()
                , _offset(0)
            {
            }
            ~SerializerImpl()
            {
            }

        public:
            bool IsIdle() const
            {
                return (_current.IsValid() == false);
            }
            uint16_t Serialize(char* stream, const uint16_t length) const {
                uint16_t loaded = 0;

                if (_current.IsValid() == false) {
                    _current = Core::ProxyType<const Core::JSON::IElement>(_parent.Element());
                }

                if (_current.IsValid() == true) {
                    loaded = _current->Serialize(stream, length, _offset);
                    if ( (_offset == 0) || (loaded != length) ) {
                        _current.Release();
                    }
#if THUNDER_PERFORMANCE
                    else {
			Core::ProxyType<const TrackingJSONRPC> tracking(_current);
                        ASSERT (tracking.IsValid() == true);
                        const_cast<TrackingJSONRPC&>(*tracking).Out(loaded);
                    }
#endif
                }

                return (loaded);
            }

        private:
            Channel& _parent;
            mutable Core::ProxyType<const Core::JSON::IElement> _current;
            mutable uint32_t _offset;
        };
        class EXTERNAL DeserializerImpl {
        public:
            DeserializerImpl() = delete;
            DeserializerImpl(const DeserializerImpl&) = delete;
            DeserializerImpl& operator=(const DeserializerImpl&) = delete;

            DeserializerImpl(Channel& parent)
                : _parent(parent)
                , _current()
                , _offset(0)
            {
            }
            ~DeserializerImpl()
            {
            }

        public:
            bool IsIdle() const
            {
                return (_current.IsValid() == false);
            }
            uint16_t Deserialize(const char* stream, const uint16_t length)
            {
                uint16_t loaded = 0;

                if (_current.IsValid() == false) {
                    if (_parent.IsOpen() == true) {
                        _current = _parent.Element(EMPTY_STRING);
                        _offset = 0;
                    }
                } 
                if (_current.IsValid() == true) {
                    loaded = _current->Deserialize(stream, length, _offset);
#if THUNDER_PERFORMANCE
		    Core::ProxyType<TrackingJSONRPC> tracking (_current);
                    ASSERT (tracking.IsValid() == true);
                    if(loaded > 0) {
                        tracking->In(loaded);
                    }
#endif
                    if ( (_offset == 0) || (loaded != length)) {
#if THUNDER_PERFORMANCE
                        tracking->In(0);
#endif
                        _parent.Received(_current);
                        _current.Release();
                    }
                }

				return (loaded);
            }

        private:
            Channel& _parent;
            Core::ProxyType<Core::JSON::IElement> _current;
            uint32_t _offset;
        };

    public:
        enum ChannelState : uint16_t {
            CLOSED = 0x01,
            WEB = 0x02,
            JSON = 0x04,
            RAW = 0x08,
            TEXT = 0x10,
            JSONRPC = 0x20,
            PINGED = 0x4000,
            NOTIFIED = 0x8000
        };

    public:
        Channel() = delete;
        Channel(const Channel& copy) = delete;
        Channel& operator=(const Channel&) = delete;
        Channel(const SOCKET& connector, const Core::NodeId& remoteId);
        ~Channel() override;

    public:
        bool HasActivity() const
        {
            Lock();

            bool result(BaseClass::HasActivity());

            // Check if we had a forced activity, if so than the result = the result no second chance;
            if ((_state & PINGED) == PINGED) {
                // Let clear it, if the activity succeeed, we are good to go, if not, too bad :-)
                _state &= (~PINGED);
                Unlock();
            } 
            else if ((result == true) || (IsWebSocket() == false)) {
                Unlock();
            } 
            else {
                // We are a websiocket and had no activity, time to send a ping..
                _state |= PINGED;
                Unlock();

                const_cast<BaseClass*>(static_cast<const BaseClass*>(this))->Ping();
                result = true;
            }

            return (result);
        }
        string Name() const
        {
            return string(_nameOffset != static_cast<uint32_t>(~0) ? &(BaseClass::Path().c_str()[_nameOffset]) : BaseClass::Path().c_str());
        }
        uint32_t Id() const
        {
            return (_ID);
        }
        ChannelState State() const
        {
            Lock();

            ChannelState result =  static_cast<ChannelState>(_state & 0x0FFF);

            Unlock();

            return (result);
        }
        bool IsNotified() const
        {
            return ((_state & NOTIFIED) != 0);
        }
        void Submit(const string& text)
        {
            if (IsOpen() == true) {

                BaseClass::Lock();

                _sendQueue.emplace_back(text);

                bool trigger = (_sendQueue.size() == 1);

                BaseClass::Unlock();

                if (trigger == true) {
                    BaseClass::Trigger();
                }
            }
        }
        void Submit(const Core::ProxyType<Core::JSON::IElement>& entry)
        {
            if (IsOpen() == true) {

                BaseClass::Lock();

                _sendQueue.emplace_back(entry);

                bool trigger = (_sendQueue.size() == 1);

                BaseClass::Unlock();

                if (trigger == true) {
                    BaseClass::Trigger();
                }
            }
        }
        void Submit(const Core::ProxyType<Web::Response>& entry)
        {
            BaseClass::Submit(entry);
        }
        void RequestOutbound()
        {
            BaseClass::Trigger();
        }

    protected:
        void SetId(const uint32_t id)
        {
            _ID = id;
        }
        void Lock() const 
		{
            BaseClass::Lock();
        }
        void Unlock() const 
		{
            BaseClass::Unlock();
        }
        void Properties(const uint32_t offset)
        {
            _nameOffset = offset;
        }
        void State(const ChannelState state, const bool notification)
        {
            BaseClass::Lock();

            Binary(state == RAW);

            _state = state | (notification ? NOTIFIED : 0x0000);

            BaseClass::Unlock();
        }
        uint16_t Serialize(uint8_t* dataFrame, const uint16_t maxSendSize)
        {
            uint16_t size = 0;

            switch (State()) {
            case JSON:
            case JSONRPC: {
                // Seems we are sending JSON structs
                size = _serializer.Serialize(reinterpret_cast<char*>(dataFrame), maxSendSize);

                if (_serializer.IsIdle() == false) {
                    ASSERT(size != 0);
                }
                else {
                    bool trigger = false;

                    // See if there is more to do..
                    BaseClass::Lock();

                    if (_sendQueue.size() != 0) {
                        _sendQueue.pop_front();
                        trigger = (_sendQueue.size() > 0);
                    }

                    BaseClass::Unlock();

                    if (trigger == true) {
                        BaseClass::Trigger();
                    }
                }

                break;
            }
            case TEXT: {
                // Seems we need to send plain strings...
                BaseClass::Lock();

                if (_sendQueue.size() != 0) {
                    Package& data(_sendQueue.front());
                    uint16_t neededBytes(static_cast<uint16_t>(data.Text().length() - _offset));

                    if (neededBytes <= maxSendSize) {
                        ::memcpy(dataFrame, &(data.Text().c_str()[_offset]), neededBytes);
                        size = neededBytes;
                        _offset = 0;

                        // See if there is more to do..
                        _sendQueue.pop_front();
                    }
                    else {
                        uint16_t addedBytes = maxSendSize - size;
                        ::memcpy(dataFrame, &(data.Text().c_str()[_offset]), addedBytes);
                        _offset += addedBytes;
                        size = addedBytes;
                    }

                    ASSERT(size != 0);
                }

                BaseClass::Unlock();

                break;
            }
            case CLOSED:
                break;
            case RAW:
                ASSERT(false);
                break;
            case WEB:
                ASSERT(false);
                break;
            default:
                ASSERT(false);
                break;
            }

            return (size);
        }
        uint16_t Deserialize(const uint8_t* dataFrame, const uint16_t receivedSize)
        {
            uint16_t handled = receivedSize;

            switch (State()) {
            case JSON:
            case JSONRPC: {
                handled = _deserializer.Deserialize(reinterpret_cast<const char*>(dataFrame), receivedSize);
                break;
            }
            case TEXT: {
                _text += string(reinterpret_cast<const TCHAR*>(dataFrame), receivedSize);

                if (BaseClass::IsCompleted() == true) {
                    Received(_text);
                    _text.clear();
                }
                break;
            }
            case CLOSED:
                break;
            case RAW:
                ASSERT(false);
                break;
            case WEB:
                ASSERT(false);
                break;
            default:
                ASSERT(false);
                break;
            }

            return (handled);
        }

    private:
        // Handle the WebRequest coming in.
        virtual void LinkBody(Core::ProxyType<Request>& request) = 0;
        virtual void Received(Core::ProxyType<Request>& request) = 0;
        virtual void Send(const Core::ProxyType<Web::Response>& response) = 0;

        // Handle the JSON structs flowing over the WebSocket.
        // [INBOUND]  Completed deserialized JSON objects that are Received, will trigger the Element/Received.
        // [OUTBOUND] Completed serialized JSON objects that are send out, will trigger the Send.
        virtual void Send(const Core::ProxyType<Core::JSON::IElement>& element) = 0;
        virtual Core::ProxyType<Core::JSON::IElement> Element(const string& identifier) = 0;
        virtual void Received(Core::ProxyType<Core::JSON::IElement>& element) = 0;

        // We are in an upgraded mode, we are a websocket. Time to "deserialize and serialize
        // INBOUND and OUTBOUND information.
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) = 0;
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) = 0;

        // If it is a WebSocket, protocol TEXT, This is the feeding back virtual
        virtual void Received(const string& text) = 0;

        // Whenever there is a state change on the link, it is reported here.
        virtual void StateChange() = 0;

        virtual bool IsIdle() const
        {
            return ((BaseClass::IsWebSocket() == false) || ((_serializer.IsIdle() == true) && (_deserializer.IsIdle() == true)));
        }
        Core::ProxyType<Core::JSON::IElement> Element() {
            Core::ProxyType<Core::JSON::IElement> result;

            BaseClass::Lock();

            if (_sendQueue.size() > 0) {
                result = _sendQueue.front().JSON();

            }
            BaseClass::Unlock();

            return (result);
        }

    private:
        uint32_t _ID;
        uint32_t _nameOffset;
        mutable uint16_t _state;
        SerializerImpl _serializer;
        DeserializerImpl _deserializer;
        string _text;
        uint32_t _offset;
        std::list<Package> _sendQueue;

        // All requests needed by any instance of this webserver are coming from this web server. They are extracted
        // from a pool. If the request is nolonger needed, the request returns to this pool.
        static RequestPool _requestAllocator;
    };
}
} // namespace Server

#endif // __PLUGIN_FRAMEWORK_CHANNEL__
