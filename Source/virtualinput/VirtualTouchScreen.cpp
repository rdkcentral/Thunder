#include "VirtualTouchScreen.h"
#include "../core/core.h"

namespace WPEFramework {

namespace VirtualTouchScreen {

    class Controller {
    private:
        struct TouchData {
            touchactiontype Action;
            uint16_t Index;
            uint16_t X;
            uint16_t Y;
        };

        typedef Core::IPCMessageType<0, TouchData, Core::IPC::Void> TouchMessage;
        typedef Core::IPCMessageType<1, Core::IPC::Void, Core::IPC::Text<20>> NameMessage;

        Controller() = delete;
        Controller(const Controller&) = delete;
        Controller& operator=(const Controller&) = delete;

        class TouchEventHandler : public Core::IIPCServer {
        public:
            TouchEventHandler() = delete;
            TouchEventHandler(const TouchEventHandler&) = delete;
            TouchEventHandler& operator=(const TouchEventHandler&) = delete;

            TouchEventHandler(FNTouchEvent callback)
                : _callback(callback)
            {
            }

            virtual ~TouchEventHandler()
            {
            }

            virtual void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data)
            {
                Core::ProxyType<TouchMessage> message(data);
                ASSERT(_callback != nullptr);
                _callback(message->Parameters().Action, message->Parameters().Index, message->Parameters().X, message->Parameters().Y);
                source.ReportResponse(data);
            }

        private:
            FNTouchEvent _callback;
        };

        class NameEventHandler : public Core::IIPCServer {
        private:
            NameEventHandler() = delete;
            NameEventHandler(const NameEventHandler&) = delete;
            NameEventHandler& operator=(const NameEventHandler&) = delete;

        public:
            NameEventHandler(const string& name)
                : _name(name)
            {
            }
            virtual ~NameEventHandler()
            {
            }

        public:
            virtual void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data)
            {
                Core::ProxyType<NameMessage> message(data);
                message->Response() = _name;
                source.ReportResponse(data);
            }

        private:
            string _name;
        };

    public:
        Controller(const string& name, const Core::NodeId& source, FNTouchEvent callback)
            : _channel(source, 32)
        {
            _channel.CreateFactory<TouchMessage>(1);
            _channel.CreateFactory<NameMessage>(1);
            _channel.Register(TouchMessage::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<TouchEventHandler>::Create(callback)));
            _channel.Register(NameMessage::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<NameEventHandler>::Create(name)));
            _channel.Open(2000);
        }

        ~Controller()
        {
            _channel.Close(Core::infinite);
            _channel.Unregister(TouchMessage::Id());
            _channel.Unregister(NameMessage::Id());
            _channel.DestroyFactory<TouchMessage>();
            _channel.DestroyFactory<NameMessage>();
        }

    private:
        Core::IPCChannelClientType<Core::Void, false, true> _channel;
    };
}

}

#ifdef __cplusplus
extern "C" {
#endif

using namespace WPEFramework;

void* ConstructTouchScreen(const char listenerName[], const char connector[], FNTouchEvent callback)
{
    Core::NodeId remoteId(connector);
    return (new VirtualTouchScreen::Controller(listenerName, remoteId, callback));
}

void DestructTouchScreen(void* handle)
{
    delete reinterpret_cast<VirtualTouchScreen::Controller*>(handle);
}

#ifdef __cplusplus
}
#endif
