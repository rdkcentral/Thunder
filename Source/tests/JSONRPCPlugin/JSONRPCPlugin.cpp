#include "JSONRPCPlugin.h"

namespace WPEFramework {

	namespace Plugin {

		SERVICE_REGISTRATION(JSONRPCPlugin, 1, 0);

		JSONRPCPlugin::JSONRPCPlugin()
			: PluginHost::JSONRPC()
			, _job(Core::ProxyType<PeriodicSync>::Create(this))
		{
			// PluginHost::JSONRPC method to register a JSONRPC method invocation for the method "time".
			Register<string, string>(_T("time"), &JSONRPCPlugin::time, this);
		}

		/* virtual */ JSONRPCPlugin::~JSONRPCPlugin()
		{
		}

		/* virtual */ const string JSONRPCPlugin::Initialize(PluginHost::IShell* /* service */)
		{
			_job->Period(5);
			PluginHost::WorkerPool::Instance().Schedule(Core::Time::Now().Add(5000), _job);

			// On success return empty, to indicate there is no error text.
			return (string());
		}

		/* virtual */ void JSONRPCPlugin::Deinitialize(PluginHost::IShell* /* service */)
		{
			_job->Period(0);
			PluginHost::WorkerPool::Instance().Revoke(_job);
		}

		/* virtual */ string JSONRPCPlugin::Information() const
		{
			// No additional info to report.
			return (string());
		}

		void JSONRPCPlugin::SendTime() {
			// PluginHost::JSONRPC method to send out a JSONRPC message to all subscribers to the event "clock".
			Notify(_T("clock"), Core::JSON::String(Core::Time::Now().ToRFC1123()));
		}

	} // namespace Plugin

} // namespace WPEFramework
