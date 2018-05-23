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
        enum enumState {
            INCOMPLETE,
            OBLIVIOUS,
            MISSING_CALLSIGN,
            COMPLETE
        };

    private:
        Request(const Request&) = delete;
        Request& operator=(const Request&) = delete;

    public:
		Request();
		virtual ~Request();

    public:
        inline enumState State() const
        {
            return (_state);
        }
        inline Core::ProxyType<PluginHost::Service>& Service()
        {
            return (_service);
        }

		void Clear();
		void Service(const bool correctSignature, const Core::ProxyType<PluginHost::Service>& service);

    private:
        enumState _state;
        Core::ProxyType<PluginHost::Service> _service;
    };

    typedef Core::ProxyPoolType<PluginHost::Request> RequestPool;
}
}

#endif // __PLUGIN_FRAMEWORK_REQUEST_
