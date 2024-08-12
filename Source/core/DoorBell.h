 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#pragma once

#include "Module.h"
#include "ResourceMonitor.h"
#include "SocketPort.h"
#include "Sync.h"

namespace Thunder {
namespace Core {

    class EXTERNAL DoorBell {
    private:
        class EXTERNAL Connector : public IResource {
        private:
            static constexpr char SIGNAL_DOORBELL = 0x55;

        public:
            Connector() = delete;
            Connector(const Connector&) = delete;
            Connector& operator=(const Connector&) = delete;

            Connector(DoorBell& parent, const Core::NodeId& node);
            virtual ~Connector();

        public:
            // _bound is not expected to be thread safe, as this is only called once, by a single thread
            // who will always do the waiting  for the doorbell to be rang!!
            bool Bind() const;
            void Unbind() const;
            bool Ring()
            {
                const char message[] = { SIGNAL_DOORBELL };
                int sendSize = ::sendto(_sendSocket, message, sizeof(message), 0, static_cast<const NodeId&>(_doorbell), _doorbell.Size());
                return (sendSize == sizeof(message));
            }

        private:
            virtual IResource::handle Descriptor() const override;
            virtual uint16_t Events() override;
            virtual void Handle(const uint16_t events) override;
            void Read()
            {
                int size;
                char buffer[16];

                do {
                    size = ::recv(_receiveSocket, buffer, sizeof(buffer), 0);

                    if (size != SOCKET_ERROR) {
                        for (int index = 0; index < size; index++) {
                            if (buffer[index] == SIGNAL_DOORBELL) {
                                _parent.Ringing();
                            }
                        }
                    }

                } while ((size != 0) && (size != SOCKET_ERROR));
            }

        private:
            DoorBell& _parent;
            Core::NodeId _doorbell;
            SOCKET _sendSocket;
            mutable SOCKET _receiveSocket;
            mutable uint16_t _registered;
        };

    public:
        DoorBell() = delete;
        DoorBell(const DoorBell&) = delete;
        DoorBell& operator=(const DoorBell&) = delete;

        DoorBell(const TCHAR sourceName[]);
        ~DoorBell();

    public:
        void Ring()
        {
            _connectPoint.Ring();
            _signal.SetEvent();
        }
        void Acknowledge()
        {
            _signal.ResetEvent();
        }
        uint32_t Wait(const uint32_t waitTime) const
        {
            uint32_t result = ERROR_UNAVAILABLE;

            if (_connectPoint.Bind() == true) {
                result = _signal.Lock(waitTime);
            }

            return (result);
        }
        void Relinquish() {
            _connectPoint.Unbind();
        }

    private:
        void Ringing()
        {
            _signal.SetEvent();
        }

    private:
        Connector _connectPoint;
        mutable Core::Event _signal;
    };
}
} // namespace Core
