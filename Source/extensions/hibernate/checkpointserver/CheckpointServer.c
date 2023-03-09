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
#define MODULE "CheckpointServer"

#include "../hibernate.h"
#include "../common/Log.h"

#include <assert.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <stdbool.h>
#include <unistd.h>

typedef enum {
    MEMCR_CHECKPOINT = 100,
    MEMCR_RESTORE
} ServerRequestCode;

typedef enum {
    MEMCR_OK = 0,
    MEMCR_ERROR = -1
} ServerResponseCode;

typedef struct {
    ServerRequestCode reqCode;
    pid_t pid;
} __attribute__((packed)) ServerRequest;

typedef struct {
    ServerResponseCode respCode;
} __attribute__((packed)) ServerResponse;

static bool SendRcvCmd(const ServerRequest* cmd, ServerResponse* resp, uint32_t timeoutMs, const char* serverIpAddr)
{
    int cd;
    int ret;
    struct sockaddr_in addr = {0};
    resp->respCode = MEMCR_ERROR;

	cd = socket(AF_INET, SOCK_STREAM, 0);
	if (cd < 0) {
		LOGERR("Socket create failed");
		return false;
	}

    char host[64] = {0};
    strncpy(host, serverIpAddr, 64);
    char *port = strstr(host, ":");
    if(port == NULL)
    {
        LOGERR("Invalid Server Ip Address: %s", host);
        return false;
    }
    //Add NULL delimer between host and port
    *port = 0;
    port++;    

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = htons(atoi(port));

    struct timeval rcvTimeout;
    rcvTimeout.tv_sec = timeoutMs / 1000;
    rcvTimeout.tv_usec = (timeoutMs % 1000) * 1000;

    setsockopt(cd, SOL_SOCKET, SO_RCVTIMEO, &rcvTimeout, sizeof(rcvTimeout));

    ret = connect(cd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
    if (ret < 0) {
        LOGERR("Socket connect failed: %d with %s", ret, serverIpAddr);
        close(cd);
        return false;
    }

    ret = write(cd, cmd, sizeof(ServerRequest));
    if (ret != sizeof(ServerRequest)) {
        LOGERR("Socket write failed: ret %d", ret);
        close(cd);
        return false;
    }

    ret = read(cd, resp, sizeof(ServerResponse));
    if (ret != sizeof(ServerResponse)) {
        LOGERR("Socket read failed: ret %d", ret);
        close(cd);
        return false;
    }

    close(cd);

    return (resp->respCode == MEMCR_OK);
}

uint32_t HibernateProcess(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], void** storage)
{
    ServerRequest req = {
        .reqCode = MEMCR_CHECKPOINT,
        .pid = pid
    };
    ServerResponse resp;

    if (SendRcvCmd(&req, &resp, timeout, data_dir)) {
        LOGINFO("Hibernate process PID %d success", pid);
        return HIBERNATE_ERROR_NONE;
    } else {
        LOGERR("Error Hibernate process PID %d ret %d", pid, resp.respCode);
        return HIBERNATE_ERROR_GENERAL;
    }
}

uint32_t WakeupProcess(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], void** storage)
{
    ServerRequest req = {
        .reqCode = MEMCR_RESTORE,
        .pid = pid
    };
    ServerResponse resp;

    if (SendRcvCmd(&req, &resp, timeout, data_dir)) {
        LOGINFO("Wakeup process PID %d success", pid);
        return HIBERNATE_ERROR_NONE;
    } else {
        LOGERR("Error Wakeup process PID %d ret %d", pid, resp.respCode);
        return HIBERNATE_ERROR_GENERAL;
    }
}
