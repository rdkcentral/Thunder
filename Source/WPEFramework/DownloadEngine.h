#ifndef __DOWNLOADENGINE_H
#define __DOWNLOADENGINE_H

#include "Module.h"

namespace WPEFramework {
	namespace PluginHost {

		class DownloadEngine : public Web::ClientTransferType<Core::SocketStream, Web::SignedFileBodyType<Crypto::SHA256> > {
		private:
			typedef Web::ClientTransferType<Core::SocketStream, Web::SignedFileBodyType<Crypto::SHA256> > BaseClass;

			DownloadEngine() = delete;
			DownloadEngine(const DownloadEngine&) = delete;
			DownloadEngine& operator=(const DownloadEngine&) = delete;

		public:
			DownloadEngine(const string& downloadStorage) :
				BaseClass (false, Core::NodeId(_T("0.0.0.0")), Core::NodeId(), 1024, 1024),
				_storage(downloadStorage.c_str(), false) {
			}
			virtual ~DownloadEngine() {
			}

		public:
			uint32_t Start(const string& locator, const string& destination, const uint8_t hash[Crypto::HASH_SHA256])
			{
				Core::URL url(locator);
				uint32_t result = (url.IsValid() == true ? Core::ERROR_INPROGRESS : Core::ERROR_INCORRECT_URL);

				if (result == Core::ERROR_INPROGRESS) {

					_adminLock.Lock();

					if (_storage.IsOpen() == false) {

						result = Core::ERROR_OPENING_FAILED;

						if (_storage.Create() == true) {

							result = Core::ERROR_NONE;

							_current._destination = destination;
							_current._source = locator;
							::memcpy (_current._hash, hash, sizeof(_current._hash));

							BaseClass::Download(url, _storage);
						}
					}

					_adminLock.Unlock();
				}

				return (result);
			}

			virtual void Transfered(const uint32_t result, const string& source, const string& destination) = 0;

		private:
			virtual void Transfered(const uint32_t result, const Web::SignedFileBodyType<Crypto::SHA256>& destination) override {
				// Do you file magic stuff here, validate the hash and if it validates oke, move the file to the destination
				// location. Depending on these actions, mofidy the resukt and notify the parent.
				// For now we move on :-)
				// TODO: Implement above sequence
				Transfered(result, _current._source, _current._destination);

				_adminLock.Lock();
				_storage.Close();
				_adminLock.Unlock();
			}

			virtual bool Setup(const Core::URL& remote) override
			{
				bool result = false;

				if (remote.Host().IsSet() == true) {
					uint16_t portNumber(remote.Port().IsSet() ? remote.Port().Value() : 80);

					BaseClass::Link().RemoteNode(Core::NodeId(remote.Host().Value().Text().c_str(), portNumber));

					result = true;
				}
				return (result);
			}
 
		private:
			struct DownloadInfo {
				string _source;
				string _destination;
				uint8_t _hash[Crypto::HASH_SHA256];
			} _current;

			Core::CriticalSection _adminLock;
			Core::File _storage;
		};
	}
}

#endif // __DOWNLOADENGINE_H
