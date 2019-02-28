#include "Channel.h"

namespace WPEFramework {

namespace JSONRPC {

    static Channel::FactoryImpl& Channel::FactoryImpl::Instance() {
        static FactoryImpl _singleton;

        return (_singleton);
    }

/* static */ Channel::ChannelProxy::Administrator _singleton;

}
}
