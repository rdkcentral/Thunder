#include "virtualinput.h"
#include "IVirtualInput.h"

namespace WPEFramework {
namespace VirtualInput{

    class KeyEventHandler : public Core::IIPCServer {
    private:
        KeyEventHandler() = delete;
        KeyEventHandler(const KeyEventHandler&) = delete;
        KeyEventHandler& operator=(const KeyEventHandler&) = delete;

    public:
        KeyEventHandler(FNKeyEvent callback)
            : _callback(callback)
        {
        }
        virtual ~KeyEventHandler()
        {
        }
       
    private:
        virtual void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data)
        {
            Core::ProxyType<IVirtualInput::KeyMessage> message(Core::proxy_cast<IVirtualInput::KeyMessage>(data));
            ASSERT((_callback != nullptr) && (message.IsValid() == true));
            _callback(static_cast<keyactiontype>(message->Parameters().Action), message->Parameters().Code);
            source.ReportResponse(data);
        }

    private:
        FNKeyEvent _callback;
    };

    class MouseEventHandler : public Core::IIPCServer {
    private:
        MouseEventHandler() = delete;
        MouseEventHandler(const MouseEventHandler&) = delete;
        MouseEventHandler& operator=(const MouseEventHandler&) = delete;

    public:
        MouseEventHandler(FNMouseEvent callback)
            : _callback(callback)
        {
        }
        virtual ~MouseEventHandler()
        {
        }
       
    private:
        virtual void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data)
        {
            Core::ProxyType<IVirtualInput::MouseMessage> message(Core::proxy_cast<IVirtualInput::MouseMessage>(data));
            ASSERT((_callback != nullptr) && (message.IsValid() == true));
            _callback(static_cast<mouseactiontype>(message->Parameters().Action), message->Parameters().Button, message->Parameters().Horizontal, message->Parameters().Vertical);
            source.ReportResponse(data);
        }

    private:
        FNMouseEvent _callback;
    };

    class TouchEventHandler : public Core::IIPCServer {
    private:
        TouchEventHandler() = delete;
        TouchEventHandler(const TouchEventHandler&) = delete;
        TouchEventHandler& operator=(const TouchEventHandler&) = delete;

    public:
        TouchEventHandler(FNTouchEvent callback)
            : _callback(callback)
        {
        }
        virtual ~TouchEventHandler()
        {
        }
       
    private:
        virtual void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data)
        {
            Core::ProxyType<IVirtualInput::TouchMessage> message(Core::proxy_cast<IVirtualInput::TouchMessage>(data));
            ASSERT((_callback != nullptr) && (message.IsValid() == true));
            _callback(static_cast<touchactiontype>(message->Parameters().Action), message->Parameters().Index, message->Parameters().X, message->Parameters().Y);
            source.ReportResponse(data);
        }

    private:
        FNTouchEvent _callback;
    };

    class Controller {
    private:
        class NameEventHandler : public Core::IIPCServer {
        private:
            NameEventHandler() = delete;
            NameEventHandler(const NameEventHandler&) = delete;
            NameEventHandler& operator=(const NameEventHandler&) = delete;

        public:
            NameEventHandler(const string& name, const uint8_t mode)
                : _name(name)
                , _mode(mode)
            {
            }
            virtual ~NameEventHandler()
            {
            }

        public:
            virtual void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data)
            {
                TRACE_L1("In NameEventHandler::Procedure -- %d", __LINE__);

                Core::ProxyType<IVirtualInput::NameMessage> message(data);
                ::strncpy(message->Response().Name, _name.c_str(), sizeof(IVirtualInput::LinkInfo::Name));
                message->Response().Mode = _mode;
                source.ReportResponse(data);
            }

        private:
            string _name;
            uint8_t _mode;
        };

    private:
        Controller() = delete;
        Controller(const Controller&) = delete;
        Controller& operator=(const Controller&) = delete;

    public:
        Controller(const string& name, const Core::NodeId& source)
            : _channel(source, 32)
        {
            if (_keyCallback.IsValid() ==  true) {
                _channel.CreateFactory<IVirtualInput::KeyMessage>(1);
                _channel.Register(IVirtualInput::KeyMessage::Id(), _keyCallback);
            }

            if (_mouseCallback.IsValid() ==  true) {
                _channel.CreateFactory<IVirtualInput::MouseMessage>(1);
                _channel.Register(IVirtualInput::MouseMessage::Id(), _mouseCallback);
            }

            if (_touchCallback.IsValid() ==  true) {
                _channel.CreateFactory<IVirtualInput::TouchMessage>(1);
                _channel.Register(IVirtualInput::TouchMessage::Id(), _touchCallback);
            }

            _channel.CreateFactory<IVirtualInput::NameMessage>(1);
            _channel.Register(IVirtualInput::NameMessage::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<NameEventHandler>::Create(name, Mode())));

            _channel.Open(2000); // Try opening this channel for 2S
        }
        ~Controller()
        {
            _channel.Close(Core::infinite);

            if (_keyCallback.IsValid() == true) {
                _channel.Unregister(IVirtualInput::KeyMessage::Id());
                _channel.DestroyFactory<IVirtualInput::KeyMessage>();
            }

            if (_mouseCallback.IsValid() == true) {
                _channel.Unregister(IVirtualInput::MouseMessage::Id());
                _channel.DestroyFactory<IVirtualInput::MouseMessage>();
            }

            if (_touchCallback.IsValid() == true) {
                _channel.Unregister(IVirtualInput::TouchMessage::Id());
                _channel.DestroyFactory<IVirtualInput::TouchMessage>();
            }

            _channel.Unregister(IVirtualInput::NameMessage::Id());
            _channel.DestroyFactory<IVirtualInput::NameMessage>();
        }

        uint8_t Mode() const 
        {
            return (_keyCallback.IsValid()   ? IVirtualInput::INPUT_KEY   : 0) |
                   (_mouseCallback.IsValid() ? IVirtualInput::INPUT_MOUSE : 0) |
                   (_touchCallback.IsValid() ? IVirtualInput::INPUT_TOUCH : 0) ;
        }

        static void KeyHandler(FNKeyEvent callback) 
        {
            ASSERT ((_keyCallback.IsValid() == false) ^ (callback == nullptr));
           
            if (callback == nullptr) {
                _keyCallback.Release();
            }
            else { 
                _keyCallback = Core::ProxyType<Core::IIPCServer>(Core::ProxyType<KeyEventHandler>::Create(callback));
            }
        } 
        static void MouseHandler(FNMouseEvent callback) 
        {
            ASSERT ( (_mouseCallback.IsValid() == false) ^ (callback == nullptr) );
            
            if (callback == nullptr) {
                _mouseCallback.Release();
            }
            else { 
                _mouseCallback = Core::ProxyType<Core::IIPCServer>(Core::ProxyType<MouseEventHandler>::Create(callback));
            }
        } 
        static void TouchHandler(FNTouchEvent callback) 
        {
            ASSERT ( (_touchCallback.IsValid() == false) ^ (callback == nullptr) );
            
            if (callback == nullptr) {
                _touchCallback.Release();
            }
            else { 
                _touchCallback = Core::ProxyType<Core::IIPCServer>(Core::ProxyType<TouchEventHandler>::Create(callback));
            }
        } 
 
    private:
        Core::IPCChannelClientType<Core::Void, false, true> _channel;
        static Core::ProxyType<Core::IIPCServer> _keyCallback;
        static Core::ProxyType<Core::IIPCServer> _mouseCallback;
        static Core::ProxyType<Core::IIPCServer> _touchCallback;
    };
}
}

#ifdef __cplusplus
extern "C" {
#endif

using namespace WPEFramework;

void callback_keyboard(FNKeyEvent callback) 
{
    VirtualInput::Controller::KeyHandler(callback);
}

void callback_mouse(FNMouseEvent callback) 
{
    VirtualInput::Controller::MouseHandler(callback);
}

void callback_touch(FNTouchEvent callback)
{
    VirtualInput::Controller::TouchHandler(callback);
}

// Producer, Consumer, We produce the virtual keyboard, the receiver needs
// to destruct it once the done with the virtual keyboard.
// Use the Destruct, to destruct it.
void* virtualinput_open(const char listenerName[], const char connector[])
{
    Core::NodeId remoteId(connector);

    return (new VirtualInput::Controller(listenerName, remoteId));
}

void virtualinput_close(void* handle)
{
    delete reinterpret_cast<VirtualInput::Controller*>(handle);
}

#ifdef __cplusplus
}
#endif
