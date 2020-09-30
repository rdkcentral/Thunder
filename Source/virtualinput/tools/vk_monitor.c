/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

/*
 * virtual keyboard monitor
 */

#include <stdio.h>
#include <signal.h>

#include "../VirtualKeyboard.h"

static volatile int keepRunning = 1;

static const char *ActionToString[] = {
        "Released", "Pressed", "Repeat", "Completed"
};

void intHandler(int pietertje) {
    keepRunning = 0;
}

static void KeyEvent(enum actiontype type, unsigned int code)
{
    printf("VirtualKeyboard event, keycode %u, action %s\n", code, ActionToString[type]);
}

void printUsage()
{
    printf("vk_monitor connector\n");
    printf("For example: vk_monitor test /tmp/keyhandler \n") ;
    printf("To stop monitoring hit ctrl-c\n") ;
}

int main(int argc, const char *argv[])
{
    if (argc != 3)
    {
        printUsage();
        return 1;
    }

    signal(SIGINT, intHandler);

    void* handle = 0;

    const char *listenerName = argv[1];
    const char *Connector = argv[2];

    handle = Construct(listenerName, Connector, KeyEvent);

    while(keepRunning);

    Destruct(handle);

    return 0;
}
