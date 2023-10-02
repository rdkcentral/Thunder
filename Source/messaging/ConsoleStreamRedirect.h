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
				if (OperationalStream::StandardError::IsEnabled() == true) {
					Core::Messaging::MessageInfo messageInfo(OperationalStream::StandardError::Metadata(), Core::Time::Now().Ticks());
					Core::Messaging::IStore::OperationalStream operationalStream(messageInfo);
					string text(buffer, length);
					TextMessage data(text);
					MessageUnit::Instance().Push(operationalStream, &data);
				}
			}
		};

		class EXTERNAL ConsoleStandardOut : public Core::TextStreamRedirectType<StandardOut> {
		public:
			ConsoleStandardOut(ConsoleStandardOut&&) = delete;
			ConsoleStandardOut(const ConsoleStandardOut&) = delete;
			ConsoleStandardOut& operator=(ConsoleStandardOut&&) = delete;
			ConsoleStandardOut& operator=(const ConsoleStandardOut&) = delete;

		private:
			ConsoleStandardOut()
#ifdef __WINDOWS__
				: Core::TextStreamRedirectType<StandardOut>(::_fileno(stdout)) {
#else
				: Core::TextStreamRedirectType<StandardOut>(STDOUT_FILENO) {
#endif
			}

		public:
			~ConsoleStandardOut() = default;

			static ConsoleStandardOut& Instance()
			{
				static ConsoleStandardOut singleton;

				return (singleton);
			}
		};

		class EXTERNAL ConsoleStandardError : public Core::TextStreamRedirectType<StandardError> {
		public:
			ConsoleStandardError(ConsoleStandardError&&) = delete;
			ConsoleStandardError(const ConsoleStandardError&) = delete;
			ConsoleStandardError& operator=(ConsoleStandardError&&) = delete;
			ConsoleStandardError& operator=(const ConsoleStandardError&) = delete;

		private:
			ConsoleStandardError()
#ifdef __WINDOWS__
				: Core::TextStreamRedirectType<StandardError>(::_fileno(stderr)) {
#else
				: Core::TextStreamRedirectType<StandardError>(STDERR_FILENO) {
#endif
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
