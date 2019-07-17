#include "VirtualMouse.h"
#include "../core/core.h"

namespace WPEFramework {

namespace VirtualMouse{

    class Controller {
    private:
        struct MouseData {
            mouseactiontype Action;
            uint16_t Button;
            int16_t Horizontal;
            int16_t Vertical;
        };

        typedef Core::IPCMessageType<0, MouseData, Core::IPC::Void> MouseMessage;
        typedef Core::IPCMessageType<1, Core::IPC::Void, Core::IPC::Text<20>> NameMessage;

        Controller() = delete;
        Controller(const Controller&) = delete;
        Controller& operator=(const Controller&) = delete;

        class MouseEventHandler : public Core::IIPCServer {
        public:
            MouseEventHandler() = delete;
            MouseEventHandler(const MouseEventHandler&) = delete;
            MouseEventHandler& operator=(const MouseEventHandler&) = delete;

            MouseEventHandler(FNMouseEvent callback)
                : _callback(callback)
            {
            }

            virtual ~MouseEventHandler()
            {
            }

            virtual void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data)
            {
                Core::ProxyType<MouseMessage> message(data);
                ASSERT(_callback != nullptr);
                _callback(message->Parameters().Action, message->Parameters().Button, message->Parameters().Horizontal, message->Parameters().Vertical);
                source.ReportResponse(data);
            }

        private:
            FNMouseEvent _callback;
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
        Controller(const string& name, const Core::NodeId& source, FNMouseEvent callback)
            : _channel(source, 32)
        {
            _channel.CreateFactory<MouseMessage>(1);
            _channel.CreateFactory<NameMessage>(1);
            _channel.Register(MouseMessage::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<MouseEventHandler>::Create(callback)));
            _channel.Register(NameMessage::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<NameEventHandler>::Create(name)));
            _channel.Open(2000);
        }

        ~Controller()
        {
            _channel.Close(Core::infinite);
            _channel.Unregister(MouseMessage::Id());
            _channel.Unregister(NameMessage::Id());
            _channel.DestroyFactory<MouseMessage>();
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

void* ConstructMouse(const char listenerName[], const char connector[], FNMouseEvent callback)
{
    Core::NodeId remoteId(connector);
    return (new VirtualMouse::Controller(listenerName, remoteId, callback));
}

void DestructMouse(void* handle)
{
    delete reinterpret_cast<VirtualMouse::Controller*>(handle);
}

#ifdef __cplusplus
}
#endif
