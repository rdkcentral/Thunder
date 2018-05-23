#include "VirtualKeyboard.h"

#include "../core/core.h"

namespace WPEFramework {
	namespace VirtualKeyboard {

		class Controller {

		private:
			struct KeyData {
				actiontype Action;
				uint32_t Code;
			};
			typedef Core::IPCMessageType<0, KeyData, Core::IPC::Void> KeyMessage;
			typedef Core::IPCMessageType<1, Core::IPC::Void, Core::IPC::Text<20> > NameMessage;

			class KeyEventHandler : public Core::IPCServerType<KeyMessage> {
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

			public:
				virtual void Procedure(Core::IPCChannel& source, Core::ProxyType<KeyMessage>& data)
				{
					ASSERT(_callback != nullptr);
					_callback(data->Parameters().Action, data->Parameters().Code);
					Core::ProxyType<Core::IIPC> returnData(Core::proxy_cast<Core::IIPC>(data));
					source.ReportResponse(returnData);
				}

			private:
				FNKeyEvent _callback;
			};

			class NameEventHandler : public Core::IPCServerType<NameMessage> {
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
				virtual void Procedure(Core::IPCChannel& source, Core::ProxyType<NameMessage>& data)
				{
					TRACE_L1("In NameEventHandler::Procedure -- %d", __LINE__);

					data->Response() = _name;
					Core::ProxyType<Core::IIPC> returnData(Core::proxy_cast<Core::IIPC>(data));
					source.ReportResponse(returnData);
				}

			private:
				string _name;
			};

		private:
			Controller() = delete;
			Controller(const Controller&) = delete;
			Controller& operator=(const Controller&) = delete;

		public:
			Controller(const string& name, const Core::NodeId& source, FNKeyEvent callback)
				: _channel(source, 32)
				, _keyHandler(Core::ProxyType<KeyEventHandler>::Create(callback))
				, _nameHandler(Core::ProxyType<NameEventHandler>::Create(name))
			{
				_channel.CreateFactory<KeyMessage>(1);
				_channel.CreateFactory<NameMessage>(1);

				_channel.Register(_keyHandler);
				_channel.Register(_nameHandler);

				_channel.Open(2000); // Try opening this channel for 2S
			}
			~Controller()
			{
				_channel.Close(Core::infinite);

				_channel.Unregister(_keyHandler);
				_channel.Unregister(_nameHandler);

				_channel.DestroyFactory<KeyMessage>();
				_channel.DestroyFactory<NameMessage>();
			}

		private:
			Core::IPCChannelClientType<Core::Void, false, true> _channel;
			Core::ProxyType<Core::IIPCServer> _keyHandler;
			Core::ProxyType<Core::IIPCServer> _nameHandler;
		};
	}
}

#ifdef __cplusplus
extern "C" {
#endif

using namespace WPEFramework;

// Producer, Consumer, We produce the virtual keyboard, the receiver needs
// to destruct it once the done with the virtual keyboard.
// Use the Destruct, to destruct it.
void* Construct(const char listenerName[], const char connector[], FNKeyEvent callback)
{
    Core::NodeId remoteId(connector);

    return (new VirtualKeyboard::Controller(listenerName, remoteId, callback));
}

void Destruct(void* handle)
{
    delete reinterpret_cast<VirtualKeyboard::Controller*>(handle);
}

#ifdef __cplusplus
}
#endif
