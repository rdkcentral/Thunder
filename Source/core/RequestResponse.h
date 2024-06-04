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
 
#ifndef __REQUESTRESPONSE_H
#define __REQUESTRESPONSE_H

#include "Module.h"

namespace Thunder {
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
	RequestResponseType(RequestResponseType<REQUEST, RESPONSE>&& move)
            : m_Request(std::move(move.m_Request))
            , m_Response(std::move(move.m_Response))
            , m_State(std::move(move.m_State))
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
        RequestResponseType<REQUEST, RESPONSE>& operator=(RequestResponseType<REQUEST, RESPONSE>&& move)
        {
            if (this != &move) {
                m_Request = std::move(move.m_Request);
                m_Response = std::move(move.m_Response);
                m_State = std::move(move.m_State);
            }
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
