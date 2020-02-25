 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#ifndef __TRACEMEDIA_H
#define __TRACEMEDIA_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "ITraceMedia.h"
#include "Module.h"

// ---- Referenced classes and types ----

// ---- Helper functions ----

// ---- Class Definition ----
namespace WPEFramework {
namespace Trace {
    // ---- Helper types and constants ----
    struct ITrace;

    class EXTERNAL TraceMedia : public ITraceMedia {
    private:
        class Channel : public Core::SocketDatagram {
        public:
            Channel(TraceMedia& parent, const Core::NodeId& remoteNode)
                : Core::SocketDatagram(false, remoteNode.Origin(), remoteNode, 2048, TRACINGBUFFERSIZE + 512)
                , _loaded(0)
                , // 32 bytes for preamble
                _parent(parent)
            {
            }
            virtual ~Channel()
            {
                Close(Core::infinite);
            }

        public:
            // Methods to extract and insert data into the socket buffers
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {
                // We used it all up..
                // TODO: make thread safe.

                uint16_t actualByteCount = _loaded > maxSendSize ? maxSendSize : _loaded;
                memcpy(dataFrame, _traceBuffer, actualByteCount);
                _loaded = 0;

                return (actualByteCount);
            }
            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
            {
                _parent.HandleMessage(dataFrame, receivedSize);

                return (receivedSize);
            }
            // Signal a state change, Opened, Closed or Accepted
            virtual void StateChange()
            {
            }

            // TODO: lock to keep this consistent
            char _traceBuffer[1500];
            uint16_t _loaded;

            // TODO: create message here, not in parent

        private:
            TraceMedia& _parent;
        };

    private:
        TraceMedia();
        TraceMedia(const TraceMedia&);
        TraceMedia& operator=(const TraceMedia&);

    public:
        TraceMedia(const Core::NodeId& nodeId);
        ~TraceMedia();

        virtual void Output(
            const char fileName[],
            const uint32_t lineNumber,
            const char className[],
            const ITrace* information);

    private:
        char* CopyText(char* destination, const char* source, uint32_t maxSize);
        void HandleMessage(const uint8_t* dataFrame, const uint16_t receivedSize);
        void Write(const uint8_t* dataFrame, const uint16_t receivedSize);

    private:
        Channel m_Output;
    };
}
} // namespace Trace

#endif // __TRACEMEDIA_H
