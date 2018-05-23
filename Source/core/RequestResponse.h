#ifndef __REQUESTRESPONSE_H
#define __REQUESTRESPONSE_H

#include "Module.h"

namespace WPEFramework {
namespace Platform {

    template <typename REQUEST, typename RESPONSE>
    class RequestResponseType {
    public:
        enum RequestResponseState {
            REQUESTED,
            RESPONDED,
            FAILED
        };

    private:
        RequestResponseType();

    public:
        RequestResponseType(Core::ProxyType<REQUEST>& request)
            : m_Request(request)
            , m_Response()
            , m_State(REQUESTED)
        {
        }
        RequestResponseType(RequestResponseType<REQUEST, RESPONSE>& copy)
            : m_Request(copy.m_Request)
            , m_Response(copy.m_Response)
            , m_State(copy.m_State)
        {
        }
        ~RequestResponseType()
        {
        }

        RequestResponseType<REQUEST, RESPONSE>& operator=(const RequestResponseType<REQUEST, RESPONSE>& RHS)
        {
            m_Request = RHS.m_Request;
            m_Response = RHS.m_Response;
            m_State = RHS.m_State;

            return (*this);
        }

    public:
        inline RequestResponseState State() const
        {
            return (m_Responded);
        }

        inline REQUEST& Request()
        {
            return (*m_Request);
        }

        inline RESPONSE& Response()
        {
            return (*m_Request);
        }

    private:
        RequestResponseState m_State;
        Core::ProxyType<REQUEST> m_Request;
        Core::ProxyType<RESPONSE> m_Response;
    };
}
} // namespace Platform

#endif // __REQUESTRESPONSE_H
