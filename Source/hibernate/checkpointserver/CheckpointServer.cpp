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
#include "hibernate/Hibernate.h"

namespace WPEFramework {
namespace Hibernate {

    struct CheckpointMetadata {
        pid_t pid;
    };

    enum ServerRequestCode {
        MEMCR_CHECKPOINT = 100,
        MEMCR_RESTORE
    };

    enum ServerResponseCode {
        MEMCR_OK = 0,
        MEMCR_ERROR = -1
    };

    struct ServerRequest {
        ServerRequestCode reqCode;
        pid_t pid;
        int timeout;
    } __attribute__((packed));

    struct ServerResponse {
        ServerResponseCode respCode;
    } __attribute__((packed));

    const char* MEMCR_SERVER_SOCKET = "/tmp/memcrservice";

    bool SendRcvCmd(ServerRequest& cmd, ServerResponse& resp, uint32_t timeoutMs)
    {
        int cd;
        int ret;
        struct sockaddr_un addr = { 0 };
        resp.respCode = ServerResponseCode::MEMCR_ERROR;

        cd = socket(PF_UNIX, SOCK_STREAM, 0);
        if (cd < 0) {
            printf("Socket create failed: %d", cd);
            fflush(stdout);
            return false;
        }

        struct timeval rcvTimeout;
        rcvTimeout.tv_sec = timeoutMs / 1000;
        rcvTimeout.tv_usec = (timeoutMs % 1000) * 1000;

        setsockopt(cd, SOL_SOCKET, SO_RCVTIMEO, &rcvTimeout, sizeof(rcvTimeout));

        addr.sun_family = PF_UNIX;
        strncpy(addr.sun_path, MEMCR_SERVER_SOCKET, sizeof(addr.sun_path));

        ret = connect(cd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
        if (ret < 0) {
            printf("Socket connect failed: %d with %s", ret, MEMCR_SERVER_SOCKET);
            fflush(stdout);
            close(cd);
            return false;
        }

        ret = write(cd, &cmd, sizeof(cmd));
        if (ret != sizeof(cmd)) {
            printf("Socket write failed: ret %d", ret);
            fflush(stdout);
            close(cd);
            return false;
        }

        ret = read(cd, &resp, sizeof(resp));
        if (ret != sizeof(resp)) {
            printf("Socket read failed: ret %d", ret);
            fflush(stdout);
            close(cd);
            return false;
        }

        close(cd);

        return (resp.respCode == ServerResponseCode::MEMCR_OK);
    }

    uint32_t Hibernate(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], void** storage)
    {
        ASSERT(*storage == nullptr);

        ServerRequest req = {
            .reqCode = MEMCR_CHECKPOINT,
            .pid = pid,
            .timeout = static_cast<int>(timeout)
        };
        ServerResponse resp;

        if (SendRcvCmd(req, resp, timeout)) {
            printf("Hibernate process PID %d success", pid);
            fflush(stdout);
            CheckpointMetadata* metadata = new CheckpointMetadata;
            if (metadata) {
                metadata->pid = pid;
            }

            *storage = static_cast<void*>(metadata);
            return Core::ErrorCodes::ERROR_NONE;
        } else {
            printf("Error Hibernate process PID %d ret %d", pid, resp.respCode);
            fflush(stdout);
            return Core::ErrorCodes::ERROR_GENERAL;
        }
    }

    uint32_t Wakeup(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], void** storage)
    {
        ASSERT(*storage != nullptr);
        CheckpointMetadata* metaData = static_cast<CheckpointMetadata*>(*storage);
        ASSERT(metaData->pid == pid);
        delete metaData;
        *storage = nullptr;

        ServerRequest req = {
            .reqCode = MEMCR_RESTORE,
            .pid = pid,
            .timeout = static_cast<int>(timeout)
        };
        ServerResponse resp;

        if (SendRcvCmd(req, resp, timeout)) {
            printf("Wakeup process PID %d success", pid);
            fflush(stdout);
            return Core::ErrorCodes::ERROR_NONE;
        } else {
            printf("Error Wakeup process PID %d ret %d", pid, resp.respCode);
            fflush(stdout);
            return Core::ErrorCodes::ERROR_GENERAL;
        }

        return 0;
    }

}
}