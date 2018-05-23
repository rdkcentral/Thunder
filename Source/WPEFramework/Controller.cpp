#include "Controller.h"
#include "SystemInfo.h"

namespace WPEFramework {

		namespace Plugin {

			SERVICE_REGISTRATION(Controller, 1, 0);

			// Signing will be done on BackOffice level. The Controller I/F will never be exposed to the outside world.
			// Access to this interface will be through the BackOffice Plugin, if external exposure is required !!!
			// typedef Web::SignedJSONBodyType<Plugin::Config, Crypto::SHA256HMAC> SignedConfig;
			// typedef Web::SignedJSONBodyType<Controller::Download, Crypto::SHA256HMAC> SignedDownload;
			// Signing will be done on BackOffice level. The Controller I/F will never be exposed to the outside world.
			static Core::ProxyPoolType<Web::JSONBodyType<PluginHost::MetaData> > jsonBodyMetaDataFactory(1);
			static Core::ProxyPoolType<Web::JSONBodyType<PluginHost::MetaData::Service> > jsonBodyServiceFactory(1);
			static Core::ProxyPoolType<Web::TextBody> jsonBodyConfigureFactory(1);
			static Core::ProxyPoolType<Web::JSONBodyType<Controller::Download> > jsonBodyDownloadFactory(1);

			static void DeleteDirectory(const string& directory)
			{
				Core::Directory dir(directory.c_str());

				while (dir.Next() == true) {
					Core::File file(dir.Current());

					if (file.IsDirectory() == true) {

						string name(file.FileName());

						// We can not delete the "." or  ".." entries....
						if ((name.length() > 2) ||
							((name.length() > 1) && (name[1] != '.')) ||
							((name.length() > 0) && (name[0] != '.'))) {
							DeleteDirectory(dir.Current());
							file.Destroy();
						}
					}
					else {
						file.Destroy();
					}
				}
			}

        void Controller::SubSystems(Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem> >::ConstIterator& index) {

            PluginHost::ISubSystem* subSystem = _service->SubSystems();

            if (subSystem != nullptr) {
                uint32_t entries = ((1 << PluginHost::ISubSystem::END_LIST) - 1);

                while (index.Next() == true) {
                    if (index.Current() < PluginHost::ISubSystem::END_LIST) {
                        entries &= ~(1 << index.Current());
                    }
                }

                uint8_t setFlag = 0;
                while (setFlag < PluginHost::ISubSystem::END_LIST) {
                    if ((entries & (1 << setFlag)) != 0) {
			TRACE_L1("Setting the default SubSystem: %d", setFlag);
                        subSystem->Set(static_cast<PluginHost::ISubSystem::subsystem>(setFlag), nullptr);
                    }
                    setFlag++;
                }

                subSystem->Release();
            }
        }
 
			// Access to this interface will be through the BackOffice Plugin, if external exposure is required !!!
			/* virtual */ const string Controller::Initialize(PluginHost::IShell* service)
			{
				ASSERT(_service == nullptr);
				ASSERT(_downloader == nullptr);
				ASSERT(_probe == nullptr);

				_resumes.clear();
				_service = service;
				_skipURL = static_cast<uint8_t>(_service->WebPrefix().length());

				Config config;
				config.FromString(_service->ConfigLine());

				if (config.DownloadStore.IsSet() == false) {

					_downloader = new Downloader(*this, _service->PersistentPath() + _T("DownloadStore"));
				}
				else {
					_downloader = new Downloader(*this, config.DownloadStore.Value());
				}

				_probe = new Probe(_service->Accessor(), config.TTL.Value(), service->Model());
                                Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem> >::ConstIterator eventListIterator (static_cast<const Config&>(config).SubSystems.Elements());
				SubSystems (eventListIterator);

				if ((config.Resumes.IsSet() == true) && (config.Resumes.Length() > 0)) {
					Core::JSON::ArrayType<Core::JSON::String>::Iterator index(config.Resumes.Elements());

					while (index.Next() == true) {
						_resumes.push_back(index.Current().Value());
					}

					if (_resumes.size() > 0) {
						_service->Register(&_systemInfoReport);
					}
				}

				_service->EnableWebServer(_T("UI"), EMPTY_STRING);

				// On succes return a name as a Callsign to be used in the URL, after the "service"prefix
				return (_T(""));
			}

			/* virtual */ void Controller::Deinitialize(PluginHost::IShell* service)
			{
				ASSERT(_service == service);
				ASSERT(_downloader != nullptr);

				// Detach the SubSystems, we are shutting down..
				PluginHost::ISubSystem* subSystems (_service->SubSystems());

				ASSERT (subSystems != nullptr);

				if (subSystems != nullptr) {
					subSystems->Unregister(&_systemInfoReport);
                                }


				delete _downloader;
				_downloader = nullptr;

				delete _probe;
				_probe = nullptr;

				if (_resumes.size() > 0) {
					_service->Unregister(&_systemInfoReport);
				}

				/* stop the file serving over http.... */
				service->DisableWebServer();
			}

			/* virtual */ string Controller::Information() const
			{
				// No additional info to report.
				return (string());
			}

			/* virtual */ void Controller::Inbound(Web::Request& request)
			{
				ASSERT(request.HasBody() == false);

				if (request.Verb == Web::Request::HTTP_PUT) {
					Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

					// Always skip the first one, it is an empty part because we start with a '/' if tehre are more parameters.
					index.Next();

					// We might be receiving a plugin download request.
					if (index.Next() == true) {
						if (index.Remainder() == _T("Download")) {
							request.Body(Core::proxy_cast<Web::IBody>(jsonBodyDownloadFactory.Element()));
						}
						else if (index.Current() == _T("Configuration")) {
							request.Body(Core::proxy_cast<Web::IBody>(jsonBodyConfigureFactory.Element()));
						}
					}
				}
			}

			/* virtual */ Core::ProxyType<Web::Response> Controller::Process(const Web::Request& request)
			{
				ASSERT(_pluginServer != nullptr);

				TRACE(Trace::Information, (string(_T("Received request"))));

				Core::ProxyType<Web::Response> result;
				Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

				// Always skip the first one, it is an empty part because we start with a '/' if tehre are more parameters.
				index.Next();

				// For now, whatever the URL, we will just, on a get, drop all info we have
				if (request.Verb == Web::Request::HTTP_GET) {
					result = GetMethod(index);
				}
				else if (request.Verb == Web::Request::HTTP_PUT) {
					result = PutMethod(index, request);
				}
				else if (request.Verb == Web::Request::HTTP_DELETE) {
					// Time to remove a plugin, indicated by Current.
					result = DeleteMethod(index, request);
				}

				return (result);
			}

			Core::ProxyType<Web::Response> Controller::GetMethod(Core::TextSegmentIterator& index) const
			{
				Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
				result->ContentType = Web::MIME_JSON;

				if (index.Next() == false) {
					Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData> > response(jsonBodyMetaDataFactory.Element());

					// No more parameters, flush it all..
					_pluginServer->Dispatcher().GetMetaData(response->Channels);
					_pluginServer->Services().GetMetaData(response->Plugins);
					PluginHost::WorkerPool::Instance().GetMetaData(response->Process);

					result->Body(Core::proxy_cast<Web::IBody>(response));
				}
				else if (index.Current() == _T("Links")) {
					Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData> > response(jsonBodyMetaDataFactory.Element());

					_pluginServer->Dispatcher().GetMetaData(response->Channels);

					result->Body(Core::proxy_cast<Web::IBody>(response));
				}
				else if (index.Current() == _T("Plugins")) {
					Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData> > response(jsonBodyMetaDataFactory.Element());

					_pluginServer->Services().GetMetaData(response->Plugins);

					result->Body(Core::proxy_cast<Web::IBody>(response));
				}
				else if (index.Current() == _T("Environment")) {
					// We do not want Environment to be included in the variable
					if (index.Next() == true) {
						string value;

						if (Core::SystemInfo::GetEnvironment(index.Remainder().Text(), value) == true) {
							Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData> > response(jsonBodyMetaDataFactory.Element());
							response->Value = value;

							result->Body(Core::proxy_cast<Web::IBody>(response));
						}
						else {
							result->ErrorCode = Web::STATUS_NO_CONTENT;
							result->Message = _T("Environment variable does not exist");
						}
					}
				}
				else if (index.Current() == _T("Plugin")) {
					if (index.Next() == true) {
						Core::ProxyType<const PluginHost::Server::Service> serviceInfo(_pluginServer->Services().FromLocator(index.Current().Text()));

						if (serviceInfo.IsValid() == true) {
							Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData::Service> > response(jsonBodyServiceFactory.Element());

							serviceInfo->GetMetaData(*response);

							result->Body(Core::proxy_cast<Web::IBody>(response));
						}
					}
				}
				else if (index.Current() == _T("Configuration")) {
					if (index.Next() == true) {
						Core::ProxyType<const PluginHost::Service> serviceInfo(_pluginServer->Services().FromLocator(index.Current().Text()));

						if (serviceInfo.IsValid() == true) {
							Core::ProxyType<Web::TextBody> response(jsonBodyConfigureFactory.Element());

							*response = serviceInfo->ConfigLine();

							result->Body(Core::proxy_cast<Web::IBody>(response));
						}
					}
				}
				else if (index.Current() == _T("Process")) {
					Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData> > response(jsonBodyMetaDataFactory.Element());

					PluginHost::WorkerPool::Instance().GetMetaData(response->Process);

					result->Body(Core::proxy_cast<Web::IBody>(response));
				}
				else if (index.Current() == _T("Discovery")) {
					Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData> > response(jsonBodyMetaDataFactory.Element());

					Probe::Iterator index(_probe->Instances());

					while (index.Next() == true) {
						PluginHost::MetaData::Bridge newElement((*index).URL().Text().Text(), (*index).Latency(), (*index).Model(), (*index).IsSecure());
						response->Bridges.Add(newElement);
					}

					result->Body(Core::proxy_cast<Web::IBody>(response));
				}
				else if (index.Current() == _T("SubSystems")) {
                                        PluginHost::ISubSystem* subSystem = _service->SubSystems();
					Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData> > response(jsonBodyMetaDataFactory.Element());

					uint8_t index(0);
                                        if (subSystem != nullptr) {
					    while (index < PluginHost::ISubSystem::END_LIST) {
						PluginHost::ISubSystem::subsystem current (static_cast<PluginHost::ISubSystem::subsystem >(index));
						response->SubSystems.Add(current, subSystem->IsActive(current) );
						++index;
					    }
                                            subSystem->Release();
                                        }

					result->Body(Core::proxy_cast<Web::IBody>(response));
				}

				return (result);
			}
			Core::ProxyType<Web::Response> Controller::PutMethod(Core::TextSegmentIterator& index, const Web::Request& request)
			{
				Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());

				// All PUT commands require an additional parameter, so go look for it.
				if (index.Next() == true) {
					if (index.Current() == _T("Activate")) {
						if (index.Next()) {
							const string callSign(index.Current().Text());
							if (callSign == _pluginServer->ControllerName()) {
								result->ErrorCode = Web::STATUS_FORBIDDEN;
								result->Message = _T("The PluginHost Controller can not be activated.");
							}
							else {
								Core::ProxyType<PluginHost::Server::Service> pluginInfo(_pluginServer->Services().FromLocator(callSign));

								if (pluginInfo.IsValid()) {
									if (pluginInfo->State() == PluginHost::IShell::DEACTIVATED) {
										// Activate the plugin.
										uint32_t returnCode = pluginInfo->Activate(PluginHost::IShell::REQUESTED);

										// See if this was a successful activation...
										if (pluginInfo->HasError() == true) {
											// Oops seems we failed. Send out the error message
											result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
											result->Message = pluginInfo->ErrorMessage();
										}
										else if (returnCode != Core::ERROR_NONE) {
											result->ErrorCode = Web::STATUS_NOT_MODIFIED;
											result->Message = _T("Activation already in progress.");
										}
									}
								}
								else {
									result->ErrorCode = Web::STATUS_NOT_FOUND;
									result->Message = _T("There is no callsign: ") + callSign;
								}
							}
						}
					}
					else if (index.Current() == _T("Deactivate")) {
						if (index.Next()) {
							const string callSign(index.Current().Text());
							if (callSign == _pluginServer->ControllerName()) {
								result->ErrorCode = Web::STATUS_FORBIDDEN;
								result->Message = _T("The PluginHost Controller can not be deactivated.");
							}
							else {
								Core::ProxyType<PluginHost::Server::Service> pluginInfo(_pluginServer->Services().FromLocator(callSign));

								if (pluginInfo.IsValid()) {
									if (pluginInfo->State() == PluginHost::IShell::ACTIVATED) {
										// Deactivate the plugin.
										if (pluginInfo->Deactivate(PluginHost::IShell::REQUESTED) != Core::ERROR_NONE) {
											result->ErrorCode = Web::STATUS_NOT_MODIFIED;
											result->Message = _T("Deactivation already in progress.");
										}
									}
								}
								else {
									result->ErrorCode = Web::STATUS_NOT_FOUND;
									result->Message = _T("There is no callsign: ") + callSign;
								}
							}
						}
					}
					else if (index.Current() == _T("Configuration")) {
						if ((index.Next() == true) && (request.HasBody() == true)) {

							Core::ProxyType<PluginHost::Service> serviceInfo(_pluginServer->Services().FromLocator(index.Current().Text()));
							Core::ProxyType<const Web::TextBody> data(request.Body<const Web::TextBody>());

							if ((data.IsValid() == false) || (serviceInfo.IsValid() == false)) {

								result->ErrorCode = Web::STATUS_BAD_REQUEST;
								result->Message = _T("Not sufficent data to comply to the request");
							}
							else {

								uint32_t error;

								if ((error = serviceInfo->ConfigLine(*data)) == Core::ERROR_NONE) {
									result->ErrorCode = Web::STATUS_OK;
								}
								else {
									result->ErrorCode = Web::STATUS_BAD_REQUEST;
									result->Message = _T("Could not update the config. Error: ") + Core::NumberType<uint32_t>(error).Text();
								}
							}
						}
					}
					else if (index.Current() == _T("Download")) {
						if (request.HasBody() == true) {
							Core::ProxyType<const Web::JSONBodyType<Controller::Download> > data(request.Body<const Web::JSONBodyType<Controller::Download> >());

							if (data.IsValid() == true) {
								string destination(_service->PersistentPath() + data->Destination.Value());

								if (Core::File(destination).Create() == false) {

									result->ErrorCode = Web::STATUS_BAD_REQUEST;
									result->Message = _T("Could not open destination file: ") + data->Destination.Value();
								}
								else {
									uint8_t hash[Crypto::HASH_SHA256];
									uint16_t length = (sizeof(hash));
										
									if (Core::FromString(data->Hash.Value(), hash, length) != data->Hash.Value().length()) {
										// Oops could not open the destination
										result->ErrorCode = Web::STATUS_BAD_REQUEST;
										result->Message = _T("The hash was longer than what we expect.");
									}
									else {
										uint32_t code = _downloader->Start(data->Source.Value(), destination, hash);

										if (code == Core::ERROR_INPROGRESS) {
											// Oops could not open the destination
											result->ErrorCode = Web::STATUS_MOVED_TEMPORARY;
											result->Message = _T("A transfer is currently in progress, so please be patient.");
										}
										else if (code == Core::ERROR_INCORRECT_URL) {
											// Oops do not understand the URL
											result->ErrorCode = Web::STATUS_NOT_FOUND;
											result->Message = _T("The URL for the source is incorrect.");
										}
										else if (code != Core::ERROR_NONE) {
											// Uhhhh whatever.. this is unknown
											result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
											result->Message = _T("oops, have no clue what this error [") + Core::NumberType<uint32_t>(code).Text() + _T("] means");
										}
										else {
											// Download started, add it to the 
											result->ErrorCode = Web::STATUS_OK;
										}
									}
								}
							}
						}
					}
					else if (index.Current() == _T("Discovery")) {
						ASSERT(_probe != nullptr);
						Core::URL::KeyValue options(request.Query.Value());
						uint8_t ttl = options.Number<uint8_t>(_T("TTL"), 0);

						_probe->Ping(ttl);

						result->ErrorCode = Web::STATUS_OK;
						result->Message = _T("Discovery cycle initiated");
					}
					else if (index.Current() == _T("Persist")) {

						_pluginServer->Services().Persist();

						result->ErrorCode = Web::STATUS_OK;
						result->Message = _T("Current configuration stored");
					}
					else if (index.Current() == _T("Harakiri")) {
						uint32_t status = Core::System::Reboot();
						if (status == Core::ERROR_NONE) {
							result->ErrorCode = Web::STATUS_OK;
						}
						else {
							result->ErrorCode = Web::STATUS_BAD_REQUEST;
							result->Message = _T("Could not issue a reboot request. Error: ") + Core::NumberType<uint32_t>(status).Text();
						}
					}
				}
				return (result);
			}

			Core::ProxyType<Web::Response> Controller::DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& /* request */) {
				Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());

				// All delete commands require an additional parameter, so go look for it.
				if (index.Next() == true) {
					if (index.Current() == _T("Persistent")) {
						// move over the persistent keyword.
						index.Next();

						string remainder;

						while (index.Next() == true) {
							string element(index.Current().Text());

							// Check that we are not moving back up the directory chain. Remove all double dots..
							if ((element.length() > 2) ||
								((element.length() > 1) && (element[1] != '.')) ||
								((element.length() > 0) && (element[0] != '.'))) {
								if (remainder.empty() == false) remainder += '/';
								remainder += element;
							}
						}

						DeleteDirectory(_service->PersistentPath() + remainder);

						result->Message = "OK";
					}
				}
				return (result);
			}

			void Controller::Transfered(const uint32_t result,const string& source, const string& destination) {

				// Report over a socket a download is completed with its state.
			}


			void Controller::StateChange(PluginHost::IShell* plugin) {

				if (plugin->State() == PluginHost::IShell::ACTIVATED) {
					string callsign (plugin->Callsign());
					std::list<string>::const_iterator index(_resumes.begin());

					ASSERT(_service != nullptr);
	
					while ((index != _resumes.end()) && (*index != callsign)) { index++; }

					if (index != _resumes.end()) {
	
						PluginHost::IStateControl* stateControl = plugin->QueryInterface<PluginHost::IStateControl>();

						TRACE_L1("Resuming %s", callsign.c_str());

						if (stateControl != nullptr) {
							uint32_t  error = stateControl->Request(PluginHost::IStateControl::RESUME);

							if (error != Core::ERROR_NONE) {
								TRACE_L1("Failed to resume %s, error %d", callsign.c_str(), error);
							}

							stateControl->Release();
						}

						_resumes.erase(index);

						if (_resumes.size() == 0) {
							// No more resumes required, unregister your self...
							_service->Unregister(&_systemInfoReport);
						}
					}
				}
			}


        		void Controller::SubSystems() {
				string message;
				PluginHost::MetaData response;
                                PluginHost::ISubSystem* subSystem = _service->SubSystems();

				// Now prepare a message for the Javascript world.
			        bool sendReport = false;
				uint8_t index(0);

				_adminLock.Lock();

                                if (subSystem != nullptr) {
				    uint32_t reportMask = 0;

				    while (index < PluginHost::ISubSystem::END_LIST) {
					PluginHost::ISubSystem::subsystem current (static_cast<PluginHost::ISubSystem::subsystem >(index));
                                        uint32_t bit (1 << index);
                                        reportMask |= (subSystem->IsActive(current) ? bit : 0);

					if (((reportMask & bit) != 0) ^ ((_lastReported & bit) != 0)) {
					    response.SubSystems.Add(current, ((reportMask & bit) != 0));
					    sendReport = true;
					}
					++index;
                                    }

				    _lastReported = reportMask;
                                    subSystem->Release();
				}

				_adminLock.Unlock();

				if (sendReport == true) {

				    response.ToString(message);

				    TRACE_L1("Sending out a SubSystem change notification. %s", message.c_str());

				    _pluginServer->Notify(message);
				}
       			}
		}
	}
