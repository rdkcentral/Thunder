
// C++ classes for Messenger JSON-RPC API.
// Generated automatically from 'MessengerAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace Messenger {

        // Common classes
        //

        class JoinResultInfo : public Core::JSON::Container {
        public:
            JoinResultInfo()
                : Core::JSON::Container()
            {
                Add(_T("roomid"), &Roomid);
            }

            JoinResultInfo(const JoinResultInfo&) = delete;
            JoinResultInfo& operator=(const JoinResultInfo&) = delete;

        public:
            Core::JSON::String Roomid; // Unique ID of the room
        }; // class JoinResultInfo

        // Method params/result classes
        //

        class JoinParamsData : public Core::JSON::Container {
        public:
            JoinParamsData()
                : Core::JSON::Container()
            {
                Add(_T("user"), &User);
                Add(_T("room"), &Room);
            }

            JoinParamsData(const JoinParamsData&) = delete;
            JoinParamsData& operator=(const JoinParamsData&) = delete;

        public:
            Core::JSON::String User; // User name to join the room under (must not be empty)
            Core::JSON::String Room; // Name of the room to join (must not be empty)
        }; // class JoinParamsData

        class MessageParamsData : public Core::JSON::Container {
        public:
            MessageParamsData()
                : Core::JSON::Container()
            {
                Add(_T("user"), &User);
                Add(_T("message"), &Message);
            }

            MessageParamsData(const MessageParamsData&) = delete;
            MessageParamsData& operator=(const MessageParamsData&) = delete;

        public:
            Core::JSON::String User; // Name of the user that has sent the message
            Core::JSON::String Message; // Content of the message
        }; // class MessageParamsData

        class RoomupdateParamsData : public Core::JSON::Container {
        public:
            // Specifies the room status change, e.g. created or destroyed
            enum class ActionType {
                CREATED,
                DESTROYED
            };

            RoomupdateParamsData()
                : Core::JSON::Container()
            {
                Add(_T("room"), &Room);
                Add(_T("action"), &Action);
            }

            RoomupdateParamsData(const RoomupdateParamsData&) = delete;
            RoomupdateParamsData& operator=(const RoomupdateParamsData&) = delete;

        public:
            Core::JSON::String Room; // Name of the room this notification relates to
            Core::JSON::EnumType<ActionType> Action; // Specifies the room status change, e.g. created or destroyed
        }; // class RoomupdateParamsData

        class SendParamsData : public Core::JSON::Container {
        public:
            SendParamsData()
                : Core::JSON::Container()
            {
                Add(_T("roomid"), &Roomid);
                Add(_T("message"), &Message);
            }

            SendParamsData(const SendParamsData&) = delete;
            SendParamsData& operator=(const SendParamsData&) = delete;

        public:
            Core::JSON::String Roomid; // ID of the room to send the message to
            Core::JSON::String Message; // The message content to send
        }; // class SendParamsData

        class UserupdateParamsData : public Core::JSON::Container {
        public:
            // Specifies the user status change, e.g. join or leave a room
            enum class ActionType {
                JOINED,
                LEFT
            };

            UserupdateParamsData()
                : Core::JSON::Container()
            {
                Add(_T("user"), &User);
                Add(_T("action"), &Action);
            }

            UserupdateParamsData(const UserupdateParamsData&) = delete;
            UserupdateParamsData& operator=(const UserupdateParamsData&) = delete;

        public:
            Core::JSON::String User; // Name of the user that has this notification relates to
            Core::JSON::EnumType<ActionType> Action; // Specifies the user status change, e.g. join or leave a room
        }; // class UserupdateParamsData

    } // namespace Messenger

} // namespace JsonData

}

