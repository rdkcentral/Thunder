#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define MODULE_NAME application_redirect

#include <core/core.h>

using namespace Thunder;

namespace Thunder {
namespace Test {
	template<typename STREAMTYPE>
	class TextStreamRedirectType {
	private:
		using ParentClass = TextStreamRedirectType<STREAMTYPE>;

		class Reader {
		private:
			enum mode : uint8_t {
				IDLE = 0x00,
				SINGLE = 0x01,
				DOUBLE = 0x02,
				PENDING_RETURN = 0x11,
				PENDING_LINEFEED = 0x21,
				DROP_AND_RESET = 0x81
			};

		public:
			Reader() = delete;
			Reader(Reader&&) = delete;
			Reader(const Reader&) = delete;
			Reader& operator=(Reader&&) = delete;
			Reader& operator=(const Reader&) = delete;

			Reader(ParentClass& parent)
				: _handler(parent)
				, _offset(0)
				, _delimiter(mode::IDLE) {
			}
			~Reader() = default;

		public:
			mode IsTermination(const mode lastMode, const int length, const TCHAR buffer[]) const {
				mode result = mode::IDLE;

				ASSERT(length > 0);

				if (((lastMode == PENDING_RETURN) && (buffer[0] == '\r')) ||
					((lastMode == PENDING_LINEFEED) && (buffer[0] == '\n'))) {
					result = mode::DROP_AND_RESET;
				}
				else if ((lastMode & 0x30) == 0x00) {
					// Nothing pending from the last time, start from scratch..
					if (buffer[0] == '\n') {
						if (length == 1) {
							result = mode::PENDING_RETURN;
						}
						else if (buffer[1] == '\r') {
							result = mode::DOUBLE;
						}
						else {
							result = mode::SINGLE;
						}
					}
					else if (buffer[0] == '\r') {
						if (length == 1) {
							result = mode::PENDING_LINEFEED;
						}
						else if (buffer[1] == '\n') {
							result = mode::DOUBLE;
						}
						else {
							result = mode::SINGLE;
						}
					}
				}
				return (result);
			}
			void ProcessBuffer(const int readBytes) {
				int index = 0;
				int count = readBytes;

				ASSERT(readBytes > 0);

				// See if we have text termination in the new data..
				while (index < count) {

					_delimiter = IsTermination(_delimiter, (readBytes - index), &(_buffer[_offset]));

					// See if we need to skip characters...
					if ((_delimiter & 0x03) == 0) {
						index++;
						_offset++;
					}
					else if (_delimiter == mode::DROP_AND_RESET) {
						ASSERT(index == 0);
						ASSERT(_offset == 0);
						// This is a left over from the last check, just drop 1 character
						::memmove(_buffer, &(_buffer[1]), (readBytes - 1));
						count -= 1;
					}
					else {
						// Time to send out a newline..
						_handler.Output(_offset, _buffer);
						count -= (index + (_delimiter & 0x03));
						::memmove(_buffer, &_buffer[_offset + (_delimiter & 0x03)], count);
						_offset = 0;
						index = 0;
					}
				}

				if (_offset == sizeof(_buffer)) {
					_offset -= 32;
				}
			}
			void Flush() {
				if (_offset != 0) {
					_handler.Output(_offset, _buffer);
					_offset = 0;
				}
			}
			TCHAR* Buffer() {
				return (&(_buffer[_offset]));
			}
			uint16_t Length() {
				return (sizeof(_buffer) - _offset);
			}

		private:
			ParentClass& _handler;
			uint16_t _offset;
			mode _delimiter;
			TCHAR _buffer[1024];
		};

#ifdef __WINDOWS__
		class ReaderImplementation : public Reader {
		private:
			class ResourceMonitor : public Core::Thread {
			private:
				using Implementations = std::vector<ReaderImplementation*>;
				friend class Core::SingletonType<ResourceMonitor>;

				ResourceMonitor()
					: Core::Thread(Core::Thread::DefaultStackSize(), _T("FileResourceMonitor"))
					, _adminLock()
					, _resources() {
					::memset(&_event, 0, sizeof(OVERLAPPED));
					_event.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
				}
			public:
				ResourceMonitor(ResourceMonitor&&) = delete;
				ResourceMonitor(const ResourceMonitor&) = delete;
				ResourceMonitor& operator= (ResourceMonitor&&) = delete;
				ResourceMonitor& operator= (const ResourceMonitor&) = delete;

				static ResourceMonitor& Instance() {
					static ResourceMonitor instance;
					return (instance);
				}
				~ResourceMonitor() override {
					Core::Thread::Stop();
					Core::Thread::Wait(Core::Thread::BLOCKED | Core::Thread::STOPPED, Core::infinite);
				}

			public:
				void Register(ReaderImplementation& source) {
					_adminLock.Lock();
					ASSERT(std::find(_resources.begin(), _resources.end(), &source) == _resources.end());
					_resources.push_back(&source);
					source.Read(_event);
					if (_resources.size() == 1) {
						Core::Thread::Run();
					}
					_adminLock.Unlock();
				}
				void Unregister(ReaderImplementation& source) {
					_adminLock.Lock();
					typename Implementations::iterator entry(std::find(_resources.begin(), _resources.end(), &source));
					ASSERT(entry != _resources.end());
					if (entry != _resources.end()) {
						source.Cancel(_event);
						_resources.erase(entry);
					}
					if (_resources.empty() == true) {
						Core::Thread::Block();
						::SetEvent(_event.hEvent);
					}
					_adminLock.Unlock();
				}

				uint32_t Worker() override {
					if (::WaitForSingleObject(_event.hEvent, Core::infinite) == WAIT_OBJECT_0) {
						::ResetEvent(_event.hEvent);
						_adminLock.Lock();
						// Iterate through the list of entries to flush the data read:
						for (ReaderImplementation* entry : _resources) {
							entry->Process(_event);
						}
						_adminLock.Unlock();
					}
					return(Core::infinite);
				}

			private:
				Core::CriticalSection _adminLock;
				Implementations _resources;
				OVERLAPPED _event;
			};

		public:
			ReaderImplementation() = delete;
			ReaderImplementation(ReaderImplementation&&) = delete;
			ReaderImplementation(const ReaderImplementation&) = delete;
			ReaderImplementation& operator=(ReaderImplementation&&) = delete;
			ReaderImplementation& operator=(const ReaderImplementation&) = delete;

			ReaderImplementation(ParentClass& parent)
				: Reader(parent)
				, _index(Core::IResource::INVALID)
				, _copy(Core::IResource::INVALID)
				, _handle(nullptr) {
			}
			~ReaderImplementation() {
				Close();
			}

		public:
			bool Open(const Core::IResource::handle replacing) {

				ASSERT(_index == Core::IResource::INVALID);
				ASSERT(replacing != Core::IResource::INVALID);

				if (replacing != Core::IResource::INVALID) {
					Core::IResource::handle newDescriptor;

					if (CreateOverlappedPipe(_handle, newDescriptor) != false) {
						_flushall();
						_copy = ::_dup(replacing);
						if (::_dup2(newDescriptor, replacing) == -1) {
							::_close(newDescriptor);
							::CloseHandle(_handle);
						}
						else {
							_index = replacing;
							ResourceMonitor::Instance().Register(*this);
							::_close(newDescriptor);
						}
					}
				}
				return (_index != Core::IResource::INVALID);
			}
			bool Close() {
				if (_index != Core::IResource::INVALID) {
					_flushall();
					if (::_dup2(_copy, _index) != -1) {
						::_close(_copy);
						ResourceMonitor::Instance().Unregister(*this);
						_index = Core::IResource::INVALID;
						::CloseHandle(_handle);
					}
					Reader::Flush();
				}
				return (_index == Core::IResource::INVALID);
			}

		private:
			bool CreateOverlappedPipe(HANDLE& readPipe, int& writePipe)
			{
				static volatile uint32_t sequenceNumber = 0;
				uint32_t newIndex = Core::InterlockedIncrement(sequenceNumber);

				HANDLE pipeHandle, pipeOutput;
				TCHAR PipeNameBuffer[MAX_PATH];

				sprintf(PipeNameBuffer,
					_T("\\\\.\\Pipe\\ThunderRedirectPipe.%08x.%08x"), Core::ProcessInfo().Id(), newIndex);

				pipeHandle = CreateNamedPipeA(
					PipeNameBuffer,
					PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
					PIPE_TYPE_BYTE | PIPE_WAIT,
					1,
					4096,
					4096,
					120 * 1000,
					nullptr
				);

				if (pipeHandle != INVALID_HANDLE_VALUE) {
					pipeOutput = CreateFileA(
						PipeNameBuffer,
						GENERIC_WRITE,
						0,
						nullptr,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						nullptr);

					if (pipeOutput == INVALID_HANDLE_VALUE) {
						::CloseHandle(pipeHandle);
					}
					else {
						writePipe = _open_osfhandle(reinterpret_cast<intptr_t>(pipeOutput), 0);
						if (writePipe == -1) {
							::CloseHandle(pipeHandle);
							::CloseHandle(pipeOutput);
						}
						else {
							readPipe = pipeHandle;
							return (true);
						}
					}
				}
				return(false);
			}
			void Read(OVERLAPPED& event) {
				::ReadFile(_handle, Reader::Buffer(), Reader::Length(), nullptr, &event);
			}
			void Cancel(OVERLAPPED& event) {
				// Maybe some data was still flushed, first flush that...
				DWORD bytesRead = 0;

				if ((::GetOverlappedResult(_handle, &event, &bytesRead, FALSE) != 0) && (bytesRead > 0)) {
					Reader::ProcessBuffer(bytesRead);
				}

				::CancelIoEx(_handle, &event);
			}
			void Process(OVERLAPPED& event) {
				DWORD bytesRead = 0;

				if ((::GetOverlappedResult(_handle, &event, &bytesRead, FALSE) != 0) && (bytesRead > 0)) {
					Reader::ProcessBuffer(bytesRead);
					Read(event);
				}
			}

		private:
			Core::IResource::handle _index;
			Core::IResource::handle _copy;
			HANDLE _handle;
		};

		// friend class Core::SingletonType<ReaderImplementation::ResourceMonitor>;

#else
		class ReaderImplementation : public Core::IResource, public Reader {
		public:
			ReaderImplementation() = delete;
			ReaderImplementation(ReaderImplementation&&) = delete;
			ReaderImplementation(const ReaderImplementation&) = delete;
			ReaderImplementation& operator=(ReaderImplementation&&) = delete;
			ReaderImplementation& operator=(const ReaderImplementation&) = delete;

			ReaderImplementation(ParentClass& parent, const Core::IResource::handle replacing)
				: Reader(parent)
				, _index(replacing)
				, _copy(Core::IResource::INVALID) {
				ASSERT(replacing != Core::IResource::INVALID);
				_handle[0] = Core::IResource::INVALID;
				_handle[1] = Core::IResource::INVALID;
			}
			~ReaderImplementation() override {
				Close();
				_index = Core::IResource::INVALID;
			}

		public:
			bool Open() {

				ASSERT(_index != Core::IResource::INVALID);

				if ((_handle[0] == Core::IResource::INVALID) && (::pipe(_handle) == 0)) {
					_copy = ::dup(_index);
					::fsync(_index);
					int flags = fcntl(_handle[0], F_GETFL);
					fcntl(_handle[0], F_SETFL, (flags | O_NONBLOCK));
					::dup2(_handle[1], _index);
					close(_handle[1]);
					_handle[1] = Core::IResource::INVALID;
					Core::ResourceMonitor::Instance().Register(*this);
				}
				return (_handle[0] == Core::IResource::INVALID);
			}
			bool Close() {
				if (_handle[0] != Core::IResource::INVALID) {

					::fsync(_handle[0]);
					::dup2(_copy, _index);
					::close(_handle[0]);
					::close(_copy);
					Core::ResourceMonitor::Instance().Unregister(*this);
					_handle[0] = Core::IResource::INVALID;
					_copy = Core::IResource::INVALID;
					Reader::Flush();
				}
				return (true);
			}
			Core::IResource::handle Origin() const {
				return (_copy == Core::IResource::INVALID ? _index : _copy);
			}
			Core::IResource::handle Descriptor() const override {
				return (_handle[0]);
			}
			uint16_t Events() override {
				if (_handle[0] != Core::IResource::INVALID) {
					return (POLLHUP | POLLRDHUP | POLLIN);
				}
				return (0);
			}
			void Handle(const uint16_t events) override {
				// If we have an event, read and see if we have a full line..
				if ((events & POLLIN) != 0) {
					int readBytes;

					do {
						readBytes = read(_handle[0], Reader::Buffer(), Reader::Length());

						if (readBytes > 0) {
							Reader::ProcessBuffer(readBytes);
						}

					} while (readBytes > 0);
				}
			}

		private:
			Core::IResource::handle _index;
			Core::IResource::handle _copy;
			Core::IResource::handle _handle[2];
		};
#endif

	public:
		TextStreamRedirectType(TextStreamRedirectType<STREAMTYPE>&&) = delete;
		TextStreamRedirectType(const TextStreamRedirectType<STREAMTYPE>&) = delete;
		TextStreamRedirectType<STREAMTYPE> operator=(TextStreamRedirectType<STREAMTYPE>&&) = delete;
		TextStreamRedirectType<STREAMTYPE> operator=(const TextStreamRedirectType<STREAMTYPE>&) = delete;

		TextStreamRedirectType(const Core::IResource::handle source)
			: _channel(*this, source) {
		}
		~TextStreamRedirectType() = default;

	public:
		bool Open() {
			return (_channel.Open());
		}
		bool Close() {
			return (_channel.Close());
		}
		void Format(const TCHAR format[], ...)
		{
			string dst;
			va_list ap;
			va_start(ap, format);
			int length;
			va_list apStrLen;
			va_copy(apStrLen, ap);
			length = vsnprintf(nullptr, 0, format, apStrLen);
			va_end(apStrLen);

			if (length > 0) {
				dst.resize(length);
				vsnprintf((char*)dst.data(), dst.size() + 1, format, ap);
				write(_channel.Origin(), dst.c_str(), length);
			}
			else {
				dst = "Format error! format: ";
				dst.append(format);
			}

			va_end(ap);
		}

	private:
		void Output(const uint16_t length, const TCHAR buffer[]) {
			_metadata.Output(length, buffer);
		}

	private:
		ReaderImplementation _channel;
		STREAMTYPE _metadata;
	};

	class RedirectedStream {
	public:
		RedirectedStream(RedirectedStream&&) = delete;
		RedirectedStream(const RedirectedStream&) = delete;
		RedirectedStream& operator=(RedirectedStream&&) = delete;
		RedirectedStream& operator=(const RedirectedStream&) = delete;

		RedirectedStream() = default;
		~RedirectedStream() = default;

	public:
		void Output(const uint16_t length, const TCHAR buffer[]) {
			string text(buffer, length);
			fprintf(stdout, "Redirected: \"%s\"\n", text.c_str());
		}
	};
}
}


void help() {
    printf ("W -> Write some text to stderr\n");
    printf ("O -> Open the redirect from stderr\n");
    printf ("C -> Close the redirect from stderr\n");
    printf ("Q -> Quit\n>");
}

int main(int argc, char** argv)
{
    {
        int element;
        uint32_t counter = 0;
        Test::TextStreamRedirectType<Test::RedirectedStream> redirector(fileno(stderr));

        help();
        do {
            element = toupper(getchar());

            switch (element) {
                case 'W': {
                    counter++;
                    fprintf(stdout, "Sending the number [%d] to a potentially redirected stream...\n", counter);
                    fprintf(stderr, "Sending: [%d] ...\n", counter);
                    break;
                }
                case 'O': {
                    fprintf(stdout, "Opening the redirection.\n");
                    redirector.Open();
                    break;
                }
                case 'C': {
                    fprintf(stdout, "Closing the redirection.\n");
                    redirector.Close();
                    break;
                }
                case 'Q': break;
                default: {
                }
            }
        } while (element != 'Q');
    }

    Core::Singleton::Dispose();

    return (0);
}
