#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Trace {

    class EXTERNAL TraceFormatter : public Core::IMessageAssembler {
    public:
        TraceFormatter() = default;
        ~TraceFormatter() = default;
        TraceFormatter(const TraceFormatter&) = delete;
        TraceFormatter& operator=(const TraceFormatter&) = delete;

        inline string Prepare(const bool abbreviateMessage, const Core::MessageInformation& info, const Core::IMessageEvent* message) const override
        {
            _output.str("");
            _output.clear();

            message->ToString(_deserializedMessage);

            if (abbreviateMessage == true) {
                string time(Core::Time::Now().ToTimeOnly(true));
                _output << '[' << time.c_str() << ']' << '[' << info.MetaData().Category() << "]: " << _deserializedMessage << std::endl;
            } else {
                string time(Core::Time::Now().ToRFC1123(true));
                _output << '[' << time.c_str() << "]:[" << Core::FileNameOnly(info.FileName().c_str()) << ':' << info.LineNumber() << "] "
                        << info.MetaData().Category() << ": " << _deserializedMessage << std::endl;
            }

            return _output.str();
        }

    private:
        mutable string _deserializedMessage;
        mutable std::ostringstream _output;
    };
}
}