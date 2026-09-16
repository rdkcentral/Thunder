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

//
// INTENTIONAL_CRASH() reproducer
// ===============================
// Demonstrates the always-fatal INTENTIONAL_CRASH() macro (Source/core/Trace.h): forks a child
// that calls it directly, then verifies the child was terminated by SIGABRT - the same signal
// PluginHost's ExitDaemonHandler (Source/WPEFramework/PluginHost.cpp) listens for in a real
// deployment to dump WorkerPool/callstack metadata before the platform crash-reporter takes over.
//
// How to build (from your WPEFramework build directory):
//   cmake -DINTENTIONAL_CRASH_TEST=ON ...
//   make IntentionalCrashTest
//

#include "Module.h"

#include <cstdio>
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    pid_t pid = fork();

    if (pid == 0) {
        // --- child: this call never returns ---
        INTENTIONAL_CRASH("example reason: value=%d, text=%s", 42, "demo");
        _exit(1); // unreachable
    }

    int status = 0;
    waitpid(pid, &status, 0);

    if ((WIFSIGNALED(status) != 0) && (WTERMSIG(status) == SIGABRT)) {
        fprintf(stdout, "[Main] PASS: child process was terminated by SIGABRT via INTENTIONAL_CRASH, as expected\n");
        return (0);
    }

    fprintf(stdout, "[Main] FAIL: child process ended unexpectedly (status=%d)\n", status);
    return (1);
}
