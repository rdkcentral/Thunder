#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Trace {

    class EXTERNAL Formatter : public Core::Messaging::IAssembler {
    public:
        Formatter() = default;
        ~Formatter() = default;
        Formatter(const Formatter&) = delete;
        Formatter& operator=(const Formatter&) = delete;

        inline string Prepare(const bool abbreviateMessage, const Core::Messaging::Information& info, const Core::Messaging::IEvent* message) const override
        {
            _output.str("");
            _output.clear();

            message->ToString(_deserializedMessage);

            if (abbreviateMessage == true) {
                string time(Core::Time::Now().ToTimeOnly(true));
                _output << '[' << time.c_str() << ']' << '[' << info.MessageMetaData().Category() << "]: " << _deserializedMessage << std::endl;
            } else {
                string time(Core::Time::Now().ToRFC1123(true));
                _output << '[' << time.c_str() << "]:[" << Core::FileNameOnly(info.FileName().c_str()) << ':' << info.LineNumber() << "] "
                        << info.MessageMetaData().Category() << ": " << _deserializedMessage << std::endl;
            }

            return _output.str();
        }

    private:
        mutable string _deserializedMessage;
        mutable std::ostringstream _output;
    };
}
}