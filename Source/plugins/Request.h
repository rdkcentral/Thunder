#ifndef __PLUGIN_FRAMEWORK_REQUEST_H
#define __PLUGIN_FRAMEWORK_REQUEST_H

#include "Module.h"

namespace WPEFramework {
namespace PluginHost {

    // Forward declaration. We only have a smart pointer to a service.
    class Service;

    // Request related classes
    // These classes realize the allocation and supply of requests. Whenever
    // a reuest is coming in, all information, gathered during the loading
    // of the request, is stored in the request. The pool, recycles requests
    // used but are not actively used.
    class EXTERNAL Request : public Web::Request {
    public:
        enum enumState : uint8_t {
            INCOMPLETE = 0x01,
            OBLIVIOUS = 0x02,
            MISSING_CALLSIGN = 0x04,
            INVALID_VERSION = 0x08,
            COMPLETE = 0x10,
            SERVICE_CALL = 0x80
        };

    private:
        Request(const Request&) = delete;
        Request& operator=(const Request&) = delete;

    public:
        Request();
        virtual ~Request();

    public:
        inline bool ServiceCall() const
        {
            return ((_state & SERVICE_CALL) != 0);
        }
        inline enumState State() const
        {
            return (static_cast<enumState>(_state & 0x3F));
        }
        inline Core::ProxyType<PluginHost::Service>& Service()
        {
            return (_service);
        }

        void Clear();
        void Service(const uint32_t errorCode, const Core::ProxyType<PluginHost::Service>& service, const bool serviceCall);

    private:
        uint8_t _state;
        Core::ProxyType<PluginHost::Service> _service;
    };

    typedef Core::ProxyPoolType<PluginHost::Request> RequestPool;
}
}

#endif // __PLUGIN_FRAMEWORK_REQUEST_
