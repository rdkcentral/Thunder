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
#include <ctype.h>
#include <displayinfo.h>
#include <stdint.h>
#include <stdio.h>

#define Trace(fmt, ...)                                 \
    do {                                                \
        fprintf(stdout, "<< " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout);                                 \
    } while (0)

static uint32_t updatedCount = 0;

static void displayinfo_updated(struct displayinfo_type* session, void* data)
{
    updatedCount++;
    fprintf(stdout, "## %d display updated event%s received.\n", updatedCount, (updatedCount == 1) ? "" : "s");
}

void ShowMenu()
{
    printf("Enter\n"
           "\tC : Check display connection.\n"
           "\tD : Get current display resolution.\n"
           "\tH : Get current HDR standard\n"
           "\tP : Get current HDCP protection\n"
           "\tR : Enable display info events\n"
           "\tU : Disable display info events\n"
           "\t? : Help\n"
           "\tQ : Quit\n");
}

int main(int argc, char* argv[])
{
    Trace("%s test tool", argv[0]);

    uint8_t displayCount = 0;

    while (displayinfo_enumerate(displayCount, 0, NULL)) {
        displayCount++;
    }

    Trace("Found %d instance%s", displayCount, displayCount == 1 ? "" : "s");

    char instanceName[42];
    if (displayinfo_enumerate(0, sizeof(instanceName), instanceName) == false) {
        Trace("Exiting: requested instances not found.");
        return -1;
    }

    struct displayinfo_type* display = displayinfo_instance(instanceName);

    if (display == NULL) {
        Trace("Exiting: getting interface for failed.");
        return -2;
    }

    ShowMenu();

    int character;
    do {
        character = toupper(getc(stdin));

        switch (character) {
        case 'C': {
            Trace("Display %s connected", displayinfo_connected(display) ? "is" : "not");
            break;
        }
        case 'D': {
            Trace("Display resolution %dhx%dw", displayinfo_height(display), displayinfo_width(display));
            break;
        }
        case 'H': {
            switch (displayinfo_hdr(display)) {
            case DISPLAYINFO_HDR_OFF: {
                Trace("HDR: OFF");
                break;
            }
            case DISPLAYINFO_HDR_10: {
                Trace("HDR: HDR10");
                break;
            }
            case DISPLAYINFO_HDR_10PLUS: {
                Trace("HDR: HDR10 Plus");
                break;
            }
            case DISPLAYINFO_HDR_DOLBYVISION: {
                Trace("HDR: DolbyVision");
                break;
            }
            case DISPLAYINFO_HDR_TECHNICOLOR: {
                Trace("HDR: Technicolor");
                break;
            }
            default: {
                Trace("HDR: Unknown");
                break;
            }
            }
            break;
        }
        case 'P': {
            switch (displayinfo_hdcp_protection(display)) {
            case DISPLAYINFO_HDCP_UNENCRYPTED: {
                Trace("HDCP: Unencrypted");
                break;
            }
            case DISPLAYINFO_HDCP_1X: {
                Trace("HDCP: 1.x Enabled");
                break;
            }
            case DISPLAYINFO_HDCP_2X: {
                Trace("HDCP: 2.x Enabled");
                break;
            }
            default: {
                Trace("HDCP: Unknown");
                break;
            }
            }
            break;
        }
        case 'R': {
            displayinfo_register(display, displayinfo_updated, NULL);
            Trace("Display events enabled");
            break;
        }
        case 'U': {
            displayinfo_unregister(display, displayinfo_updated);
            Trace("Display events disabled");
            break;
        }
        case '?': {
            ShowMenu();
            break;
        }
        default:
            break;
        }
    } while (character != 'Q');

    displayinfo_release(display);

    Trace("Done");

    return 0;
}