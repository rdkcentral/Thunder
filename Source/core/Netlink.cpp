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
 
#include <linux/rtnetlink.h>

#include "Netlink.h"
#include "Sync.h"

// #define DEBUG_FRAMES 1

namespace Thunder {

namespace Core {

#ifdef DEBUG_FRAMES
    static void DumpFrame(const TCHAR prefix[], const uint8_t frame[], const uint16_t length)
    {
        fprintf(stderr, "\n%s:", prefix);
        for (int teller = (8 * 3); teller < length; teller++) {
            if (teller % 8 == 0) {
                fprintf(stderr, "\n");
            }
            fprintf(stderr, "0x%02X ", frame[teller]);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
    }
#endif

    /* static */ uint32_t Netlink::_sequenceId = 0;

    uint16_t Netlink::Serialize(uint8_t stream[], const uint16_t length) const
    {
        uint16_t result = 0;

        if (sizeof(struct nlmsghdr) < length) {

            memset(stream, 0, sizeof(struct nlmsghdr));

            struct nlmsghdr* message = reinterpret_cast<struct nlmsghdr*>(stream);

            _mySequence = Core::InterlockedIncrement(_sequenceId);

            size_t nlmsg_len = NLMSG_LENGTH(Write(static_cast<uint8_t*>(NLMSG_DATA(message)), length - sizeof(struct nlmsghdr)));

            message->nlmsg_len = nlmsg_len;
            message->nlmsg_type = Type();
            message->nlmsg_flags = Flags();
            message->nlmsg_seq = _mySequence;
            message->nlmsg_pid = 0; /* sender ID, we don't make use of it */

            result = nlmsg_len;
        }

        return (result);
    }

    uint16_t Netlink::Deserialize(const uint8_t stream[], const uint16_t streamLength)
    {
        const nlmsghdr* header = reinterpret_cast<const nlmsghdr*>(stream);
        uint16_t dataLeft = streamLength;
        bool completed = true;

        while (NLMSG_OK(header, dataLeft)) {
            if (header->nlmsg_type != NLMSG_NOOP) {
                _type = header->nlmsg_type;
                _flags = header->nlmsg_flags;
                _mySequence = header->nlmsg_seq;

                if ((header->nlmsg_len - sizeof(header)) > 0) {
                    completed = (Read(reinterpret_cast<const uint8_t *>(NLMSG_DATA(header)), header->nlmsg_len - sizeof(header)) > 0);
                }

                completed = completed && ((header->nlmsg_type == NLMSG_DONE) || ((header->nlmsg_flags & NLM_F_MULTI) == 0));
            }

            header = NLMSG_NEXT(header, dataLeft);
        }

        return (completed ? streamLength : 0);
    }

    uint32_t SocketNetlink::Send(const Core::Netlink& outbound, const uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_BAD_REQUEST;

        _adminLock.Lock();

        _pending.emplace_back(outbound);

        Message& myEntry = _pending.back();

        _adminLock.Unlock();

        Core::SocketDatagram::Trigger();

        if (myEntry.Wait(waitTime) == false) {
            result = Core::ERROR_RPC_CALL_FAILED;
        } else {
            result = Core::ERROR_NONE;
        }

        // if we leave we need to take out "our" element.
        _adminLock.Lock();

        PendingList::iterator index(std::find(_pending.begin(), _pending.end(), outbound));

        ASSERT(index != _pending.end());

        if (index != _pending.end()) {
            _pending.erase(index);
        }

        _adminLock.Unlock();

        return (result);
    }

    uint32_t SocketNetlink::Exchange(const Core::Netlink& outbound, Core::Netlink& inbound, const uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_BAD_REQUEST;

        _adminLock.Lock();

        _pending.emplace_back(outbound, inbound);

        Message& myEntry = _pending.back();

        _adminLock.Unlock();

        Core::SocketDatagram::Trigger();

        if (myEntry.Wait(waitTime) == false) {
            result = Core::ERROR_RPC_CALL_FAILED;
        } else {
            result = Core::ERROR_NONE;
        }

        // if we leave we need to take out "our" element.
        _adminLock.Lock();

        PendingList::iterator index(std::find(_pending.begin(), _pending.end(), outbound));

        ASSERT(index != _pending.end());

        if (index != _pending.end()) {
            _pending.erase(index);
        }

        _adminLock.Unlock();

        return (result);
    }

    // Methods to extract and insert data into the socket buffers
    /* virtual */ uint16_t SocketNetlink::SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
    {
        uint16_t result = 0;

        if (_pending.size() > 0) {

            _adminLock.Lock();

            PendingList::iterator index(_pending.begin());

            // Skip all, already send items
            while ((index != _pending.end()) && (index->IsSend() == true)) {
                index++;
            }

            if (index != _pending.end()) {
                result = index->Serialize(dataFrame, maxSendSize);
            }

            _adminLock.Unlock();

#ifdef DEBUG_FRAMES
            DumpFrame("SEND", dataFrame, result);
#endif

            result = NLMSG_ALIGN(result);
        }
        return (result);
    }

    /* virtual */ uint16_t SocketNetlink::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
    {

        uint16_t result = receivedSize;
        Netlink::Frames frames(dataFrame, receivedSize);

#ifdef DEBUG_FRAMES
        DumpFrame("RECEIVED", dataFrame, result);
#endif

        _adminLock.Lock();

        while (frames.Next() == true) {

            PendingList::iterator index(_pending.begin());

            // Check if this is a response to something pending..
            while ((index != _pending.end()) && (index->Sequence() != frames.Sequence())) {
                index++;
            }

            if (index != _pending.end()) {

                index->Deserialize(frames);
            } else {
                Deserialize(frames.RawData(), frames.RawSize());
            }
        }

        _adminLock.Unlock();

        return (result);
    }

    // Signal a state change, Opened, Closed or Accepted
    /* virtual */ void SocketNetlink::StateChange()
    {
    }
}
} // namespace Thunder::Core
