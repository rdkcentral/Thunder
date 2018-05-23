#include <linux/rtnetlink.h>

#include "Netlink.h"
#include "Sync.h"

// #define DEBUG_FRAMES 1

namespace WPEFramework {

namespace Core {

#ifdef DEBUG_FRAMES
static void DumpFrame (const TCHAR prefix[], const uint8_t frame[], const uint16_t length) {
    fprintf (stderr, "\n%s:", prefix);
    for (int teller = (8 * 3); teller < length; teller++) {
        if (teller % 8 == 0) { fprintf (stderr, "\n"); }
        fprintf (stderr, "0x%02X ", frame[teller]);
    }
    fprintf (stderr, "\n");
    fflush (stderr);
}
#endif

/* static */  uint32_t Netlink::_sequenceId = 0;

uint16_t Netlink::Serialize (uint8_t stream[], const uint16_t length) const {

    uint16_t result    = 0;

    if (sizeof(struct nlmsghdr) < length) {

        memset(stream, 0, sizeof(struct nlmsghdr));

        struct nlmsghdr* message = reinterpret_cast<struct nlmsghdr*>(stream);

        _mySequence = Core::InterlockedIncrement(_sequenceId);

        size_t nlmsg_len = NLMSG_LENGTH(Write(static_cast<uint8_t*>(NLMSG_DATA(message)), length - sizeof(struct nlmsghdr)));

        message->nlmsg_len = nlmsg_len;
        message->nlmsg_type = Type();
        message->nlmsg_flags = Flags();
        message->nlmsg_seq = _mySequence;
        message->nlmsg_pid = 0;             /* send to the kernel */

        result = nlmsg_len;
    }

    return (result);
}

uint16_t Netlink::Deserialize (const uint8_t stream[], const uint16_t streamLength) {
    bool completed = false;
    Frames parser (stream, streamLength);

    _type = NLMSG_ERROR;

    while ((completed == false) && (parser.Next() == true)) {

        _type  = parser.Type();
        _flags = parser.Flags();
        _mySequence = parser.Sequence();

        completed = (Read(parser.Data(), parser.Size()) < parser.Size());
    }

    // Return the amount of data we have eaten...
    return (completed == false ? 0 : streamLength);
}

uint32_t SocketNetlink::Exchange (const Core::Netlink& outbound, Core::Netlink& inbound, const uint32_t waitTime) {
    uint32_t result = Core::ERROR_BAD_REQUEST;
            
    _adminLock.Lock();

    _pending.emplace_back(outbound, inbound);

    Message& myEntry = _pending.back();

    _adminLock.Unlock();

    Core::SocketDatagram::Trigger();

    if (myEntry.Wait(waitTime) == false) {
        result = Core::ERROR_RPC_CALL_FAILED;
    }
    else {
        result = Core::ERROR_NONE;
    }

    // if we leave we need to take out "our" element.
    _adminLock.Lock();

    PendingList::iterator index (std::find(_pending.begin(), _pending.end(), outbound));

    ASSERT (index != _pending.end());

    if (index != _pending.end()) {
        _pending.erase(index);
    }

    _adminLock.Unlock();

    return (result);
}

// Methods to extract and insert data into the socket buffers
/* virtual */ uint16_t SocketNetlink::SendData(uint8_t* dataFrame, const uint16_t maxSendSize) {
    uint16_t result = 0;

    if (_pending.size() > 0) {
                
        _adminLock.Lock();

        PendingList::iterator index(_pending.begin());

        // Skip all, already send items
        while ((index != _pending.end()) && (index->IsSend() == true)) { index++; }

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

/* virtual */ uint16_t SocketNetlink::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) {

    uint16_t result = receivedSize;
    Netlink::Frames frames(dataFrame, receivedSize);

    #ifdef DEBUG_FRAMES
    DumpFrame("RECEIVED", dataFrame, result);
    #endif

    _adminLock.Lock();

    while (frames.Next() == true) {

        PendingList::iterator index(_pending.begin());

        // Check if this is a response to something pending..
        while ((index != _pending.end()) && (index->Sequence() != frames.Sequence())) { index++; }

        if (index != _pending.end()) {

            index->Deserialize(frames);
        }
        else {
            Deserialize (frames.RawData(), frames.RawSize());
        }
    }

    _adminLock.Unlock();

    return (result);
}

// Signal a state change, Opened, Closed or Accepted
/* virtual */ void SocketNetlink::StateChange() {
}
 
} } // namespace WPEFramework::Core
