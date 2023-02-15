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

#pragma once

#include "Module.h"
#include "WebSocketLink.h"

namespace WPEFramework {

	namespace JSONRPC {

		using namespace Core::TypeTraits;

		template<typename INTERFACE>
		class LinkType {
		private:
			typedef std::function<void(const Core::JSONRPC::Message&)> CallbackFunction;

			class CommunicationChannel {
			private:
				// -----------------------------------------------------------------------------------------------
				// Create a resource allocator for all JSON objects used in these tests
				// -----------------------------------------------------------------------------------------------
				class FactoryImpl {
				private:
					FactoryImpl(const FactoryImpl&) = delete;
					FactoryImpl& operator=(const FactoryImpl&) = delete;

					class WatchDog {
					private:
						WatchDog() = delete;
						WatchDog& operator=(const WatchDog&) = delete;

					public:
						WatchDog(LinkType<INTERFACE>* client)
							: _client(client)
						{
						}
						WatchDog(const WatchDog& copy)
							: _client(copy._client)
						{
						}
						~WatchDog()
						{
						}

						bool operator==(const WatchDog& rhs) const
						{
							return (rhs._client == _client);
						}
						bool operator!=(const WatchDog& rhs) const
						{
							return (!operator==(rhs));
						}

					public:
						uint64_t Timed(const uint64_t scheduledTime) {
							return (_client->Timed());
						}

					private:
						LinkType<INTERFACE>* _client;
					};

					friend Core::SingletonType<FactoryImpl>;

					FactoryImpl()
						: _jsonRPCFactory(2)
						, _watchDog(Core::Thread::DefaultStackSize(), _T("JSONRPCCleaner"))
					{
					}

				public:
					static FactoryImpl& Instance()
					{
						static FactoryImpl& _singleton = Core::SingletonType<FactoryImpl>::Instance();

						return (_singleton);
					}


					~FactoryImpl()
					{
					}

				public:
					Core::ProxyType<Core::JSONRPC::Message> Element(const string&)
					{
						return (_jsonRPCFactory.Element());
					}
					void Trigger(const uint64_t& time, LinkType<INTERFACE>* client)
					{
						_watchDog.Trigger(time, client);
					}
					void Revoke(LinkType<INTERFACE>* client)
					{
						_watchDog.Revoke(client);
					}

				private:
					Core::ProxyPoolType<Core::JSONRPC::Message> _jsonRPCFactory;
					Core::TimerType<WatchDog> _watchDog;
				};

				class ChannelImpl : public Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, FactoryImpl&, INTERFACE> {
				private:
					ChannelImpl(const ChannelImpl&) = delete;
					ChannelImpl& operator=(const ChannelImpl&) = delete;

					typedef Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, FactoryImpl&, INTERFACE> BaseClass;

				public:
					ChannelImpl(CommunicationChannel* parent, const Core::NodeId& remoteNode, const string& callsign, const string& query)
						: BaseClass(5, FactoryImpl::Instance(), callsign, _T("JSON"), query, "", false, false, false, remoteNode.AnyInterface(), remoteNode, 256, 256)
						, _parent(*parent)
					{
					}
					~ChannelImpl() override = default;

				public:
					virtual void Received(Core::ProxyType<INTERFACE>& jsonObject) override
					{
						Core::ProxyType<Core::JSONRPC::Message> inbound(jsonObject);

						ASSERT(inbound.IsValid() == true);

						if (inbound.IsValid() == true) {
							_parent.Inbound(inbound);
						}
					}
					virtual void Send(Core::ProxyType<INTERFACE>& jsonObject) override
					{
#ifdef __DEBUG__
						string message;
						ToMessage(jsonObject, message);
						TRACE_L1("Message: %s send", message.c_str());
#endif
					}
					virtual void StateChange() override
					{
						_parent.StateChange();
					}
					virtual bool IsIdle() const
					{
						return (true);
					}

				private:
					void ToMessage(const Core::ProxyType<Core::JSON::IElement>& jsonObject, string& message) const
					{
						Core::ProxyType<Core::JSONRPC::Message> inbound(jsonObject);

						ASSERT(inbound.IsValid() == true);
						if (inbound.IsValid() == true) {
							inbound->ToString(message);
						}
					}
					void ToMessage(const Core::ProxyType<Core::JSON::IMessagePack>& jsonObject, string& message) const
					{
						Core::ProxyType<Core::JSONRPC::Message> inbound(jsonObject);

						ASSERT(inbound.IsValid() == true);
						if (inbound.IsValid() == true) {
							std::vector<uint8_t> values;
							inbound->ToBuffer(values);
							if (values.empty() != true) {
								Core::ToString(values.data(), static_cast<uint16_t>(values.size()), false, message);
							}
						}
					}

				private:
					CommunicationChannel& _parent;
				};

			protected:
				CommunicationChannel(const Core::NodeId& remoteNode, const string& callsign, const string& query)
					: _channel(this, remoteNode, callsign, query)
					, _sequence(0)
				{
				}

			public:
				virtual ~CommunicationChannel() = default;
				static Core::ProxyType<CommunicationChannel> Instance(const Core::NodeId& remoteNode, const string& callsign, const string& query)
				{
					static Core::ProxyMapType<string, CommunicationChannel> channelMap;

					string searchLine = remoteNode.HostAddress() + '@' + callsign;

					return (channelMap.template Instance<CommunicationChannel>(searchLine, remoteNode, callsign, query));
				}

			public:
				static void Trigger(const uint64_t& time, LinkType<INTERFACE>* client)
				{
					FactoryImpl::Instance().Trigger(time, client);
				}
				static Core::ProxyType<Core::JSONRPC::Message> Message()
				{
					return (FactoryImpl::Instance().Element(string()));
				}
				uint32_t Sequence() const
				{
					return (++_sequence);
				}
				void Register(LinkType<INTERFACE>& client)
				{
					_adminLock.Lock();
					ASSERT(std::find(_observers.begin(), _observers.end(), &client) == _observers.end());
					_observers.push_back(&client);
					if (_channel.IsOpen() == true) {
						client.Opened();
					}
					_adminLock.Unlock();
				}
				void Unregister(LinkType<INTERFACE>& client)
				{
					_adminLock.Lock();
					typename std::list<LinkType<INTERFACE>* >::iterator index(std::find(_observers.begin(), _observers.end(), &client));
					if (index != _observers.end()) {
						_observers.erase(index);
					}
					FactoryImpl::Instance().Revoke(&client);
					_adminLock.Unlock();
				}
				void Submit(const Core::ProxyType<INTERFACE>& message)
				{
					_channel.Submit(message);
				}
				bool IsSuspended() const
				{
					return (_channel.IsSuspended());
				}
				uint32_t Initialize()
				{
					return (Open(0));
				}
				void Deinitialize()
				{
					Close();
				}

			protected:
				void StateChange()
				{
					_adminLock.Lock();
					typename std::list<LinkType<INTERFACE>* >::iterator index(_observers.begin());
					while (index != _observers.end()) {
						if (_channel.IsOpen() == true) {
							(*index)->Opened();
						}
						else {
							(*index)->Closed();
						}
						index++;
					}
					_adminLock.Unlock();
				}
				bool Open(const uint32_t waitTime)
				{
					bool result = true;
					if (_channel.IsClosed() == true) {
						result = (_channel.Open(waitTime) == Core::ERROR_NONE);
					}
					return (result);
				}
				void Close()
				{
					_channel.Close(Core::infinite);
				}

			private:
				uint32_t Inbound(const Core::ProxyType<Core::JSONRPC::Message>& inbound)
				{
					uint32_t result = Core::ERROR_UNAVAILABLE;
					_adminLock.Lock();
					typename std::list<LinkType<INTERFACE>*>::iterator index(_observers.begin());
					while ((result != Core::ERROR_NONE) && (index != _observers.end())) {
						result = (*index)->Inbound(inbound);
						index++;
					}
					_adminLock.Unlock();

					return (result);
				}

			private:
				Core::CriticalSection _adminLock;
				ChannelImpl _channel;
				mutable std::atomic<uint32_t> _sequence;
				std::list< LinkType<INTERFACE>*> _observers;
			};
			class Entry {
			private:
				Entry(const Entry&) = delete;
				Entry& operator=(const Entry&) = delete;
				struct Synchronous {
					Synchronous()
						: _signal(false, true)
						, _response()
					{
					}
					Core::Event _signal;
					Core::ProxyType<Core::JSONRPC::Message> _response;
				};
				struct ASynchronous {
					ASynchronous(const uint32_t waitTime, const CallbackFunction& completed)
						: _waitTime(Core::Time::Now().Add(waitTime).Ticks())
						, _completed(completed)
					{
					}
					uint64_t _waitTime;
					CallbackFunction _completed;
				};

			public:
				Entry()
					: _synchronous(true)
					, _info()
				{
				}
				Entry(const uint32_t waitTime, const CallbackFunction& completed)
					: _synchronous(false)
					, _info(waitTime, completed)
				{
				}
				~Entry()
				{
					if (_synchronous == true) {
						_info.sync.~Synchronous();
					}
					else {
						_info.async.~ASynchronous();
					}
				}

			public:
				const Core::ProxyType<Core::JSONRPC::Message>& Response() const
				{
					return (_info.sync._response);
				}
				bool Signal(const Core::ProxyType<Core::JSONRPC::Message>& response)
				{
					if (_synchronous == true) {
						_info.sync._response = response;
						_info.sync._signal.SetEvent();
					}
					else {
						_info.async._completed(*response);
					}

					return (_synchronous == false);
				}
				const uint64_t& Expiry() const
				{
					return (_info.async._waitTime);
				}
				void Abort(const uint32_t id)
				{
					if (_synchronous == true) {
						_info.sync._signal.SetEvent();
					}
					else {
						Core::JSONRPC::Message message;
						message.Id = id;
						message.Error.Code = Core::ERROR_ASYNC_ABORTED;
						message.Error.Text = _T("Pending call has been aborted");
						_info.async._completed(message);
					}
				}
				bool Expired(const uint32_t id, const uint64_t& currentTime, uint64_t& nextTime)
				{
					bool expired = false;

					if (_synchronous == false) {
						if (_info.async._waitTime > currentTime) {
							if (_info.async._waitTime < nextTime) {
								nextTime = _info.async._waitTime;
							}
						}
						else {
							Core::JSONRPC::Message message;
							message.Id = id;
							message.Error.Code = Core::ERROR_TIMEDOUT;
							message.Error.Text = _T("Pending a-sync call has timed out");
							_info.async._completed(message);
							expired = true;
						}
					}
					return (expired);
				}
				bool WaitForResponse(const uint32_t waitTime)
				{
					return (_info.sync._signal.Lock(waitTime) == Core::ERROR_NONE);
				}

			private:
				bool _synchronous;
				union Info {
				public:
					Info()
						: sync()
					{
					}
					Info(const uint32_t waitTime, const CallbackFunction& completed)
						: async(waitTime, completed)
					{
					}
					~Info()
					{
					}
					Synchronous sync;
					ASynchronous async;
				} _info;
			};
			static Core::NodeId RemoteNodeId()
			{
				Core::NodeId result;
				string remoteNodeId;
				if ((Core::SystemInfo::GetEnvironment(_T("THUNDER_ACCESS"), remoteNodeId) == true) && (remoteNodeId.empty() == false)) {
					result = Core::NodeId(remoteNodeId.c_str());
				}
				return (result);
			}
			static uint8_t DetermineVersion(const string& designator)
			{
				uint8_t version = (designator.length() > 1 ? Core::JSONRPC::Message::Version(designator) : ~0);
				return (version == static_cast<uint8_t>(~0) ? 1 : version);
			}

			using PendingMap = std::unordered_map<uint32_t, Entry>;
			using InvokeFunction = Core::JSONRPC::InvokeFunction;

		protected:
			static constexpr uint32_t DefaultWaitTime = 10000;

			LinkType(const string& callsign, const string connectingCallsign, const TCHAR* localCallsign, const string& query)
				: _adminLock()
				, _connectId(RemoteNodeId())
				, _channel(CommunicationChannel::Instance(_connectId, string("/jsonrpc/") + connectingCallsign, query))
				, _handler({ DetermineVersion(callsign) })
				, _callsign(callsign.empty() ? string() : Core::JSONRPC::Message::Callsign(callsign + '.'))
				, _localSpace()
				, _pendingQueue()
				, _scheduledTime(0)
				, _versionstring()
			{
				if (localCallsign == nullptr) {
					static uint32_t sequence;
					_localSpace = string("temporary") + Core::NumberType<uint32_t>(Core::InterlockedIncrement(sequence)).Text();
				}
				else {
					_localSpace = localCallsign;
				}

				uint8_t version = Core::JSONRPC::Message::Version(callsign + '.');
				if (version != static_cast<uint8_t>(~0)) {
					_versionstring = '.' + Core::NumberType<uint8_t>(version).Text();
				}
			}
			void Announce() {
				_channel->Register(*this);
			}

		public:
			LinkType() = delete;
			LinkType(const LinkType&) = delete;
			LinkType& operator=(LinkType&) = delete;

			LinkType(const string& callsign, const bool directed = false, const string& query = "")
				: LinkType(callsign, (directed ? callsign : string()), nullptr, query)
			{
				_channel->Register(*this);
			}
			LinkType(const string& callsign, const TCHAR localCallsign[], const bool directed = false, const string& query = "")
				: LinkType(callsign, (directed ? callsign : string()), localCallsign, query)
			{
				_channel->Register(*this);
			}
			virtual ~LinkType()
			{
				_channel->Unregister(*this);

				for (auto& element : _pendingQueue) {
					element.second.Abort(element.first);
				}
			}

		public:
			const string& Namespace() const
			{
				return (_localSpace);
			}
			const string& Callsign() const
			{
				return (_callsign);
			}
			Core::JSONRPC::Handler::EventIterator Events() const
			{
				return (_handler.Events());
			}
			template <typename INBOUND, typename METHOD>
			void Assign(const string& eventName, const METHOD& method)
			{
				std::function<void(const INBOUND& parameters)> actualMethod = method;
				InvokeFunction implementation = [actualMethod](const Core::JSONRPC::Context&, const string&, const string& parameters, string& result) -> uint32_t {
					INBOUND inbound;
					inbound.FromString(parameters);
					actualMethod(inbound);
					result.clear();
					return (Core::ERROR_NONE);
				};

				_handler.Register(eventName, implementation);
			}
			template <typename INBOUND, typename METHOD, typename REALOBJECT>
			void Assign(const string& eventName, const METHOD& method, REALOBJECT* objectPtr)
			{
				// using INBOUND = typename Core::TypeTraits::func_traits<METHOD>::template argument<0>::type;
				std::function<void(INBOUND parameters)> actualMethod = std::bind(method, objectPtr, std::placeholders::_1);
				InvokeFunction implementation = [actualMethod](const Core::JSONRPC::Context&, const string&, const string& parameters, string& result) -> uint32_t {
					INBOUND inbound;
					inbound.FromString(parameters);
					actualMethod(inbound);
					result.clear();
					return (Core::ERROR_NONE);
				};
				_handler.Register(eventName, implementation);
			}
			void Revoke(const string& eventName)
			{
				_handler.Unregister(eventName);
			}
			template <typename INBOUND, typename METHOD>
			uint32_t Subscribe(const uint32_t waitTime, const string& eventName, const METHOD& method)
			{
				Assign<INBOUND, METHOD>(eventName, method);

				const string parameters("{ \"event\": \"" + eventName + "\", \"id\": \"" + _localSpace + "\"}");
				Core::ProxyType<Core::JSONRPC::Message> response;

				uint32_t result = Send(waitTime, "register", parameters, response);

				if ((result != Core::ERROR_NONE) || (response.IsValid() == false) || (response->Error.IsSet() == true)) {
					_handler.Unregister(eventName);
					if ((result == Core::ERROR_NONE) && (response->Error.IsSet() == true)) {
						result = response->Error.Code.Value();
					}
				}

				return (result);
			}
			template <typename INBOUND, typename METHOD, typename REALOBJECT>
			uint32_t Subscribe(const uint32_t waitTime, const string& eventName, const METHOD& method, REALOBJECT* objectPtr)
			{
				Assign<INBOUND, METHOD, REALOBJECT>(eventName, method, objectPtr);
				const string parameters("{ \"event\": \"" + eventName + "\", \"id\": \"" + _localSpace + "\"}");
				Core::ProxyType<Core::JSONRPC::Message> response;

				uint32_t result = Send(waitTime, "register", parameters, response);

				if ((result != Core::ERROR_NONE) || (response.IsValid() == false) || (response->Error.IsSet() == true)) {
					_handler.Unregister(eventName);
					if ((result == Core::ERROR_NONE) && (response->Error.IsSet() == true)) {
						result = response->Error.Code.Value();
					}
				}

				return (result);
			}
			void Unsubscribe(const uint32_t waitTime, const string& eventName)
			{
				const string parameters("{ \"event\": \"" + eventName + "\", \"id\": \"" + _localSpace + "\"}");
				Core::ProxyType<Core::JSONRPC::Message> response;

				Send(waitTime, "unregister", parameters, response);

				_handler.Unregister(eventName);
			}

			template <typename PARAMETERS, typename RESPONSE>
			typename std::enable_if<(!std::is_same<PARAMETERS, void>::value && !std::is_same<RESPONSE, void>::value), uint32_t>::type
				Invoke(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, RESPONSE& inbound)
			{
				return InternalInvoke<PARAMETERS>(waitTime, method, parameters, inbound);
			}

			template <typename PARAMETERS, typename RESPONSE>
			typename std::enable_if<(std::is_same<PARAMETERS, void>::value&& std::is_same<RESPONSE, void>::value), uint32_t>::type
				Invoke(const uint32_t waitTime, const string& method)
			{
				string emptyString(EMPTY_STRING);
				return InternalInvoke<string>(waitTime, method, emptyString);
			}

			template <typename PARAMETERS, typename RESPONSE>
			typename std::enable_if<(!std::is_same<PARAMETERS, void>::value&& std::is_same<RESPONSE, void>::value), uint32_t>::type
				Invoke(const uint32_t waitTime, const string& method, const PARAMETERS& parameters)
			{
				return InternalInvoke<PARAMETERS>(waitTime, method, parameters);
			}

			template <typename PARAMETERS, typename RESPONSE>
			typename std::enable_if<(std::is_same<PARAMETERS, void>::value && !std::is_same<RESPONSE, void>::value), uint32_t>::type
				Invoke(const uint32_t waitTime, const string& method, RESPONSE& inbound)
			{
				string emptyString(EMPTY_STRING);
				return InternalInvoke<string>(waitTime, method, emptyString, inbound);
			}

			template <typename PARAMETERS, typename HANDLER>
			typename std::enable_if<(std::is_same<PARAMETERS, void>::value&& std::is_same<typename Core::TypeTraits::func_traits<HANDLER>::classtype, void>::value), uint32_t>::type
				Dispatch(const uint32_t waitTime, const string& method, const HANDLER& callback)
			{
				using ERRORCODE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<1>::type;

				string emptyString(EMPTY_STRING);
				return (InternalInvoke<string, HANDLER>(
					::TemplateIntToType<std::is_same<ERRORCODE, Core::JSONRPC::Error*>::value>(),
					waitTime,
					method,
					emptyString,
					callback));
			}
			template <typename PARAMETERS, typename HANDLER>
			inline typename std::enable_if<!(std::is_same<PARAMETERS, void>::value&& std::is_same<typename Core::TypeTraits::func_traits<HANDLER>::classtype, void>::value), uint32_t>::type
				Dispatch(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, const HANDLER& callback)
			{
				using ERRORCODE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<1>::type;

				return (InternalInvoke<PARAMETERS, HANDLER>(
					::TemplateIntToType<std::is_same<ERRORCODE, Core::JSONRPC::Error*>::value>(),
					waitTime,
					method,
					parameters,
					callback));
			}
			template <typename PARAMETERS, typename HANDLER, typename REALOBJECT = typename Core::TypeTraits::func_traits<HANDLER>::classtype>
			typename std::enable_if<(std::is_same<PARAMETERS, void>::value && !std::is_same<typename Core::TypeTraits::func_traits<HANDLER>::classtype, void>::value), uint32_t>::type
				Dispatch(const uint32_t waitTime, const string& method, const HANDLER& callback, REALOBJECT* objectPtr)
			{
				using ERRORCODE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<1>::type;

				string emptyString(EMPTY_STRING);
				return (InternalInvoke<string, HANDLER, REALOBJECT>(
					::TemplateIntToType<std::is_same<ERRORCODE, Core::JSONRPC::Error*>::value>(),
					waitTime,
					method,
					emptyString,
					callback,
					objectPtr));
			}
			template <typename PARAMETERS, typename HANDLER, typename REALOBJECT = typename Core::TypeTraits::func_traits<HANDLER>::classtype>
			inline typename std::enable_if<!(std::is_same<PARAMETERS, void>::value && !std::is_same<typename Core::TypeTraits::func_traits<HANDLER>::classtype, void>::value), uint32_t>::type
				Dispatch(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, const HANDLER& callback, REALOBJECT* objectPtr)
			{
				using ERRORCODE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<1>::type;

				return (InternalInvoke<PARAMETERS, HANDLER, REALOBJECT>(
					::TemplateIntToType<std::is_same<ERRORCODE, Core::JSONRPC::Error*>::value>(),
					waitTime,
					method,
					parameters,
					callback,
					objectPtr));
			}
			template <typename PARAMETERS, typename... TYPES>
			uint32_t Set(const uint32_t waitTime, const string& method, const TYPES&&... args)
			{
				PARAMETERS sendObject(args...);
				return (Set<PARAMETERS>(waitTime, method, sendObject));
			}
			template <typename PARAMETERS>
			uint32_t Set(const uint32_t waitTime, const string& method, const string& index, const PARAMETERS& sendObject)
			{
				string fullMethod = method + '@' + index;
				return (Set<PARAMETERS>(waitTime, fullMethod, sendObject));
			}
			template <typename PARAMETERS, typename NUMBER>
			uint32_t Set(const uint32_t waitTime, const string& method, const NUMBER index, const PARAMETERS& sendObject)
			{
				string fullMethod = method + '@' + Core::NumberType<NUMBER>(index).Text();
				return (Set<PARAMETERS>(waitTime, fullMethod, sendObject));
			}
			template <typename PARAMETERS>
			uint32_t Set(const uint32_t waitTime, const string& method, const PARAMETERS& sendObject)
			{
				Core::ProxyType<Core::JSONRPC::Message> response;
				uint32_t result = Send(waitTime, method, sendObject, response);
				if ((result == Core::ERROR_NONE) && (response->Error.IsSet() == true)) {
					result = response->Error.Code.Value();
				}
				return (result);
			}
			template <typename PARAMETERS>
			uint32_t Get(const uint32_t waitTime, const string& method, const string& index, PARAMETERS& sendObject)
			{
				string fullMethod = method + '@' + index;
				return (Get<PARAMETERS>(waitTime, fullMethod, sendObject));
			}
			template <typename PARAMETERS, typename NUMBER>
			uint32_t Get(const uint32_t waitTime, const string& method, const NUMBER& index, PARAMETERS& sendObject)
			{
				string fullMethod = method + '@' + Core::NumberType<NUMBER>(index).Text();
				return (Get<PARAMETERS>(waitTime, fullMethod, sendObject));
			}
			template <typename PARAMETERS>
			uint32_t Get(const uint32_t waitTime, const string& method, PARAMETERS& sendObject)
			{
				Core::ProxyType<Core::JSONRPC::Message> response;
				string emptyString(EMPTY_STRING);
				uint32_t result = Send(waitTime, method, emptyString, response);
				if (result == Core::ERROR_NONE) {
					if (response->Error.IsSet() == true) {
						result = response->Error.Code.Value();
					}
					else if ((response->Result.IsSet() == true) && (response->Result.Value().empty() == false)) {
						sendObject.Clear();
						FromMessage((INTERFACE*)&sendObject, *response);
					}
				}
				return (result);
			}
			uint32_t Invoke(const uint32_t waitTime, const string& method, const string& parameters, Core::ProxyType<Core::JSONRPC::Message>& response)
			{
				return (Send(waitTime, method, parameters, response));
			}

			// Generic JSONRPC methods.
			// Anything goes!
			// these objects have no type chacking, will consume more memory and processing takes more time
			// Advice: Use string typed variants above!!!
			// =====================================================================================================
			uint32_t Invoke(const char method[], const Core::JSON::VariantContainer& parameters, Core::JSON::VariantContainer& response, const uint32_t waitTime = DefaultWaitTime)
			{
				return (Invoke<Core::JSON::VariantContainer, Core::JSON::VariantContainer>(waitTime, method, parameters, response));
			}
			uint32_t SetProperty(const char method[], const Core::JSON::VariantContainer& object, const uint32_t waitTime = DefaultWaitTime)
			{
				return (Set<Core::JSON::VariantContainer>(waitTime, method, object));
			}
			uint32_t GetProperty(const char method[], Core::JSON::VariantContainer& object, const uint32_t waitTime = DefaultWaitTime)
			{
				return (Get<Core::JSON::VariantContainer>(waitTime, method, object));
			}
			template <typename RESPONSE = Core::JSON::VariantContainer>
			DEPRECATED uint32_t Invoke(const uint32_t waitTime, const string& method, RESPONSE& inbound)
				// Note: use of Invoke without indicating both Parameters and Response type is deprecated -> replace this one by Invoke<void, ResponeType>(..
			{
				string emptyString(EMPTY_STRING);
				return InternalInvoke<string>(waitTime, method, emptyString, inbound);
			}
			//template <typename PARAMETERS = Core::JSON::VariantContainer, typename RESPONSE = Core::JSON::VariantContainer>
			template <typename PARAMETERS = Core::JSON::VariantContainer>
			DEPRECATED uint32_t Invoke(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, Core::JSON::VariantContainer& inbound)
				// Note: use of Invoke without indicating both Parameters and Response type is deprecated -> replace this one by Invoke<PARAMETER type, Core::JSON::VariantContainer>(..
			{
				return InternalInvoke<PARAMETERS>(waitTime, method, parameters, inbound);
			}

		private:
			friend CommunicationChannel;

			uint64_t Timed()
			{
				uint64_t result = ~0;
				uint64_t currentTime = Core::Time::Now().Ticks();

				// Lets see if some callback are expire. If so trigger and remove...
				_adminLock.Lock();

				typename PendingMap::iterator index = _pendingQueue.begin();

				while (index != _pendingQueue.end()) {

					if (index->second.Expired(index->first, currentTime, result) == true) {
						index = _pendingQueue.erase(index);
					}
					else {
						index++;
					}
				}
				_scheduledTime = (result != static_cast<uint64_t>(~0) ? result : 0);

				_adminLock.Unlock();

				return (_scheduledTime);
			}
			template <typename PARAMETERS, typename RESPONSE>
			uint32_t InternalInvoke(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, RESPONSE& inbound)
			{
				Core::ProxyType<Core::JSONRPC::Message> response;
				uint32_t result = Send(waitTime, method, parameters, response);
				if (result == Core::ERROR_NONE) {
					if (response->Error.IsSet() == true) {
						result = response->Error.Code.Value();
					}
					else if ((response->Result.IsSet() == true)
						&& (response->Result.Value().empty() == false)) {
						FromMessage((INTERFACE*)&inbound, *response);
					}
				}

				return (result);
			}
			template <typename PARAMETERS>
			uint32_t InternalInvoke(const uint32_t waitTime, const string& method, const PARAMETERS& parameters)
			{
				Core::ProxyType<Core::JSONRPC::Message> response;
				uint32_t result = Send(waitTime, method, parameters, response);
				if (result == Core::ERROR_NONE) {
					if (response->Error.IsSet() == true) {
						result = response->Error.Code.Value();
					}
				}
				return (result);
			}
			template <typename PARAMETERS, typename HANDLER>
			uint32_t InternalInvoke(const ::TemplateIntToType<0>&, const uint32_t waitTime, const string& method, const PARAMETERS& parameters, const HANDLER& callback)
			{
				using RESPONSE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<0>::type;

				CallbackFunction implementation = [callback, this](const Core::JSONRPC::Message& inbound) -> void {
					typename std::remove_const<typename std::remove_reference<RESPONSE>::type>::type response;
					if (inbound.Error.IsSet() == false) {
						FromMessage((INTERFACE*)&response, inbound);
					}
					callback(response);
				};

				uint32_t result = Send(waitTime, method, parameters, implementation);
				return (result);
			}
			template <typename PARAMETERS, typename HANDLER>
			uint32_t InternalInvoke(const ::TemplateIntToType<1>&, const uint32_t waitTime, const string& method, const PARAMETERS& parameters, const HANDLER& callback)
			{
				using RESPONSE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<0>::type;

				CallbackFunction implementation = [callback, this](const Core::JSONRPC::Message& inbound) -> void {
					typename std::remove_const<typename std::remove_reference<RESPONSE>::type>::type response;

					if (inbound.Error.IsSet() == false) {
						FromMessage((INTERFACE*)&response, inbound);
						callback(response, nullptr);
					}
					else {
						callback(response, &(response.Error));
					}
				};

				uint32_t result = Send(waitTime, method, parameters, implementation);
				return (result);
			}
			template <typename PARAMETERS, typename HANDLER, typename REALOBJECT>
			uint32_t InternalInvoke(const ::TemplateIntToType<1>&, const uint32_t waitTime, const string& method, const PARAMETERS& parameters, const HANDLER& callback, REALOBJECT* objectPtr)
			{
				using RESPONSE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<0>::type;

				std::function<void(RESPONSE)> actualMethod = std::bind(callback, objectPtr, std::placeholders::_1);
				CallbackFunction implementation = [actualMethod, this](const Core::JSONRPC::Message& inbound) -> void {
					typename std::remove_const<typename std::remove_reference<RESPONSE>::type>::type response;

					if (inbound.Error.IsSet() == false) {
						FromMessage((INTERFACE*)&response, inbound);
					}

					actualMethod(response);
				};

				uint32_t result = Send(waitTime, method, parameters, implementation);
				return (result);
			}
			template <typename PARAMETERS, typename HANDLER, typename REALOBJECT>
			uint32_t InternalInvoke(const ::TemplateIntToType<0>&, const uint32_t waitTime, const string& method, const PARAMETERS& parameters, const HANDLER& callback, REALOBJECT* objectPtr)
			{
				using RESPONSE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<0>::type;

				std::function<void(RESPONSE, const Core::JSONRPC::Message::Info* result)> actualMethod = std::bind(callback, objectPtr, std::placeholders::_1, std::placeholders::_2);
				CallbackFunction implementation = [actualMethod, this](const Core::JSONRPC::Message& inbound) -> void {
					typename std::remove_const<typename std::remove_reference<RESPONSE>::type>::type response;
					if (inbound.Error.IsSet() == false) {
						FromMessage((INTERFACE*)&response, inbound);
						actualMethod(response, nullptr);
					}
					else {
						actualMethod(response, &(inbound.Error));
					}
				};

				uint32_t result = Send(waitTime, method, parameters, implementation);
				return (result);
			}
			virtual void Opened()
			{
				// Nice to know :-)
			}
			void Closed()
			{
				// Abort any in progress RPC command:
				_adminLock.Lock();

				// See if we issued anything, if so abort it..
				while (_pendingQueue.size() != 0) {

					_pendingQueue.begin()->second.Abort(_pendingQueue.begin()->first);
					_pendingQueue.erase(_pendingQueue.begin());
				}

				_adminLock.Unlock();
			}
			template <typename PARAMETERS>
			uint32_t Send(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, Core::ProxyType<Core::JSONRPC::Message>& response)
			{
				uint32_t result = Core::ERROR_UNAVAILABLE;

				if ((_channel.IsValid() == true) && (_channel->IsSuspended() == true)) {
					result = Core::ERROR_ASYNC_FAILED;
				}
				else if (_channel.IsValid() == true) {

					result = Core::ERROR_ASYNC_FAILED;

					Core::ProxyType<Core::JSONRPC::Message> message(CommunicationChannel::Message());
					uint32_t id = _channel->Sequence();
					message->Id = id;
					if (_callsign.empty() == false) {
						message->Designator = _callsign + _versionstring + '.' + method;
					}
					else {
						message->Designator = method;
					}
					ToMessage(parameters, message);

					_adminLock.Lock();

					typename std::pair< typename PendingMap::iterator, bool> newElement = _pendingQueue.emplace(std::piecewise_construct,
						std::forward_as_tuple(id),
						std::forward_as_tuple());
					ASSERT(newElement.second == true);

					if (newElement.second == true) {

						Entry& slot(newElement.first->second);

						_adminLock.Unlock();

						_channel->Submit(Core::ProxyType<INTERFACE>(message));

						message.Release();

						if (slot.WaitForResponse(waitTime) == true) {
							response = slot.Response();

							// See if we have a response, maybe it was just the connection
							// that closed?
							if (response.IsValid() == true) {
								result = Core::ERROR_NONE;
							}
						}
						else {
							result = Core::ERROR_TIMEDOUT;
						}

						_adminLock.Lock();

						_pendingQueue.erase(id);
					}

					_adminLock.Unlock();
				}

				return (result);
			}

			template <typename PARAMETERS>
			uint32_t Send(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, CallbackFunction& response)
			{
				uint32_t result = Core::ERROR_UNAVAILABLE;

				if ((_channel.IsValid() == true) && (_channel->IsSuspended() == true)) {
					result = Core::ERROR_ASYNC_FAILED;
				}
				else if (_channel.IsValid() == true) {

					result = Core::ERROR_ASYNC_FAILED;

					Core::ProxyType<Core::JSONRPC::Message> message(CommunicationChannel::Message());
					uint32_t id = _channel->Sequence();
					message->Id = id;
					if (_callsign.empty() == false) {
						message->Designator = _callsign + _versionstring + '.' + method;
					}
					else {
						message->Designator = method;
					}
					ToMessage(parameters, message);

					_adminLock.Lock();

					typename std::pair<typename PendingMap::iterator, bool> newElement = _pendingQueue.emplace(std::piecewise_construct,
						std::forward_as_tuple(id),
						std::forward_as_tuple(waitTime, response));
					ASSERT(newElement.second == true);

					if (newElement.second == true) {
						uint64_t expiry = newElement.first->second.Expiry();
						_adminLock.Unlock();

						_channel->Submit(Core::ProxyType<INTERFACE>(message));

						result = Core::ERROR_NONE;

						message.Release();
						_adminLock.Lock();
						if ((_scheduledTime == 0) || (_scheduledTime > expiry)) {
							_scheduledTime = expiry;
							CommunicationChannel::Trigger(_scheduledTime, this);
						}
					}

					_adminLock.Unlock();
				}

				return (result);
			}
			uint32_t Inbound(const Core::ProxyType<Core::JSONRPC::Message>& inbound)
			{
				uint32_t result = Core::ERROR_INVALID_SIGNATURE;

				ASSERT(inbound.IsValid() == true);

				if ((inbound->Id.IsSet() == true) && (inbound->Result.IsSet() || inbound->Error.IsSet())) {
					// Looks like this is a response..
					ASSERT(inbound->Parameters.IsSet() == false);
					ASSERT(inbound->Designator.IsSet() == false);

					_adminLock.Lock();

					// See if we issued this..
					typename PendingMap::iterator index = _pendingQueue.find(inbound->Id.Value());

					if (index != _pendingQueue.end()) {

						if (index->second.Signal(inbound) == true) {
							_pendingQueue.erase(index);
						}

						result = Core::ERROR_NONE;
					}

					_adminLock.Unlock();
				}
				else {
					// check if we understand this message (correct callsign?)
					string callsign(inbound->Callsign());
					string version(inbound->VersionAsString());
					if (version.empty() == false) {
						if (callsign.empty() == false) {
							callsign += _T('.');
						}
						callsign += version;
					}

					if (callsign == _localSpace) {
						// Looks like this is an event.
						ASSERT(inbound->Id.IsSet() == false);

						string response;
						_handler.Invoke(Core::JSONRPC::Context(), inbound->FullMethod(), inbound->Parameters.Value(), response);
					}
				}

				return (result);
			}

		private:
			void ToMessage(const string& parameters, Core::ProxyType<Core::JSONRPC::Message>& message) const
			{
				if (parameters.empty() != true) {
					message->Parameters = parameters;
				}
			}
			template <typename PARAMETERS>
			void ToMessage(PARAMETERS& parameters, Core::ProxyType<Core::JSONRPC::Message>& message) const
			{
				ToMessage((INTERFACE*)(&parameters), message);
				return;
			}
			void ToMessage(Core::JSON::IMessagePack* parameters, Core::ProxyType<Core::JSONRPC::Message>& message) const
			{
				std::vector<uint8_t> values;
				parameters->ToBuffer(values);
				if (values.empty() != true) {
					string strValues(values.begin(), values.end());
					message->Parameters = strValues;
				}
				return;
			}
			void ToMessage(Core::JSON::IElement* parameters, Core::ProxyType<Core::JSONRPC::Message>& message) const
			{
				string values;
				parameters->ToString(values);
				if (values.empty() != true) {
					message->Parameters = values;
				}
				return;
			}
			void FromMessage(Core::JSON::IElement* response, const Core::JSONRPC::Message& message)
			{
				response->FromString(message.Result.Value());
			}
			void FromMessage(Core::JSON::IMessagePack* response, const Core::JSONRPC::Message& message)
			{
				string value = message.Result.Value();
				std::vector<uint8_t> result(value.begin(), value.end());
				response->FromBuffer(result);
			}

		private:
			Core::CriticalSection _adminLock;
			Core::NodeId _connectId;
			Core::ProxyType< CommunicationChannel > _channel;
			Core::JSONRPC::Handler _handler;
			string _callsign;
			string _localSpace;
			PendingMap _pendingQueue;
			uint64_t _scheduledTime;
			string _versionstring;
		};

		// This is for backward compatibility. Please use the template and not the typedef below!!!
		typedef LinkType<Core::JSON::IElement> DEPRECATED Client;
		enum JSONPluginState {
			DEACTIVATED,
			ACTIVATED
		};

		template <typename INTERFACE>
		class SmartLinkType {
		private:
			class Connection : public LinkType<INTERFACE> {
			private:
				using Base = LinkType<INTERFACE>;
			public:
				static constexpr uint32_t DefaultWaitTime = Base::DefaultWaitTime;
			private:
				class Statechange : public Core::JSON::Container {
				public:
					Statechange()
						: Core::JSON::Container()
						, State(JSONRPC::JSONPluginState::DEACTIVATED)
					{
						Add(_T("callsign"), &Callsign);
						Add(_T("state"), &State);
					}

					Statechange(const Statechange& copy)
						: Core::JSON::Container()
						, Callsign(copy.Callsign)
						, State(copy.State)
					{
						Add(_T("callsign"), &Callsign);
						Add(_T("state"), &State);
					}
					Statechange& operator=(const Statechange&) = delete;

				public:
					Core::JSON::String Callsign; // Callsign of the plugin that changed state
					Core::JSON::EnumType<JSONRPC::JSONPluginState> State; // State of the plugin
				}; // class StatechangeParamsData
				class CurrentState : public Core::JSON::Container {
				public:
					CurrentState()
						: Core::JSON::Container()
						, State(JSONRPC::JSONPluginState::DEACTIVATED)
					{
						Add(_T("state"), &State);
					}

					CurrentState(const CurrentState& copy)
						: Core::JSON::Container()
						, State(copy.State)
					{
						Add(_T("state"), &State);
					}
					CurrentState& operator=(const CurrentState&) = delete;

				public:
					Core::JSON::EnumType<JSONRPC::JSONPluginState> State; // State of the plugin
				}; // class State

			public:
				enum state {
					UNKNOWN,
					DEACTIVATED,
					LOADING,
					ACTIVATED
				};
			public:
				Connection() = delete;
				Connection(const Connection&) = delete;
				Connection& operator=(const Connection&) = delete;

				// TODO: Constructos of the Client with version are bogus. Clean i tup
				Connection(SmartLinkType<INTERFACE>& parent, const string& callsign, const TCHAR* localCallsign, const string& query)
					: Base(callsign, string(), localCallsign, query)
					, _monitor(string(), false)
					, _parent(parent)
					, _state(UNKNOWN)
				{
					_monitor.template Assign<Statechange>(_T("statechange"), &Connection::state_change, this);
					LinkType<INTERFACE>::Announce();
				}
				~Connection() override
				{
					_monitor.Revoke(_T("statechange"));
				}

			public:
				template <typename INBOUND, typename METHOD>
				uint32_t Subscribe(const uint32_t waitTime, const string& eventName, const METHOD& method)
				{
					return Subscribe<INBOUND, METHOD>(waitTime, eventName, method);
				}
				template <typename INBOUND, typename METHOD, typename REALOBJECT>
				uint32_t Subscribe(const uint32_t waitTime, const string& eventName, const METHOD& method, REALOBJECT* objectPtr)
				{
					return Subscribe<INBOUND, METHOD, REALOBJECT>(waitTime, eventName, method, objectPtr);
				}
				bool IsActivated()
				{
					return (_state == ACTIVATED);
				}

			private:
				void SetState(const JSONRPC::JSONPluginState value)
				{
					if (value == JSONRPC::JSONPluginState::ACTIVATED) {
						if ((_state != ACTIVATED) && (_state != LOADING)) {
							_state = state::LOADING;
							auto index(Base::Events());
							while (index.Next() == true) {
								_events.push_back(index.Event());
							}
							next_event(Core::JSON::String(), nullptr);
						}
						else if (_state == LOADING) {
							_state = state::ACTIVATED;
							_parent.StateChange();

						}
					}
					else if (value == JSONRPC::JSONPluginState::DEACTIVATED) {
						if (_state != DEACTIVATED) {
							_state = DEACTIVATED;
							_parent.StateChange();
						}
					}
				}
				void state_change(const Statechange& info)
				{
					if ((info.State.IsSet() == true) && (info.Callsign.Value() == Base::Callsign())) {
						SetState(info.State.Value());
					}
				}
				void monitor_response(const Core::JSON::ArrayType<CurrentState>& info, const Core::JSONRPC::Error* result)
				{
					if ((result == nullptr) && (info.Length() == 1)) {
						if (info[0].State.IsSet() == true) {
							SetState(info[0].State.Value());
						}
					}
				}
				void monitor_on(const Core::JSON::String& parameters, const Core::JSONRPC::Error* result)
				{
					if (result == nullptr) {
						string method = string("status@") + Base::Callsign();
						_monitor.template Dispatch<void>(DefaultWaitTime, method, &Connection::monitor_response, this);
					}
				}
				void next_event(const Core::JSON::String& parameters, const Core::JSONRPC::Error* result)
				{
					// See if there are events pending for registration...
					if (_events.empty() == false) {
						const string parameters("{ \"event\": \"" + _events.front() + "\", \"id\": \"" + Base::Namespace() + "\"}");
						_events.pop_front();
						LinkType<INTERFACE>::Dispatch(DefaultWaitTime, _T("register"), parameters, &Connection::next_event, this);
					}
					else {
						SetState(JSONRPC::JSONPluginState::ACTIVATED);
					}
				}

				void Opened() override
				{
					// Time to open up the monitor
					const string parameters("{ \"event\": \"statechange\", \"id\": \"" + _monitor.Namespace() + "\"}");

					_monitor.template Dispatch<string>(DefaultWaitTime, "register", parameters, &Connection::monitor_on, this);
				}

			private:
				LinkType<INTERFACE> _monitor;
				SmartLinkType<INTERFACE>& _parent;
				std::list<string> _events;
				state _state;
			};

		public:
PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
			SmartLinkType(const string& remoteCallsign, const TCHAR* localCallsign, const string& query = "")
				: _connection(*this, remoteCallsign, localCallsign, query)
				, _callsign(remoteCallsign)
			{
			}
POP_WARNING()
			~SmartLinkType()
			{
			}
		public:
			template <typename INBOUND, typename METHOD>
			uint32_t Subscribe(const uint32_t waitTime, const string& eventName, const METHOD& method)
			{
				return _connection.template Subscribe<INBOUND, METHOD>(waitTime, eventName, method);
			}
			template <typename INBOUND, typename METHOD, typename REALOBJECT>
			uint32_t Subscribe(const uint32_t waitTime, const string& eventName, const METHOD& method, REALOBJECT* objectPtr)
			{
				return _connection.template Subscribe<INBOUND, METHOD, REALOBJECT>(waitTime, eventName, method, objectPtr);
			}
			void Unsubscribe(const uint32_t waitTime, const string& eventName)
			{
				return _connection.Unsubscribe(waitTime, eventName);
			}

			// -------------------------------------------------------------------------------------------
			// Synchronous invoke methods
			// -------------------------------------------------------------------------------------------
			template <typename PARAMETERS, typename RESPONSE>
			inline typename std::enable_if<(!std::is_same<PARAMETERS, void>::value && !std::is_same<RESPONSE, void>::value), uint32_t>::type
				Invoke(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, RESPONSE& inbound)
			{
				return _connection.template Invoke<PARAMETERS, RESPONSE>(waitTime, method, parameters, inbound);
			}

			template <typename PARAMETERS, typename RESPONSE>
			inline typename std::enable_if<(std::is_same<PARAMETERS, void>::value&& std::is_same<RESPONSE, void>::value), uint32_t>::type
				Invoke(const uint32_t waitTime, const string& method)
			{
				return _connection.template Invoke<void, void>(waitTime, method);
			}

			template <typename PARAMETERS, typename RESPONSE>
			inline typename std::enable_if<(!std::is_same<PARAMETERS, void>::value&& std::is_same<RESPONSE, void>::value), uint32_t>::type
				Invoke(const uint32_t waitTime, const string& method, const PARAMETERS& parameters)
			{
				return _connection.template Invoke<PARAMETERS, void>(waitTime, method, parameters);
			}

			template <typename PARAMETERS, typename RESPONSE>
			inline typename std::enable_if<(std::is_same<PARAMETERS, void>::value && !std::is_same<RESPONSE, void>::value), uint32_t>::type
				Invoke(const uint32_t waitTime, const string& method, RESPONSE& inbound)
			{
				return _connection.template Invoke<void, RESPONSE>(waitTime, method, inbound);
			}
			// -------------------------------------------------------------------------------------------
			// A-Synchronous invoke methods
			// -------------------------------------------------------------------------------------------
			template <typename PARAMETERS, typename HANDLER>
			inline uint32_t Dispatch(const uint32_t waitTime, const string& method, const HANDLER& callback)
			{
				return (_connection.template Dispatch<PARAMETERS, HANDLER>(waitTime, method, callback));
			}
			template <typename PARAMETERS, typename HANDLER, typename REALOBJECT = typename Core::TypeTraits::func_traits<HANDLER>::classtype>
			inline uint32_t Dispatch(const uint32_t waitTime, const string& method, const HANDLER& callback, REALOBJECT* objectPtr)
			{
				return (_connection.template Dispatch<PARAMETERS, HANDLER, REALOBJECT>(waitTime, method, callback, objectPtr));
			}
			// -------------------------------------------------------------------------------------------
			// SET Properties
			// -------------------------------------------------------------------------------------------
			template <typename PARAMETERS, typename... TYPES>
			inline uint32_t Set(const uint32_t waitTime, const string& method, const TYPES&&... args)
			{
				PARAMETERS sendObject(args...);
				return (_connection.template Set<PARAMETERS>(waitTime, method, sendObject));
			}
			template <typename PARAMETERS>
			inline uint32_t Set(const uint32_t waitTime, const string& method, const string& index, const PARAMETERS& sendObject)
			{
				return (_connection.template Set<PARAMETERS>(waitTime, method, index, sendObject));
			}
			template <typename PARAMETERS, typename NUMBER>
			inline uint32_t Set(const uint32_t waitTime, const string& method, const NUMBER index, const PARAMETERS& sendObject)
			{
				return (_connection.template Set<PARAMETERS, NUMBER>(waitTime, method, index, sendObject));
			}
			template <typename PARAMETERS>
			inline uint32_t Set(const uint32_t waitTime, const string& method, const PARAMETERS& sendObject)
			{
				return (_connection.template Set<PARAMETERS>(waitTime, method, sendObject));
			}
			// -------------------------------------------------------------------------------------------
			// GET Properties
			// -------------------------------------------------------------------------------------------
			template <typename PARAMETERS>
			inline uint32_t Get(const uint32_t waitTime, const string& method, const string& index, PARAMETERS& sendObject)
			{
				return (_connection.template Get<PARAMETERS>(waitTime, method, index, sendObject));
			}
			template <typename PARAMETERS, typename NUMBER>
			inline uint32_t Get(const uint32_t waitTime, const string& method, const NUMBER& index, PARAMETERS& sendObject)
			{
				return (_connection.template Get<PARAMETERS, NUMBER>(waitTime, method, index, sendObject));
			}
			template <typename PARAMETERS>
			inline uint32_t Get(const uint32_t waitTime, const string& method, PARAMETERS& sendObject)
			{
				return (_connection.template Get<PARAMETERS>(waitTime, method, sendObject));
			}
			inline uint32_t Invoke(const uint32_t waitTime, const string& method, const string& parameters, Core::ProxyType<Core::JSONRPC::Message>& response)
			{
				return (_connection.Invoke(waitTime, method, parameters, response));
			}

			// Opaque JSON structure methods.
			// Anything goes!
			// ===================================================================================
			uint32_t Invoke(const char method[], const Core::JSON::VariantContainer& parameters, Core::JSON::VariantContainer& response, const uint32_t waitTime = Connection::DefaultWaitTime)
			{
				return (_connection.Invoke(waitTime, method, parameters, response));
			}
			uint32_t SetProperty(const char method[], const Core::JSON::VariantContainer& object, const uint32_t waitTime = Connection::DefaultWaitTime)
			{
				return (_connection.Set(waitTime, method, object));
			}
			uint32_t GetProperty(const char method[], Core::JSON::VariantContainer& object, const uint32_t waitTime = Connection::DefaultWaitTime)
			{
				return (_connection.template Get<Core::JSON::VariantContainer>(waitTime, method, object));
			}
			bool IsActivated()
			{
				return (_connection.IsActivated());
			}
		private:
			virtual void StateChange()
			{
			}

		private:
			Connection _connection;
			string _callsign;
		};
	}
} // namespace WPEFramework::JSONRPC
