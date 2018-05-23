#include "TraceUnit.h"
#include "TraceMedia.h"
#include "TraceControl.h"

namespace WPEFramework {
namespace Trace {
// Allow runtime enabling of the reaces, using the same channel as the trace information is send over:
// This is done, using a simple protocol:
// <Type:1><Sequence:4><Command:1><Data:N>
//
// The first byte of the UDP message determines the type of message:
// [T] => Trace line
// [C] => Command
// [R] => Response

#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif

    TraceMedia::TraceMedia(const Core::NodeId& nodeId)
        : m_Output(*this, nodeId)
    {
        m_Output.Open(0);
    }

#ifdef __WIN32__
#pragma warning(default : 4355)
#endif

    TraceMedia::~TraceMedia()
    {
    }

    /* virtual */ void TraceMedia::Output(
        const char fileName[],
        const uint32_t lineNumber,
        const char className[],
        const ITrace* information)
    {
        uint64_t current = Core::Time::Now().Ticks();
        Core::TextFragment cleanClassName(Core::ClassNameOnly(className));

        m_Output._traceBuffer[0] = 'T';
        m_Output._traceBuffer[1] = (current >> 56) & 0xFF;
        m_Output._traceBuffer[2] = (current >> 48) & 0xFF;
        m_Output._traceBuffer[3] = (current >> 40) & 0xFF;
        m_Output._traceBuffer[4] = (current >> 32) & 0xFF;
        m_Output._traceBuffer[5] = (current >> 24) & 0xFF;
        m_Output._traceBuffer[6] = (current >> 16) & 0xFF;
        m_Output._traceBuffer[7] = (current >> 8) & 0xFF;
        m_Output._traceBuffer[8] = (current >> 0) & 0xFF;
        m_Output._traceBuffer[9] = (lineNumber >> 24) & 0xFF;
        m_Output._traceBuffer[10] = (lineNumber >> 16) & 0xFF;
        m_Output._traceBuffer[11] = (lineNumber >> 8) & 0xFF;
        m_Output._traceBuffer[12] = (lineNumber >> 0) & 0xFF;

        char* result = CopyText(&m_Output._traceBuffer[13], fileName, sizeof(m_Output._traceBuffer) - 13);

        ASSERT(cleanClassName.Length() < (sizeof(m_Output._traceBuffer) - static_cast<uint32_t>(result - m_Output._traceBuffer)));

        memcpy(result, cleanClassName.Data(), cleanClassName.Length());
        result = &(result[cleanClassName.Length()]);
        *result++ = '\0';

		uint16_t length = static_cast<uint16_t>(result - m_Output._traceBuffer);
		uint16_t bufferRemaining = sizeof(m_Output._traceBuffer) - length;
		uint16_t copiedBytes = (bufferRemaining > information->Length()) ? information->Length() : bufferRemaining;

		memcpy(result, information->Data(), copiedBytes);

        m_Output._loaded = length + copiedBytes;

        m_Output.Trigger();
    }

    void TraceMedia::HandleMessage(const uint8_t* dataFrame, const uint16_t receivedSize)
    {
        if ((receivedSize >= 6) && (dataFrame[0] == 'C')) {
            static char response[TRACINGBUFFERSIZE];

            response[0] = 'R';
            response[1] = dataFrame[1];
            response[2] = dataFrame[2];
            response[3] = dataFrame[3];
            response[4] = dataFrame[4];

            switch (dataFrame[5]) {
            case 0: { // List all trace categories !!
                TraceUnit::Iterator index = TraceUnit::Instance().GetCategories();

                response[5] = dataFrame[5];

                // Send out all categories/modules
                while (index.Next() == true) {
                    uint32_t size = sizeof(response) - 8;

                    response[6] = ((*index)->Enabled() == true ? '1' : '0');
                    response[7] = '\0';

                    // Add line
                    char* last = CopyText(&response[8], (*index)->Category(), size);
                    last = CopyText(last, (*index)->Module(), size);

                    // Give 1s per line to send ;-)
                    Write(reinterpret_cast<const uint8_t*>(response), static_cast<uint32_t>(sizeof(response)) - size);
                }
                break;
            }
            case 1: { // Toggle tracing
                const char* module = reinterpret_cast<const char*>(::memchr(&dataFrame[8], '\0', receivedSize - 8));
                if (module != nullptr) {
                    uint32_t count = TraceUnit::Instance().SetCategories((dataFrame[6] == '1'), &module[1], reinterpret_cast<const char*>(&dataFrame[8]));

                    // affter the command we get the the change count
                    response[6] = (count >> 24) & 0xFF;
                    response[7] = (count >> 16) & 0xFF;
                    response[8] = (count >> 8) & 0xFF;
                    response[9] = (count >> 0) & 0xFF;

                    Write(reinterpret_cast<const uint8_t*>(response), 10);
                }
                break;
            }
            }
        }
    }

    void TraceMedia::Write(const uint8_t* dataFrame VARIABLE_IS_NOT_USED, const uint16_t receivedSize VARIABLE_IS_NOT_USED)
    {
    }

    char* TraceMedia::CopyText(char* destination, const char* source, uint32_t maxSize)
    {
        uint32_t size = maxSize - 1;
        const char* end = reinterpret_cast<const char*>(::memchr(source, '\0', size));

        if (end != nullptr) {
            size = static_cast<uint32_t>(end - source);
        }

        ::memcpy(destination, source, size);
        destination = &destination[size];
        *destination++ = '\0';

        return (destination);
    }
}
} // namespace Trace
