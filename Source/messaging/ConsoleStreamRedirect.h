#pragma once

#include "Control.h"
#include "OperationalCategories.h"

namespace WPEFramework {
	namespace Messaging {
		class EXTERNAL StandardOut {
		public:
			StandardOut(StandardOut&&);
			StandardOut(const StandardOut&);
			StandardOut& operator=(StandardOut&&);
			StandardOut& operator=(const StandardOut&);

			StandardOut() = default;
			~StandardOut() = default;

		public:
			void Output(const uint16_t length, const TCHAR buffer[])
			{
				if (OperationalStream::StandardOut::IsEnabled() == true) {
					Core::Messaging::MessageInfo messageInfo(OperationalStream::StandardOut::Metadata(), Core::Time::Now().Ticks());
					Core::Messaging::IStore::OperationalStream operationalStream(messageInfo);
					string text(buffer, length);
					TextMessage data(text);
					MessageUnit::Instance().Push(operationalStream, &data);
				}
			}

		};

		class EXTERNAL StandardError {
		public:
			StandardError(StandardError&&);
			StandardError(const StandardError&);
			StandardError& operator=(StandardError&&);
			StandardError& operator=(const StandardError&);

			StandardError() = default;
			~StandardError() = default;

		public:
			void Output(const uint16_t length, const TCHAR buffer[])
			{
				// TO-DO: Remove the text, fprintf(), and fflush() when it works
				if (OperationalStream::StandardError::IsEnabled() == true) {
				string flow(buffer, length);
				fprintf(stdout, "######### -> Redirected to MESSAGEPUMP: \"%s\"\n", flow.c_str());
				fflush(stdout);
					Core::Messaging::MessageInfo messageInfo(OperationalStream::StandardError::Metadata(), Core::Time::Now().Ticks());
					Core::Messaging::IStore::OperationalStream operationalStream(messageInfo);
					string text(buffer, length);
					TextMessage data(text);
					MessageUnit::Instance().Push(operationalStream, &data);
				}
			}
		};

		// TO-DO: Make sure by asking Pierre if it is okay here to delete move/copy constructors and assignment operators
		// And if so, does it matter if they are deleted as public or private? (since the constructor is private for this class)
		class EXTERNAL ConsoleStandardOut : public Core::TextStreamRedirectType<StandardOut> {
		public:
			ConsoleStandardOut(ConsoleStandardOut&&) = delete;
			ConsoleStandardOut(const ConsoleStandardOut&) = delete;
			ConsoleStandardOut& operator=(ConsoleStandardOut&&) = delete;
			ConsoleStandardOut& operator=(const ConsoleStandardOut&) = delete;

		private:
			ConsoleStandardOut()
				: Core::TextStreamRedirectType<StandardOut>(STDOUT_FILENO) {
			}

		public:
			~ConsoleStandardOut() = default;

			static ConsoleStandardOut& Instance()
			{
				static ConsoleStandardOut singleton;

				return (singleton);
			}
		};

		// TO-DO: Same as with the above class
		class EXTERNAL ConsoleStandardError : public Core::TextStreamRedirectType<StandardError> {
		public:
			ConsoleStandardError(ConsoleStandardError&&) = delete;
			ConsoleStandardError(const ConsoleStandardError&) = delete;
			ConsoleStandardError& operator=(ConsoleStandardError&&) = delete;
			ConsoleStandardError& operator=(const ConsoleStandardError&) = delete;

		private:
			ConsoleStandardError()
				: Core::TextStreamRedirectType<StandardError>(STDERR_FILENO) {
			}

		public:
			~ConsoleStandardError() = default;
			static ConsoleStandardError& Instance()
			{
				static ConsoleStandardError singleton;

				return (singleton);
			}
		};
	}
}
