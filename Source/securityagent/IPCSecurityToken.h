#pragma once

#include <core/core.h>

using namespace WPEFramework;

namespace IPC {

#ifdef __DEBUG__
    enum { CommunicationTimeOut = Core::infinite }; // Time in ms. Forever
#else
    enum { CommunicationTimeOut = 2000 }; // Time in ms. 2 Seconds
#endif

namespace SecurityAgent {

    typedef Core::IPCMessageType<10, Core::IPC::BufferType<512>, Core::IPC::BufferType<1024> > TokenData;
}

} // namespace IPC::SecurityAgent
