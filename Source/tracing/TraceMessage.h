#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Trace {

    template <typename CATEGORY, const char** MODULENAME>
    class TraceMessage : public Core::IMessageEvent {
    public:
        TraceMessage() = default;
        TraceMessage(const string& classname, const string& text)
            : _moduleName(*MODULENAME)
            , _classname(classname)
            , _text(text)
        {
        }

        uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const override
        {
            Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
            Core::FrameType<0>::Writer writer(frame, 0);

            writer.NullTerminatedText(_moduleName);
            writer.NullTerminatedText(_classname);
            writer.NullTerminatedText(_text);

            return writer.Offset();
        }
        uint16_t Deserialize(uint8_t buffer[], const uint16_t bufferSize) override
        {
            Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
            Core::FrameType<0>::Reader reader(frame, 0);

            _moduleName = reader.NullTerminatedText();
            _classname = reader.NullTerminatedText();
            _text = reader.NullTerminatedText();

            return _moduleName.size() + 1 + _classname.size() + 1 + _text.size() + 1;
        }

        void ToString(string& text) const override
        {
            text = Core::Format("[%s][%s]", _moduleName.c_str(), _text.c_str());
        }

    private:
        string _moduleName;
        string _classname;
        string _text;
    };
}
}