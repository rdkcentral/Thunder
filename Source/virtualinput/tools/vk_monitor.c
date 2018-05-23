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
    printf("For example: vk_monitor /tmp/keyhandler \n") ;
    printf("To stop monitoring hit ctrl-c\n") ;
}

int main(int argc, const char *argv[])
{
    if (argc != 2)
    {
        printUsage();
        return 1;
    }

    signal(SIGINT, intHandler);

    void* handle = 0;

    const char *listenerName = argv[0];
    const char *Connector = argv[1];

    handle = Construct(listenerName, Connector, KeyEvent);

    while(keepRunning);

    Destruct(handle);

    return 0;
}