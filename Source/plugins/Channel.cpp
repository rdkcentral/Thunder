#include "Channel.h"

namespace WPEFramework {
namespace PluginHost {

    /* static */ RequestPool Channel::_requestAllocator(10);

#ifdef __WIN32__ 
#pragma warning( disable : 4355 )
#endif
    Channel::Channel(const SOCKET& connector, const Core::NodeId& remoteId)
        : BaseClass(true, false, 5, _requestAllocator, false, connector, remoteId, 1024, 1024)
        , _adminLock()
        , _ID(0)
        , _nameOffset(~0)
        , _state(WEB)
        , _serializer()
        , _deserializer(*this)
        , _text()
        , _offset(0)
        , _sendQueue()
    {
    }
#ifdef __WIN32__ 
#pragma warning( default : 4355 )
#endif

    /* virtual */ Channel::~Channel()
    {
        Close(0);
    }
}
}
