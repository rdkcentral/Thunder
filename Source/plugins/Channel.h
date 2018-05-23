#ifndef __PLUGIN_FRAMEWORK_CHANNEL__
#define __PLUGIN_FRAMEWORK_CHANNEL__

#include "Module.h"
#include "Request.h"

namespace WPEFramework {
namespace PluginHost {

    class EXTERNAL Channel : public Web::WebSocketLinkType<Core::SocketStream, Request, Web::Response, RequestPool&> {
    private:
        typedef Web::WebSocketLinkType<Core::SocketStream, Request, Web::Response, RequestPool&> BaseClass;

        union EXTERNAL Package {
        private:
            Package() = delete;
            Package(const Package&) = delete;
            Package& operator=(const Package&) = delete;

        public:
            explicit Package(const Core::ProxyType<Core::JSON::IElement>& json) : _json (json)
            {
            }
            explicit Package(const string& text) : _text(text)
            {
            }
            ~Package() {
            }

        public:
            const string& Text() const {
                return (_text);
            }
            const Core::ProxyType<Core::JSON::IElement>& JSON() const {
                return (_json);
            }

        private:
            Core::ProxyType<Core::JSON::IElement> _json;
            string _text;
        };
        class EXTERNAL SerializerImpl : public Core::JSON::IElement::Serializer {
        private:
            SerializerImpl(const SerializerImpl&) = delete;
            SerializerImpl& operator=(const SerializerImpl&) = delete;

        public:
            SerializerImpl()
            {
            }
            virtual ~SerializerImpl()
            {
            }

        public:
            inline void Submit(const Core::ProxyType<Core::JSON::IElement>& entry)
            {
                ASSERT (entry.IsValid() == true);

                _current = entry;
                Core::JSON::IElement::Serializer::Submit(*entry);
            }
            inline bool IsIdle() const {
                return (_current.IsValid() == false);
            }
        private:
            virtual void Serialized(const Core::JSON::IElement& element)
            {
                ASSERT(&(*(_current)) == &element);
                DEBUG_VARIABLE(element);

                _current.Release();
            }

        private:
            Core::ProxyType<const Core::JSON::IElement> _current;
        };
 
        class EXTERNAL DeserializerImpl : public Core::JSON::IElement::Deserializer {
        private:
            DeserializerImpl() = delete;
            DeserializerImpl(const DeserializerImpl&) = delete;
            DeserializerImpl& operator=(const DeserializerImpl&) = delete;

        public:
            DeserializerImpl(Channel& parent)
                : _parent(parent)
            {
            }
            virtual ~DeserializerImpl()
            {
            }

        public:
            inline bool IsIdle() const
            {
                return (_current.IsValid() == false);
            }

        private:
            virtual Core::JSON::IElement* Element(const string& identifier)
            {
                if (_parent.IsOpen() == true) {
                    _current = _parent.Element(identifier);

                    return (_current.IsValid() == true ? &(*_current) : nullptr);
                }
                return (nullptr);
            }
            virtual void Deserialized(Core::JSON::IElement& element)
            {
                ASSERT(&element == &(*_current));
                DEBUG_VARIABLE(element);

                _parent.Received(_current);

                _current.Release();
            }

        private:
            Channel& _parent;
            Core::ProxyType<Core::JSON::IElement> _current;
        };

        Channel() = delete;
        Channel(const Channel& copy) = delete;
        Channel& operator=(const Channel&) = delete;

    public:
        enum ChannelState {
            CLOSED = 0x01,
            WEB = 0x02,
            JSON = 0x04,
            RAW = 0x08,
            TEXT = 0x10
        };

    public:
        Channel(const SOCKET& connector, const Core::NodeId& remoteId);
        virtual ~Channel();

    public:
        inline bool HasActivity() const
        {
            bool forcedActivity((_state & 0x4000) == 0x4000);
            bool result(BaseClass::HasActivity());

            if ((forcedActivity == true) || ((result == false) && (IsWebSocket() == true))) {
                Lock();

                if (forcedActivity == true) {
                    _state &= (~0x4000);
                }
                else {
                    // We can try to fire a ping/pong
                    _state |= 0x4000;
                    const_cast<BaseClass*>(static_cast<const BaseClass*>(this))->Ping();
                    result = true;
                }

                Unlock();
            }

            return (result);
        }
        const string Name() const
        {
            return string(_nameOffset != static_cast<uint32_t>(~0) ? &(BaseClass::Path().c_str()[_nameOffset]) : EMPTY_STRING);
        }
        inline uint32_t Id() const
        {
            return (_ID);
        }
        inline ChannelState State() const
        {
            return static_cast<ChannelState>(_state & 0x0FFF);
        }
        inline bool IsNotified() const
        {
            return ((_state & 0x8000) != 0);
        }
        inline void Submit(const string& text)
        {
            if (IsOpen() == true) {

                _adminLock.Lock();

                _sendQueue.emplace_back(text);

                bool trigger = (_sendQueue.size() == 1);

                _adminLock.Unlock();

                if (trigger == true) {
                    BaseClass::Trigger();
                }
            }
        }
        inline void Submit(const Core::ProxyType<Core::JSON::IElement>& entry)
        {
            if (IsOpen() == true) {

                _adminLock.Lock();

                _sendQueue.emplace_back(entry);

                bool trigger = (_sendQueue.size() == 1);

                if (trigger == true) {
                    ASSERT(_serializer.IsIdle() == true);
                    _serializer.Submit(entry);
                }

                _adminLock.Unlock();

                if (trigger == true) {
                    BaseClass::Trigger();
                }
            }
        }
        inline void Submit(const Core::ProxyType<Web::Response>& entry)
        {
            BaseClass::Submit(entry);
        }
        inline void RequestOutbound()
        {
            BaseClass::Trigger();
        }

    protected:
        inline void SetId(const uint32_t id)
        {
            _ID = id;
        }
        inline void Lock() const
        {
            _adminLock.Lock();
        }
        inline void Unlock() const
        {
            _adminLock.Unlock();
        }
        inline void Properties(const uint32_t offset)
        {
            _nameOffset = offset;
        }
        inline void State(const ChannelState state, const bool notification)
        {
            Binary(state == RAW);
            _state = state | (notification ? 0x8000 : 0x0000);
        }
        inline uint16_t Serialize(uint8_t* dataFrame, const uint16_t maxSendSize)
        {
            uint16_t size = 0;

            if (_sendQueue.size() != 0) {

                switch(State()) {
                    case JSON:
                    {
                        // Seems we are sending JSON structs
                        size = _serializer.Serialize(dataFrame, maxSendSize);

                        if (_serializer.IsIdle() == true) {

                            // See if there is more to do..
                            _adminLock.Lock();
                            _sendQueue.pop_front();
							bool trigger (_sendQueue.size() > 0);
							
							if (trigger == true) {
                                _serializer.Submit(_sendQueue.front().JSON());
                            }
                            _adminLock.Unlock();

							if (trigger == true) {
								BaseClass::Trigger();
							}
                        }
						else {
							ASSERT(size != 0);
						}

                        break;
                    }
                    case TEXT:
                    {
                        // Seems we need to send plain strings...
                        Package& data (_sendQueue.front());
                        uint16_t neededBytes(static_cast<uint16_t>(data.Text().length() - _offset));

                        if (neededBytes <= maxSendSize) {
                            ::memcpy(dataFrame, &(data.Text().c_str()[_offset]), neededBytes);
                            size = neededBytes;
                            _offset = 0;

                            // See if there is more to do..
                            _adminLock.Lock();
                            _sendQueue.pop_front();
                            _adminLock.Unlock();
						}
                        else {
                            uint16_t addedBytes = maxSendSize - size;
                            ::memcpy(dataFrame, &(data.Text().c_str()[_offset]), addedBytes);
                            _offset += addedBytes;
                            size = addedBytes;
		    	        }

                        ASSERT(size != 0);

                        break;
                    }
                    case CLOSED: break;
                    case RAW:    ASSERT(false); break;
                    case WEB:    ASSERT(false); break;
                    default:     ASSERT(false); break;
                }
            }

            return (size);
        }
        inline uint16_t Deserialize(const uint8_t* dataFrame, const uint16_t receivedSize)
        {
            uint16_t handled = receivedSize;

            switch(State()) {
                case JSON:
                {
                    handled = _deserializer.Deserialize(dataFrame, receivedSize);
                    break;
                }
                case TEXT:
                {
                    _text += string(reinterpret_cast<const TCHAR*>(dataFrame), receivedSize);

                    if (BaseClass::IsCompleted() == true) {
                        Received(_text);
                        _text.clear();
                    }
                    break;
                }
                case CLOSED: break;
                case RAW:    ASSERT(false); break;
                case WEB:    ASSERT(false); break;
                default:     ASSERT(false); break;
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
        virtual void Received (const string& text) = 0;

        // Whenever there is a state change on the link, it is reported here.
        virtual void StateChange() = 0;

        virtual bool IsIdle() const
        {
            return ((BaseClass::IsWebSocket() == false) || ((_serializer.IsIdle() == true) && (_deserializer.IsIdle() == true)));
        }

    private:
        mutable Core::CriticalSection _adminLock;
        uint32_t _ID;
        uint32_t _nameOffset;
        mutable uint32_t _state;
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
